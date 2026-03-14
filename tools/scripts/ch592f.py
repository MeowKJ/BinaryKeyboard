#!/usr/bin/env python3
"""
tools/scripts/ch592f.py — CH592F CMake build helper
"""

from __future__ import annotations

import argparse
import ctypes
import json
import platform
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional

from build_report import UsageRow, fmt_bytes, pct_color_code, render_usage_report, rpad, usage_bar
from firmware_naming import ch592_filename_for_keyboard, normalize_keyboard_name
from tool_cache import resolve_tool_path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent.resolve()
FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
LINKER_SCRIPT = FIRMWARE_DIR / "SDK" / "Ld" / "Link.ld"
DEFAULT_KEYBOARD = "5KEY"
DEFAULT_PROFILE = "release"


def _enable_win_ansi() -> None:
    if platform.system() == "Windows":
        try:
            kernel32 = ctypes.windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except Exception:
            pass


_enable_win_ansi()
_USE_COLOR = sys.stdout.isatty()


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text


def info(msg: str) -> None:
    print(_c("36", "[INFO]"), msg)


def ok(msg: str) -> None:
    print(_c("32", "[ OK ]"), msg)


def warn(msg: str) -> None:
    print(_c("33", "[WARN]"), msg)


def sep() -> None:
    print(_c("2", "-" * 44))


def die(msg: str) -> None:
    print(_c("31", "[ERR ]"), msg, file=sys.stderr)
    sys.exit(1)


def _candidate_windows_cmake_paths() -> list[Path]:
    return [
        Path(r"C:\Program Files\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files (x86)\CMake\bin\cmake.exe"),
        Path(r"C:\ProgramData\chocolatey\bin\cmake.exe"),
        Path(r"C:\Users\KJ\scoop\shims\cmake.exe"),
        Path(r"C:\Users\KJ\scoop\apps\cmake\current\bin\cmake.exe"),
        Path(r"C:\msys64\ucrt64\bin\cmake.exe"),
        Path(r"C:\msys64\mingw64\bin\cmake.exe"),
        Path(r"C:\msys64\usr\bin\cmake.exe"),
        Path(r"C:\Tools\CMake\bin\cmake.exe"),
        Path(r"C:\App\Tools\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
    ]


def find_cmake() -> Optional[Path]:
    binary = "cmake.exe" if platform.system() == "Windows" else "cmake"
    candidates = _candidate_windows_cmake_paths() if platform.system() == "Windows" else []
    return resolve_tool_path("cmake", binary, env_name="CMAKE_PATH", candidates=candidates)


def _parse_cmake_cache_var(cache_file: Path, key: str) -> Optional[str]:
    if not cache_file.is_file():
        return None
    prefix = f"{key}:"
    try:
        for line in cache_file.read_text(errors="ignore").splitlines():
            if line.startswith(prefix):
                parts = line.split("=", 1)
                if len(parts) == 2:
                    return parts[1].strip()
    except Exception:
        return None
    return None


def _parse_size_row_from_output(text: str) -> Optional[tuple[int, int, int]]:
    m = re.search(r'^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9a-fA-F]+\s+\S+\.elf\s*$',
                  text, re.MULTILINE)
    if not m:
        return None
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def _read_memory_regions_from_linker(linker_script: Path) -> dict[str, tuple[int, int]]:
    out: dict[str, tuple[int, int]] = {}
    if not linker_script.is_file():
        return out

    unit_mul = {"B": 1, "K": 1024, "M": 1024 * 1024}
    mem_re = re.compile(
        r'^\s*(\w+)\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*LENGTH\s*=\s*([0-9]+(?:\.[0-9]+)?)\s*([BbKkMm]?)'
    )
    try:
        for line in linker_script.read_text(errors="ignore").splitlines():
            m = mem_re.match(line)
            if not m:
                continue
            name, origin_s, num, unit = m.groups()
            origin = int(origin_s, 0)
            length = int(float(num) * unit_mul.get((unit or "B").upper(), 1))
            out[name.upper()] = (origin, length)
    except Exception:
        return {}
    return out


def _read_memory_lengths_from_linker(linker_script: Path) -> dict[str, int]:
    return {name: length for name, (_, length) in _read_memory_regions_from_linker(linker_script).items()}


def _size_row_from_artifact(build_dir: Path) -> Optional[tuple[int, int, int]]:
    elf_file = build_dir / "CH592F.elf"
    cache_file = build_dir / "CMakeCache.txt"
    if not elf_file.is_file() or not cache_file.is_file():
        return None

    cross_size = _parse_cmake_cache_var(cache_file, "CROSS_SIZE")
    if not cross_size:
        return None

    try:
        res = subprocess.run(
            [cross_size, "--format=berkeley", str(elf_file)],
            capture_output=True, text=True, check=True
        )
    except Exception:
        return None

    return _parse_size_row_from_output(res.stdout)


def _artifact_region_usage_from_objdump(build_dir: Path) -> Optional[dict[str, int]]:
    elf_file = build_dir / "CH592F.elf"
    cache_file = build_dir / "CMakeCache.txt"
    if not elf_file.is_file() or not cache_file.is_file():
        return None

    cross_objdump = _parse_cmake_cache_var(cache_file, "CROSS_OBJDUMP")
    if not cross_objdump:
        return None

    regions = _read_memory_regions_from_linker(LINKER_SCRIPT)
    if "FLASH" not in regions or "RAM" not in regions:
        return None

    flash_origin, flash_len = regions["FLASH"]
    ram_origin, ram_len = regions["RAM"]
    flash_end = flash_origin + flash_len
    ram_end = ram_origin + ram_len

    try:
        res = subprocess.run(
            [cross_objdump, "-h", str(elf_file)],
            capture_output=True, text=True, check=True
        )
    except Exception:
        return None

    flash_used = 0
    ram_used = 0
    pending: Optional[tuple[int, int, int]] = None

    for line in res.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 7 and parts[0].isdigit():
            try:
                size = int(parts[2], 16)
                vma = int(parts[3], 16)
                lma = int(parts[4], 16)
                pending = (size, vma, lma)
            except ValueError:
                pending = None
            continue

        if pending is None:
            continue

        if "ALLOC" in line:
            size, vma, lma = pending
            if size > 0:
                if (("LOAD" in line) or ("CONTENTS" in line)) and (flash_origin <= lma < flash_end):
                    flash_used += size
                if ram_origin <= vma < ram_end:
                    ram_used += size
            pending = None
        elif line.strip():
            pending = None

    return {"FLASH": flash_used, "RAM": ram_used}


def _resolve_preset(preset: str) -> str:
    user = FIRMWARE_DIR / "CMakeUserPresets.json"
    if user.is_file():
        try:
            data = json.loads(user.read_text())
            build_presets = {p["name"]: p for p in data.get("buildPresets", [])}
            configure_presets = {p["name"]: p for p in data.get("configurePresets", [])}
            local = f"local-{preset}"
            if local in build_presets:
                configure_name = build_presets[local].get("configurePreset", "")
                if not configure_name:
                    return local
                if _preset_toolchain_exists(configure_name, configure_presets):
                    return local
        except Exception:
            pass
    return preset


def _preset_toolchain_exists(name: str, presets: dict[str, dict], seen: Optional[set[str]] = None) -> bool:
    if not name:
        return False
    if seen is None:
        seen = set()
    if name in seen:
        return False
    seen.add(name)

    preset = presets.get(name)
    if not preset:
        return False

    cache_vars = preset.get("cacheVariables", {})
    toolchain_root = str(cache_vars.get("MRS_TOOLCHAIN_ROOT", "")).strip()
    if toolchain_root:
        return Path(toolchain_root).is_dir()

    inherits = preset.get("inherits", [])
    if isinstance(inherits, str):
        inherits = [inherits]
    for parent in inherits:
        if _preset_toolchain_exists(parent, presets, seen):
            return True
    return False


def _normalize_keyboard(value: str) -> str:
    keyboard = normalize_keyboard_name(value)
    if keyboard == "BASIC":
        die("CH592F does not support BASIC keyboard.")
    return keyboard


def _normalize_profile(value: str) -> str:
    profile = value.strip().lower()
    if profile not in {"release", "debug"}:
        die(f"Unsupported profile: {value}. Expected release or debug.")
    return profile


def preset_for(keyboard: str, profile: str) -> str:
    return f"{_normalize_profile(profile)}-{_normalize_keyboard(keyboard).lower()}"


def build_dir_for(preset: str) -> Path:
    return FIRMWARE_DIR / "build" / _resolve_preset(preset)


def raw_artifact_paths(build_dir: Path) -> dict[str, Path]:
    return {
        "elf": build_dir / "CH592F.elf",
        "bin": build_dir / "CH592F.bin",
        "hex": build_dir / "CH592F.hex",
        "map": build_dir / "CH592F.map",
    }


def artifact_paths(build_dir: Path, keyboard: str) -> dict[str, Path]:
    return {
        "elf": raw_artifact_paths(build_dir)["elf"],
        "bin": build_dir / ch592_filename_for_keyboard(keyboard, "bin"),
        "hex": build_dir / ch592_filename_for_keyboard(keyboard, "hex"),
        "map": raw_artifact_paths(build_dir)["map"],
    }


def export_named_artifacts(build_dir: Path, keyboard: str) -> dict[str, Path]:
    raw = raw_artifact_paths(build_dir)
    exported = artifact_paths(build_dir, keyboard)
    for name in ("bin", "hex"):
        src = raw[name]
        dst = exported[name]
        if src.is_file():
            shutil.copy2(src, dst)
    return exported


def run(cmd: list[str], cwd: Optional[Path] = None) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def run_and_capture(cmd: list[str], cwd: Optional[Path] = None) -> tuple[list[str], float]:
    t0 = time.time()
    lines: list[str] = []
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(cwd) if cwd else None,
        text=True,
        bufsize=1,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.stdout is not None
    for line in proc.stdout:
        print(line, end="")
        lines.append(line)
    proc.wait()
    if proc.returncode != 0:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    return lines, time.time() - t0


def _build_report(lines: list[str], preset: str, build_dir: Path, elapsed: float) -> None:
    mem_re = re.compile(r'(\w+):\s+(\d+)\s+B\s+([\d.]+)\s+(KB|B)\s+([\d.]+)%')
    size_re = re.compile(r'^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9a-fA-F]+\s+\S+\.elf\s*$')

    regions = []
    size_row = None
    report_source = "linker"
    for line in lines:
        m = mem_re.search(line)
        if m:
            name, used_b, tot_v, tot_u, pct = m.groups()
            used = int(used_b)
            total = int(float(tot_v) * 1024) if tot_u == 'KB' else int(float(tot_v))
            regions.append((name, used, total, total - used, float(pct)))
        m2 = size_re.match(line)
        if m2:
            size_row = (int(m2.group(1)), int(m2.group(2)), int(m2.group(3)))

    if size_row is None:
        size_row = _size_row_from_artifact(build_dir)
        report_source = "cached-elf"

    if not regions and size_row is not None:
        mem = _read_memory_lengths_from_linker(LINKER_SCRIPT)
        flash_total = mem.get("FLASH")
        ram_total = mem.get("RAM")
        text, data, bss = size_row
        usage = _artifact_region_usage_from_objdump(build_dir)
        if usage:
            report_source = "cached-elf+objdump"
        flash_used = usage.get("FLASH") if usage else (text + data)
        ram_used = usage.get("RAM") if usage else (data + bss)
        if flash_total and flash_used is not None:
            flash_pct = (flash_used / flash_total * 100) if flash_total else 0.0
            regions.append(("FLASH", flash_used, flash_total, flash_total - flash_used, flash_pct))
        if ram_total and ram_used is not None:
            ram_pct = (ram_used / ram_total * 100) if ram_total else 0.0
            regions.append(("RAM", ram_used, ram_total, ram_total - ram_used, ram_pct))

    if not regions and size_row is None:
        return

    bin_file = raw_artifact_paths(build_dir)["bin"]
    bin_sz = bin_file.stat().st_size if bin_file.is_file() else None

    detail = ""
    if size_row is not None or bin_sz is not None:
        if size_row:
            text, data, bss = size_row
            detail += f".text {_c('36', str(text))}   "
            detail += f".data {_c('33', str(data))}   "
            detail += f".bss  {_c('35', str(bss))}"
        if bin_sz is not None:
            detail += f"   .bin {_c('32;1', fmt_bytes(bin_sz))}"

    render_usage_report(
        title="◆ CH592F Memory Report",
        subtitle=f"preset: {preset}  ·  built in {elapsed:.1f}s  ·  source: {report_source}",
        rows=[UsageRow(name, used, total, free, pct) for name, used, total, free, pct in regions],
        detail_line=detail,
        colorize=_c,
        use_color=_USE_COLOR,
    )


def configure(keyboard: str, profile: str) -> tuple[str, Path]:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or add it to PATH.")
    keyboard = _normalize_keyboard(keyboard)
    profile = _normalize_profile(profile)
    preset = preset_for(keyboard, profile)
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    sep()
    info(f"Configuring CH592F ({keyboard}, {profile})")
    run(
        [
            str(cmake),
            "--preset",
            actual,
        ],
        cwd=FIRMWARE_DIR,
    )
    ok(f"Configure complete → {build_dir}")
    return actual, build_dir


def build(keyboard: str, profile: str) -> tuple[str, Path]:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or add it to PATH.")

    keyboard = _normalize_keyboard(keyboard)
    profile = _normalize_profile(profile)
    preset = preset_for(keyboard, profile)
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    cache_file = build_dir / "CMakeCache.txt"
    cached_keyboard = _parse_cmake_cache_var(cache_file, "KEYBOARD")
    cached_model = _parse_cmake_cache_var(cache_file, "KBD_MODEL")
    if (
        not cache_file.is_file()
        or (cached_keyboard and _normalize_keyboard(cached_keyboard) != keyboard)
        or (cached_model and _normalize_keyboard(cached_model) != keyboard)
    ):
        configure(keyboard, profile)

    sep()
    info(f"Building CH592F ({keyboard}, {profile})")
    lines, elapsed = run_and_capture([str(cmake), "--build", "--preset", actual], cwd=FIRMWARE_DIR)
    _build_report(lines, actual, build_dir, elapsed)

    artifacts = export_named_artifacts(build_dir, keyboard)
    artifacts["elf"] = raw_artifact_paths(build_dir)["elf"]
    artifacts["map"] = raw_artifact_paths(build_dir)["map"]
    for name, path in artifacts.items():
        if path.is_file():
            ok(f"{name.upper()} → {path}")

    return actual, build_dir


def rebuild(keyboard: str, profile: str) -> tuple[str, Path]:
    preset = preset_for(keyboard, profile)
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    if build_dir.is_dir():
        sep()
        info(f"Removing {build_dir}")
        shutil.rmtree(build_dir)
    configure(keyboard, profile)
    return build(keyboard, profile)


def clean(keyboard: str, profile: str) -> None:
    build_dir = build_dir_for(preset_for(keyboard, profile))
    sep()
    if build_dir.is_dir():
        shutil.rmtree(build_dir)
        ok(f"Removed {build_dir}")
    else:
        warn(f"Build directory does not exist: {build_dir}")


def status(keyboard: str, profile: str) -> None:
    preset = preset_for(keyboard, profile)
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    cache_file = build_dir / "CMakeCache.txt"

    sep()
    info(f"Keyboard: {keyboard}")
    info(f"Profile: {profile}")
    info(f"Preset: {preset} (actual: {actual})")
    info(f"CMake: {find_cmake() if find_cmake() else 'not found'}")
    info(f"Build directory: {build_dir}")
    info(f"Configured: {'yes' if cache_file.is_file() else 'no'}")
    raw_artifacts = raw_artifact_paths(build_dir)
    if raw_artifacts["bin"].is_file() or raw_artifacts["hex"].is_file():
        export_named_artifacts(build_dir, keyboard)
    for name, path in artifact_paths(build_dir, keyboard).items():
        if path.is_file():
            info(f"{name.upper()}: {path} ({path.stat().st_size} B)")
        else:
            info(f"{name.upper()}: missing")


def show_artifact(keyboard: str, profile: str, artifact_type: str) -> int:
    preset = preset_for(keyboard, profile)
    build_dir = build_dir_for(preset)
    raw = raw_artifact_paths(build_dir)
    if raw["bin"].is_file() or raw["hex"].is_file():
        export_named_artifacts(build_dir, keyboard)
    artifacts = artifact_paths(build_dir, keyboard)

    if artifact_type == "all":
        sep()
        for name, path in artifacts.items():
            print(f"{name}: {path}")
        return 0

    path = artifacts[artifact_type]
    print(path)
    return 0 if path.is_file() else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="tools/scripts/ch592f.py",
        description="CH592F CMake build helper",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    for name in ("status", "configure", "build", "rebuild", "clean"):
        p = sub.add_parser(name, help=f"{name.capitalize()} CH592F firmware")
        p.add_argument("-k", "--keyboard", default=DEFAULT_KEYBOARD, help="5KEY / KNOB")
        p.add_argument("--profile", default=DEFAULT_PROFILE, help="release / debug")
        p.add_argument("-p", "--preset", dest="preset_legacy", help=argparse.SUPPRESS)

    p_artifact = sub.add_parser("artifact", help="Print artifact path")
    p_artifact.add_argument("-k", "--keyboard", default=DEFAULT_KEYBOARD, help="5KEY / KNOB")
    p_artifact.add_argument("--profile", default=DEFAULT_PROFILE, help="release / debug")
    p_artifact.add_argument("-p", "--preset", dest="preset_legacy", help=argparse.SUPPRESS)
    p_artifact.add_argument(
        "-t",
        "--type",
        default="all",
        choices=("all", "elf", "bin", "hex", "map"),
        help="Artifact type to print",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    keyboard = args.keyboard
    profile = args.profile
    if getattr(args, "preset_legacy", None):
        preset = args.preset_legacy.lower()
        profile = "debug" if preset.startswith("debug-") else "release"
        keyboard = "KNOB" if "knob" in preset else "5KEY"

    try:
        if args.command == "status":
            status(keyboard, profile)
            return 0
        if args.command == "configure":
            configure(keyboard, profile)
            return 0
        if args.command == "build":
            build(keyboard, profile)
            return 0
        if args.command == "rebuild":
            rebuild(keyboard, profile)
            return 0
        if args.command == "clean":
            clean(keyboard, profile)
            return 0
        if args.command == "artifact":
            return show_artifact(keyboard, profile, args.type)
        parser.error(f"Unsupported command: {args.command}")
    except subprocess.CalledProcessError as exc:
        die(f"Command failed (exit {exc.returncode})")
    except KeyboardInterrupt:
        print()
        warn("Interrupted.")
        return 130

    return 0


if __name__ == "__main__":
    sys.exit(main())
