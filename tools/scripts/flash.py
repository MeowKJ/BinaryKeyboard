#!/usr/bin/env python3
"""
tools/scripts/flash.py — Generic WCH ISP tool wrapper
Cross-platform (macOS / Linux / Windows)
Supports: flash, verify, erase, reset, info, probe, eeprom, config
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

from common import colorize as _c, die, find_meowisp, info, ok, sep, warn
from versioning import ch552_filename_for_keyboard, ch592_filename_for_keyboard


KNOWN_BAD_CH59X_USER_CFG = "0x4FFF0FD5"


def _run_capture(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(c) for c in cmd],
        capture_output=True,
        text=True,
        check=False,
    )


def _extract_chip_name(output: str) -> str | None:
    match = re.search(r"Chip:\s+([A-Za-z0-9]+)", output)
    return match.group(1) if match else None


def _extract_user_cfg(output: str) -> str | None:
    match = re.search(r"USER_CFG:\s*(0x[0-9A-Fa-f]+)", output)
    return match.group(1).upper() if match else None


def _print_device_summary(output: str) -> None:
    for line in output.splitlines():
        if any(key in line for key in ("Chip:", "BTVER", "UID", "USER_CFG:")):
            print(f"  {_c('2', line.strip())}")


def isp_cmd(isp: Path, extra: list[str], *parts: str | Path) -> list[str]:
    return [str(isp), *extra, *[str(part) for part in parts]]


def _read_device_info(isp: Path, extra: list[str]) -> subprocess.CompletedProcess[str]:
    return _run_capture(isp_cmd(isp, extra, "info"))


def _repair_known_ch59x_verify_cfg(
    isp: Path, extra: list[str], info_output: str
) -> str:
    chip_name = _extract_chip_name(info_output)
    user_cfg = _extract_user_cfg(info_output)
    if not chip_name or user_cfg != KNOWN_BAD_CH59X_USER_CFG:
        return info_output
    if not chip_name.startswith("CH59"):
        return info_output

    warn(
        "Detected CH59x USER_CFG 0x4FFF0FD5, a known state that causes ISP verify to fail "
        "after a successful write. Resetting config registers before flashing."
    )
    run(isp_cmd(isp, extra, "config", "reset"))

    repaired = _read_device_info(isp, extra)
    if repaired.returncode != 0:
        die("Config reset succeeded, but the device could not be re-identified afterward.")

    repaired_cfg = _extract_user_cfg(repaired.stdout)
    if repaired_cfg == KNOWN_BAD_CH59X_USER_CFG:
        die("Config reset did not clear the known-bad CH59x USER_CFG state.")

    info("Config repair applied:")
    _print_device_summary(repaired.stdout)
    return repaired.stdout


def run(cmd: list[str], check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run([str(c) for c in cmd], check=check)


def resolve_file(file_arg: str) -> Path:
    path = Path(file_arg)
    if not path.is_file():
        die(f"File not found: {path}")
    return path


def check_device(isp: Path, extra: list[str]) -> str:
    info("Checking for ISP device...")
    result = _read_device_info(isp, extra)
    if result.returncode != 0:
        die(
            "No WCH ISP device found.\n\n"
            f"  {_c('33', 'Hint: Hold BOOT button, then connect USB')}\n"
            f"        (or press RESET while holding BOOT)"
        )
    _print_device_summary(result.stdout)
    return result.stdout


def confirm(prompt: str) -> None:
    try:
        answer = input(f"  {prompt} [y/N] ").strip().lower()
    except (EOFError, KeyboardInterrupt):
        print()
        sys.exit(0)
    if answer != "y":
        info("Aborted.")
        sys.exit(0)


def cmd_flash(args, isp: Path, extra: list[str]) -> None:
    file_path = resolve_file(args.file)
    sep()
    device_info = check_device(isp, extra)
    _repair_known_ch59x_verify_cfg(isp, extra, device_info)

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
    run(isp_cmd(isp, extra, "flash", *flash_args, "--file", file_path))
    sep()
    ok("Flash complete.")


def cmd_verify(args, isp: Path, extra: list[str]) -> None:
    file_path = resolve_file(args.file)
    sep()
    device_info = check_device(isp, extra)
    if _extract_chip_name(device_info or "") and _extract_user_cfg(device_info or "") == KNOWN_BAD_CH59X_USER_CFG:
        warn(
            "Detected CH59x USER_CFG 0x4FFF0FD5. This state is known to make ISP verify fail "
            "even after a successful flash. Run flash once without --skip-verify to auto-repair it."
        )
    sep()
    info(f"Verifying: {_c('1', file_path.name)}")
    run(isp_cmd(isp, extra, "verify", "--file", file_path))
    sep()
    ok("Verify passed.")


def cmd_erase(args, isp: Path, extra: list[str]) -> None:
    sep()
    check_device(isp, extra)
    warn("This will erase ALL code flash on the chip!")
    confirm("Continue?")
    sep()
    run(isp_cmd(isp, extra, "erase"))
    ok("Erase complete.")


def cmd_reset(args, isp: Path, extra: list[str]) -> None:
    sep()
    check_device(isp, extra)
    info("Resetting chip...")
    run(isp_cmd(isp, extra, "reset"))
    ok("Reset sent.")


def cmd_info(args, isp: Path, extra: list[str]) -> None:
    sep()
    info("Chip information:")
    sep()
    run(isp_cmd(isp, extra, "info"))


def cmd_probe(args, isp: Path, extra: list[str]) -> None:
    sep()
    info("Probing connected WCH ISP devices...")
    sep()
    run(isp_cmd(isp, extra, "probe"))


def cmd_eeprom(args, isp: Path, extra: list[str]) -> None:
    sep()
    check_device(isp, extra)
    sep()
    if args.eeprom_cmd == "dump":
        out = Path(args.out) if args.out else Path("eeprom_dump.bin")
        info(f"Dumping EEPROM -> {_c('1', str(out))}")
        run(isp_cmd(isp, extra, "eeprom", "dump", "--out", out))
        ok(f"EEPROM dumped to: {out}")
    elif args.eeprom_cmd == "erase":
        warn("This will erase the DATA EEPROM (not code flash).")
        confirm("Continue?")
        run(isp_cmd(isp, extra, "eeprom", "erase"))
        ok("EEPROM erased.")
    elif args.eeprom_cmd == "write":
        file_path = resolve_file(args.file)
        info(f"Writing EEPROM from: {_c('1', str(file_path))}")
        run(isp_cmd(isp, extra, "eeprom", "write", "--file", file_path))
        ok("EEPROM write complete.")


def cmd_config(args, isp: Path, extra: list[str]) -> None:
    sep()
    check_device(isp, extra)
    sep()
    if args.config_cmd == "info":
        info("Config registers:")
        run(isp_cmd(isp, extra, "config", "info"))
    elif args.config_cmd == "reset":
        warn("This will reset config registers to factory defaults!")
        confirm("Continue?")
        run(isp_cmd(isp, extra, "config", "reset"))
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

    isp = find_meowisp()
    if not isp:
        die(
            "BinaryKeyboard ISP not found.\n\n"
            "  Run:  python tools/scripts/setup.py          (build from local Rust source)\n"
            "  Or:   set BINARYKEYBOARD_ISP_PATH=/path/to/meowisp"
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
        dispatch[args.command](args, isp=isp, extra=extra)
    except subprocess.CalledProcessError as exc:
        die(f"Command failed (exit {exc.returncode})")
    except KeyboardInterrupt:
        print()
        info("Interrupted.")
        sys.exit(130)


if __name__ == "__main__":
    main()
