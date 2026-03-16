#!/usr/bin/env python3
"""
tools/scripts/ch592f.py — CH592F CMake build helper
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

from build_report import UsageRow, fmt_bytes, render_usage_report
from common import (
    PROJECT_ROOT,
    colorize as _c,
    die,
    find_cmake,
    find_ninja,
    info,
    ok,
    parse_cmake_cache_var as _parse_cmake_cache_var,
    run as _run,
    run_and_capture as _run_and_capture,
    sep,
    use_color,
    warn,
)
from firmware_naming import ch592_filename_for_keyboard, normalize_keyboard_name


FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
LINKER_SCRIPT = FIRMWARE_DIR / "SDK" / "Ld" / "Link.ld"
DEFAULT_KEYBOARD = "5KEY"
DEFAULT_PROFILE = "release"


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


def build_dir_for(keyboard: str, profile: str) -> Path:
    return FIRMWARE_DIR / "build" / preset_for(keyboard, profile)


def raw_artifact_paths(build_dir: Path) -> dict[str, Path]:
    return {
        "elf": build_dir / "CH592F.elf",
        "bin": build_dir / "CH592F.bin",
        "hex": build_dir / "CH592F.hex",
        "map": build_dir / "CH592F.map",
    }


def artifact_paths(build_dir: Path, keyboard: str) -> dict[str, Path]:
    return {
        "elf": build_dir / ch592_filename_for_keyboard(keyboard, "elf"),
        "bin": build_dir / ch592_filename_for_keyboard(keyboard, "bin"),
        "hex": build_dir / ch592_filename_for_keyboard(keyboard, "hex"),
        "map": build_dir / ch592_filename_for_keyboard(keyboard, "map"),
    }


def export_named_artifacts(build_dir: Path, keyboard: str) -> dict[str, Path]:
    raw = raw_artifact_paths(build_dir)
    exported = artifact_paths(build_dir, keyboard)
    for name in ("bin", "hex", "elf", "map"):
        src = raw[name]
        dst = exported[name]
        if src.is_file() and src != dst:
            shutil.copy2(src, dst)
    return exported


def _build_env() -> dict[str, str]:
    env = os.environ.copy()
    ninja = find_ninja()
    if ninja:
        ninja_bin = str(ninja.parent.resolve())
        path_value = env.get("PATH", "")
        path_parts = path_value.split(os.pathsep) if path_value else []
        if ninja_bin not in path_parts:
            env["PATH"] = os.pathsep.join([ninja_bin, *path_parts]) if path_parts else ninja_bin
    return env


def _generator_configure_args() -> list[str]:
    """Return CMake generator flags (Ninja + CMAKE_MAKE_PROGRAM on Windows)."""
    args = ["-G", "Ninja"]
    if platform.system() == "Windows":
        ninja = find_ninja()
        if not ninja:
            die(
                "Ninja not found. Install Ninja or set NINJA_PATH to ninja.exe.\n"
                "CH592F builds use the Ninja generator."
            )
        args.append(f"-DCMAKE_MAKE_PROGRAM={ninja}")
    return args


def cmake_configure_args(cmake: Path, keyboard: str, profile: str, build_dir: Path) -> list[str]:
    build_type = {"release": "MinSizeRel", "debug": "Debug"}[profile]
    return [
        str(cmake),
        "-S", str(FIRMWARE_DIR),
        "-B", str(build_dir),
        *_generator_configure_args(),
        f"-DCMAKE_TOOLCHAIN_FILE={FIRMWARE_DIR / 'cmake' / 'toolchain-ch59x.cmake'}",
        f"-DKEYBOARD={keyboard}",
        f"-DKBD_MODEL={keyboard}",
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]


def run(cmd: list[str], cwd: Optional[Path] = None) -> None:
    _run(cmd, cwd=cwd, env=_build_env())


def run_and_capture(cmd: list[str], cwd: Optional[Path] = None) -> tuple[list[str], float]:
    return _run_and_capture(cmd, cwd=cwd, env=_build_env())


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
        subtitle=f"preset: {preset}  ·  built in {elapsed:.1f}s",
        rows=[UsageRow(name, used, total, free, pct) for name, used, total, free, pct in regions],
        detail_line=detail,
        colorize=_c,
        use_color=use_color(),
    )


def configure(keyboard: str, profile: str) -> Path:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or add it to PATH.")
    keyboard = _normalize_keyboard(keyboard)
    profile = _normalize_profile(profile)
    build_dir = build_dir_for(keyboard, profile)
    build_dir.mkdir(parents=True, exist_ok=True)
    sep()
    info(f"Configuring CH592F ({keyboard}, {profile})")
    run(
        cmake_configure_args(cmake, keyboard, profile, build_dir),
        cwd=PROJECT_ROOT,
    )
    ok(f"Configure complete → {build_dir}")
    return build_dir


def build(keyboard: str, profile: str) -> Path:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or add it to PATH.")

    keyboard = _normalize_keyboard(keyboard)
    profile = _normalize_profile(profile)
    build_dir = build_dir_for(keyboard, profile)
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
    lines, elapsed = run_and_capture([str(cmake), "--build", str(build_dir)], cwd=PROJECT_ROOT)
    _build_report(lines, preset_for(keyboard, profile), build_dir, elapsed)

    artifacts = export_named_artifacts(build_dir, keyboard)
    for name, path in artifacts.items():
        if path.is_file():
            ok(f"{name.upper()} → {path.parent}{os.sep}{_c('32;1', path.name)}")

    return build_dir


def rebuild(keyboard: str, profile: str) -> Path:
    keyboard = _normalize_keyboard(keyboard)
    profile = _normalize_profile(profile)
    build_dir = build_dir_for(keyboard, profile)
    if build_dir.is_dir():
        sep()
        info(f"Removing {build_dir}")
        shutil.rmtree(build_dir)
    configure(keyboard, profile)
    return build(keyboard, profile)


def clean(keyboard: str, profile: str) -> None:
    build_dir = build_dir_for(keyboard, profile)
    sep()
    if build_dir.is_dir():
        shutil.rmtree(build_dir)
        ok(f"Removed {build_dir}")
    else:
        warn(f"Build directory does not exist: {build_dir}")


def status(keyboard: str, profile: str) -> None:
    build_dir = build_dir_for(keyboard, profile)
    cache_file = build_dir / "CMakeCache.txt"

    sep()
    info(f"Keyboard: {keyboard}")
    info(f"Profile: {profile}")
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
    build_dir = build_dir_for(keyboard, profile)
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


def emit_size_json(keyboard: str, profile: str, out: Path) -> int:
    """Write build size metrics to a JSON file for CI consumption."""
    build_dir = build_dir_for(keyboard, profile)
    mem = _read_memory_lengths_from_linker(LINKER_SCRIPT)
    usage = _artifact_region_usage_from_objdump(build_dir)
    if not usage:
        size_row = _size_row_from_artifact(build_dir)
        if size_row:
            text, data, bss = size_row
            usage = {"FLASH": text + data, "RAM": data + bss}

    if not usage:
        warn(f"Cannot determine size for {keyboard} ({profile})")
        return 1

    result: dict = {"chip": "CH592F", "keyboard": keyboard, "regions": {}}
    for region in ("FLASH", "RAM"):
        used = usage.get(region)
        total = mem.get(region)
        if used is not None and total:
            result["regions"][region] = {"used": used, "total": total}

    bin_file = raw_artifact_paths(build_dir)["bin"]
    if bin_file.is_file():
        result["bin_size"] = bin_file.stat().st_size

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(result, indent=2))
    ok(f"Size JSON → {out}")
    return 0


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

    p_artifact = sub.add_parser("artifact", help="Print artifact path")
    p_artifact.add_argument("-k", "--keyboard", default=DEFAULT_KEYBOARD, help="5KEY / KNOB")
    p_artifact.add_argument("--profile", default=DEFAULT_PROFILE, help="release / debug")
    p_artifact.add_argument(
        "-t",
        "--type",
        default="all",
        choices=("all", "elf", "bin", "hex", "map"),
        help="Artifact type to print",
    )

    p_size = sub.add_parser("size-json", help="Emit build size report as JSON")
    p_size.add_argument("-k", "--keyboard", default=DEFAULT_KEYBOARD, help="5KEY / KNOB")
    p_size.add_argument("--profile", default=DEFAULT_PROFILE, help="release / debug")
    p_size.add_argument("-o", "--out", required=True, type=Path, help="Output JSON path")

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    keyboard = args.keyboard
    profile = args.profile

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
        if args.command == "size-json":
            return emit_size_json(keyboard, profile, args.out)
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
