#!/usr/bin/env python3
"""
tools/scripts/flash.py — WCH CH592F ISP tool wrapper
Cross-platform (macOS / Linux / Windows)
Supports: flash, build, verify, erase, reset, info, probe, eeprom, config
"""

import argparse
import ctypes
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import time
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


# ── Build report helpers ───────────────────────────────────────────────────────

def _strip_ansi(s: str) -> str:
    return re.sub(r'\033\[[^m]*m', '', s)

def _vlen(s: str) -> int:
    return len(_strip_ansi(s))

def _rpad(s: str, w: int) -> str:
    """Right-pad to visual width w (ANSI-aware)."""
    return s + ' ' * max(0, w - _vlen(s))

def _usage_bar(pct: float, width: int = 15) -> str:
    filled = round(pct / 100 * width)
    bar = '█' * filled + '░' * (width - filled)
    code = '31' if pct >= 90 else '33' if pct >= 70 else '32'
    return _c(code, bar)

def _pct_clr(pct: float) -> str:
    return '31' if pct >= 90 else '33' if pct >= 70 else '32'

def _fmt_b(n: int) -> str:
    return f'{n / 1024:.1f} KB' if n >= 1024 else f'{n} B'


def _parse_cmake_cache_var(cache_file: Path, key: str) -> Optional[str]:
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


def _build_dir_needs_configure(build_dir: Path) -> bool:
    """Return True when the build directory is missing or incompletely generated."""
    if not build_dir.is_dir():
        return True

    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.is_file():
        return True

    generator = _parse_cmake_cache_var(cache_file, "CMAKE_GENERATOR")
    make_program = _parse_cmake_cache_var(cache_file, "CMAKE_MAKE_PROGRAM")

    # Incomplete Ninja generation leaves no build.ninja and often an empty
    # CMAKE_MAKE_PROGRAM, which later surfaces as a misleading "permission denied".
    if generator == "Ninja":
        if not (build_dir / "build.ninja").is_file():
            return True
        if not make_program:
            return True

    return False


def _parse_size_row_from_output(text: str) -> Optional[tuple[int, int, int]]:
    # e.g. " 178204    1740   13232  193176   2f298 /path/CH592F.elf"
    m = re.search(r'^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9a-fA-F]+\s+\S+\.elf\s*$',
                  text, re.MULTILINE)
    if not m:
        return None
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def _read_memory_regions_from_linker(linker_script: Path) -> dict[str, tuple[int, int]]:
    """Parse MEMORY regions from GNU ld script: NAME -> (origin, length)."""
    out: dict[str, tuple[int, int]] = {}
    if not linker_script.is_file():
        return out

    unit_mul = {"B": 1, "K": 1024, "M": 1024 * 1024}
    mem_re = re.compile(
        r'^\s*(\w+)\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*LENGTH\s*=\s*([0-9]+(?:\.[0-9]+)?)\s*([BbKkMm]?)'
    )
    try:
        for line in linker_script.read_text(errors="ignore").splitlines():
            m = mem_re.match(line)
            if not m:
                continue
            name, origin_s, num, unit = m.groups()
            unit = (unit or "B").upper()
            origin = int(origin_s, 0)
            length = int(float(num) * unit_mul.get(unit, 1))
            out[name.upper()] = (origin, length)
    except Exception:
        return {}
    return out


def _read_memory_lengths_from_linker(linker_script: Path) -> dict[str, int]:
    return {name: length for name, (_, length) in _read_memory_regions_from_linker(linker_script).items()}


def _size_row_from_artifact(build_dir: Path) -> Optional[tuple[int, int, int]]:
    """Run CROSS_SIZE on existing ELF when incremental build skips relinking."""
    elf_file = build_dir / "CH592F.elf"
    cache_file = build_dir / "CMakeCache.txt"
    if not elf_file.is_file() or not cache_file.is_file():
        return None

    cross_size = _parse_cmake_cache_var(cache_file, "CROSS_SIZE")
    if not cross_size:
        return None

    try:
        res = subprocess.run(
            [cross_size, "--format=berkeley", str(elf_file)],
            capture_output=True, text=True, check=True
        )
    except Exception:
        return None

    return _parse_size_row_from_output(res.stdout)


def _artifact_region_usage_from_objdump(build_dir: Path) -> Optional[dict[str, int]]:
    """Compute region usage from existing ELF section headers (no-op build fallback).

    FLASH usage is summed by section LMA in FLASH range.
    RAM usage is summed by section VMA in RAM range.
    """
    elf_file = build_dir / "CH592F.elf"
    cache_file = build_dir / "CMakeCache.txt"
    if not elf_file.is_file() or not cache_file.is_file():
        return None

    cross_objdump = _parse_cmake_cache_var(cache_file, "CROSS_OBJDUMP")
    if not cross_objdump:
        return None

    regions = _read_memory_regions_from_linker(FIRMWARE_DIR / "Ld" / "Link.ld")
    if "FLASH" not in regions or "RAM" not in regions:
        return None
    flash_origin, flash_len = regions["FLASH"]
    ram_origin, ram_len = regions["RAM"]
    flash_end = flash_origin + flash_len
    ram_end = ram_origin + ram_len

    try:
        res = subprocess.run(
            [cross_objdump, "-h", str(elf_file)],
            capture_output=True, text=True, check=True
        )
    except Exception:
        return None

    flash_used = 0
    ram_used = 0
    pending: Optional[tuple[int, int, int]] = None  # (size, vma, lma)

    for line in res.stdout.splitlines():
        parts = line.split()
        # Section header row: Idx Name Size VMA LMA FileOff Algn
        if len(parts) >= 7 and parts[0].isdigit():
            try:
                size = int(parts[2], 16)
                vma = int(parts[3], 16)
                lma = int(parts[4], 16)
                pending = (size, vma, lma)
            except ValueError:
                pending = None
            continue

        if pending is None:
            continue

        # Flags row follows the section header row; ALLOC marks memory occupancy.
        if "ALLOC" in line:
            size, vma, lma = pending
            if size > 0:
                # FLASH usage should only include sections that occupy bytes in the
                # image (e.g. exclude NOBITS like .bss/.stack even if they have an
                # LMA due to linker script AT>FLASH).
                if (("LOAD" in line) or ("CONTENTS" in line)) and (flash_origin <= lma < flash_end):
                    flash_used += size
                if ram_origin <= vma < ram_end:
                    ram_used += size
            pending = None
        elif line.strip():
            # Some unexpected line; stop tracking the pending section.
            pending = None

    return {"FLASH": flash_used, "RAM": ram_used}


def _resolve_preset(preset: str) -> str:
    """Prefer local-{preset} when CMakeUserPresets.json defines it.

    On CI (no user presets file) the plain preset name is used unchanged,
    so existing CI workflows are unaffected.
    """
    user = FIRMWARE_DIR / "CMakeUserPresets.json"
    if user.is_file():
        try:
            data = json.loads(user.read_text())
            names = {p["name"] for p in data.get("buildPresets", [])}
            local = f"local-{preset}"
            if local in names:
                return local
        except Exception:
            pass
    return preset


def _cmake_define_args(defines: list[str]) -> list[str]:
    args: list[str] = []
    for item in defines:
        if "=" not in item:
            die(f"Invalid --cmake-define '{item}'. Expected KEY=VALUE.")
        args.append(f"-D{item}")
    return args


# ── Locate wchisp ──────────────────────────────────────────────────────────────
def find_wchisp() -> Optional[Path]:
    # 1. Env var override
    env = os.environ.get("WCHISP_PATH")
    if env:
        p = Path(env)
        if p.is_file():
            return p

    # 2. Project-local binary (downloaded by tools/scripts/setup.py)
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


def run_and_capture(cmd: list, cwd=None):
    """Run a command, stream stdout+stderr to terminal, return (lines, elapsed_s)."""
    t0 = time.time()
    lines = []
    proc = subprocess.Popen(
        [str(c) for c in cmd],
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        cwd=cwd, text=True, bufsize=1,
    )
    for line in proc.stdout:
        print(line, end='')
        lines.append(line)
    proc.wait()
    if proc.returncode != 0:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    return lines, time.time() - t0


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


# ── Build report ───────────────────────────────────────────────────────────────

def _build_report(lines: list, preset: str, build_dir: Path, elapsed: float) -> None:
    """Parse cmake/linker output and print a colored memory-usage table."""
    # e.g.  "           FLASH:      179944 B       448 KB     39.22%"
    mem_re = re.compile(
        r'(\w+):\s+(\d+)\s+B\s+([\d.]+)\s+(KB|B)\s+([\d.]+)%'
    )
    # e.g.  " 178204    1740   13232  193176   2f298 /path/CH592F.elf"
    size_re = re.compile(
        r'^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9a-f]+\s+\S+\.elf\s*$'
    )

    regions = []
    size_row = None
    report_source = "linker"
    for line in lines:
        m = mem_re.search(line)
        if m:
            name, used_b, tot_v, tot_u, pct = m.groups()
            used  = int(used_b)
            total = int(float(tot_v) * 1024) if tot_u == 'KB' else int(float(tot_v))
            regions.append((name, used, total, total - used, float(pct)))
        m2 = size_re.match(line)
        if m2:
            size_row = (int(m2.group(1)), int(m2.group(2)), int(m2.group(3)))

    # Incremental no-op builds won't run the linker, so no memory-usage lines are
    # printed by ld. Fall back to the existing ELF + CMakeCache to keep the
    # report visible on every task build.
    if size_row is None:
        size_row = _size_row_from_artifact(build_dir)
        report_source = "cached-elf"

    if not regions and size_row is not None:
        mem = _read_memory_lengths_from_linker(FIRMWARE_DIR / "Ld" / "Link.ld")
        flash_total = mem.get("FLASH")
        ram_total = mem.get("RAM")
        text, data, bss = size_row
        usage = _artifact_region_usage_from_objdump(build_dir)
        if usage:
            report_source = "cached-elf+objdump"
        flash_used = usage.get("FLASH") if usage else (text + data)
        ram_used = usage.get("RAM") if usage else (data + bss)
        if flash_total and flash_used is not None:
            flash_pct = (flash_used / flash_total * 100) if flash_total else 0.0
            regions.append(("FLASH", flash_used, flash_total, flash_total - flash_used, flash_pct))
        if ram_total and ram_used is not None:
            ram_pct = (ram_used / ram_total * 100) if ram_total else 0.0
            regions.append(("RAM", ram_used, ram_total, ram_total - ram_used, ram_pct))

    if not regions and size_row is None:
        return

    bin_file = build_dir / 'CH592F.bin'
    bin_sz   = bin_file.stat().st_size if bin_file.is_file() else None

    W  = 68   # inner visual width
    BC = '1;36'

    def hline(l, r, fill='─'):
        return _c(BC, l + fill * W + r) if _USE_COLOR else l + fill * W + r

    def brow(cells: str) -> str:
        inner = _rpad(cells, W)
        if _USE_COLOR:
            return _c(BC, '│') + inner + _c(BC, '│')
        return '│' + inner + '│'

    print()
    print(hline('┌', '┐'))
    print(brow('  ' + _c('1', 'CH592F Memory Report') +
               _c('2', f'  ·  preset: {preset}  ·  built in {elapsed:.1f}s  ·  source: {report_source}')))
    print(hline('├', '┤'))

    hdr  = '  ' + _rpad(_c('2', 'Region'), 9)
    hdr += _rpad(_c('2', 'Used / Total'), 22)
    hdr += _rpad(_c('2', 'Free'), 12)
    hdr += _c('2', 'Usage')
    print(brow(hdr))
    print(hline('├', '┤'))

    for name, used, total, free, pct in regions:
        cc = _pct_clr(pct)
        c1 = '  ' + _rpad(_c('1', name), 8) + ' '
        c2 = _rpad(f'{_fmt_b(used)} / {_fmt_b(total)}', 22)
        c3 = _rpad(_c(cc, _fmt_b(free)), 12)
        c4 = _usage_bar(pct) + '  ' + _c(cc, f'{pct:5.1f}%')
        print(brow(c1 + c2 + c3 + c4))

    if size_row is not None or bin_sz is not None:
        print(hline('├', '┤'))
        det = '  '
        if size_row:
            text, data, bss = size_row
            det += f'.text {_c("36", str(text))}   '
            det += f'.data {_c("33", str(data))}   '
            det += f'.bss  {_c("35", str(bss))}'
        if bin_sz is not None:
            det += f'   .bin {_c("32;1", _fmt_b(bin_sz))}'
        print(brow(det))

    print(hline('└', '┘'))


# ── Build helper ───────────────────────────────────────────────────────────────
def build_firmware(preset: str, cmake_defines: Optional[list[str]] = None) -> Path:
    actual = _resolve_preset(preset)
    define_args = _cmake_define_args(cmake_defines or [])
    sep()
    info(f"Building preset: {_c('1', actual)}")
    build_dir = FIRMWARE_DIR / "build" / actual
    needs_configure = _build_dir_needs_configure(build_dir)
    if needs_configure or define_args:
        if needs_configure and build_dir.is_dir():
            warn("Build directory is incomplete; reconfiguring preset.")
        info("Configuring..." if build_dir.is_dir() else "Configuring (first build)...")
        run(["cmake", "--preset", actual] + define_args, cwd=str(FIRMWARE_DIR))
    lines, elapsed = run_and_capture(
        ["cmake", "--build", "--preset", actual], cwd=str(FIRMWARE_DIR)
    )
    _build_report(lines, actual, build_dir, elapsed)
    bin_file = FIRMWARE_DIR / "build" / actual / "CH592F.bin"
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
    return FIRMWARE_DIR / "build" / _resolve_preset(preset) / "CH592F.bin"


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
        bin_file = build_firmware(args.preset, args.cmake_define)

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
    build_firmware(args.preset, args.cmake_define)
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
        prog="tools/scripts/flash.py",
        description="WCH CH592F ISP tool — cross-platform wrapper for wchisp",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
examples:
  python tools/scripts/flash.py flash                        build release + flash
  python tools/scripts/flash.py flash --preset debug         build debug + flash
  python tools/scripts/flash.py flash --file build/app.bin   flash specific file
  python tools/scripts/flash.py flash --skip-verify          flash without verification
  python tools/scripts/flash.py eeprom dump --out data.bin   dump EEPROM
  python tools/scripts/flash.py info                         show chip info
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
    p_flash.add_argument("-D", "--cmake-define", action="append", default=[],
                         help="Extra CMake cache definition (KEY=VALUE). Repeatable.")
    p_flash.add_argument("-f", "--file", metavar="FILE",
                         help="Flash a specific .bin/.hex/.elf (skip build)")
    p_flash.add_argument("--skip-erase",  action="store_true")
    p_flash.add_argument("--skip-verify", action="store_true")
    p_flash.add_argument("--skip-reset",  action="store_true")

    # ── build ──────────────────────────────────────────────────────────────────
    p_build = sub.add_parser("build", help="Build firmware only")
    p_build.add_argument("-p", "--preset", default=DEFAULT_PRESET)
    p_build.add_argument("-D", "--cmake-define", action="append", default=[],
                         help="Extra CMake cache definition (KEY=VALUE). Repeatable.")

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
            "  Run:  python tools/scripts/setup.py          (auto-download)\n"
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
