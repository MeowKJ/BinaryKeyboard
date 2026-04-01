#!/usr/bin/env python3
"""
tools/scripts/flash.py — Generic WCH ISP tool wrapper
Cross-platform (macOS / Linux / Windows)
Supports: flash, verify, erase, reset, info, probe, eeprom, config
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from common import colorize as _c, die, find_wchisp, info, ok, sep, warn
from versioning import ch552_filename_for_keyboard, ch592_filename_for_keyboard


def run(cmd: list[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run([str(c) for c in cmd], check=check)


def resolve_file(file_arg: str) -> Path:
    path = Path(file_arg)
    if not path.is_file():
        die(f"File not found: {path}")
    return path


def check_device(wchisp: Path, extra: list[str]) -> None:
    info("Checking for ISP device...")
    result = subprocess.run(
        [str(wchisp), "info"] + extra,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        die(
            "No WCH ISP device found.\n\n"
            f"  {_c('33', 'Hint: Hold BOOT button, then connect USB')}\n"
            f"        (or press RESET while holding BOOT)"
        )
    for line in result.stdout.splitlines():
        if any(key in line for key in ("Chip:", "BTVER", "UID")):
            print(f"  {_c('2', line.strip())}")


def confirm(prompt: str) -> None:
    try:
        answer = input(f"  {prompt} [y/N] ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        sys.exit(0)
    if answer != "y":
        info("Aborted.")
        sys.exit(0)


def cmd_flash(args, wchisp: Path, extra: list[str]) -> None:
    file_path = resolve_file(args.file)
    sep()
    check_device(wchisp, extra)

    flash_args: list[str] = []
    if args.skip_erase:
        flash_args.append("--skip-erase")
    if args.skip_verify:
        flash_args.append("--skip-verify")
    if args.skip_reset:
        flash_args.append("--skip-reset")

    size_kb = file_path.stat().st_size / 1024
    sep()
    info(f"Flashing: {_c('1', file_path.name)}  ({size_kb:.1f} KB)")
    run([str(wchisp), "flash"] + extra + flash_args + [str(file_path)])
    sep()
    ok("Flash complete.")


def cmd_verify(args, wchisp: Path, extra: list[str]) -> None:
    file_path = resolve_file(args.file)
    sep()
    check_device(wchisp, extra)
    sep()
    info(f"Verifying: {_c('1', file_path.name)}")
    run([str(wchisp), "verify"] + extra + [str(file_path)])
    sep()
    ok("Verify passed.")


def cmd_erase(args, wchisp: Path, extra: list[str]) -> None:
    sep()
    check_device(wchisp, extra)
    warn("This will erase ALL code flash on the chip!")
    confirm("Continue?")
    sep()
    run([str(wchisp), "erase"] + extra)
    ok("Erase complete.")


def cmd_reset(args, wchisp: Path, extra: list[str]) -> None:
    sep()
    check_device(wchisp, extra)
    info("Resetting chip...")
    run([str(wchisp), "reset"] + extra)
    ok("Reset sent.")


def cmd_info(args, wchisp: Path, extra: list[str]) -> None:
    sep()
    info("Chip information:")
    sep()
    run([str(wchisp), "info"] + extra)


def cmd_probe(args, wchisp: Path, extra: list[str]) -> None:
    sep()
    info("Probing connected WCH ISP devices...")
    sep()
    run([str(wchisp), "probe"] + extra)


def cmd_eeprom(args, wchisp: Path, extra: list[str]) -> None:
    sep()
    check_device(wchisp, extra)
    sep()
    if args.eeprom_cmd == "dump":
        out = Path(args.out) if args.out else Path("eeprom_dump.bin")
        info(f"Dumping EEPROM -> {_c('1', str(out))}")
        run([str(wchisp), "eeprom", "dump"] + extra + ["--out", str(out)])
        ok(f"EEPROM dumped to: {out}")
    elif args.eeprom_cmd == "erase":
        warn("This will erase the DATA EEPROM (not code flash).")
        confirm("Continue?")
        run([str(wchisp), "eeprom", "erase"] + extra)
        ok("EEPROM erased.")
    elif args.eeprom_cmd == "write":
        file_path = resolve_file(args.file)
        info(f"Writing EEPROM from: {_c('1', str(file_path))}")
        run([str(wchisp), "eeprom", "write"] + extra + [str(file_path)])
        ok("EEPROM write complete.")


def cmd_config(args, wchisp: Path, extra: list[str]) -> None:
    sep()
    check_device(wchisp, extra)
    sep()
    if args.config_cmd == "info":
        info("Config registers:")
        run([str(wchisp), "config", "info"] + extra)
    elif args.config_cmd == "reset":
        warn("This will reset config registers to factory defaults!")
        confirm("Continue?")
        run([str(wchisp), "config", "reset"] + extra)
        ok("Config registers reset.")


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

    wchisp = find_wchisp()
    if not wchisp:
        die(
            "wchisp not found.\n\n"
            "  Run:  python tools/scripts/setup.py          (auto-download)\n"
            "  Or:   set WCHISP_PATH=/path/to/wchisp"
        )

    extra: list[str] = []
    if args.device:
        extra += ["--device", args.device]
    if args.serial:
        extra += ["--serial"]
    if args.port:
        extra += ["--port", args.port]

    dispatch = {
        "flash": cmd_flash,
        "verify": cmd_verify,
        "erase": cmd_erase,
        "reset": cmd_reset,
        "info": cmd_info,
        "probe": cmd_probe,
        "eeprom": cmd_eeprom,
        "config": cmd_config,
    }

    try:
        dispatch[args.command](args, wchisp=wchisp, extra=extra)
    except subprocess.CalledProcessError as exc:
        die(f"Command failed (exit {exc.returncode})")
    except KeyboardInterrupt:
        print()
        info("Interrupted.")
        sys.exit(130)


if __name__ == "__main__":
    main()
