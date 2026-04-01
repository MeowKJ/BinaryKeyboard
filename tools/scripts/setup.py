#!/usr/bin/env python3
"""
setup.py — BinaryKeyboard development environment setup
Builds MeowISP from source (tools/meowisp/) via cargo.
"""

import platform
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
MEOWISP_DIR = PROJECT_ROOT / "tools" / "meowisp"


# ── Terminal colors ────────────────────────────────────────────────────────────
def _c(code, text): return f"\033[{code}m{text}\033[0m" if sys.stdout.isatty() else text
def info(msg):    print(_c("36", "[INFO]"), msg)
def ok(msg):      print(_c("32", "[ OK ]"), msg)
def warn(msg):    print(_c("33", "[WARN]"), msg)
def error(msg):   print(_c("31", "[ERR ]"), msg, file=sys.stderr)
def sep():        print(_c("2", "-" * 44))


def _binary_path() -> Path:
    name = "meowisp.exe" if platform.system() == "Windows" else "meowisp"
    return MEOWISP_DIR / "target" / "debug" / name


def _current_version(binary: Path) -> str:
    try:
        result = subprocess.run(
            [str(binary), "--version"],
            capture_output=True, text=True, timeout=5
        )
        return (result.stdout or result.stderr).strip()
    except Exception:
        return ""


def _check_cargo() -> bool:
    if shutil.which("cargo"):
        return True
    error("cargo not found in PATH.")
    error("Install Rust toolchain: https://rustup.rs/")
    return False


def _fetch_windows_dll() -> None:
    dll_cache = MEOWISP_DIR / ".cache" / "windows-assets" / "CH375DLL64.dll"
    if dll_cache.is_file():
        ok("CH375DLL64.dll already cached")
        return
    fetch_script = MEOWISP_DIR / "scripts" / "fetch_windows_dll.py"
    if not fetch_script.is_file():
        warn(f"DLL fetch script not found: {fetch_script}")
        return
    info("Fetching Windows CH375 DLL...")
    result = subprocess.run([sys.executable, str(fetch_script)], cwd=PROJECT_ROOT, check=False)
    if result.returncode != 0:
        warn("DLL fetch failed — meowisp may not work with CH375 USB adapters")


def _build_meowisp() -> bool:
    info("Building meowisp via cargo...")
    result = subprocess.run(
        [
            "cargo", "build",
            "--manifest-path", str(MEOWISP_DIR / "Cargo.toml"),
            "--bin", "meowisp",
        ],
        cwd=str(PROJECT_ROOT),
        check=False,
    )
    return result.returncode == 0


# ── Main ───────────────────────────────────────────────────────────────────────
def main():
    sep()
    print(_c("1", "  BinaryKeyboard — Development Setup"))
    sep()

    binary = _binary_path()
    force = "--force" in sys.argv

    if binary.is_file() and not force:
        ver = _current_version(binary)
        ok(f"meowisp already built: {ver or 'unknown version'}")
        info(f"Location: {binary}")
        info("Use --force to rebuild.")
        sep()
        print_next_steps()
        return

    if not _check_cargo():
        sys.exit(1)

    if platform.system() == "Windows":
        _fetch_windows_dll()

    if not _build_meowisp():
        error("meowisp build failed.")
        sys.exit(1)

    if not binary.is_file():
        error("Build reported success but binary not found.")
        sys.exit(1)

    ver = _current_version(binary)
    sep()
    ok(f"meowisp built: {ver or 'ok'}")
    ok(f"Location: {binary}")
    sep()
    print_next_steps()


def print_next_steps():
    print(_c("1", "  Next steps:"))
    print("  1. Build firmware:")
    print("     CH592F: python tools/scripts/ch592f.py build --keyboard 5KEY --profile release")
    print("     CH552G: python tools/scripts/ch552g.py build --keyboard BASIC")
    print("  2. Connect target via USB while holding BOOT button")
    print("  3. Flash artifact:")
    print("     python tools/scripts/flash.py flash --file <firmware-artifact.bin>")
    sep()


if __name__ == "__main__":
    main()
