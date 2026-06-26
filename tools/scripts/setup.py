#!/usr/bin/env python3
"""Build the local BinaryKeyboard ISP helper.

This script intentionally does not download upstream ch32-rs/wchisp binaries.
BinaryKeyboard uses the vendored ISP backend in tools/meowisp/vendor-wchisp so
Windows builds can use the CH375 DLL path instead of requiring Zadig/WinUSB.
"""

from __future__ import annotations

import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[2]
MEOWISP_ROOT = PROJECT_ROOT / "tools" / "meowisp"
VENDOR_WCHISP_MANIFEST = MEOWISP_ROOT / "vendor-wchisp" / "Cargo.toml"
TARGET_DIR = MEOWISP_ROOT / "target"
FETCH_WINDOWS_DLL_SCRIPT = MEOWISP_ROOT / "scripts" / "fetch_windows_dll.py"
WINDOWS_DLL_CACHE = MEOWISP_ROOT / ".cache" / "windows-assets" / "CH375DLL64.dll"


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if sys.stdout.isatty() else text


def info(msg: str) -> None:
    print(_c("36", "[INFO]"), msg)


def ok(msg: str) -> None:
    print(_c("32", "[ OK ]"), msg)


def error(msg: str) -> None:
    print(_c("31", "[ERR ]"), msg, file=sys.stderr)


def sep() -> None:
    print(_c("2", "-" * 44))


def binary_name() -> str:
    return "wchisp.exe" if platform.system() == "Windows" else "wchisp"


def release_binary() -> Path:
    return TARGET_DIR / "release" / binary_name()


def current_version(binary: Path) -> str | None:
    try:
        result = subprocess.run(
            [str(binary), "--version"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        return result.stdout.strip() or result.stderr.strip()
    except Exception:
        return None


def require_rust() -> None:
    missing = [tool for tool in ("cargo", "rustc") if shutil.which(tool) is None]
    if not missing:
        return

    joined = ", ".join(missing)
    raise SystemExit(
        "Rust toolchain not found: "
        f"{joined}\n\n"
        "BinaryKeyboard ISP is built from this repository's vendored Rust source.\n"
        "Install Rust with rustup, then open a new terminal and run this again:\n"
        "  https://rustup.rs/\n\n"
        "This script will not install Rust automatically."
    )


def ensure_windows_dll() -> None:
    if platform.system() != "Windows":
        return
    if WINDOWS_DLL_CACHE.is_file():
        return
    if not FETCH_WINDOWS_DLL_SCRIPT.is_file():
        raise SystemExit(f"CH375 DLL fetch script not found: {FETCH_WINDOWS_DLL_SCRIPT}")

    info("Fetching CH375DLL64.dll for Windows CH375 backend...")
    subprocess.run(
        [sys.executable, str(FETCH_WINDOWS_DLL_SCRIPT), "--out", str(WINDOWS_DLL_CACHE)],
        cwd=PROJECT_ROOT,
        check=True,
    )


def build_binarykeyboard_isp() -> Path:
    require_rust()
    ensure_windows_dll()

    if not VENDOR_WCHISP_MANIFEST.is_file():
        raise SystemExit(f"BinaryKeyboard ISP manifest not found: {VENDOR_WCHISP_MANIFEST}")

    env = os.environ.copy()
    if platform.system() == "Windows":
        env.setdefault("WCHISP_CH375_DLL", str(WINDOWS_DLL_CACHE))

    info("Building BinaryKeyboard ISP from local vendored Rust source...")
    subprocess.run(
        [
            "cargo",
            "build",
            "--release",
            "--manifest-path",
            str(VENDOR_WCHISP_MANIFEST),
            "--target-dir",
            str(TARGET_DIR),
        ],
        cwd=PROJECT_ROOT,
        env=env,
        check=True,
    )

    binary = release_binary()
    if not binary.is_file():
        raise SystemExit(f"Build completed but binary was not found: {binary}")
    return binary


def main() -> None:
    sep()
    print(_c("1", "  BinaryKeyboard ISP Setup"))
    sep()

    binary = release_binary()
    if binary.exists() and "--force" not in sys.argv:
        ver = current_version(binary)
        ok(f"BinaryKeyboard ISP already built: {ver or 'unknown version'}")
        info(f"Location: {binary}")
        info("Use --force to rebuild.")
        sep()
        print_next_steps()
        return

    try:
        binary = build_binarykeyboard_isp()
    except subprocess.CalledProcessError as exc:
        error(f"BinaryKeyboard ISP build failed (exit {exc.returncode})")
        sys.exit(exc.returncode)

    ver = current_version(binary)
    sep()
    ok(f"BinaryKeyboard ISP ready: {ver or 'unknown version'}")
    ok(f"Location: {binary}")
    sep()
    print_next_steps()


def print_next_steps() -> None:
    print(_c("1", "  Next steps:"))
    print("  1. Connect target via USB while holding BOOT button")
    print("  2. Probe:")
    print("     python tools/scripts/flash.py probe")
    print("  3. Flash artifact:")
    print("     python tools/scripts/flash.py flash --file <firmware-artifact.bin>")
    sep()


if __name__ == "__main__":
    main()
