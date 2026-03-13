#!/usr/bin/env python3
"""
tools/scripts/ch592f.py — CH592F CMake build helper
"""

from __future__ import annotations

import argparse
import ctypes
import json
import platform
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Optional


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent.resolve()
FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
DEFAULT_PRESET = "release"


def _enable_win_ansi() -> None:
    if platform.system() == "Windows":
        try:
            kernel32 = ctypes.windll.kernel32
            kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
        except Exception:
            pass


_enable_win_ansi()
_USE_COLOR = sys.stdout.isatty()


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text


def info(msg: str) -> None:
    print(_c("36", "[INFO]"), msg)


def ok(msg: str) -> None:
    print(_c("32", "[ OK ]"), msg)


def warn(msg: str) -> None:
    print(_c("33", "[WARN]"), msg)


def sep() -> None:
    print(_c("2", "-" * 44))


def die(msg: str) -> None:
    print(_c("31", "[ERR ]"), msg, file=sys.stderr)
    sys.exit(1)


def _candidate_windows_cmake_paths() -> list[Path]:
    return [
        Path(r"C:\Program Files\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files (x86)\CMake\bin\cmake.exe"),
        Path(r"C:\ProgramData\chocolatey\bin\cmake.exe"),
        Path(r"C:\Users\KJ\scoop\shims\cmake.exe"),
        Path(r"C:\Users\KJ\scoop\apps\cmake\current\bin\cmake.exe"),
        Path(r"C:\msys64\ucrt64\bin\cmake.exe"),
        Path(r"C:\msys64\mingw64\bin\cmake.exe"),
        Path(r"C:\msys64\usr\bin\cmake.exe"),
        Path(r"C:\Tools\CMake\bin\cmake.exe"),
        Path(r"C:\App\Tools\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
    ]


def find_cmake() -> Optional[Path]:
    binary = "cmake.exe" if platform.system() == "Windows" else "cmake"
    found = shutil.which(binary)
    if found:
        return Path(found).resolve()

    if platform.system() == "Windows":
        for candidate in _candidate_windows_cmake_paths():
            if candidate.is_file():
                return candidate.resolve()

    return None


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


def _parse_size_row_from_output(text: str) -> Optional[tuple[int, int, int]]:
    m = re.search(r'^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9a-fA-F]+\s+\S+\.elf\s*$',
                  text, re.MULTILINE)
    if not m:
        return None
    return (int(m.group(1)), int(m.group(2)), int(m.group(3)))


def _read_memory_regions_from_linker(linker_script: Path) -> dict[str, tuple[int, int]]:
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
            origin = int(origin_s, 0)
            length = int(float(num) * unit_mul.get((unit or "B").upper(), 1))
            out[name.upper()] = (origin, length)
    except Exception:
        return {}
    return out


def _read_memory_lengths_from_linker(linker_script: Path) -> dict[str, int]:
    return {name: length for name, (_, length) in _read_memory_regions_from_linker(linker_script).items()}


def _size_row_from_artifact(build_dir: Path) -> Optional[tuple[int, int, int]]:
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
    pending: Optional[tuple[int, int, int]] = None

    for line in res.stdout.splitlines():
        parts = line.split()
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

        if "ALLOC" in line:
            size, vma, lma = pending
            if size > 0:
                if (("LOAD" in line) or ("CONTENTS" in line)) and (flash_origin <= lma < flash_end):
                    flash_used += size
                if ram_origin <= vma < ram_end:
                    ram_used += size
            pending = None
        elif line.strip():
            pending = None

    return {"FLASH": flash_used, "RAM": ram_used}


def _strip_ansi(s: str) -> str:
    return re.sub(r'\033\[[^m]*m', '', s)


def _vlen(s: str) -> int:
    return len(_strip_ansi(s))


def _rpad(s: str, w: int) -> str:
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


def _resolve_preset(preset: str) -> str:
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


def build_dir_for(preset: str) -> Path:
    return FIRMWARE_DIR / "build" / _resolve_preset(preset)


def artifact_paths(build_dir: Path) -> dict[str, Path]:
    return {
        "elf": build_dir / "CH592F.elf",
        "bin": build_dir / "CH592F.bin",
        "hex": build_dir / "CH592F.hex",
        "map": build_dir / "CH592F.map",
    }


def run(cmd: list[str], cwd: Optional[Path] = None) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, check=True)


def run_and_capture(cmd: list[str], cwd: Optional[Path] = None) -> tuple[list[str], float]:
    t0 = time.time()
    lines: list[str] = []
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(cwd) if cwd else None,
        text=True,
        bufsize=1,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.stdout is not None
    for line in proc.stdout:
        print(line, end="")
        lines.append(line)
    proc.wait()
    if proc.returncode != 0:
        raise subprocess.CalledProcessError(proc.returncode, cmd)
    return lines, time.time() - t0


def _build_report(lines: list[str], preset: str, build_dir: Path, elapsed: float) -> None:
    mem_re = re.compile(r'(\w+):\s+(\d+)\s+B\s+([\d.]+)\s+(KB|B)\s+([\d.]+)%')
    size_re = re.compile(r'^\s*(\d+)\s+(\d+)\s+(\d+)\s+\d+\s+[0-9a-fA-F]+\s+\S+\.elf\s*$')

    regions = []
    size_row = None
    report_source = "linker"
    for line in lines:
        m = mem_re.search(line)
        if m:
            name, used_b, tot_v, tot_u, pct = m.groups()
            used = int(used_b)
            total = int(float(tot_v) * 1024) if tot_u == 'KB' else int(float(tot_v))
            regions.append((name, used, total, total - used, float(pct)))
        m2 = size_re.match(line)
        if m2:
            size_row = (int(m2.group(1)), int(m2.group(2)), int(m2.group(3)))

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

    bin_file = build_dir / "CH592F.bin"
    bin_sz = bin_file.stat().st_size if bin_file.is_file() else None

    w = 68
    bc = "1;36"

    def hline(l: str, r: str, fill: str = "─") -> str:
        return _c(bc, l + fill * w + r) if _USE_COLOR else l + fill * w + r

    def brow(cells: str) -> str:
        inner = _rpad(cells, w)
        if _USE_COLOR:
            return _c(bc, "│") + inner + _c(bc, "│")
        return "│" + inner + "│"

    print()
    print(hline("┌", "┐"))
    print(brow("  " + _c("1", "CH592F Memory Report") +
               _c("2", f"  ·  preset: {preset}  ·  built in {elapsed:.1f}s  ·  source: {report_source}")))
    print(hline("├", "┤"))

    hdr = "  " + _rpad(_c("2", "Region"), 9)
    hdr += _rpad(_c("2", "Used / Total"), 22)
    hdr += _rpad(_c("2", "Free"), 12)
    hdr += _c("2", "Usage")
    print(brow(hdr))
    print(hline("├", "┤"))

    for name, used, total, free, pct in regions:
        cc = _pct_clr(pct)
        c1 = "  " + _rpad(_c("1", name), 8) + " "
        c2 = _rpad(f"{_fmt_b(used)} / {_fmt_b(total)}", 22)
        c3 = _rpad(_c(cc, _fmt_b(free)), 12)
        c4 = _usage_bar(pct) + "  " + _c(cc, f"{pct:5.1f}%")
        print(brow(c1 + c2 + c3 + c4))

    if size_row is not None or bin_sz is not None:
        print(hline("├", "┤"))
        det = "  "
        if size_row:
            text, data, bss = size_row
            det += f".text {_c('36', str(text))}   "
            det += f".data {_c('33', str(data))}   "
            det += f".bss  {_c('35', str(bss))}"
        if bin_sz is not None:
            det += f"   .bin {_c('32;1', _fmt_b(bin_sz))}"
        print(brow(det))

    print(hline("└", "┘"))


def configure(preset: str) -> tuple[str, Path]:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or add it to PATH.")
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    sep()
    info(f"Configuring CH592F preset: {_c('1', actual)}")
    run([str(cmake), "--preset", actual], cwd=FIRMWARE_DIR)
    ok(f"Configure complete → {build_dir}")
    return actual, build_dir


def build(preset: str) -> tuple[str, Path]:
    cmake = find_cmake()
    if not cmake:
        die("CMake not found. Install CMake or add it to PATH.")

    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.is_file():
        configure(preset)

    sep()
    info(f"Building CH592F preset: {_c('1', actual)}")
    lines, elapsed = run_and_capture([str(cmake), "--build", "--preset", actual], cwd=FIRMWARE_DIR)
    _build_report(lines, actual, build_dir, elapsed)

    for name, path in artifact_paths(build_dir).items():
        if path.is_file():
            ok(f"{name.upper()} → {path}")

    return actual, build_dir


def rebuild(preset: str) -> tuple[str, Path]:
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    if build_dir.is_dir():
        sep()
        info(f"Removing {build_dir}")
        shutil.rmtree(build_dir)
    configure(preset)
    return build(preset)


def clean(preset: str) -> None:
    build_dir = build_dir_for(preset)
    sep()
    if build_dir.is_dir():
        shutil.rmtree(build_dir)
        ok(f"Removed {build_dir}")
    else:
        warn(f"Build directory does not exist: {build_dir}")


def status(preset: str) -> None:
    actual = _resolve_preset(preset)
    build_dir = build_dir_for(preset)
    cache_file = build_dir / "CMakeCache.txt"

    sep()
    info(f"Preset: {preset} (actual: {actual})")
    info(f"CMake: {find_cmake() if find_cmake() else 'not found'}")
    info(f"Build directory: {build_dir}")
    info(f"Configured: {'yes' if cache_file.is_file() else 'no'}")
    for name, path in artifact_paths(build_dir).items():
        if path.is_file():
            info(f"{name.upper()}: {path} ({path.stat().st_size} B)")
        else:
            info(f"{name.upper()}: missing")


def show_artifact(preset: str, artifact_type: str) -> int:
    build_dir = build_dir_for(preset)
    artifacts = artifact_paths(build_dir)

    if artifact_type == "all":
        sep()
        for name, path in artifacts.items():
            print(f"{name}: {path}")
        return 0

    path = artifacts[artifact_type]
    print(path)
    return 0 if path.is_file() else 1


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="tools/scripts/ch592f.py",
        description="CH592F CMake build helper",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    for name in ("status", "configure", "build", "rebuild", "clean"):
        p = sub.add_parser(name, help=f"{name.capitalize()} CH592F firmware")
        p.add_argument("-p", "--preset", default=DEFAULT_PRESET)

    p_artifact = sub.add_parser("artifact", help="Print artifact path")
    p_artifact.add_argument("-p", "--preset", default=DEFAULT_PRESET)
    p_artifact.add_argument(
        "-t",
        "--type",
        default="all",
        choices=("all", "elf", "bin", "hex", "map"),
        help="Artifact type to print",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        if args.command == "status":
            status(args.preset)
            return 0
        if args.command == "configure":
            configure(args.preset)
            return 0
        if args.command == "build":
            build(args.preset)
            return 0
        if args.command == "rebuild":
            rebuild(args.preset)
            return 0
        if args.command == "clean":
            clean(args.preset)
            return 0
        if args.command == "artifact":
            return show_artifact(args.preset, args.type)
        parser.error(f"Unsupported command: {args.command}")
    except subprocess.CalledProcessError as exc:
        die(f"Command failed (exit {exc.returncode})")
    except KeyboardInterrupt:
        print()
        warn("Interrupted.")
        return 130

    return 0


if __name__ == "__main__":
    sys.exit(main())
