#!/usr/bin/env python3
"""
tools/scripts/ch552g.py — CH552G CMake build helper
"""

from __future__ import annotations

import argparse
import ctypes
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent.resolve()
FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH552G"
BUILD_ROOT = FIRMWARE_DIR / "build"

VALID_VARIANTS = ("BASIC", "KNOB", "5KEYS")
DEFAULT_VARIANT = "BASIC"


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


def _normalize_variant(value: str) -> str:
    variant = value.strip().upper()
    if variant not in VALID_VARIANTS:
        die(f"Unsupported variant: {value}. Expected one of: {', '.join(VALID_VARIANTS)}")
    return variant


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


def build_dir_for(variant: str) -> Path:
    return BUILD_ROOT / variant.lower()


def artifact_paths(build_dir: Path) -> dict[str, Path]:
    return {
        "ihx": build_dir / "CH552G.ihx",
        "hex": build_dir / "CH552G.hex",
        "bin": build_dir / "CH552G.bin",
    }


def run(cmd: list[str], cwd: Optional[Path] = None) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def cmake_configure_args(variant: str, build_dir: Path) -> list[str]:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or set CMAKE_PATH to the cmake executable.")

    args = [
        str(cmake),
        "-S",
        str(FIRMWARE_DIR),
        "-B",
        str(build_dir),
        f"-DKBD_VARIANT={variant}",
    ]

    sdcc = find_sdcc()
    if sdcc:
        args.append(f"-DSDCC_ROOT={sdcc.parent.parent}")

    return args


def configure(variant: str) -> Path:
    build_dir = build_dir_for(variant)
    build_dir.mkdir(parents=True, exist_ok=True)
    sep()
    info(f"Configuring CH552G ({variant})")
    run(cmake_configure_args(variant, build_dir), cwd=PROJECT_ROOT)
    ok(f"Configure complete → {build_dir}")
    return build_dir


def build(variant: str) -> Path:
    build_dir = build_dir_for(variant)
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.is_file():
        configure(variant)

    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or set CMAKE_PATH to the cmake executable.")

    sep()
    info(f"Building CH552G ({variant})")
    run([str(cmake), "--build", str(build_dir)], cwd=PROJECT_ROOT)

    artifacts = artifact_paths(build_dir)
    for name, path in artifacts.items():
        if path.is_file():
            ok(f"{name.upper()} → {path}")

    return build_dir


def rebuild(variant: str) -> Path:
    build_dir = build_dir_for(variant)
    if build_dir.is_dir():
        sep()
        info(f"Removing {build_dir}")
        shutil.rmtree(build_dir)
    return build(variant)


def clean(variant: str) -> None:
    build_dir = build_dir_for(variant)
    sep()
    if build_dir.is_dir():
        shutil.rmtree(build_dir)
        ok(f"Removed {build_dir}")
    else:
        warn(f"Build directory does not exist: {build_dir}")


def status(variant: str) -> None:
    build_dir = build_dir_for(variant)
    cache_file = build_dir / "CMakeCache.txt"
    cmake = find_cmake()
    sdcc = find_sdcc()

    sep()
    info(f"Variant: {variant}")
    info(f"CMake: {cmake if cmake else 'not found'}")
    info(f"SDCC:  {sdcc if sdcc else 'not found'}")
    info(f"Build directory: {build_dir}")
    info(f"Configured: {'yes' if cache_file.is_file() else 'no'}")

    for name, path in artifact_paths(build_dir).items():
        if path.is_file():
            info(f"{name.upper()}: {path} ({path.stat().st_size} B)")
        else:
            info(f"{name.upper()}: missing")


def show_artifact(variant: str, artifact_type: str) -> int:
    build_dir = build_dir_for(variant)
    artifacts = artifact_paths(build_dir)

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
        p.add_argument("-v", "--variant", default=DEFAULT_VARIANT, help="BASIC / KNOB / 5KEYS")

    p_artifact = sub.add_parser("artifact", help="Print artifact path")
    p_artifact.add_argument("-v", "--variant", default=DEFAULT_VARIANT, help="BASIC / KNOB / 5KEYS")
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
    variant = _normalize_variant(args.variant)

    try:
        if args.command == "status":
            status(variant)
            return 0
        if args.command == "configure":
            configure(variant)
            return 0
        if args.command == "build":
            build(variant)
            return 0
        if args.command == "rebuild":
            rebuild(variant)
            return 0
        if args.command == "clean":
            clean(variant)
            return 0
        if args.command == "artifact":
            return show_artifact(variant, args.type)
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
