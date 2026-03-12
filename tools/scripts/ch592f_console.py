#!/usr/bin/env python3
"""Interactive terminal console for BinaryKeyboard project helpers."""

from __future__ import annotations

import argparse
import glob as globmod
import json
import locale
import os
import shlex
import shutil
import subprocess
import sys
import time
import unicodedata
import webbrowser
from pathlib import Path

try:
    import curses
except ImportError:  # pragma: no cover - Windows fallback
    curses = None

# ── Line cache for expensive calls (git, doctor, etc.) ────────────────────────
_line_cache: dict[str, tuple[float, list[str]]] = {}

try:
    locale.setlocale(locale.LC_ALL, "")
except locale.Error:
    pass


def _cached(key: str, fn, ttl: float = 5.0) -> list[str]:
    """Return cached result of fn() if within TTL, else recompute."""
    now = time.monotonic()
    entry = _line_cache.get(key)
    if entry and now - entry[0] < ttl:
        return list(entry[1])
    result = fn()
    _line_cache[key] = (now, result)
    return list(result)


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
STUDIO_DIR = PROJECT_ROOT / "tools" / "studio"
DOCS_DIR = PROJECT_ROOT / "docs"
FLASH_SCRIPT = SCRIPT_DIR / "flash.py"
SETUP_SCRIPT = SCRIPT_DIR / "setup.py"
STATE_FILE = SCRIPT_DIR / ".binarykeyboard_console_state.json"
LEGACY_STATE_FILE = SCRIPT_DIR / ".ch592f_console_state.json"
USER_PRESETS = FIRMWARE_DIR / "CMakeUserPresets.json"
USER_PRESETS_EXAMPLE = FIRMWARE_DIR / "CMakeUserPresets.json.example"

DOC_URLS = {
    "MRS download": "http://www.mounriver.com/download",
    "wchisp release": "https://github.com/ch32-rs/wchisp/releases",
    "WCH homepage": "https://www.wch-ic.com/",
    "Project repo": "https://github.com/MeowKJ/BinaryKeyboard",
}

DEFAULT_STATE = {
    "layout": "5KEY",
    "build_type": "release",
    "toolchain_root": "",
}


def load_state() -> dict:
    state_path = STATE_FILE if STATE_FILE.is_file() else LEGACY_STATE_FILE
    if not state_path.is_file():
        return dict(DEFAULT_STATE)
    try:
        data = json.loads(state_path.read_text())
    except Exception:
        return dict(DEFAULT_STATE)
    state = dict(DEFAULT_STATE)
    state.update({k: v for k, v in data.items() if k in state})
    return state


def save_state(state: dict) -> None:
    STATE_FILE.write_text(json.dumps(state, indent=2, ensure_ascii=True) + "\n")


def base_preset(state: dict) -> str:
    suffix = "knob" if state["layout"] == "KNOB" else "5key"
    return f'{state["build_type"]}-{suffix}'


def resolved_preset_label(state: dict) -> str:
    shared = base_preset(state)
    local = f"local-{shared}"
    return local if USER_PRESETS.is_file() else shared


def prompt_line(stdscr, prompt: str, default: str = "") -> str:
    suspend_curses(stdscr)
    try:
        shown = f"{prompt}"
        if default:
            shown += f" [{default}]"
        shown += ": "
        value = input(shown).strip()
    finally:
        resume_curses(stdscr)
    return value or default


def choose_directory_dialog(default: str = "") -> str:
    if sys.platform == "darwin":
        path = _choose_directory_macos(default) or _choose_directory_tk(default)
    elif sys.platform.startswith("win"):
        path = _choose_directory_windows(default) or _choose_directory_tk(default)
    else:
        path = _choose_directory_linux(default) or _choose_directory_tk(default)
    return path.strip()


def _choose_directory_macos(default: str = "") -> str:
    script = 'POSIX path of (choose folder with prompt "Select MRS Toolchain root")'
    if default and Path(default).exists():
        escaped = default.replace("\\", "\\\\").replace('"', '\\"')
        script = (
            'POSIX path of (choose folder with prompt "Select MRS Toolchain root" '
            f'default location POSIX file "{escaped}")'
        )
    code, out, _ = capture_command(["osascript", "-e", script])
    return out if code == 0 else ""


def _choose_directory_windows(default: str = "") -> str:
    shell = shutil.which("powershell") or shutil.which("pwsh")
    if not shell:
        return ""

    script_lines = [
        "Add-Type -AssemblyName System.Windows.Forms",
        "$dialog = New-Object System.Windows.Forms.FolderBrowserDialog",
        "$dialog.Description = 'Select MRS Toolchain root'",
        "$dialog.ShowNewFolderButton = $false",
    ]
    if default and Path(default).exists():
        escaped = default.replace("'", "''")
        script_lines.append(f"$dialog.SelectedPath = '{escaped}'")
    script_lines.extend([
        "if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {",
        "  [Console]::Out.Write($dialog.SelectedPath)",
        "}",
    ])
    code, out, _ = capture_command([shell, "-NoProfile", "-STA", "-Command", "\n".join(script_lines)])
    return out if code == 0 else ""


def _choose_directory_linux(default: str = "") -> str:
    # Only attempt GUI pickers when a desktop session is available.
    if not (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        return ""

    initial = default if default and Path(default).exists() else str(PROJECT_ROOT)

    zenity = shutil.which("zenity")
    if zenity:
        code, out, _ = capture_command([
            zenity,
            "--file-selection",
            "--directory",
            "--title=Select MRS Toolchain root",
            f"--filename={initial.rstrip('/')}/",
        ])
        if code == 0:
            return out

    kdialog = shutil.which("kdialog")
    if kdialog:
        code, out, _ = capture_command([
            kdialog,
            "--getexistingdirectory",
            initial,
            "--title",
            "Select MRS Toolchain root",
        ])
        if code == 0:
            return out

    return ""


def _choose_directory_tk(default: str = "") -> str:
    try:
        import tkinter as tk
        from tkinter import filedialog
    except Exception:
        return ""

    initialdir = default if default and Path(default).exists() else str(PROJECT_ROOT)
    root = tk.Tk()
    root.withdraw()
    try:
        root.attributes("-topmost", True)
    except Exception:
        pass
    try:
        return filedialog.askdirectory(
            title="Select MRS Toolchain root",
            initialdir=initialdir,
            mustexist=True,
        )
    finally:
        root.destroy()


def show_text(stdscr, title: str, lines: list[str]) -> None:
    suspend_curses(stdscr)
    try:
        print(f"\n== {title} ==")
        for line in lines:
            print(line)
        input("\nPress Enter to return...")
    finally:
        resume_curses(stdscr)


def suspend_curses(stdscr) -> None:
    if curses is None:
        return
    curses.def_prog_mode()
    curses.endwin()


def resume_curses(stdscr) -> None:
    if curses is None:
        return
    curses.reset_prog_mode()
    stdscr.refresh()


def run_command(stdscr, cmd: list[str], cwd: Path = PROJECT_ROOT) -> None:
    suspend_curses(stdscr)
    try:
        print("\n$ " + " ".join(shlex.quote(part) for part in cmd))
        subprocess.run(cmd, cwd=str(cwd), check=False)
        input("\nPress Enter to return...")
    finally:
        resume_curses(stdscr)


def capture_command(cmd: list[str], cwd: Path = PROJECT_ROOT) -> tuple[int, str, str]:
    try:
        result = subprocess.run(
            cmd,
            cwd=str(cwd),
            capture_output=True,
            text=True,
            check=False,
        )
    except FileNotFoundError as exc:
        return (127, "", str(exc))
    return (result.returncode, result.stdout.strip(), result.stderr.strip())


def char_cells(ch: str) -> int:
    if not ch:
        return 0
    if unicodedata.combining(ch):
        return 0
    if unicodedata.east_asian_width(ch) in ("W", "F"):
        return 2
    return 1


def text_cells(text: str) -> int:
    return sum(char_cells(ch) for ch in text)


def trim_to_cells(text: str, max_cells: int) -> str:
    if max_cells <= 0:
        return ""
    cells = 0
    out: list[str] = []
    for ch in text:
        width = char_cells(ch)
        if cells + width > max_cells:
            break
        out.append(ch)
        cells += width
    return "".join(out)


def safe_addnstr(stdscr, y: int, x: int, text: str, max_len: int, attr: int = 0) -> None:
    height, width = stdscr.getmaxyx()
    if y < 0 or y >= height or x < 0 or x >= width or max_len <= 0:
        return
    try:
        clipped = trim_to_cells(text, min(max_len, width - x))
        stdscr.addstr(y, x, clipped, attr)
    except curses.error:
        pass


def safe_hline(stdscr, y: int, x: int, ch: int, count: int) -> None:
    height, width = stdscr.getmaxyx()
    if y < 0 or y >= height or x < 0 or x >= width or count <= 0:
        return
    try:
        stdscr.hline(y, x, ch, min(count, width - x))
    except curses.error:
        pass


# ── CMakeUserPresets.json helpers ─────────────────────────────────────────────

def read_toolchain_from_presets() -> str:
    """Read MRS_TOOLCHAIN_ROOT from existing CMakeUserPresets.json, or empty."""
    if not USER_PRESETS.is_file():
        return ""
    try:
        data = json.loads(USER_PRESETS.read_text())
        for preset in data.get("configurePresets", []):
            val = preset.get("cacheVariables", {}).get("MRS_TOOLCHAIN_ROOT", "")
            if val and val != "/path/to/MRS_Toolchain/Toolchain":
                return val
    except Exception:
        pass
    return ""


def _effective_toolchain(state: dict) -> str:
    """Return the effective toolchain root from state, presets, or env."""
    return state["toolchain_root"] or read_toolchain_from_presets() or os.environ.get("MRS_TOOLCHAIN_ROOT", "")


def write_user_presets(toolchain_root: str) -> str:
    if not toolchain_root.strip():
        return "Toolchain root is empty."
    if not USER_PRESETS_EXAMPLE.is_file():
        return f"Missing template: {USER_PRESETS_EXAMPLE}"
    data = json.loads(USER_PRESETS_EXAMPLE.read_text())
    for preset in data.get("configurePresets", []):
        cache = preset.setdefault("cacheVariables", {})
        cache["MRS_TOOLCHAIN_ROOT"] = toolchain_root
    USER_PRESETS.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n")
    return f"Wrote {USER_PRESETS}"


# ── Tool validation helpers ───────────────────────────────────────────────────

def _find_gcc_in_toolchain(toolchain_root: str) -> str | None:
    """Find riscv-*-elf-gcc under the toolchain root, or None."""
    if not toolchain_root or not Path(toolchain_root).is_dir():
        return None
    pattern = str(Path(toolchain_root) / "**" / "bin" / "riscv-*-elf-gcc")
    hits = globmod.glob(pattern, recursive=True)
    return hits[0] if hits else None


def _tool_version(cmd: list[str]) -> str:
    """Run a command and return stdout first line, or empty."""
    code, out, err = capture_command(cmd)
    if code == 0:
        return (out or err).splitlines()[0] if (out or err) else "ok"
    return ""


def detect_tools(state: dict) -> list[str]:
    wchisp_local = SCRIPT_DIR / ("wchisp.exe" if os.name == "nt" else "wchisp")
    toolchain = _effective_toolchain(state)
    items = [
        f"python: {sys.executable}",
        f"cmake: {shutil.which('cmake') or 'missing'}",
        f"ninja: {shutil.which('ninja') or 'missing'}",
        f"wchisp: {str(wchisp_local) if wchisp_local.is_file() else (shutil.which('wchisp') or 'missing')}",
        f"CMakeUserPresets: {'present' if USER_PRESETS.is_file() else 'missing'}",
        f"MRS_TOOLCHAIN_ROOT: {toolchain or 'unset'}",
    ]
    if toolchain:
        gcc = _find_gcc_in_toolchain(toolchain)
        items.append(f"  riscv-gcc: {gcc or 'not found in toolchain path'}")
    return items


def git_dirty() -> bool:
    code, out, _ = capture_command(["git", "status", "--porcelain"])
    return code == 0 and bool(out.strip())


def doctor_lines(state: dict) -> list[str]:
    items: list[str] = []

    # 1. Required tools — existence + version
    cmake_path = shutil.which("cmake")
    if cmake_path:
        ver = _tool_version([cmake_path, "--version"])
        items.append(f"[OK] cmake: {cmake_path} ({ver})")
    else:
        items.append("[WARN] cmake: missing")

    ninja_path = shutil.which("ninja")
    if ninja_path:
        ver = _tool_version([ninja_path, "--version"])
        items.append(f"[OK] ninja: {ninja_path} ({ver})")
    else:
        items.append("[WARN] ninja: missing")

    # 2. wchisp
    local_wchisp = SCRIPT_DIR / ("wchisp.exe" if os.name == "nt" else "wchisp")
    wchisp_path = str(local_wchisp) if local_wchisp.is_file() else shutil.which("wchisp")
    if wchisp_path:
        ver = _tool_version([wchisp_path, "--version"])
        items.append(f"[OK] wchisp: {wchisp_path} ({ver})")
    else:
        items.append("[WARN] wchisp: missing — run 'Install wchisp'")

    # 3. CMakeUserPresets.json
    if USER_PRESETS.is_file():
        items.append(f"[OK] CMakeUserPresets.json: present")
    else:
        items.append("[WARN] CMakeUserPresets.json: missing — run 'Configure toolchain'")

    # 4. Toolchain — path exists + compiler found
    toolchain = _effective_toolchain(state)
    if toolchain:
        if Path(toolchain).is_dir():
            gcc = _find_gcc_in_toolchain(toolchain)
            if gcc:
                items.append(f"[OK] toolchain: {toolchain}")
                items.append(f"[OK] riscv-gcc: {Path(gcc).name}")
            else:
                items.append(f"[WARN] toolchain: {toolchain} (no riscv-*-elf-gcc found)")
        else:
            items.append(f"[WARN] toolchain: {toolchain} (path does not exist)")
    else:
        items.append("[WARN] toolchain: not configured")

    # 5. Optional tools
    for name in ("pnpm", "node", "git"):
        path = shutil.which(name)
        items.append(f"[{'OK' if path else 'WARN'}] {name}: {path or 'missing'}")

    # 6. Git state
    items.append(f"[{'WARN' if git_dirty() else 'OK'}] git worktree: {'dirty' if git_dirty() else 'clean'}")

    return items


# ── Actions ───────────────────────────────────────────────────────────────────

def action_check_tools(state: dict, stdscr) -> None:
    show_text(stdscr, "Environment", detect_tools(state))


def action_install_wchisp(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(SETUP_SCRIPT)])


def action_toggle_layout(state: dict, stdscr) -> None:
    state["layout"] = "KNOB" if state["layout"] == "5KEY" else "5KEY"
    save_state(state)


def action_toggle_build_type(state: dict, stdscr) -> None:
    state["build_type"] = "debug" if state["build_type"] == "release" else "release"
    save_state(state)


def action_configure_toolchain(state: dict, stdscr) -> None:
    """Prompt for toolchain root and always write CMakeUserPresets.json."""
    default = _effective_toolchain(state) or "/path/to/MRS_Toolchain/Toolchain"
    toolchain_root = choose_directory_dialog(default)
    if not toolchain_root:
        toolchain_root = prompt_line(stdscr, "MRS_TOOLCHAIN_ROOT", default)
    if not toolchain_root.strip():
        show_text(stdscr, "Toolchain", ["Toolchain root is empty, aborted."])
        return
    state["toolchain_root"] = toolchain_root
    save_state(state)
    msg = write_user_presets(toolchain_root)
    # Validate the path
    gcc = _find_gcc_in_toolchain(toolchain_root)
    lines = [msg, ""]
    if gcc:
        lines.append(f"[OK] Found compiler: {Path(gcc).name}")
    elif Path(toolchain_root).is_dir():
        lines.append("[WARN] No riscv-*-elf-gcc found under this path")
    else:
        lines.append("[WARN] Path does not exist on disk")
    lines.append(f"Preset will resolve to: local-*")
    show_text(stdscr, "Toolchain", lines)


def action_open_url(name: str, url: str, state: dict, stdscr) -> None:
    if webbrowser.open(url):
        show_text(stdscr, "Open URL", [f"Opened {name}: {url}"])
    else:
        show_text(stdscr, "Open URL", [f"Failed to open browser automatically.", url])


def action_show_commands(state: dict, stdscr) -> None:
    preset = base_preset(state)
    lines = [
        f"Preset: {resolved_preset_label(state)}",
        "",
        "Build:",
        f"  python3 {FLASH_SCRIPT} build --preset {preset}",
        "",
        "Flash:",
        f"  python3 {FLASH_SCRIPT} flash --preset {preset}",
    ]
    show_text(stdscr, "Commands", lines)


def isp_lines(state: dict) -> list[str]:
    wchisp_local = SCRIPT_DIR / ("wchisp.exe" if os.name == "nt" else "wchisp")
    wchisp_path = str(wchisp_local) if wchisp_local.is_file() else (shutil.which("wchisp") or "missing")
    preset = resolved_preset_label(state)
    return [
        f"wchisp: {wchisp_path}",
        f"flash wrapper: {FLASH_SCRIPT}",
        f"default preset: {preset}",
        "",
        "Transport options:",
        "  -d/--device N   choose USB device index",
        "  -s/--serial     use serial transport",
        "  --port PORT     serial port path",
        "",
        "ISP commands:",
        "  info                chip info / UID / bootloader",
        "  probe               list connected ISP devices",
        "  flash               build or use file and program flash",
        "  verify              verify chip flash against image",
        "  erase               erase code flash",
        "  reset               reset target chip",
        "  eeprom dump         dump EEPROM to file",
        "  eeprom erase        erase data EEPROM",
        "  eeprom write        write file into EEPROM",
        "  config info         read config registers",
        "  config reset        reset config registers",
    ]


def action_show_isp_commands(state: dict, stdscr) -> None:
    preset = base_preset(state)
    lines = [
        "ISP command sheet",
        "",
        f"python3 {FLASH_SCRIPT} info",
        f"python3 {FLASH_SCRIPT} probe",
        f"python3 {FLASH_SCRIPT} flash --preset {preset}",
        f"python3 {FLASH_SCRIPT} verify --preset {preset}",
        f"python3 {FLASH_SCRIPT} erase",
        f"python3 {FLASH_SCRIPT} reset",
        f"python3 {FLASH_SCRIPT} eeprom dump --out eeprom_dump.bin",
        f"python3 {FLASH_SCRIPT} eeprom erase",
        f"python3 {FLASH_SCRIPT} eeprom write --file eeprom_dump.bin",
        f"python3 {FLASH_SCRIPT} config info",
        f"python3 {FLASH_SCRIPT} config reset",
        "",
        "Optional transport flags:",
        "  -d 0",
        "  -s --port /dev/cu.usbmodemXXXX",
    ]
    show_text(stdscr, "ISP Commands", lines)


def action_build(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "build", "--preset", base_preset(state)])


def action_flash(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "flash", "--preset", base_preset(state)])


def action_probe(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "probe"])


def action_isp_info(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "info"])


def action_isp_verify(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "verify", "--preset", base_preset(state)])


def action_isp_erase(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "erase"])


def action_isp_reset(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "reset"])


def action_isp_eeprom_dump(state: dict, stdscr) -> None:
    out_file = prompt_line(stdscr, "EEPROM dump output file", "eeprom_dump.bin")
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "eeprom", "dump", "--out", out_file])


def action_isp_eeprom_write(state: dict, stdscr) -> None:
    file_path = prompt_line(stdscr, "EEPROM input file", "eeprom_dump.bin")
    if not file_path.strip():
        show_text(stdscr, "EEPROM Write", ["File path is empty, aborted."])
        return
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "eeprom", "write", "--file", file_path])


def action_isp_eeprom_erase(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "eeprom", "erase"])


def action_isp_config_info(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "config", "info"])


def action_isp_config_reset(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "config", "reset"])


def action_isp_chip_menu(state: dict, stdscr) -> None:
    choice = prompt_line(stdscr, "Chip command [probe/info/verify/erase/reset]", "probe").strip().lower()
    actions = {
        "probe": action_probe,
        "info": action_isp_info,
        "verify": action_isp_verify,
        "erase": action_isp_erase,
        "reset": action_isp_reset,
    }
    fn = actions.get(choice)
    if not fn:
        show_text(stdscr, "ISP Chip Command", [f"Unknown command: {choice}"])
        return
    fn(state, stdscr)


def action_isp_eeprom_menu(state: dict, stdscr) -> None:
    choice = prompt_line(stdscr, "EEPROM command [dump/write/erase]", "dump").strip().lower()
    actions = {
        "dump": action_isp_eeprom_dump,
        "write": action_isp_eeprom_write,
        "erase": action_isp_eeprom_erase,
    }
    fn = actions.get(choice)
    if not fn:
        show_text(stdscr, "ISP EEPROM Command", [f"Unknown command: {choice}"])
        return
    fn(state, stdscr)


def action_isp_config_menu(state: dict, stdscr) -> None:
    choice = prompt_line(stdscr, "Config command [info/reset]", "info").strip().lower()
    actions = {
        "info": action_isp_config_info,
        "reset": action_isp_config_reset,
    }
    fn = actions.get(choice)
    if not fn:
        show_text(stdscr, "ISP Config Command", [f"Unknown command: {choice}"])
        return
    fn(state, stdscr)


def action_studio_install(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "install"], cwd=STUDIO_DIR)


def action_studio_build(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "build"], cwd=STUDIO_DIR)


def action_docs_install(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "install"], cwd=DOCS_DIR)


def action_docs_build(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "build"], cwd=DOCS_DIR)


def action_doctor_report(state: dict, stdscr) -> None:
    show_text(stdscr, "Doctor", doctor_lines(state))


# ── Tab layout ────────────────────────────────────────────────────────────────

def build_tabs(state: dict) -> list[dict]:
    toolchain = _effective_toolchain(state) or "(unset)"
    preset_status = "present" if USER_PRESETS.is_file() else "missing — run Configure toolchain"
    return [
        {
            "name": "Home",
            "lines": [
                "BinaryKeyboard project console",
                "",
                f"Layout: {state['layout']}    Build: {state['build_type']}",
                f"Preset: {resolved_preset_label(state)}",
                f"Toolchain: {toolchain}",
                f"CMakeUserPresets: {preset_status}",
            ],
            "actions": [
                {"label": f"Toggle layout  [{state['layout']}]", "hint": "Switch between 5KEY and KNOB.", "fn": action_toggle_layout},
                {"label": f"Toggle build type  [{state['build_type']}]", "hint": "Switch between release and debug.", "fn": action_toggle_build_type},
                {"label": "Configure toolchain", "hint": "Set MRS_TOOLCHAIN_ROOT and write CMakeUserPresets.json.", "fn": action_configure_toolchain},
                {"label": "Build selected preset", "hint": "Run tools/scripts/flash.py build.", "fn": action_build},
                {"label": "Flash selected preset", "hint": "Build + flash.", "fn": action_flash},
                {"label": "Show build commands", "hint": "Print the resolved cmake commands.", "fn": action_show_commands},
                {"label": "Install or update wchisp", "hint": "Run tools/scripts/setup.py.", "fn": action_install_wchisp},
                {"label": "Probe ISP devices", "hint": "List connected WCH ISP devices.", "fn": action_probe},
            ],
        },
        {
            "name": "ISP",
            "lines": isp_lines(state),
            "actions": [
                {"label": "Show ISP command sheet", "hint": "Print all flash.py ISP commands and examples.", "fn": action_show_isp_commands},
                {"label": "Probe devices", "hint": "Run flash.py probe.", "fn": action_probe},
                {"label": "Chip commands...", "hint": "Choose info / verify / erase / reset.", "fn": action_isp_chip_menu},
                {"label": "EEPROM commands...", "hint": "Choose dump / write / erase.", "fn": action_isp_eeprom_menu},
                {"label": "Config commands...", "hint": "Choose config info / reset.", "fn": action_isp_config_menu},
            ],
        },
        {
            "name": "Studio",
            "lines": [
                f"Path: {STUDIO_DIR}",
                "Stack: Vue 3 + Vite + PrimeVue",
            ],
            "actions": [
                {"label": "Install dependencies", "hint": "Run pnpm install in tools/studio.", "fn": action_studio_install},
                {"label": "Build studio", "hint": "Run pnpm run build in tools/studio.", "fn": action_studio_build},
            ],
        },
        {
            "name": "Docs",
            "lines": [
                f"Path: {DOCS_DIR}",
                "Stack: VitePress",
            ],
            "actions": [
                {"label": "Install dependencies", "hint": "Run pnpm install in docs.", "fn": action_docs_install},
                {"label": "Build docs", "hint": "Run pnpm run build in docs.", "fn": action_docs_build},
            ],
        },
        {
            "name": "Doctor",
            "lines": _cached("doctor", lambda: doctor_lines(state)),
            "actions": [
                {"label": "Show doctor report", "hint": "Rerun all health checks.", "fn": action_doctor_report},
                {"label": "Check tools", "hint": "Show cmake/ninja/wchisp/toolchain detection.", "fn": action_check_tools},
            ],
        },
        {
            "name": "Links",
            "lines": [f"{name}: {url}" for name, url in DOC_URLS.items()],
            "actions": [
                {"label": "Open MRS download", "hint": "MounRiver Studio download page.", "fn": lambda s, w: action_open_url("MRS download", DOC_URLS["MRS download"], s, w)},
                {"label": "Open wchisp releases", "hint": "wchisp release page.", "fn": lambda s, w: action_open_url("wchisp release", DOC_URLS["wchisp release"], s, w)},
                {"label": "Open WCH homepage", "hint": "Official WCH site.", "fn": lambda s, w: action_open_url("WCH homepage", DOC_URLS["WCH homepage"], s, w)},
                {"label": "Open project repo", "hint": "BinaryKeyboard repository.", "fn": lambda s, w: action_open_url("Project repo", DOC_URLS["Project repo"], s, w)},
            ],
        },
    ]


# ── Curses UI ─────────────────────────────────────────────────────────────────

def _content_attr(line: str) -> int:
    """Return curses attr for a content line based on prefix."""
    stripped = line.lstrip()
    if stripped.startswith("[OK]"):
        return curses.color_pair(5)
    if stripped.startswith("[WARN]"):
        return curses.color_pair(7)
    return curses.A_NORMAL


def draw_screen(stdscr, state: dict, tabs: list[dict], tab_index: int, action_index: int) -> tuple[list[tuple[int, int, int]], list[tuple[int, int]]]:
    stdscr.erase()
    height, width = stdscr.getmaxyx()
    tab_regions: list[tuple[int, int, int]] = []
    action_rows: list[tuple[int, int]] = []
    compact = width < 96 or height < 26
    show_hints = not compact and width >= 96 and height >= 26

    # ── Title ─────────────────────────────────────────────────────────────────
    title = "BK Console" if compact else "BinaryKeyboard Console"
    subline = "Arrows/Enter  click select  dbl-click run  q quit" if compact else "Mouse click selects, double-click runs. Arrow keys + Enter also work. Press q to quit."
    safe_addnstr(stdscr, 0, 2, title, max(0, width - 4), curses.color_pair(1) | curses.A_BOLD)
    safe_addnstr(stdscr, 1, 2, subline, max(0, width - 4), curses.A_DIM)

    # ── Tab bar ───────────────────────────────────────────────────────────────
    tab_labels = [f"[ {tab['name']} ]" for tab in tabs]
    start_idx = 0
    while start_idx < tab_index:
        reserved = 2 if start_idx > 0 else 0
        needed = sum(text_cells(tab_labels[i]) + 1 for i in range(start_idx, tab_index + 1))
        if 2 + reserved + needed <= max(8, width - 2):
            break
        start_idx += 1

    x = 2
    if start_idx > 0:
        safe_addnstr(stdscr, 3, x, "< ", 2, curses.color_pair(1) | curses.A_BOLD)
        x += 2

    last_visible = start_idx - 1
    for idx in range(start_idx, len(tabs)):
        label = tab_labels[idx]
        if x >= width - 2:
            break
        remaining = width - x - 1
        if idx < len(tabs) - 1 and remaining <= text_cells(label):
            break
        if idx == tab_index:
            attr = curses.color_pair(2) | curses.A_BOLD
        else:
            attr = curses.A_NORMAL
        safe_addnstr(stdscr, 3, x, label, max(0, width - x - 1), attr)
        tab_width = min(text_cells(label), max(0, width - x - 1))
        tab_regions.append((x, x + max(0, tab_width - 1), idx))
        x += tab_width + 1
        last_visible = idx

    if last_visible < len(tabs) - 1 and width >= 4:
        safe_addnstr(stdscr, 3, max(2, width - 3), " >", 2, curses.color_pair(1) | curses.A_BOLD)

    # ── Content area ──────────────────────────────────────────────────────────
    body = tabs[tab_index]
    safe_hline(stdscr, 4, 1, curses.ACS_HLINE, max(0, width - 2))
    action_row_height = 2 if show_hints else 1
    action_block_height = max(1, len(body["actions"]) * action_row_height)
    content_top = 5 if compact else 6
    content_limit = max(content_top, height - action_block_height - 3)
    for offset, line in enumerate(body["lines"]):
        y = content_top + offset
        if y >= content_limit:
            break
        safe_addnstr(stdscr, y, 2, line, max(0, width - 4), _content_attr(line))

    # ── Action list ───────────────────────────────────────────────────────────
    action_start = content_limit + 1
    safe_hline(stdscr, action_start - 1, 1, curses.ACS_HLINE, max(0, width - 2))
    for idx, action in enumerate(body["actions"]):
        y = action_start + idx * action_row_height
        if y >= height - 2:
            break
        action_rows.append((y, idx))
        if idx == action_index:
            safe_addnstr(stdscr, y, 2, f" {idx + 1}. {action['label']} ", max(0, width - 4), curses.color_pair(4) | curses.A_BOLD)
        else:
            num_str = f" {idx + 1}."
            safe_addnstr(stdscr, y, 2, num_str, text_cells(num_str), curses.color_pair(3))
            safe_addnstr(stdscr, y, 2 + text_cells(num_str), f" {action['label']}", max(0, width - 4 - text_cells(num_str)), curses.A_NORMAL)
        if show_hints and y + 1 < height - 2:
            safe_addnstr(stdscr, y + 1, 5, action["hint"], max(0, width - 7), curses.A_DIM)

    # ── Status bar ────────────────────────────────────────────────────────────
    if height > 2:
        status = (
            f" {tabs[tab_index]['name']} | \u2190\u2192 tabs \u2191\u2193 select Enter run click select dbl-click run q quit "
            if compact
            else f" {tabs[tab_index]['name']}  |  \u2190\u2192 tabs  \u2191\u2193 select  Enter run  click select  double-click run  r refresh  q quit "
        )
        try:
            stdscr.addstr(height - 1, 0, trim_to_cells(status.ljust(width), width), curses.color_pair(6))
        except curses.error:
            pass

    stdscr.refresh()
    return tab_regions, action_rows


def run_curses(stdscr) -> None:
    state = load_state()
    tab_index = 0
    action_index = 0

    curses.curs_set(0)
    curses.start_color()
    curses.use_default_colors()
    bg = -1
    curses.init_pair(1, curses.COLOR_CYAN, bg)       # title
    curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_CYAN)   # active tab
    curses.init_pair(3, curses.COLOR_CYAN, bg)       # action number
    curses.init_pair(4, curses.COLOR_BLACK, curses.COLOR_GREEN)  # selected action
    curses.init_pair(5, curses.COLOR_GREEN, bg)      # [OK]
    curses.init_pair(6, curses.COLOR_BLACK, curses.COLOR_WHITE)  # status bar
    curses.init_pair(7, curses.COLOR_YELLOW, bg)     # [WARN]
    curses.mouseinterval(180)
    curses.mousemask(
        curses.BUTTON1_CLICKED
        | curses.BUTTON1_DOUBLE_CLICKED
        | curses.BUTTON1_RELEASED
    )
    stdscr.keypad(True)

    while True:
        tabs = build_tabs(state)
        action_count = len(tabs[tab_index]["actions"])
        action_index = max(0, min(action_index, action_count - 1))
        tab_regions, action_rows = draw_screen(stdscr, state, tabs, tab_index, action_index)
        key = stdscr.getch()

        if key in (ord("q"), ord("Q")):
            save_state(state)
            return
        if key in (ord("r"), ord("R")):
            _line_cache.clear()
            continue
        if key in (curses.KEY_RIGHT, ord("l")):
            tab_index = (tab_index + 1) % len(tabs)
            action_index = 0
            continue
        if key in (curses.KEY_LEFT, ord("h")):
            tab_index = (tab_index - 1) % len(tabs)
            action_index = 0
            continue
        if key in (curses.KEY_DOWN, ord("j")):
            action_index = (action_index + 1) % action_count
            continue
        if key in (curses.KEY_UP, ord("k")):
            action_index = (action_index - 1) % action_count
            continue
        if key in (10, 13, curses.KEY_ENTER):
            tabs[tab_index]["actions"][action_index]["fn"](state, stdscr)
            _line_cache.clear()
            save_state(state)
            continue
        if ord("1") <= key <= ord("9"):
            idx = key - ord("1")
            if idx < action_count:
                action_index = idx
                tabs[tab_index]["actions"][action_index]["fn"](state, stdscr)
                _line_cache.clear()
                save_state(state)
            continue
        if key == curses.KEY_MOUSE:
            try:
                _, mx, my, _, bstate = curses.getmouse()
            except curses.error:
                continue
            left_click = bool(bstate & curses.BUTTON1_CLICKED)
            left_double = bool(bstate & curses.BUTTON1_DOUBLE_CLICKED)
            if not (left_click or left_double):
                continue
            for x0, x1, idx in tab_regions:
                if my == 3 and x0 <= mx <= x1:
                    tab_index = idx
                    action_index = 0
                    break
            else:
                for y, idx in action_rows:
                    if my == y:
                        action_index = idx
                        if left_double:
                            tabs[tab_index]["actions"][action_index]["fn"](state, stdscr)
                            _line_cache.clear()
                            save_state(state)
                        break


# ── Text mode fallback ────────────────────────────────────────────────────────

def run_text_mode() -> None:
    _color = sys.stdout.isatty()

    def _c(code: str, text: str) -> str:
        return f"\033[{code}m{text}\033[0m" if _color else text

    state = load_state()
    while True:
        tabs = build_tabs(state)
        print(f"\n{_c('1;36', 'BinaryKeyboard Console')}")
        print(_c("2", f"CH592F layout={state['layout']} build={state['build_type']} preset={resolved_preset_label(state)}"))
        print(_c("2", "-" * 44))
        for i, tab in enumerate(tabs, start=1):
            print(f"  {_c('36', str(i))}. {tab['name']}")
        print(f"  {_c('31', 'q')}. Quit")
        choice = input(_c("32", "> ")).strip().lower()
        if choice == "q":
            save_state(state)
            return
        if not choice.isdigit():
            continue
        tab_idx = int(choice) - 1
        if not 0 <= tab_idx < len(tabs):
            continue
        tab = tabs[tab_idx]
        print(f"\n{_c('1;36', tab['name'])}")
        print(_c("2", "-" * 44))
        for line in tab["lines"]:
            print(f"  {line}")
        print(_c("2", "-" * 44))
        for i, action in enumerate(tab["actions"], start=1):
            print(f"  {_c('36', str(i))}. {_c('1', action['label'])}  {_c('2', action['hint'])}")
        print(f"  {_c('2', 'b. Back')}")
        action_choice = input(_c("32", "> ")).strip().lower()
        if action_choice == "b":
            continue
        if action_choice.isdigit():
            idx = int(action_choice) - 1
            if 0 <= idx < len(tab["actions"]):
                tab["actions"][idx]["fn"](state, None)
                _line_cache.clear()
                save_state(state)


# ── Entry point ───────────────────────────────────────────────────────────────

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="tools/scripts/console.py",
        description="Interactive terminal console for BinaryKeyboard project workflows.",
    )
    parser.add_argument(
        "--text",
        action="store_true",
        help="Force the plain text fallback menu instead of curses UI.",
    )
    return parser


def main() -> None:
    args = build_parser().parse_args()
    if args.text or curses is None:
        run_text_mode()
        return
    curses.wrapper(run_curses)


if __name__ == "__main__":
    main()
