#!/usr/bin/env python3
"""tools/scripts/flash.py — MeowISP CLI wrapper for BinaryKeyboard."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from common import colorize as _c, die, find_meowisp, info, ok, sep, warn
from versioning import ch552_filename_for_keyboard, ch592_filename_for_keyboard


def run(cmd: list[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run([str(c) for c in cmd], check=check)


def resolve_file(file_arg: str) -> Path:
    path = Path(file_arg)
    if not path.is_file():
        die(f"File not found: {path}")
    return path


def confirm(prompt: str) -> None:
    try:
        answer = input(f"  {prompt} [y/N] ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        sys.exit(0)
    if answer != "y":
        info("Aborted.")
        sys.exit(0)


def build_parser() -> argparse.ArgumentParser:
    ch592_example = ch592_filename_for_keyboard("5KEY", "bin")
    ch552_example = ch552_filename_for_keyboard("BASIC", "bin")
    parser = argparse.ArgumentParser(
        prog="tools/scripts/flash.py",
        description="Generic WCH ISP tool wrapper",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=f"""
examples:
  python tools/scripts/flash.py flash --file firmware/CH592F/build/release-5key/{ch592_example}
  python tools/scripts/flash.py flash --file firmware/CH552G/build/basic/{ch552_example}
  python tools/scripts/flash.py verify --file firmware/CH592F/build/release-5key/{ch592_example}
  python tools/scripts/flash.py eeprom dump --out data.bin
  python tools/scripts/flash.py info
        """,
    )

    transport = parser.add_argument_group("transport")
    transport.add_argument("-d", "--device", metavar="N", help="USB device index")
    transport.add_argument("-s", "--serial", action="store_true", help="Use serial transport")
    transport.add_argument("--port", metavar="PORT", help="Serial port (e.g. COM3, /dev/cu.xxx)")

    sub = parser.add_subparsers(dest="command", metavar="command")
    sub.required = True

    p_flash = sub.add_parser("flash", help="Flash a prepared firmware artifact")
    p_flash.add_argument("-f", "--file", metavar="FILE", required=True, help="Artifact to flash (.bin/.hex/.elf)")
    p_flash.add_argument("--skip-erase", action="store_true")
    p_flash.add_argument("--skip-verify", action="store_true")
    p_flash.add_argument("--skip-reset", action="store_true")

    p_verify = sub.add_parser("verify", help="Verify chip flash matches artifact")
    p_verify.add_argument("-f", "--file", metavar="FILE", required=True)

    sub.add_parser("erase", help="Erase entire code flash")
    sub.add_parser("reset", help="Reset the target chip")
    sub.add_parser("info", help="Show chip info, UID, bootloader version")
    sub.add_parser("probe", help="List connected WCH ISP devices")

    p_eeprom = sub.add_parser("eeprom", help="EEPROM operations")
    eeprom_sub = p_eeprom.add_subparsers(dest="eeprom_cmd", metavar="subcommand")
    eeprom_sub.required = True
    p_edump = eeprom_sub.add_parser("dump", help="Dump EEPROM to file")
    p_edump.add_argument("-o", "--out", metavar="FILE", help="Output file (default: eeprom_dump.bin)")
    eeprom_sub.add_parser("erase", help="Erase data EEPROM")
    p_ewrite = eeprom_sub.add_parser("write", help="Write file to EEPROM")
    p_ewrite.add_argument("-f", "--file", metavar="FILE", required=True)

    p_config = sub.add_parser("config", help="Config register operations")
    config_sub = p_config.add_subparsers(dest="config_cmd", metavar="subcommand")
    config_sub.required = True
    config_sub.add_parser("info", help="Show config register details")
    config_sub.add_parser("reset", help="Reset config registers to factory defaults")

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    meowisp = find_meowisp()
    if not meowisp:
        die(
            "meowisp not found.\n\n"
            "  Run:  cargo build --manifest-path tools/meowisp/Cargo.toml --bin meowisp\n"
            "  Or:   set MEOWISP_PATH=/path/to/meowisp"
        )

    extra: list[str] = []
    if args.device:
        extra += ["--device", args.device]
    if args.serial:
        extra += ["--serial"]
    if args.port:
        extra += ["--port", args.port]

    cmd: list[str] = [str(meowisp), *extra]
    if args.command == "flash":
        cmd += ["flash", "--file", args.file]
        if args.skip_erase:
            cmd.append("--skip-erase")
        if args.skip_verify:
            cmd.append("--skip-verify")
        if args.skip_reset:
            cmd.append("--skip-reset")
    elif args.command == "verify":
        cmd += ["verify", "--file", args.file]
    elif args.command == "erase":
        cmd += ["erase"]
    elif args.command == "reset":
        cmd += ["reset"]
    elif args.command == "info":
        cmd += ["info"]
    elif args.command == "probe":
        cmd += ["probe"]
    elif args.command == "eeprom":
        if args.eeprom_cmd == "dump":
            cmd += ["eeprom", "dump"]
            if args.out:
                cmd += ["--out", args.out]
        elif args.eeprom_cmd == "erase":
            cmd += ["eeprom", "erase"]
        elif args.eeprom_cmd == "write":
            cmd += ["eeprom", "write", "--file", args.file]
    elif args.command == "config":
        if args.config_cmd == "info":
            cmd += ["config", "info"]
        elif args.config_cmd == "reset":
            cmd += ["config", "reset"]
    else:
        parser.error(f"Unknown command: {args.command}")

    try:
        run(cmd)
    except subprocess.CalledProcessError as exc:
        die(f"Command failed (exit {exc.returncode})")
    except KeyboardInterrupt:
        print()
        info("Interrupted.")
        sys.exit(130)


if __name__ == "__main__":
    main()
