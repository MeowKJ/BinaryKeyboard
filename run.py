#!/usr/bin/env python3
"""Cross-platform launcher for the BinaryKeyboard console."""

from pathlib import Path
import os
import shutil
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parent
CONSOLE_SCRIPT = PROJECT_ROOT / "tools" / "scripts" / "console.py"
ISP_BINARY = (
    PROJECT_ROOT
    / "tools"
    / "meowisp"
    / "target"
    / "release"
    / ("wchisp.exe" if os.name == "nt" else "wchisp")
)


def _warn_missing_rust_for_isp() -> None:
    if ISP_BINARY.is_file():
        return
    missing = [tool for tool in ("cargo", "rustc") if shutil.which(tool) is None]
    if not missing:
        return
    print(
        "[run.py] BinaryKeyboard ISP is not built and Rust is missing: "
        + ", ".join(missing),
        file=sys.stderr,
    )
    print(
        "[run.py] Install Rust from https://rustup.rs/ and reopen the terminal. "
        "Rust is not installed automatically.",
        file=sys.stderr,
    )


def main() -> None:
    if not CONSOLE_SCRIPT.is_file():
        raise SystemExit(f"Console entry not found: {CONSOLE_SCRIPT}")

    _warn_missing_rust_for_isp()
    argv = [sys.executable, str(CONSOLE_SCRIPT), *sys.argv[1:]]

    if os.name == "nt":
        completed = subprocess.run(argv, cwd=str(PROJECT_ROOT), check=False)
        raise SystemExit(completed.returncode)

    os.chdir(PROJECT_ROOT)
    os.execv(sys.executable, argv)


if __name__ == "__main__":
    main()
