#!/usr/bin/env python3
"""Unified TUI for BinaryKeyboard project helpers."""

from __future__ import annotations

import argparse
import json
import locale
import os
import re
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

from ch552g import VALID_KEYBOARDS as CH552_KEYBOARDS, find_cmake, find_sdcc
from targets.ch592 import CH592_USER_PRESETS, CH592_USER_PRESETS_EXAMPLE
from targets.ch592.profile import _find_gcc_in_toolchain
from targets.registry import TARGET_ORDER, TARGET_PROFILES, get_target_profile
from tool_cache import resolve_tool_path

# ── Line cache for expensive calls (git, doctor, etc.) ────────────────────────
_line_cache: dict[str, tuple[float, list[str]]] = {}

try:
    locale.setlocale(locale.LC_ALL, "")
except locale.Error:
    pass


_USE_COLOR = sys.stdout.isatty()
ACTION_PANEL_MIN_ITEMS = 9
TEXT_BADGE_CODES = ("1;95", "1;93", "1;96", "1;92", "1;91")
_SPINNER = ("⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏")
_spinner_tick = 0


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if _USE_COLOR else text


def _text_badge(badge: str, index: int) -> str:
    color = TEXT_BADGE_CODES[index % len(TEXT_BADGE_CODES)]
    return _c(color, f"[{badge}]")


def _text_hint(text: str) -> str:
    return _c("2;94", f"tip: {text}")


def _menu_tokens(label: str) -> list[str]:
    normalized = re.sub(r"\[[^\]]*\]", " ", label)
    return [token.upper() for token in re.findall(r"[A-Za-z0-9]+", normalized)]


def _badge_candidates(label: str) -> list[str]:
    tokens = _menu_tokens(label)
    if not tokens:
        return []

    candidates: list[str] = []

    def add(value: str) -> None:
        value = value.strip().upper()
        if value and value not in candidates:
            candidates.append(value)

    add("".join(token[0] for token in tokens[:2]))
    add(tokens[0][0])
    if len(tokens[0]) >= 2:
        add(tokens[0][:2])
    if len(tokens) >= 2:
        add(tokens[0][0] + tokens[-1][0])
    if len(tokens) >= 3:
        add("".join(token[0] for token in tokens[:3]))
    if len(tokens[0]) >= 3:
        add(tokens[0][:3])
    if len(tokens) >= 2 and len(tokens[1]) >= 2:
        add(tokens[0][0] + tokens[1][:2])

    return candidates


def _menu_badges(labels: list[str]) -> list[str]:
    used: set[str] = set()
    badges: list[str] = []
    for index, label in enumerate(labels):
        badge = ""
        for candidate in _badge_candidates(label):
            if candidate not in used:
                badge = candidate
                break
        if not badge:
            badge = str(index + 1)
            while badge in used:
                badge += "X"
        used.add(badge)
        badges.append(badge)
    return badges


def _resolve_text_choice(choice: str, badges: list[str], count: int) -> int | None:
    if choice.isdigit():
        idx = int(choice) - 1
        return idx if 0 <= idx < count else None

    upper = choice.upper()
    for idx, badge in enumerate(badges):
        if upper == badge:
            return idx

    return None


def _stylize_text_line(line: str) -> str:
    stripped = line.strip()
    if not stripped:
        return line
    if stripped.startswith("[OK]"):
        return _c("32", line)
    if stripped.startswith("[WARN]"):
        return _c("33", line)
    if stripped.startswith("[ERR]") or stripped.startswith("[ERR ]"):
        return _c("31", line)
    if stripped.endswith(":") and ":" not in stripped[:-1]:
        return _c("1;36", line)
    if ": " in line:
        key, value = line.split(": ", 1)
        return f"{_c('36', key + ':')} {_c('1', value)}"
    if stripped.startswith("- "):
        return f"{line[:line.index('-')]}{_c('35', '-')} {line[line.index('-') + 2:]}"
    return line


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
STUDIO_DIR = PROJECT_ROOT / "tools" / "studio"
DOCS_DIR = PROJECT_ROOT / "docs"
FLASH_SCRIPT = SCRIPT_DIR / "flash.py"
SETUP_SCRIPT = SCRIPT_DIR / "setup.py"
STATE_FILE = SCRIPT_DIR / ".binarykeyboard_console_state.json"
LEGACY_STATE_FILE = SCRIPT_DIR / ".ch592f_console_state.json"
VSCODE_DIR = PROJECT_ROOT / ".vscode"
VSCODE_SETTINGS = VSCODE_DIR / "settings.json"
VSCODE_CPP_PROPERTIES = VSCODE_DIR / "c_cpp_properties.json"
VSCODE_SDCC_COMPAT = VSCODE_DIR / "binarykeyboard_sdcc_compat.h"
ROOT_COMPILE_COMMANDS = PROJECT_ROOT / "compile_commands.json"

DOC_URLS = {
    "MRS download": "http://www.mounriver.com/download",
    "wchisp release": "https://github.com/ch32-rs/wchisp/releases",
    "WCH homepage": "https://www.wch-ic.com/",
    "Project repo": "https://github.com/MeowKJ/BinaryKeyboard",
}

DEFAULT_STATE = {
    "target": "CH592F",
    "keyboard": "5KEY",
    "build_type": "release",
    "toolchain_root": "",
}


def _read_state_blob() -> dict:
    state_path = STATE_FILE if STATE_FILE.is_file() else LEGACY_STATE_FILE
    if not state_path.is_file():
        return {}
    try:
        data = json.loads(state_path.read_text())
    except Exception:
        return {}
    return data if isinstance(data, dict) else {}


def load_state() -> dict:
    data = _read_state_blob()
    state = dict(DEFAULT_STATE)
    state.update({k: v for k, v in data.items() if k in state})
    if state["target"] not in TARGET_PROFILES:
        state["target"] = DEFAULT_STATE["target"]
    for profile in TARGET_PROFILES.values():
        profile.normalize_state(state)
    return state


def save_state(state: dict) -> None:
    data = _read_state_blob()
    for key, default in DEFAULT_STATE.items():
        data[key] = state.get(key, default)
    STATE_FILE.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n")


def current_target_profile(state: dict):
    return get_target_profile(state["target"])


def is_ch592_target(state: dict) -> bool:
    return state["target"] == "CH592F"


def current_build_label(state: dict) -> str:
    return current_target_profile(state).build_label(state)


def display_path(path: Path) -> str:
    try:
        return str(path.relative_to(PROJECT_ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


def current_artifact_path(state: dict) -> Path:
    return current_target_profile(state).artifact_path(state)


def current_artifact_label(state: dict) -> str:
    return display_path(current_artifact_path(state))


def current_build_dir(state: dict) -> Path:
    return current_target_profile(state).build_dir(state)


def current_build_dir_label(state: dict) -> str:
    return display_path(current_build_dir(state))


def current_build_command(state: dict) -> list[str]:
    return current_target_profile(state).build_command(state)


def current_flash_command(state: dict) -> list[str]:
    return current_target_profile(state).flash_command(state)


def current_verify_command(state: dict) -> list[str]:
    return current_target_profile(state).verify_command(state)


def current_build_command_display(state: dict) -> str:
    return current_target_profile(state).build_command_display(state)


def current_flash_command_display(state: dict) -> str:
    return current_target_profile(state).flash_command_display(state)


def current_verify_command_display(state: dict) -> str:
    return current_target_profile(state).verify_command_display(state)


def prompt_line(stdscr, prompt: str, default: str = "") -> str:
    suspend_curses(stdscr)
    try:
        shown = f"{prompt}"
        if default:
            shown += f" [{default}]"
        shown += ": "
        try:
            value = input(shown).strip()
        except (EOFError, KeyboardInterrupt):
            value = ""
    finally:
        resume_curses(stdscr)
    return value or default


def wait_for_return(prompt: str = "\nPress Enter to return...") -> None:
    try:
        input(_c("2", prompt))
    except (EOFError, KeyboardInterrupt):
        print()


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
        print(f"\n{_c('1;36', f'== {title} ==')}")
        for line in lines:
            print(_stylize_text_line(line))
        wait_for_return()
    finally:
        resume_curses(stdscr)


def suspend_curses(stdscr) -> None:
    if curses is None or stdscr is None:
        return
    curses.def_prog_mode()
    curses.endwin()


def resume_curses(stdscr) -> None:
    if curses is None or stdscr is None:
        return
    curses.reset_prog_mode()
    curses.halfdelay(10)
    stdscr.refresh()


def run_command(stdscr, cmd: list[str], cwd: Path = PROJECT_ROOT) -> None:
    suspend_curses(stdscr)
    try:
        print("\n" + _c("1;36", "$") + " " + _c("1", " ".join(shlex.quote(part) for part in cmd)))
        subprocess.run(cmd, cwd=str(cwd), check=False)
        wait_for_return()
    finally:
        resume_curses(stdscr)


def run_command_sequence(stdscr, commands: list[tuple[list[str], Path]]) -> None:
    suspend_curses(stdscr)
    try:
        for cmd, cwd in commands:
            print("\n" + _c("1;36", "$") + " " + _c("1", " ".join(shlex.quote(part) for part in cmd)))
            result = subprocess.run(cmd, cwd=str(cwd), check=False)
            if result.returncode != 0:
                print(f"\n{_c('31', f'Command failed with exit {result.returncode}.')}")
                break
        wait_for_return()
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
    if not CH592_USER_PRESETS.is_file():
        return ""
    try:
        data = json.loads(CH592_USER_PRESETS.read_text())
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
    if not CH592_USER_PRESETS_EXAMPLE.is_file():
        return f"Missing template: {CH592_USER_PRESETS_EXAMPLE}"
    data = json.loads(CH592_USER_PRESETS_EXAMPLE.read_text())
    for preset in data.get("configurePresets", []):
        cache = preset.setdefault("cacheVariables", {})
        cache["MRS_TOOLCHAIN_ROOT"] = toolchain_root
    CH592_USER_PRESETS.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n")
    return f"Wrote {CH592_USER_PRESETS}"


def _read_json_file(path: Path, default):
    if not path.is_file():
        return default
    try:
        data = json.loads(path.read_text())
    except Exception:
        return default
    return data if isinstance(data, type(default)) else default


def _write_json_file(path: Path, data) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n")


def _workspace_path(path: Path) -> str:
    try:
        rel = path.resolve().relative_to(PROJECT_ROOT.resolve())
        rel_str = str(rel).replace("\\", "/")
        return f"${{workspaceFolder}}/{rel_str}" if rel_str else "${workspaceFolder}"
    except ValueError:
        return str(path.resolve()).replace("\\", "/")


def _normalize_json_path(value: str) -> str:
    return value.replace("\\", "/")


def _strip_token_quotes(value: str) -> str:
    if len(value) >= 2 and value[0] == value[-1] and value[0] in ("'", '"'):
        return value[1:-1]
    return value


def _parse_compile_tokens(entry: dict) -> list[str]:
    arguments = entry.get("arguments")
    if isinstance(arguments, list):
        return [str(part) for part in arguments]
    command = entry.get("command")
    if not isinstance(command, str) or not command.strip():
        return []
    try:
        return shlex.split(command, posix=False)
    except ValueError:
        return command.split()


def _extract_compile_context(entries: list[dict]) -> tuple[list[str], list[str], str]:
    include_paths: list[str] = []
    defines: list[str] = []
    compiler_path = ""
    seen_includes: set[str] = set()
    seen_defines: set[str] = set()

    for entry in entries:
        tokens = _parse_compile_tokens(entry)
        if not tokens:
            continue
        if not compiler_path:
            compiler_path = _normalize_json_path(_strip_token_quotes(tokens[0]))
        idx = 1
        while idx < len(tokens):
            token = tokens[idx]
            value = ""
            advance = 1
            if token == "-I" and idx + 1 < len(tokens):
                value = tokens[idx + 1]
                advance = 2
            elif token.startswith("-I"):
                value = token[2:]
            elif token == "-D" and idx + 1 < len(tokens):
                value = tokens[idx + 1]
                advance = 2
                key = _strip_token_quotes(value)
                if key not in seen_defines:
                    seen_defines.add(key)
                    defines.append(key)
                idx += advance
                continue
            elif token.startswith("-D"):
                value = token[2:]
                key = _strip_token_quotes(value)
                if key not in seen_defines:
                    seen_defines.add(key)
                    defines.append(key)
                idx += advance
                continue

            if value:
                candidate = _normalize_json_path(_strip_token_quotes(value))
                dedupe_key = candidate.lower() if os.name == "nt" else candidate
                if dedupe_key not in seen_includes:
                    seen_includes.add(dedupe_key)
                    include_paths.append(candidate)
            idx += advance

    return include_paths, defines, compiler_path


def _selected_target_fallback(state: dict) -> tuple[list[str], list[str]]:
    if is_ch592_target(state):
        include_paths = [
            PROJECT_ROOT / "firmware" / "CH592F" / "SDK" / "StdPeriphDriver" / "inc",
            PROJECT_ROOT / "firmware" / "CH592F" / "SDK" / "RVMSIS",
            PROJECT_ROOT / "firmware" / "CH592F" / "hal" / "include",
            PROJECT_ROOT / "firmware" / "CH592F" / "usb" / "include",
            PROJECT_ROOT / "firmware" / "CH592F" / "ble" / "lib",
            PROJECT_ROOT / "firmware" / "CH592F" / "ble" / "hid" / "include",
            PROJECT_ROOT / "firmware" / "CH592F" / "ble" / "profile" / "include",
            PROJECT_ROOT / "firmware" / "CH592F" / "ble" / "core" / "include",
            PROJECT_ROOT / "firmware" / "CH592F" / "keyboard" / "include",
            current_build_dir(state) / "generated",
        ]
        layout_define = "KBD_LAYOUT_KNOB" if state["keyboard"] == "KNOB" else "KBD_LAYOUT_5KEY"
        return [str(path.resolve()).replace("\\", "/") for path in include_paths], ["DEBUG=1", layout_define]

    keyboard_define = {
        "BASIC": "USE_BASIC",
        "KNOB": "USE_KNOB",
        "5KEY": "USE_5KEYS",
    }.get(state["keyboard"], "USE_BASIC")
    include_paths = [
        PROJECT_ROOT / "firmware" / "CH552G" / "SDK" / "cores",
        PROJECT_ROOT / "firmware" / "CH552G" / "SDK" / "variants" / "ch552",
        PROJECT_ROOT / "firmware" / "CH552G" / "SDK" / "libraries" / "WS2812",
        PROJECT_ROOT / "firmware" / "CH552G" / "SDK" / "libraries" / "WS2812" / "template",
        PROJECT_ROOT / "firmware" / "CH552G" / "app",
        PROJECT_ROOT / "firmware" / "CH552G" / "hal",
        PROJECT_ROOT / "firmware" / "CH552G" / "keyboard",
        PROJECT_ROOT / "firmware" / "CH552G" / "usb",
        current_build_dir(state) / "generated",
    ]
    return [str(path.resolve()).replace("\\", "/") for path in include_paths], ["CH552=1", "F_CPU=24000000", keyboard_define]


def _compile_database_paths(state: dict) -> list[Path]:
    candidates = [current_build_dir(state) / "compile_commands.json"]
    candidates.extend(sorted((PROJECT_ROOT / "firmware" / "CH552G" / "build").glob("*/compile_commands.json")))
    candidates.extend(sorted((PROJECT_ROOT / "firmware" / "CH592F" / "build").glob("*/compile_commands.json")))

    unique: list[Path] = []
    seen: set[str] = set()
    for path in candidates:
        if not path.is_file():
            continue
        resolved = str(path.resolve()).lower() if os.name == "nt" else str(path.resolve())
        if resolved in seen:
            continue
        seen.add(resolved)
        unique.append(path.resolve())
    return unique


def _read_compile_database(path: Path) -> list[dict]:
    data = _read_json_file(path, [])
    if not isinstance(data, list):
        return []
    entries: list[dict] = []
    for entry in data:
        if isinstance(entry, dict) and isinstance(entry.get("file"), str):
            entries.append(entry)
    return entries


def _merge_compile_databases(state: dict) -> tuple[list[dict], list[dict], list[Path]]:
    db_paths = _compile_database_paths(state)
    selected_path = (current_build_dir(state) / "compile_commands.json").resolve()
    merged: list[dict] = []
    selected_entries: list[dict] = []
    seen_files: set[str] = set()

    ordered_paths = sorted(
        db_paths,
        key=lambda path: 0 if path.resolve() == selected_path else 1,
    )

    for path in ordered_paths:
        entries = _read_compile_database(path)
        if path.resolve() == selected_path:
            selected_entries = entries
        for entry in entries:
            file_path = entry.get("file")
            if not isinstance(file_path, str):
                continue
            resolved = str(Path(file_path).resolve()).lower() if os.name == "nt" else str(Path(file_path).resolve())
            if resolved in seen_files:
                continue
            seen_files.add(resolved)
            merged.append(entry)
    return merged, selected_entries, ordered_paths


def _sorted_workspace_paths(paths: list[str]) -> list[str]:
    workspace_paths = [_workspace_path(Path(path)) for path in paths]
    unique: list[str] = []
    seen: set[str] = set()
    for path in workspace_paths:
        key = path.lower() if os.name == "nt" else path
        if key in seen:
            continue
        seen.add(key)
        unique.append(path)
    return sorted(unique, key=str.lower)


def _update_vscode_settings(settings_path: Path, include_paths: list[str]) -> None:
    data = _read_json_file(settings_path, {})
    data["C_Cpp.default.compileCommands"] = _workspace_path(ROOT_COMPILE_COMMANDS)
    data["C_Cpp.default.includePath"] = include_paths
    data["C_Cpp.default.forcedInclude"] = [_workspace_path(VSCODE_SDCC_COMPAT)]
    data["C_Cpp.default.cStandard"] = "c11"
    _write_json_file(settings_path, data)


def _update_vscode_cpp_properties(cpp_path: Path, include_paths: list[str], defines: list[str]) -> None:
    data = _read_json_file(cpp_path, {})
    if not isinstance(data, dict):
        data = {}
    configurations = data.get("configurations")
    if not isinstance(configurations, list):
        configurations = []

    config_name = "BinaryKeyboard Auto"
    config_payload = {
        "name": config_name,
        "compileCommands": _workspace_path(ROOT_COMPILE_COMMANDS),
        "includePath": include_paths,
        "defines": defines,
        "forcedInclude": [_workspace_path(VSCODE_SDCC_COMPAT)],
        "cStandard": "c11",
        "browse": {
            "path": include_paths,
            "limitSymbolsToIncludedHeaders": True,
        },
    }

    updated = False
    for index, item in enumerate(configurations):
        if isinstance(item, dict) and item.get("name") == config_name:
            configurations[index] = config_payload
            updated = True
            break
    if not updated:
        configurations.append(config_payload)

    data["version"] = 4
    data["configurations"] = configurations
    _write_json_file(cpp_path, data)


def _write_sdcc_compat_header(path: Path) -> None:
    contents = """#ifndef BINARYKEYBOARD_SDCC_COMPAT_H
#define BINARYKEYBOARD_SDCC_COMPAT_H

#include <stdbool.h>

#ifndef __SDCC
#ifndef __STDC_VERSION_STDINT_H__
#define __STDC_VERSION_STDINT_H__ 201112L
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int int16_t;
typedef unsigned short int uint16_t;
typedef long int int32_t;
typedef unsigned long int uint32_t;
#endif

#define __data
#define __xdata
#define __code
#define __idata
#define __pdata
#define __near
#define __reentrant
#define __critical
#define __nonbanked
#define __interrupt(...)
#define __using(...)
#define __at(...)
#define __sbit bool
#define __sfr volatile unsigned char
#define __sfr16 volatile unsigned short
#define __sfr32 volatile unsigned long
#endif

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(contents)


def generate_ide_config(state: dict, editor: str = "all") -> list[str]:
    merged_entries, selected_entries, db_paths = _merge_compile_databases(state)
    selected_includes, selected_defines, _ = _extract_compile_context(selected_entries)
    merged_includes, _, _ = _extract_compile_context(merged_entries)

    if not merged_includes:
        fallback_includes, fallback_defines = _selected_target_fallback(state)
        merged_includes = fallback_includes
        if not selected_defines:
            selected_defines = fallback_defines
    elif not selected_defines:
        _, fallback_defines = _selected_target_fallback(state)
        selected_defines = fallback_defines

    include_paths = _sorted_workspace_paths(selected_includes + merged_includes)
    if not include_paths:
        fallback_includes, _ = _selected_target_fallback(state)
        include_paths = _sorted_workspace_paths(fallback_includes)

    lines = [
        f"Target: {state['target']}",
        f"Build config: {current_build_label(state)}",
        f"Selected build dir: {current_build_dir_label(state)}",
        f"Compile databases merged: {len(db_paths)}",
        f"Entries written: {len(merged_entries)}",
    ]

    ROOT_COMPILE_COMMANDS.write_text(json.dumps(merged_entries, indent=2, ensure_ascii=True) + "\n")
    lines.append(f"[OK] Wrote {display_path(ROOT_COMPILE_COMMANDS)}")

    if editor in ("vscode", "all"):
        _write_sdcc_compat_header(VSCODE_SDCC_COMPAT)
        _update_vscode_settings(VSCODE_SETTINGS, include_paths)
        _update_vscode_cpp_properties(VSCODE_CPP_PROPERTIES, include_paths, selected_defines)
        lines.append(f"[OK] Wrote {display_path(VSCODE_SETTINGS)}")
        lines.append(f"[OK] Wrote {display_path(VSCODE_CPP_PROPERTIES)}")
        lines.append(f"[OK] Wrote {display_path(VSCODE_SDCC_COMPAT)}")
    else:
        lines.append("[OK] CLion/clangd support written via root compile_commands.json")

    if not selected_entries:
        lines.append("[WARN] Selected build has no compile_commands.json yet; using fallback include paths where needed.")
    elif not selected_includes:
        lines.append("[WARN] Selected build compile_commands.json was found, but include flags could not be parsed.")

    lines.append("")
    lines.append("CLion note: open the firmware CMake project directly, or point it at compile_commands.json in the repo root.")
    return lines


# ── Tool validation helpers ───────────────────────────────────────────────────


def _param_bar(state: dict, compact: bool = False, ascii_safe: bool = False) -> str:
    """Build the live parameter summary shown below the title."""
    target = state["target"]
    keyboard = state["keyboard"]
    art = current_artifact_path(state)
    art_ok = ("OK" if ascii_safe else "✓") if art.is_file() else ("--" if ascii_safe else "✗")
    sep = " | " if compact else "  │  " if not ascii_safe else "  |  "
    if is_ch592_target(state):
        profile = state["build_type"]
        parts = [f"Target: {target}", f"Keyboard: {keyboard}", f"Profile: {profile}", f"Artifact: {art_ok}"]
    else:
        parts = [f"Target: {target}", f"Keyboard: {keyboard}", f"Artifact: {art_ok}"]
    return sep.join(parts)


def find_wchisp_binary() -> str | None:
    local = SCRIPT_DIR / ("wchisp.exe" if os.name == "nt" else "wchisp")
    binary = "wchisp.exe" if os.name == "nt" else "wchisp"
    path = resolve_tool_path("wchisp", binary, env_name="WCHISP_PATH", preferred_candidates=[local])
    return str(path) if path else None


def _tool_version(cmd: list[str]) -> str:
    """Run a command and return stdout first line, or empty."""
    code, out, err = capture_command(cmd)
    if code == 0:
        return (out or err).splitlines()[0] if (out or err) else "ok"
    return ""


def detect_tools(state: dict) -> list[str]:
    items = [
        f"target: {state['target']}",
        f"python: {sys.executable}",
        f"cmake: {find_cmake() or 'missing'}",
        f"wchisp: {find_wchisp_binary() or 'missing'}",
    ]
    items.extend(current_target_profile(state).detect_tool_lines(state))
    return items


def git_dirty() -> bool:
    code, out, _ = capture_command(["git", "status", "--porcelain"])
    return code == 0 and bool(out.strip())


def doctor_lines(state: dict) -> list[str]:
    items: list[str] = []
    target = state["target"]
    git_is_dirty = git_dirty()

    # 1. Required tools — existence + version
    cmake_path = find_cmake()
    if cmake_path:
        ver = _tool_version([str(cmake_path), "--version"])
        items.append(f"[OK] cmake: {cmake_path} ({ver})")
    else:
        items.append("[WARN] cmake: missing")

    wchisp_path = find_wchisp_binary()
    if wchisp_path:
        ver = _tool_version([wchisp_path, "--version"])
        items.append(f"[OK] wchisp: {wchisp_path} ({ver})")
    else:
        items.append("[WARN] wchisp: missing — run 'Install wchisp'")

    items.extend(current_target_profile(state).doctor_lines(state))

    items.append(f"[OK] target: {target}")

    for name in ("pnpm", "node", "git"):
        path = shutil.which(name)
        items.append(f"[{'OK' if path else 'WARN'}] {name}: {path or 'missing'}")

    items.append(f"[{'WARN' if git_is_dirty else 'OK'}] git worktree: {'dirty' if git_is_dirty else 'clean'}")

    return items


# ── Actions ───────────────────────────────────────────────────────────────────

# Actions that modify state or environment should set this flag so the main
# loop knows to clear cached data (doctor, ISP lines, etc.) after they run.
_invalidate_cache_after_action = False


def _mark_cache_dirty():
    """Signal that the next main-loop iteration should clear cached data."""
    global _invalidate_cache_after_action
    _invalidate_cache_after_action = True


def action_check_tools(state: dict, stdscr) -> None:
    show_text(stdscr, "Environment", detect_tools(state))


def action_install_wchisp(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(SETUP_SCRIPT)])
    _mark_cache_dirty()


def action_toggle_target(state: dict, stdscr) -> None:
    current = state["target"]
    if current not in TARGET_ORDER:
        current = TARGET_ORDER[0]
    next_index = (TARGET_ORDER.index(current) + 1) % len(TARGET_ORDER)
    state["target"] = TARGET_ORDER[next_index]
    current_target_profile(state).normalize_state(state)
    save_state(state)
    _mark_cache_dirty()


def action_cycle_keyboard(state: dict, stdscr) -> None:
    if is_ch592_target(state):
        state["keyboard"] = "KNOB" if state["keyboard"] == "5KEY" else "5KEY"
    else:
        current = state["keyboard"]
        if current not in CH552_KEYBOARDS:
            current = CH552_KEYBOARDS[0]
        next_index = (CH552_KEYBOARDS.index(current) + 1) % len(CH552_KEYBOARDS)
        state["keyboard"] = CH552_KEYBOARDS[next_index]
    save_state(state)
    _mark_cache_dirty()


def action_toggle_build_type(state: dict, stdscr) -> None:
    state["build_type"] = "debug" if state["build_type"] == "release" else "release"
    save_state(state)
    _mark_cache_dirty()


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
    _mark_cache_dirty()
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
    lines = [
        f"Target: {state['target']}",
        f"Build config: {current_build_label(state)}",
        f"Artifact: {current_artifact_label(state)}",
        "",
        "Build:",
        f"  {current_build_command_display(state)}",
        "",
        "Flash:",
        f"  {current_flash_command_display(state)}",
        "",
        "Verify:",
        f"  {current_verify_command_display(state)}",
    ]
    show_text(stdscr, "Commands", lines)


def action_generate_ide_config(state: dict, stdscr) -> None:
    show_text(stdscr, "IDE Config", generate_ide_config(state, "all"))


def target_details_lines(state: dict) -> list[str]:
    return current_target_profile(state).target_details_lines(state)


def isp_lines(state: dict) -> list[str]:
    wchisp_path = find_wchisp_binary() or "missing"
    return [
        f"target: {state['target']}",
        f"wchisp: {wchisp_path}",
        f"flash wrapper: {FLASH_SCRIPT}",
        f"default image: {current_artifact_label(state)}",
        "",
        "Transport options:",
        "  -d/--device N   choose USB device index",
        "  -s/--serial     use serial transport",
        "  --port PORT     serial port path",
        "",
        "ISP commands:",
        "  info                chip info / UID / bootloader",
        "  probe               list connected ISP devices",
        "  flash               program a prepared artifact",
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
    lines = [
        "ISP command sheet",
        "",
        "Build first:",
        f"  {current_build_command_display(state)}",
        "",
        "ISP:",
        "  python tools/scripts/flash.py info",
        "  python tools/scripts/flash.py probe",
        f"  {current_flash_command_display(state)}",
        f"  {current_verify_command_display(state)}",
        "  python tools/scripts/flash.py erase",
        "  python tools/scripts/flash.py reset",
        "  python tools/scripts/flash.py eeprom dump --out eeprom_dump.bin",
        "  python tools/scripts/flash.py eeprom erase",
        "  python tools/scripts/flash.py eeprom write --file eeprom_dump.bin",
        "  python tools/scripts/flash.py config info",
        "  python tools/scripts/flash.py config reset",
        "",
        "Optional transport flags:",
        "  -d 0",
        "  -s --port /dev/cu.usbmodemXXXX",
    ]
    show_text(stdscr, "ISP Commands", lines)


def action_build(state: dict, stdscr) -> None:
    run_command(stdscr, current_build_command(state))


def action_flash(state: dict, stdscr) -> None:
    run_command_sequence(
        stdscr,
        [
            (current_build_command(state), PROJECT_ROOT),
            (current_flash_command(state), PROJECT_ROOT),
        ],
    )


def action_probe(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "probe"])


def action_isp_info(state: dict, stdscr) -> None:
    run_command(stdscr, [sys.executable, str(FLASH_SCRIPT), "info"])


def action_isp_verify(state: dict, stdscr) -> None:
    run_command(stdscr, current_verify_command(state))


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


def action_studio_dev(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "dev"], cwd=STUDIO_DIR)


def action_studio_build(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "build"], cwd=STUDIO_DIR)


def action_studio_preview(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "preview"], cwd=STUDIO_DIR)


def action_docs_install(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "install"], cwd=DOCS_DIR)


def action_docs_dev(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "dev"], cwd=DOCS_DIR)


def action_docs_build(state: dict, stdscr) -> None:
    run_command(stdscr, ["pnpm", "run", "build"], cwd=DOCS_DIR)


def action_doctor_report(state: dict, stdscr) -> None:
    show_text(stdscr, "Doctor", doctor_lines(state))


# ── Tab layout ────────────────────────────────────────────────────────────────

ACTION_HANDLERS = {
    "toggle_target": action_toggle_target,
    "cycle_keyboard": action_cycle_keyboard,
    "toggle_build_type": action_toggle_build_type,
    "configure_toolchain": action_configure_toolchain,
    "build": action_build,
    "flash": action_flash,
    "show_commands": action_show_commands,
    "generate_ide_config": action_generate_ide_config,
    "install_wchisp": action_install_wchisp,
    "probe": action_probe,
}


def resolve_target_actions(state: dict) -> list[dict]:
    actions: list[dict] = []
    for spec in current_target_profile(state).home_actions(state):
        fn = ACTION_HANDLERS.get(spec.action_id)
        if not fn:
            continue
        actions.append({
            "label": spec.label,
            "hint": spec.hint,
            "fn": fn,
        })
    return actions

def build_tabs(state: dict) -> list[dict]:
    art = current_artifact_path(state)
    art_status = "Ready" if art.is_file() else "Not built"
    common_home_lines = [
        "Welcome to BinaryKeyboard Console",
        "",
        f"Build config: {current_build_label(state)}",
        f"Artifact: {current_artifact_label(state)}  ({art_status})",
        f"Build dir: {current_build_dir_label(state)}",
    ]

    return [
        {
            "name": "Home",
            "lines": common_home_lines,
            "actions": resolve_target_actions(state),
        },
        {
            "name": "Target",
            "lines": target_details_lines(state),
            "actions": [
                {"label": "Show build commands", "hint": "Print the resolved build / flash / verify commands.", "fn": action_show_commands},
                {"label": "Generate IDE config", "hint": "Write VSCode C/C++ settings and a root compile_commands.json.", "fn": action_generate_ide_config},
                {"label": "Check tools", "hint": "Show cmake/ninja/wchisp/toolchain detection.", "fn": action_check_tools},
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
                "Targets: CH592F / CH552G",
            ],
            "actions": [
                {"label": "Install dependencies", "hint": "Run pnpm install in tools/studio.", "fn": action_studio_install},
                {"label": "Dev server", "hint": "Run pnpm run dev in tools/studio.", "fn": action_studio_dev},
                {"label": "Build studio", "hint": "Run pnpm run build in tools/studio.", "fn": action_studio_build},
                {"label": "Preview build", "hint": "Run pnpm run preview in tools/studio.", "fn": action_studio_preview},
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
                {"label": "Dev server", "hint": "Run pnpm run dev in docs.", "fn": action_docs_dev},
                {"label": "Build docs", "hint": "Run pnpm run build in docs.", "fn": action_docs_build},
            ],
        },
        {
            "name": "Doctor",
            "lines": _cached(f"doctor:{state['target']}", lambda: doctor_lines(state)),
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


def _badge_attr(index: int) -> int:
    return curses.color_pair(8 + (index % 5)) | curses.A_BOLD


def draw_screen(stdscr, state: dict, tabs: list[dict], tab_index: int, action_index: int) -> tuple[list[tuple[int, int, int]], list[tuple[int, int]]]:
    global _spinner_tick
    stdscr.erase()
    height, width = stdscr.getmaxyx()
    tab_regions: list[tuple[int, int, int]] = []
    action_rows: list[tuple[int, int]] = []
    compact = width < 96 or height < 26
    show_hints = not compact and width >= 96 and height >= 26

    # ── Title ─────────────────────────────────────────────────────────────────
    spin = _SPINNER[_spinner_tick % len(_SPINNER)]
    _spinner_tick += 1
    title = f"{spin} BK Console" if compact else f"{spin} BinaryKeyboard Console"
    safe_addnstr(stdscr, 0, 2, title, max(0, width - 4), curses.color_pair(1) | curses.A_BOLD)

    # ── Live parameter bar ────────────────────────────────────────────────────
    bar = _param_bar(state, compact)
    safe_addnstr(stdscr, 1, 2, bar, max(0, width - 4), curses.color_pair(13))

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
    action_slot_count = max(ACTION_PANEL_MIN_ITEMS, max((len(tab["actions"]) for tab in tabs), default=0))
    action_block_height = max(1, action_slot_count * action_row_height)
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
        badge = f"[{idx + 1}]"
        if idx == action_index:
            safe_addnstr(stdscr, y, 2, f" {badge} {action['label']} ", max(0, width - 4), curses.color_pair(4) | curses.A_BOLD)
        else:
            badge_width = text_cells(badge)
            safe_addnstr(stdscr, y, 2, badge, badge_width, _badge_attr(idx))
            safe_addnstr(stdscr, y, 3 + badge_width, f" {action['label']}", max(0, width - 5 - badge_width), curses.A_BOLD)
        if show_hints and y + 1 < height - 2:
            safe_addnstr(stdscr, y + 1, 6, f"tip: {action['hint']}", max(0, width - 8), curses.color_pair(13) | curses.A_DIM)

    # ── Status bar ────────────────────────────────────────────────────────────
    if height > 2:
        status = (
            f" {tabs[tab_index]['name']} | ←→ tab  ↑↓ select  Enter run  q quit "
            if compact
            else f" {tabs[tab_index]['name']}  │  ←→ switch tab  ↑↓ select  Enter run  click+dbl-click  1-9 quick run  r refresh  q quit "
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
    curses.init_pair(8, curses.COLOR_BLACK, curses.COLOR_MAGENTA)
    curses.init_pair(9, curses.COLOR_BLACK, curses.COLOR_YELLOW)
    curses.init_pair(10, curses.COLOR_BLACK, curses.COLOR_CYAN)
    curses.init_pair(11, curses.COLOR_BLACK, curses.COLOR_GREEN)
    curses.init_pair(12, curses.COLOR_BLACK, curses.COLOR_RED)
    curses.init_pair(13, curses.COLOR_CYAN, bg)
    curses.mouseinterval(180)
    curses.mousemask(
        curses.BUTTON1_CLICKED
        | curses.BUTTON1_DOUBLE_CLICKED
        | curses.BUTTON1_RELEASED
    )
    stdscr.keypad(True)
    curses.halfdelay(10)  # 1 second timeout for spinner animation

    while True:
        global _invalidate_cache_after_action
        if _invalidate_cache_after_action:
            _line_cache.clear()
            _invalidate_cache_after_action = False
        tabs = build_tabs(state)
        action_count = len(tabs[tab_index]["actions"])
        action_index = max(0, min(action_index, action_count - 1))
        tab_regions, action_rows = draw_screen(stdscr, state, tabs, tab_index, action_index)
        key = stdscr.getch()

        if key == -1:  # halfdelay timeout — just redraw for spinner
            continue
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
            continue
        if ord("1") <= key <= ord("9"):
            idx = key - ord("1")
            if idx < action_count:
                action_index = idx
                tabs[tab_index]["actions"][action_index]["fn"](state, stdscr)
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
                        break


# ── Text mode fallback ────────────────────────────────────────────────────────

def run_text_mode() -> None:
    state = load_state()
    while True:
        tabs = build_tabs(state)
        tab_badges = _menu_badges([tab["name"] for tab in tabs])
        print(f"\n{_c('1;36', 'BinaryKeyboard Console')}")
        print(_c("36", _param_bar(state, ascii_safe=True)))
        print(_c("2", "-" * 44))
        for i, tab in enumerate(tabs, start=1):
            print(f"  {_text_badge(tab_badges[i - 1], i - 1)} {_c('1', tab['name'])}")
        print(f"  {_c('1;31', '[Q]')} Quit")
        try:
            choice = input(_c("32", "> ")).strip().lower()
        except (EOFError, KeyboardInterrupt):
            print()
            save_state(state)
            return
        if choice == "q":
            save_state(state)
            return
        tab_idx = _resolve_text_choice(choice, tab_badges, len(tabs))
        if tab_idx is None:
            continue
        if not 0 <= tab_idx < len(tabs):
            continue
        tab = tabs[tab_idx]
        action_badges = _menu_badges([action["label"] for action in tab["actions"]])
        print(f"\n{_c('1;36', tab['name'])}")
        print(_c("2", "-" * 44))
        for line in tab["lines"]:
            print(f"  {_stylize_text_line(line)}")
        print(_c("2", "-" * 44))
        for i, action in enumerate(tab["actions"], start=1):
            print(f"  {_text_badge(action_badges[i - 1], i - 1)} {_c('1', action['label'])}")
            print(f"      {_text_hint(action['hint'])}")
        print(f"  {_c('2', '[B]')} Back")
        try:
            action_choice = input(_c("32", "> ")).strip().lower()
        except (EOFError, KeyboardInterrupt):
            print()
            save_state(state)
            return
        if action_choice == "b":
            continue
        idx = _resolve_text_choice(action_choice, action_badges, len(tab["actions"]))
        if idx is not None and 0 <= idx < len(tab["actions"]):
            tab["actions"][idx]["fn"](state, None)


# ── Entry point ───────────────────────────────────────────────────────────────

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="tools/scripts/console.py",
        description="Unified TUI for BinaryKeyboard project workflows.",
    )
    parser.add_argument(
        "--text",
        action="store_true",
        help="Force the plain text fallback menu instead of the curses-based TUI.",
    )
    parser.add_argument(
        "--ide-config",
        choices=("vscode", "clion", "all"),
        help="Generate local IDE support files for the current or overridden target state.",
    )
    parser.add_argument(
        "--target",
        choices=tuple(TARGET_ORDER),
        help="Override the target used by --ide-config.",
    )
    parser.add_argument(
        "--keyboard",
        help="Override the keyboard used by --ide-config.",
    )
    parser.add_argument(
        "--profile",
        choices=("release", "debug"),
        help="Override the CH592F build profile used by --ide-config.",
    )
    return parser


def apply_cli_overrides(state: dict, args) -> None:
    if args.target:
        state["target"] = args.target
    if args.keyboard:
        state["keyboard"] = args.keyboard.upper()
    if args.profile:
        state["build_type"] = args.profile
    current_target_profile(state).normalize_state(state)


def main() -> None:
    args = build_parser().parse_args()
    try:
        if args.ide_config:
            state = load_state()
            apply_cli_overrides(state, args)
            for line in generate_ide_config(state, args.ide_config):
                print(_stylize_text_line(line))
            return
        if args.text or curses is None:
            run_text_mode()
            return
        curses.wrapper(run_curses)
    except KeyboardInterrupt:
        print()
        return


if __name__ == "__main__":
    main()
