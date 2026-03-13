#!/usr/bin/env python3
"""
tools/scripts/ch552g.py — CH552G CMake build helper
"""

from __future__ import annotations

import argparse
import ctypes
import os
import platform
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional

from build_report import UsageRow, fmt_bytes, render_usage_report
from firmware_naming import ch552_filename_for_keyboard, normalize_keyboard_name


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent.resolve()
FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH552G"
BUILD_ROOT = FIRMWARE_DIR / "build"

VALID_KEYBOARDS = ("BASIC", "KNOB", "5KEY")
DEFAULT_KEYBOARD = "BASIC"


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


def _normalize_keyboard(value: str) -> str:
    try:
        return normalize_keyboard_name(value)
    except ValueError:
        die(f"Unsupported keyboard: {value}. Expected one of: {', '.join(VALID_KEYBOARDS)}")
    return DEFAULT_KEYBOARD


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


def _candidate_windows_sdcc_paths() -> list[Path]:
    return [
        Path(r"C:\Program Files\SDCC\bin\sdcc.exe"),
        Path(r"C:\Program Files (x86)\SDCC\bin\sdcc.exe"),
        Path(r"C:\tools\sdcc\bin\sdcc.exe"),
        Path(r"C:\Users\KJ\scoop\apps\sdcc\current\bin\sdcc.exe"),
        Path(r"C:\msys64\ucrt64\bin\sdcc.exe"),
        Path(r"C:\msys64\mingw64\bin\sdcc.exe"),
    ]


def _find_tool(env_name: str, binary_name: str, win_candidates: list[Path]) -> Optional[Path]:
    env_path = os.environ.get(env_name)
    if env_path:
        p = Path(env_path)
        if p.is_file():
            return p.resolve()
        candidate = p / binary_name
        if candidate.is_file():
            return candidate.resolve()

    found = shutil.which(binary_name)
    if found:
        return Path(found).resolve()

    if platform.system() == "Windows":
        for candidate in win_candidates:
            if candidate.is_file():
                return candidate.resolve()

    return None


def find_cmake() -> Optional[Path]:
    binary = "cmake.exe" if platform.system() == "Windows" else "cmake"
    return _find_tool("CMAKE_PATH", binary, _candidate_windows_cmake_paths())


def find_sdcc() -> Optional[Path]:
    binary = "sdcc.exe" if platform.system() == "Windows" else "sdcc"
    return _find_tool("SDCC_PATH", binary, _candidate_windows_sdcc_paths())


def build_dir_for(keyboard: str) -> Path:
    keyboard = _normalize_keyboard(keyboard)
    return BUILD_ROOT / {"BASIC": "basic", "KNOB": "knob", "5KEY": "5keys"}[keyboard]


def raw_artifact_paths(build_dir: Path) -> dict[str, Path]:
    return {
        "ihx": build_dir / "CH552G.ihx",
        "hex": build_dir / "CH552G.hex",
        "bin": build_dir / "CH552G.bin",
    }


def artifact_paths(build_dir: Path, keyboard: str) -> dict[str, Path]:
    return {
        "ihx": raw_artifact_paths(build_dir)["ihx"],
        "hex": build_dir / ch552_filename_for_keyboard(keyboard, "hex"),
        "bin": build_dir / ch552_filename_for_keyboard(keyboard, "bin"),
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


def _parse_mem_report(mem_file: Path) -> tuple[list[UsageRow], str]:
    if not mem_file.is_file():
        return ([], "")

    rows: list[UsageRow] = []
    detail_parts: list[str] = []
    stack_re = re.compile(r"Stack starts at:\s*(0x[0-9a-fA-F]+).*?with\s+(\d+)\s+bytes available")
    table_re = re.compile(r"^\s*(.+?)\s+(0x[0-9A-Fa-f]+|\d+)?\s+(0x[0-9A-Fa-f]+|\d+)?\s+(\d+)\s+(\d+)\s*$")

    in_other_memory = False
    for raw in mem_file.read_text(errors="ignore").splitlines():
        line = raw.rstrip()

        m_stack = stack_re.search(line)
        if m_stack:
            stack_start = m_stack.group(1)
            stack_free = int(m_stack.group(2))
            detail_parts.append(f"stack { _c('32;1', stack_start) }  free {_c('32;1', fmt_bytes(stack_free))}")
            continue

        if "No spare internal RAM space left." in line:
            detail_parts.append(_c("31;1", "internal RAM full"))
            continue

        if line.startswith("Other memory:"):
            in_other_memory = True
            continue

        if not in_other_memory:
            continue
        if not line.strip() or line.strip().startswith("Name") or set(line.strip()) == {"-"}:
            continue

        m = table_re.match(line)
        if not m:
            continue

        name = " ".join(m.group(1).split())
        used = int(m.group(4))
        total = int(m.group(5))
        free = max(0, total - used)
        pct = (used / total * 100.0) if total else 0.0

        display_name = {
            "ROM/EPROM/FLASH": "FLASH",
            "EXTERNAL RAM": "XRAM",
            "PAGED EXT. RAM": "PXRAM",
        }.get(name, name)
        rows.append(UsageRow(display_name, used, total, free, pct))

    return (rows, "   ".join(detail_parts))


def _build_report(keyboard: str, build_dir: Path, elapsed: float) -> None:
    artifacts = raw_artifact_paths(build_dir)
    mem_rows, detail = _parse_mem_report(build_dir / "CH552G.ihx.mem")
    bin_file = artifacts["bin"]
    if bin_file.is_file():
        suffix = f".bin {_c('32;1', fmt_bytes(bin_file.stat().st_size))}"
        detail = f"{detail}   {suffix}".strip() if detail else suffix

    if not mem_rows and not detail:
        return

    render_usage_report(
        title="◆ CH552G Build Report",
        subtitle=f"keyboard: {keyboard}  ·  built in {elapsed:.1f}s  ·  source: sdcc mem",
        rows=mem_rows,
        detail_line=detail,
        colorize=_c,
        use_color=_USE_COLOR,
    )


def cmake_configure_args(keyboard: str, build_dir: Path) -> list[str]:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or set CMAKE_PATH to the cmake executable.")

    args = [
        str(cmake),
        "-S",
        str(FIRMWARE_DIR),
        "-B",
        str(build_dir),
        f"-DKEYBOARD={keyboard}",
    ]

    sdcc = find_sdcc()
    if sdcc:
        args.append(f"-DSDCC_ROOT={sdcc.parent.parent}")

    return args


def configure(keyboard: str) -> Path:
    build_dir = build_dir_for(keyboard)
    build_dir.mkdir(parents=True, exist_ok=True)
    sep()
    info(f"Configuring CH552G ({keyboard})")
    run(cmake_configure_args(keyboard, build_dir), cwd=PROJECT_ROOT)
    ok(f"Configure complete → {build_dir}")
    return build_dir


def build(keyboard: str) -> Path:
    build_dir = build_dir_for(keyboard)
    cache_file = build_dir / "CMakeCache.txt"
    cached_keyboard = _parse_cmake_cache_var(cache_file, "KEYBOARD")
    if (not cache_file.is_file()) or (cached_keyboard and _normalize_keyboard(cached_keyboard) != keyboard):
        configure(keyboard)

    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or set CMAKE_PATH to the cmake executable.")

    sep()
    info(f"Building CH552G ({keyboard})")
    _, elapsed = run_and_capture([str(cmake), "--build", str(build_dir)], cwd=PROJECT_ROOT)
    _build_report(keyboard, build_dir, elapsed)

    artifacts = export_named_artifacts(build_dir, keyboard)
    artifacts["ihx"] = raw_artifact_paths(build_dir)["ihx"]
    for name, path in artifacts.items():
        if path.is_file():
            ok(f"{name.upper()} → {path}")

    return build_dir


def rebuild(keyboard: str) -> Path:
    build_dir = build_dir_for(keyboard)
    if build_dir.is_dir():
        sep()
        info(f"Removing {build_dir}")
        shutil.rmtree(build_dir)
    return build(keyboard)


def clean(keyboard: str) -> None:
    build_dir = build_dir_for(keyboard)
    sep()
    if build_dir.is_dir():
        shutil.rmtree(build_dir)
        ok(f"Removed {build_dir}")
    else:
        warn(f"Build directory does not exist: {build_dir}")


def status(keyboard: str) -> None:
    build_dir = build_dir_for(keyboard)
    cache_file = build_dir / "CMakeCache.txt"
    cmake = find_cmake()
    sdcc = find_sdcc()

    sep()
    info(f"Keyboard: {keyboard}")
    info(f"CMake: {cmake if cmake else 'not found'}")
    info(f"SDCC:  {sdcc if sdcc else 'not found'}")
    info(f"Build directory: {build_dir}")
    info(f"Configured: {'yes' if cache_file.is_file() else 'no'}")

    artifacts = artifact_paths(build_dir, keyboard)
    raw_artifacts = raw_artifact_paths(build_dir)
    if raw_artifacts["bin"].is_file() or raw_artifacts["hex"].is_file():
        export_named_artifacts(build_dir, keyboard)
    for name, path in artifacts.items():
        if path.is_file():
            info(f"{name.upper()}: {path} ({path.stat().st_size} B)")
        else:
            info(f"{name.upper()}: missing")


def show_artifact(keyboard: str, artifact_type: str) -> int:
    build_dir = build_dir_for(keyboard)
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
        prog="tools/scripts/ch552g.py",
        description="CH552G CMake build helper",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    for name in ("status", "configure", "build", "rebuild", "clean"):
        p = sub.add_parser(name, help=f"{name.capitalize()} CH552G firmware")
        p.add_argument("-k", "--keyboard", default=DEFAULT_KEYBOARD, help="BASIC / KNOB / 5KEY")
        p.add_argument("-v", "--variant", dest="keyboard_legacy", help=argparse.SUPPRESS)

    p_artifact = sub.add_parser("artifact", help="Print artifact path")
    p_artifact.add_argument("-k", "--keyboard", default=DEFAULT_KEYBOARD, help="BASIC / KNOB / 5KEY")
    p_artifact.add_argument("-v", "--variant", dest="keyboard_legacy", help=argparse.SUPPRESS)
    p_artifact.add_argument(
        "-t",
        "--type",
        default="all",
        choices=("all", "ihx", "hex", "bin"),
        help="Artifact type to print",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    keyboard = _normalize_keyboard(getattr(args, "keyboard_legacy", None) or args.keyboard)

    try:
        if args.command == "status":
            status(keyboard)
            return 0
        if args.command == "configure":
            configure(keyboard)
            return 0
        if args.command == "build":
            build(keyboard)
            return 0
        if args.command == "rebuild":
            rebuild(keyboard)
            return 0
        if args.command == "clean":
            clean(keyboard)
            return 0
        if args.command == "artifact":
            return show_artifact(keyboard, args.type)
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
