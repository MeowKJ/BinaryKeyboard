#!/usr/bin/env python3
"""Shared utilities for BinaryKeyboard build scripts."""

from __future__ import annotations

import ctypes
import os
import platform
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional

from tool_cache import resolve_tool_path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent.resolve()

# ── ANSI color helpers ──────────────────────────────────────────────────────


def _enable_win_ansi() -> None:
    if platform.system() == "Windows":
        try:
            kernel32 = ctypes.windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except Exception:
            pass


_enable_win_ansi()
_USE_COLOR = sys.stdout.isatty()


def colorize(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text


def info(msg: str) -> None:
    print(colorize("36", "[INFO]"), msg)


def ok(msg: str) -> None:
    print(colorize("32", "[ OK ]"), msg)


def warn(msg: str) -> None:
    print(colorize("33", "[WARN]"), msg)


def sep() -> None:
    print(colorize("2", "-" * 44))


def die(msg: str) -> None:
    print(colorize("31", "[ERR ]"), msg, file=sys.stderr)
    sys.exit(1)


def use_color() -> bool:
    return _USE_COLOR


# ── Display helpers ─────────────────────────────────────────────────────────


def display_path(path: Path) -> str:
    try:
        return str(path.relative_to(PROJECT_ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


# ── Subprocess wrappers ────────────────────────────────────────────────────


def run(cmd: list[str], cwd: Optional[Path] = None, env: Optional[dict] = None) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True, env=env)


def run_and_capture(
    cmd: list[str], cwd: Optional[Path] = None, env: Optional[dict] = None
) -> tuple[list[str], float]:
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
        env=env,
    )
    assert proc.stdout is not None
    for line in proc.stdout:
        print(line, end="")
        lines.append(line)
    proc.wait()
    if proc.returncode != 0:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    return lines, time.time() - t0


# ── CMake cache parser ─────────────────────────────────────────────────────


def parse_cmake_cache_var(cache_file: Path, key: str) -> Optional[str]:
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


# ── Tool discovery (cmake / ninja / sdcc) ──────────────────────────────────


def _candidate_windows_cmake_paths() -> list[Path]:
    home = Path.home()
    candidates = [
        Path(r"C:\Program Files\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files (x86)\CMake\bin\cmake.exe"),
        Path(r"C:\ProgramData\chocolatey\bin\cmake.exe"),
        home / "scoop" / "shims" / "cmake.exe",
        home / "scoop" / "apps" / "cmake" / "current" / "bin" / "cmake.exe",
        Path(r"C:\msys64\ucrt64\bin\cmake.exe"),
        Path(r"C:\msys64\mingw64\bin\cmake.exe"),
        Path(r"C:\msys64\usr\bin\cmake.exe"),
        Path(r"C:\Tools\CMake\bin\cmake.exe"),
        Path(r"C:\App\Tools\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
    ]
    return candidates


def _candidate_windows_ninja_paths() -> list[Path]:
    candidates = [
        Path(r"C:\Program Files\CMake\bin\ninja.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"),
        Path(r"C:\App\IDE\Microsoft Visual Studio\18\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"),
        Path(r"C:\App\IDE\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"),
    ]
    local_appdata = os.environ.get("LOCALAPPDATA")
    if local_appdata:
        winget_root = Path(local_appdata) / "Microsoft" / "WinGet" / "Packages"
        if winget_root.is_dir():
            candidates.extend(sorted(winget_root.glob("Ninja-build.Ninja_*/ninja.exe")))
    return candidates


def _candidate_windows_sdcc_paths() -> list[Path]:
    home = Path.home()
    return [
        Path(r"C:\App\Environment\SDCC\bin\sdcc.exe"),
        Path(r"C:\Program Files\SDCC\bin\sdcc.exe"),
        Path(r"C:\Program Files (x86)\SDCC\bin\sdcc.exe"),
        Path(r"C:\tools\sdcc\bin\sdcc.exe"),
        home / "scoop" / "apps" / "sdcc" / "current" / "bin" / "sdcc.exe",
        Path(r"C:\msys64\ucrt64\bin\sdcc.exe"),
        Path(r"C:\msys64\mingw64\bin\sdcc.exe"),
    ]


def find_cmake() -> Optional[Path]:
    binary = "cmake.exe" if platform.system() == "Windows" else "cmake"
    candidates = _candidate_windows_cmake_paths() if platform.system() == "Windows" else []
    return resolve_tool_path("cmake", binary, env_name="CMAKE_PATH", candidates=candidates)


def find_ninja() -> Optional[Path]:
    binary = "ninja.exe" if platform.system() == "Windows" else "ninja"
    candidates = _candidate_windows_ninja_paths() if platform.system() == "Windows" else []
    return resolve_tool_path("ninja", binary, env_name="NINJA_PATH", candidates=candidates)


def find_sdcc() -> Optional[Path]:
    binary = "sdcc.exe" if platform.system() == "Windows" else "sdcc"
    candidates = _candidate_windows_sdcc_paths() if platform.system() == "Windows" else []
    return resolve_tool_path("sdcc", binary, env_name="SDCC_PATH", candidates=candidates)


def find_meowisp() -> Optional[Path]:
    binary = "meowisp.exe" if platform.system() == "Windows" else "meowisp"
    meowisp_root = PROJECT_ROOT / "tools" / "meowisp"
    preferred = [
        meowisp_root / "target" / "debug" / binary,
        meowisp_root / "target" / "release" / binary,
        SCRIPT_DIR / binary,
    ]
    return resolve_tool_path(
        "meowisp",
        binary,
        env_name="MEOWISP_PATH",
        preferred_candidates=preferred,
    )


