#!/usr/bin/env python3
"""
tools/scripts/flash.py — WCH CH592F ISP tool wrapper
Cross-platform (macOS / Linux / Windows)
Supports: flash, build, verify, erase, reset, info, probe, eeprom, config
"""

import argparse
import ctypes
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

# ── Paths ──────────────────────────────────────────────────────────────────────
SCRIPT_DIR   = Path(__file__).parent.resolve()
PROJECT_ROOT = SCRIPT_DIR.parent.parent.resolve()
FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
DEFAULT_PRESET = "release"
BIN_NAME = "wchisp.exe" if platform.system() == "Windows" else "wchisp"

# ── ANSI colors (cross-platform) ───────────────────────────────────────────────
def _enable_win_ansi():
    if platform.system() == "Windows":
        try:
            kernel32 = ctypes.windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except Exception:
            pass

_enable_win_ansi()
_USE_COLOR = sys.stdout.isatty()

def _c(code, text):
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text

def info(msg):    print(_c("36", "[INFO]"), msg)
def ok(msg):      print(_c("32", "[ OK ]"), msg)
def warn(msg):    print(_c("33", "[WARN]"), msg)
def sep():        print(_c("2",  "─" * 44))

def die(msg):
    print(_c("31", "[ERR ]"), msg, file=sys.stderr)
    sys.exit(1)


# ── Locate wchisp ──────────────────────────────────────────────────────────────
def find_wchisp() -> Optional[Path]:
    # 1. Env var override
    env = os.environ.get("WCHISP_PATH")
    if env:
        p = Path(env)
        if p.is_file():
            return p

    # 2. Project-local binary (downloaded by setup.py)
    local = SCRIPT_DIR / BIN_NAME
    if local.is_file():
        return local

    # 3. System PATH
    found = shutil.which("wchisp")
    if found:
        return Path(found)

    # 4. Platform-specific fallback locations
    system = platform.system()
    home = Path.home()
    candidates = []
    if system == "Darwin":
        candidates = [
            home / "Downloads/wchisp-macos-arm64/wchisp",
            home / "Downloads/wchisp-macos-x64/wchisp",
            Path("/opt/homebrew/bin/wchisp"),
            Path("/usr/local/bin/wchisp"),
        ]
    elif system == "Linux":
        candidates = [
            home / ".local/bin/wchisp",
            Path("/usr/local/bin/wchisp"),
            Path("/usr/bin/wchisp"),
            home / "Downloads/wchisp-linux-x64/wchisp",
            home / "Downloads/wchisp-linux-aarch64/wchisp",
        ]
    elif system == "Windows":
        appdata = Path(os.environ.get("APPDATA", ""))
        candidates = [
            appdata / "wchisp/wchisp.exe",
            Path("C:/Program Files/wchisp/wchisp.exe"),
        ]

    for c in candidates:
        if c.is_file():
            return c

    return None


# ── Run helpers ────────────────────────────────────────────────────────────────
def run(cmd: list, check=True, cwd=None) -> subprocess.CompletedProcess:
    """Run a command, streaming output to the terminal."""
    return subprocess.run([str(c) for c in cmd], check=check, cwd=cwd)


def check_device(wchisp: Path, extra: list):
    info("Checking for ISP device...")
    result = subprocess.run(
        [str(wchisp), "info"] + extra,
        capture_output=True, text=True
    )
    if result.returncode != 0:
        die(
            "No WCH ISP device found.\n\n"
            f"  {_c('33', 'Hint: Hold BOOT button, then connect USB')}\n"
            f"        (or press RESET while holding BOOT)"
        )
    for line in result.stdout.splitlines():
        if any(k in line for k in ("Chip:", "BTVER", "UID")):
            print(f"  {_c('2', line.strip())}")


# ── Build helper ───────────────────────────────────────────────────────────────
def build_firmware(preset: str) -> Path:
    sep()
    info(f"Building preset: {_c('1', preset)}")
    build_dir = FIRMWARE_DIR / "build" / preset
    if not build_dir.is_dir():
        info("Configuring (first build)...")
        run(["cmake", "--preset", preset], cwd=str(FIRMWARE_DIR))
    run(["cmake", "--build", "--preset", preset], cwd=str(FIRMWARE_DIR))
    bin_file = FIRMWARE_DIR / "build" / preset / "CH592F.bin"
    if not bin_file.is_file():
        die(f"Expected .bin not found after build: {bin_file}")
    ok(f"Build done → {bin_file}")
    return bin_file


def resolve_bin(file_arg, preset: str) -> Path:
    if file_arg:
        p = Path(file_arg)
        if not p.is_file():
            die(f"File not found: {p}")
        return p
    return FIRMWARE_DIR / "build" / preset / "CH592F.bin"


# ── Confirmation prompt ────────────────────────────────────────────────────────
def confirm(prompt: str):
    try:
        answer = input(f"  {prompt} [y/N] ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        sys.exit(0)
    if answer != "y":
        info("Aborted.")
        sys.exit(0)


# ── Commands ───────────────────────────────────────────────────────────────────
def cmd_flash(args, wchisp: Path, extra: list):
    if args.file:
        bin_file = resolve_bin(args.file, args.preset)
    else:
        bin_file = build_firmware(args.preset)

    sep()
    check_device(wchisp, extra)

    flash_args = []
    if args.skip_erase:  flash_args.append("--skip-erase")
    if args.skip_verify: flash_args.append("--skip-verify")
    if args.skip_reset:  flash_args.append("--skip-reset")

    size_kb = bin_file.stat().st_size / 1024
    sep()
    info(f"Flashing: {_c('1', bin_file.name)}  ({size_kb:.1f} KB)")
    run([wchisp, "flash"] + extra + flash_args + [str(bin_file)])
    sep()
    ok("Flash complete.")


def cmd_build(args, **_):
    build_firmware(args.preset)
    sep()
    ok(f"Build complete (preset: {args.preset})")


def cmd_verify(args, wchisp: Path, extra: list):
    bin_file = resolve_bin(args.file, args.preset)
    sep()
    check_device(wchisp, extra)
    sep()
    info(f"Verifying: {_c('1', bin_file.name)}")
    run([wchisp, "verify"] + extra + [str(bin_file)])
    sep()
    ok("Verify passed.")


def cmd_erase(args, wchisp: Path, extra: list):
    sep()
    check_device(wchisp, extra)
    warn("This will erase ALL code flash on the chip!")
    confirm("Continue?")
    sep()
    run([wchisp, "erase"] + extra)
    ok("Erase complete.")


def cmd_reset(args, wchisp: Path, extra: list):
    sep()
    check_device(wchisp, extra)
    info("Resetting chip...")
    run([wchisp, "reset"] + extra)
    ok("Reset sent.")


def cmd_info(args, wchisp: Path, extra: list):
    sep()
    info("Chip information:")
    sep()
    run([wchisp, "info"] + extra)


def cmd_probe(args, wchisp: Path, extra: list):
    sep()
    info("Probing connected WCH ISP devices...")
    sep()
    run([wchisp, "probe"] + extra)


def cmd_eeprom(args, wchisp: Path, extra: list):
    sep()
    check_device(wchisp, extra)
    sep()
    if args.eeprom_cmd == "dump":
        out = Path(args.out) if args.out else Path("eeprom_dump.bin")
        info(f"Dumping EEPROM → {_c('1', str(out))}")
        run([wchisp, "eeprom", "dump"] + extra + ["--out", str(out)])
        ok(f"EEPROM dumped to: {out}")
    elif args.eeprom_cmd == "erase":
        warn("This will erase the DATA EEPROM (not code flash).")
        confirm("Continue?")
        run([wchisp, "eeprom", "erase"] + extra)
        ok("EEPROM erased.")
    elif args.eeprom_cmd == "write":
        if not args.file:
            die("Missing --file <path> for eeprom write.")
        f = Path(args.file)
        if not f.is_file():
            die(f"File not found: {f}")
        info(f"Writing EEPROM from: {_c('1', str(f))}")
        run([wchisp, "eeprom", "write"] + extra + [str(f)])
        ok("EEPROM write complete.")


def cmd_config(args, wchisp: Path, extra: list):
    sep()
    check_device(wchisp, extra)
    sep()
    if args.config_cmd == "info":
        info("Config registers:")
        run([wchisp, "config", "info"] + extra)
    elif args.config_cmd == "reset":
        warn("This will reset config registers to factory defaults!")
        confirm("Continue?")
        run([wchisp, "config", "reset"] + extra)
        ok("Config registers reset.")


# ── Argument parser ────────────────────────────────────────────────────────────
def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="flash.py",
        description="WCH CH592F ISP tool — cross-platform wrapper for wchisp",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
examples:
  python flash.py flash                        build release + flash
  python flash.py flash --preset debug         build debug + flash
  python flash.py flash --file build/app.bin   flash specific file
  python flash.py flash --skip-verify          flash without verification
  python flash.py eeprom dump --out data.bin   dump EEPROM
  python flash.py info                         show chip info
        """,
    )

    # Global transport options
    transport = parser.add_argument_group("transport")
    transport.add_argument("-d", "--device", metavar="N", help="USB device index")
    transport.add_argument("-s", "--serial", action="store_true", help="Use serial transport")
    transport.add_argument("--port", metavar="PORT", help="Serial port (e.g. COM3, /dev/cu.xxx)")

    sub = parser.add_subparsers(dest="command", metavar="command")
    sub.required = True

    # ── flash ──────────────────────────────────────────────────────────────────
    p_flash = sub.add_parser("flash", help="Build (if needed) and flash firmware")
    p_flash.add_argument("-p", "--preset", default=DEFAULT_PRESET,
                         help=f"CMake preset (default: {DEFAULT_PRESET})")
    p_flash.add_argument("-f", "--file", metavar="FILE",
                         help="Flash a specific .bin/.hex/.elf (skip build)")
    p_flash.add_argument("--skip-erase",  action="store_true")
    p_flash.add_argument("--skip-verify", action="store_true")
    p_flash.add_argument("--skip-reset",  action="store_true")

    # ── build ──────────────────────────────────────────────────────────────────
    p_build = sub.add_parser("build", help="Build firmware only")
    p_build.add_argument("-p", "--preset", default=DEFAULT_PRESET)

    # ── verify ─────────────────────────────────────────────────────────────────
    p_verify = sub.add_parser("verify", help="Verify chip flash matches .bin")
    p_verify.add_argument("-p", "--preset", default=DEFAULT_PRESET)
    p_verify.add_argument("-f", "--file", metavar="FILE")

    # ── erase ──────────────────────────────────────────────────────────────────
    sub.add_parser("erase", help="Erase entire code flash")

    # ── reset ──────────────────────────────────────────────────────────────────
    sub.add_parser("reset", help="Reset the target chip")

    # ── info ───────────────────────────────────────────────────────────────────
    sub.add_parser("info", help="Show chip info, UID, bootloader version")

    # ── probe ──────────────────────────────────────────────────────────────────
    sub.add_parser("probe", help="List connected WCH ISP devices")

    # ── eeprom ─────────────────────────────────────────────────────────────────
    p_eeprom = sub.add_parser("eeprom", help="EEPROM operations")
    eeprom_sub = p_eeprom.add_subparsers(dest="eeprom_cmd", metavar="subcommand")
    eeprom_sub.required = True

    p_edump = eeprom_sub.add_parser("dump",  help="Dump EEPROM to file")
    p_edump.add_argument("-o", "--out", metavar="FILE", help="Output file (default: eeprom_dump.bin)")

    eeprom_sub.add_parser("erase", help="Erase data EEPROM")

    p_ewrite = eeprom_sub.add_parser("write", help="Write file to EEPROM")
    p_ewrite.add_argument("-f", "--file", metavar="FILE", required=True)

    # ── config ─────────────────────────────────────────────────────────────────
    p_config = sub.add_parser("config", help="Config register operations")
    config_sub = p_config.add_subparsers(dest="config_cmd", metavar="subcommand")
    config_sub.required = True
    config_sub.add_parser("info",  help="Show config register details")
    config_sub.add_parser("reset", help="Reset config registers to factory defaults")

    return parser


# ── Entry point ────────────────────────────────────────────────────────────────
def main():
    parser = build_parser()
    args = parser.parse_args()

    # Locate wchisp
    wchisp = find_wchisp()
    if not wchisp:
        die(
            "wchisp not found.\n\n"
            "  Run:  python setup.py          (auto-download)\n"
            "  Or:   set WCHISP_PATH=/path/to/wchisp"
        )

    # Build extra transport args
    extra = []
    if args.device: extra += ["--device", args.device]
    if args.serial: extra += ["--serial"]
    if args.port:   extra += ["--port", args.port]

    dispatch = {
        "flash":  cmd_flash,
        "build":  cmd_build,
        "verify": cmd_verify,
        "erase":  cmd_erase,
        "reset":  cmd_reset,
        "info":   cmd_info,
        "probe":  cmd_probe,
        "eeprom": cmd_eeprom,
        "config": cmd_config,
    }

    try:
        dispatch[args.command](args, wchisp=wchisp, extra=extra)
    except subprocess.CalledProcessError as e:
        die(f"Command failed (exit {e.returncode})")
    except KeyboardInterrupt:
        print()
        info("Interrupted.")
        sys.exit(130)


if __name__ == "__main__":
    main()
