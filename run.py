#!/usr/bin/env python3
"""Cross-platform launcher for the BinaryKeyboard console."""

from pathlib import Path
import os
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parent
CONSOLE_SCRIPT = PROJECT_ROOT / "tools" / "scripts" / "console.py"


def main() -> None:
    if not CONSOLE_SCRIPT.is_file():
        raise SystemExit(f"Console entry not found: {CONSOLE_SCRIPT}")

    argv = [sys.executable, str(CONSOLE_SCRIPT), *sys.argv[1:]]

    if os.name == "nt":
        completed = subprocess.run(argv, cwd=str(PROJECT_ROOT), check=False)
        raise SystemExit(completed.returncode)

    os.chdir(PROJECT_ROOT)
    os.execv(sys.executable, argv)


if __name__ == "__main__":
    main()
