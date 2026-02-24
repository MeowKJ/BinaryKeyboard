#!/usr/bin/env python3
"""Root-level entry point — delegates to tools/scripts/flash.py"""
import sys
import subprocess
from pathlib import Path

script = Path(__file__).parent / "tools" / "scripts" / "flash.py"
sys.exit(subprocess.run([sys.executable, str(script)] + sys.argv[1:]).returncode)
