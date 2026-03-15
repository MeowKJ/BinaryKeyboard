#!/usr/bin/env python3
"""Textual-based TUI for BinaryKeyboard Console — full-platform, true-color."""

from __future__ import annotations

import asyncio
import json
import os
import shlex
import shutil
import subprocess
import sys
import time
import webbrowser
from pathlib import Path

from textual import work
from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Horizontal, Vertical, ScrollableContainer
from textual.widgets import (
    Label,
    ListItem,
    ListView,
    ProgressBar,
    Rule,
    Static,
    TabbedContent,
    TabPane,
)

from ch552g import VALID_KEYBOARDS as CH552_KEYBOARDS
from common import display_path, find_cmake, find_sdcc, find_wchisp
from i18n import t, get_lang, set_lang, toggle_lang
from targets.ch592.profile import _find_gcc_in_toolchain
from targets.registry import TARGET_ORDER, TARGET_PROFILES, get_target_profile

# ── Paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
STUDIO_DIR = PROJECT_ROOT / "tools" / "studio"
DOCS_DIR = PROJECT_ROOT / "docs"
FLASH_SCRIPT = SCRIPT_DIR / "flash.py"
SETUP_SCRIPT = SCRIPT_DIR / "setup.py"
STATE_FILE = SCRIPT_DIR / ".binarykeyboard_console_state.json"
LEGACY_STATE_FILE = SCRIPT_DIR / ".ch592f_console_state.json"
VSCODE_DIR = PROJECT_ROOT / ".vscode"
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
    "lang": "",
}

# ── State persistence ─────────────────────────────────────────────────────────

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
    if state["lang"]:
        set_lang(state["lang"])
    return state


def save_state(state: dict) -> None:
    data = _read_state_blob()
    for key, default in DEFAULT_STATE.items():
        data[key] = state.get(key, default)
    STATE_FILE.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n")


# ── Helpers ───────────────────────────────────────────────────────────────────

def _profile(state: dict):
    return get_target_profile(state["target"])


def _is_ch592(state: dict) -> bool:
    return state["target"] == "CH592F"


def _artifact_path(state: dict) -> Path:
    return _profile(state).artifact_path(state)


def _build_dir(state: dict) -> Path:
    return _profile(state).build_dir(state)


def _build_label(state: dict) -> str:
    return _profile(state).build_label(state)


def _build_cmd(state: dict) -> list[str]:
    return _profile(state).build_command(state)


def _flash_cmd(state: dict) -> list[str]:
    return _profile(state).flash_command(state)


def _verify_cmd(state: dict) -> list[str]:
    return _profile(state).verify_command(state)


def _build_cmd_display(state: dict) -> str:
    return _profile(state).build_command_display(state)


def _flash_cmd_display(state: dict) -> str:
    return _profile(state).flash_command_display(state)


def _verify_cmd_display(state: dict) -> str:
    return _profile(state).verify_command_display(state)


def _effective_toolchain(state: dict) -> str:
    return state["toolchain_root"] or os.environ.get("MRS_TOOLCHAIN_ROOT", "")


def _tool_version(cmd: list[str]) -> str:
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, check=False,
                                cwd=str(PROJECT_ROOT))
        out = (result.stdout or result.stderr).strip()
        return out.splitlines()[0] if out else "ok"
    except FileNotFoundError:
        return ""


import re as _re

def _short_version(raw: str) -> str:
    """Extract a clean version number like '4.2.3' from verbose tool output."""
    m = _re.search(r'(\d+\.\d+(?:\.\d+)(?:[\w.-]*))', raw)
    return m.group(1) if m else raw


def _git_dirty() -> bool:
    try:
        result = subprocess.run(["git", "status", "--porcelain"],
                                capture_output=True, text=True, check=False,
                                cwd=str(PROJECT_ROOT))
        return result.returncode == 0 and bool(result.stdout.strip())
    except FileNotFoundError:
        return False


def _capture(cmd: list[str], cwd: Path = PROJECT_ROOT) -> tuple[int, str, str]:
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, check=False,
                           cwd=str(cwd))
    except FileNotFoundError as exc:
        return (127, "", str(exc))
    return (r.returncode, r.stdout.strip(), r.stderr.strip())


def doctor_lines(state: dict) -> list[str]:
    items: list[str] = []

    def _check(name: str, path, ver_cmd: list[str] | None = None):
        """Append a 2-line tool entry: status line + indented path."""
        if path:
            if ver_cmd:
                raw = _tool_version(ver_cmd)
                ver = _short_version(raw) if raw else "?"
            else:
                ver = "?"
            items.append(f"[OK] {name} {ver}")
            items.append(f"     {path}")
        else:
            items.append(f"[FAIL] {name}: {t('missing')}")

    # ── Build tools ──────────────────────────────────────────────────────
    items.append(t("doctor.sec_build"))
    py_ver = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
    items.append(f"[OK] python {py_ver}")
    items.append(f"     {sys.executable}")

    cmake_path = find_cmake()
    _check("cmake", cmake_path, [str(cmake_path), "--version"] if cmake_path else None)

    ninja_path = shutil.which("ninja")
    _check("ninja", ninja_path, [ninja_path, "--version"] if ninja_path else None)

    # ── Flash tools ──────────────────────────────────────────────────────
    items.append(t("doctor.sec_flash"))
    wchisp_path = find_wchisp()
    _check("wchisp", wchisp_path, [str(wchisp_path), "--version"] if wchisp_path else None)

    # ── CH552 compiler ───────────────────────────────────────────────────
    items.append(t("doctor.sec_ch552"))
    sdcc_path = find_sdcc()
    _check("sdcc", sdcc_path, [str(sdcc_path), "--version"] if sdcc_path else None)

    # ── CH592 toolchain ──────────────────────────────────────────────────
    items.append(t("doctor.sec_ch592"))
    from targets.ch592.profile import _resolve_toolchain
    resolved = _resolve_toolchain(state)
    if resolved["gcc"]:
        gcc_path = resolved["gcc"]
        raw = _tool_version([gcc_path, "--version"])
        ver = _short_version(raw) if raw else "?"
        items.append(f"[OK] riscv-gcc {ver}")
        items.append(f"     {gcc_path}")
        items.append(f"[OK] toolchain: {resolved['source']}")
        items.append(f"     {resolved['toolchain_dir']}")
    else:
        items.append(f"[FAIL] riscv-gcc: {t('missing')}")
    for warning in resolved["warnings"]:
        items.append(f"[WARN] {warning}")

    # ── Frontend / VCS ───────────────────────────────────────────────────
    items.append(t("doctor.sec_frontend"))
    for name in ("node", "pnpm", "git"):
        path = shutil.which(name)
        _check(name, path, [path, "--version"] if path else None)

    # ── Git worktree ─────────────────────────────────────────────────────
    git_is_dirty = _git_dirty()
    items.append(f"[{'FAIL' if git_is_dirty else 'OK'}] {t('doctor.git_worktree')}: "
                 f"{t('doctor.dirty') if git_is_dirty else t('doctor.clean')}")

    return items


def _param_bar_text(state: dict) -> str:
    target = state["target"]
    keyboard = state["keyboard"]
    art = _artifact_path(state)
    art_ok = "✓" if art.is_file() else "✗"
    parts = [target, keyboard]
    if _is_ch592(state):
        parts.append(state["build_type"])
    parts.append(f"{t('bar.firmware')} {art_ok}")
    return "  ·  ".join(parts)


def isp_lines(state: dict) -> list[str]:
    wchisp_path = str(find_wchisp() or t("missing"))
    return [
        f"{t('isp.target')}: {state['target']}",
        f"{t('isp.wchisp')}: {wchisp_path}",
        f"{t('isp.flash_wrapper')}: {FLASH_SCRIPT}",
        f"{t('isp.default_image')}: {display_path(_artifact_path(state))}",
        "",
        t("isp.transport_options"),
        t("isp.device_help"),
        t("isp.serial_help"),
        t("isp.port_help"),
        "",
        t("isp.commands_header"),
        t("isp.cmd.info"),
        t("isp.cmd.probe"),
        t("isp.cmd.flash"),
        t("isp.cmd.verify"),
        t("isp.cmd.erase"),
        t("isp.cmd.reset"),
        t("isp.cmd.eeprom_dump"),
        t("isp.cmd.eeprom_erase"),
        t("isp.cmd.eeprom_write"),
        t("isp.cmd.config_info"),
        t("isp.cmd.config_reset"),
    ]


# ── Tab data builders ────────────────────────────────────────────────────────

def _home_lines(state: dict) -> list[str]:
    art = _artifact_path(state)
    art_status = t("ready") if art.is_file() else t("not_built")
    return ([t("welcome")]
            + _profile(state).target_details_lines(state)
            + [f"{t('home.artifact')}: {display_path(art)}  ({art_status})"])


def _frontend_tool_line(name: str) -> str:
    path = shutil.which(name)
    if not path:
        return f"[FAIL] {name}: {t('missing')}"
    raw = _tool_version([path, "--version"])
    ver = _short_version(raw) if raw else "?"
    return f"[OK] {name} {ver}"


def _dev_lines(state: dict) -> list[str]:
    del state

    studio_deps = (STUDIO_DIR / "node_modules").is_dir()
    studio_dist = STUDIO_DIR / "dist"
    docs_deps = (DOCS_DIR / "node_modules").is_dir()
    docs_dist = DOCS_DIR / ".vitepress" / "dist"

    return [
        f"{t('dev.tools')}:",
        _frontend_tool_line("node"),
        _frontend_tool_line("pnpm"),
        "",
        f"{t('tab.studio')}:",
        f"{t('path')}: {display_path(STUDIO_DIR)}",
        t("studio.stack"),
        t("studio.targets"),
        f"[{'OK' if studio_deps else 'WARN'}] {t('dev.dependencies')}: "
        f"{t('ready') if studio_deps else t('dev.not_installed')}",
        f"[{'OK' if studio_dist.is_dir() else 'WARN'}] {t('dev.build_output')}: "
        f"{display_path(studio_dist) if studio_dist.is_dir() else t('not_built')}",
        "",
        f"{t('tab.docs')}:",
        f"{t('path')}: {display_path(DOCS_DIR)}",
        t("docs.stack"),
        f"[{'OK' if docs_deps else 'WARN'}] {t('dev.dependencies')}: "
        f"{t('ready') if docs_deps else t('dev.not_installed')}",
        f"[{'OK' if docs_dist.is_dir() else 'WARN'}] {t('dev.build_output')}: "
        f"{display_path(docs_dist) if docs_dist.is_dir() else t('not_built')}",
    ]


def _info_lines(state: dict) -> list[str]:
    return doctor_lines(state) + [""] + [f"{name}: {url}" for name, url in DOC_URLS.items()]


# ── Action definitions ────────────────────────────────────────────────────────
# Each action: (id, label, hint, callable_or_name)
# The callable receives (app, state).

def _home_actions(state: dict) -> list[dict]:
    """Dynamically build Home tab actions from target profile."""
    from targets.common import TargetActionSpec
    specs: list[TargetActionSpec] = _profile(state).home_actions(state)
    actions: list[dict] = []
    for spec in specs:
        actions.append({
            "id": spec.action_id,
            "label": spec.label,
            "hint": spec.hint,
        })
    return actions


def _isp_actions() -> list[dict]:
    return [
        {"id": "probe", "label": t("action.probe_devices"), "hint": t("hint.probe_devices")},
        {"id": "show_isp_sheet", "label": t("action.show_isp_sheet"), "hint": t("hint.show_isp_sheet")},
        {"id": "chip_menu", "label": t("action.chip_commands"), "hint": t("hint.chip_commands")},
        {"id": "eeprom_menu", "label": t("action.eeprom_commands"), "hint": t("hint.eeprom_commands")},
        {"id": "config_menu", "label": t("action.config_commands"), "hint": t("hint.config_commands")},
        {"id": "install_wchisp", "label": t("action.install_wchisp"), "hint": t("hint.install_wchisp")},
        {"id": "reset_toolchain", "label": t("action.reset_toolchain"), "hint": t("hint.reset_toolchain")},
    ]


def _dev_actions() -> list[dict]:
    return [
        {"id": "studio_install", "label": t("action.studio_install"), "hint": t("hint.install_studio")},
        {"id": "studio_dev", "label": t("action.studio_dev"), "hint": t("hint.dev_studio")},
        {"id": "studio_build", "label": t("action.build_studio"), "hint": t("hint.build_studio")},
        {"id": "docs_install", "label": t("action.docs_install"), "hint": t("hint.install_docs")},
        {"id": "docs_dev", "label": t("action.docs_dev"), "hint": t("hint.dev_docs")},
        {"id": "docs_build", "label": t("action.build_docs"), "hint": t("hint.build_docs")},
    ]


def _info_actions() -> list[dict]:
    lang_name = t("lang.en") if get_lang() == "en" else t("lang.zh")
    return [
        {"id": "toggle_lang", "label": t("action.toggle_lang", lang=lang_name), "hint": t("hint.toggle_lang")},
        {"id": "open_mrs", "label": t("link.mrs_download"), "hint": t("link.mrs_hint")},
        {"id": "open_wchisp", "label": t("link.wchisp_releases"), "hint": t("link.wchisp_hint")},
        {"id": "open_wch", "label": t("link.wch_homepage"), "hint": t("link.wch_hint")},
        {"id": "open_repo", "label": t("link.project_repo"), "hint": t("link.repo_hint")},
    ]


# Catppuccin Mocha pastel badge colors
BADGE_COLORS = ("#f5c2e7", "#f9e2af", "#89dceb", "#a6e3a1", "#fab387", "#cba6f7", "#94e2d5")


# ── Textual widgets ───────────────────────────────────────────────────────────

class ContentPanel(Vertical):
    """Fixed content pane showing info lines (no scrollbar)."""

    DEFAULT_CSS = """
    ContentPanel {
        height: 1fr;
        padding: 1 2;
        background: $surface;
        overflow: hidden;
    }
    """

    can_focus = False

    def set_lines(self, lines: list[str]) -> None:
        self.remove_children()
        for line in lines:
            label = Label(line)
            stripped = line.lstrip()
            if stripped.startswith("[OK]"):
                label.add_class("ok")
            elif stripped.startswith("[FAIL]"):
                label.add_class("fail")
            elif stripped.startswith("[WARN]"):
                label.add_class("warn")
            elif ": " in line and not line.startswith(" "):
                label.add_class("label-line")
            elif not stripped:
                label.add_class("dim")
            label.add_class("content-line")
            self.mount(label)


class ScrollableContentPanel(ScrollableContainer):
    """Scrollable content pane for Info tab (both axes)."""

    DEFAULT_CSS = """
    ScrollableContentPanel {
        height: 1fr;
        padding: 1 2;
        background: $surface;
    }
    """

    can_focus = False

    def set_lines(self, lines: list[str]) -> None:
        self.remove_children()
        for line in lines:
            label = Label(line)
            stripped = line.lstrip()
            if stripped.startswith("[OK]"):
                label.add_class("ok")
            elif stripped.startswith("[FAIL]"):
                label.add_class("fail")
            elif stripped.startswith("[WARN]"):
                label.add_class("warn")
            elif stripped.startswith("──"):
                label.add_class("label-line")
            elif line.startswith("     "):
                label.add_class("dim")
            elif ":" in line and not line.startswith(" "):
                label.add_class("label-line")
            elif not stripped:
                label.add_class("dim")
            label.add_class("content-line")
            self.mount(label)


class ActionItem(ListItem):
    """A single action entry in the action list."""

    def __init__(self, action: dict, index: int) -> None:
        super().__init__()
        self.action_data = action
        self.action_index = index

    def compose(self) -> ComposeResult:
        badge_color = BADGE_COLORS[self.action_index % len(BADGE_COLORS)]
        label_text = self.action_data["label"]
        # Highlight first verb (first word) in the label
        # Escape Rich markup brackets in label text
        safe_label = label_text.replace("[", r"\[")
        parts = safe_label.split(None, 1)
        if len(parts) == 2:
            verb, rest = parts
            label_markup = f"[bold {badge_color}]{verb}[/] {rest}"
        else:
            label_markup = f"[bold {badge_color}]{safe_label}[/]"

        badge = f"\\[{self.action_index + 1}]"
        yield Label(f"[bold {badge_color}]{badge}[/]  {label_markup}", markup=True)
        hint_text = self.action_data.get('hint', '').replace("[", r"\[")
        yield Label(f"  [dim italic #6c7086]{hint_text}[/]", markup=True)

    @property
    def hint_text(self) -> str:
        return self.action_data.get("hint", "")


class ActionPanel(ListView):
    """Bottom panel listing available actions."""

    def set_actions(self, actions: list[dict], keep_index: int | None = None) -> None:
        self.clear()
        for i, action in enumerate(actions):
            self.append(ActionItem(action, i))
        if actions:
            target_idx = keep_index if keep_index is not None and 0 <= keep_index < len(actions) else 0
            self.call_after_refresh(lambda idx=target_idx: setattr(self, 'index', idx))


# ── Main App ──────────────────────────────────────────────────────────────────

class BKConsoleApp(App):
    """BinaryKeyboard Console — Textual TUI."""

    CSS_PATH = "tui_textual.tcss"
    TITLE = "BinaryKeyboard Console"

    BINDINGS = [
        Binding("ctrl+c", "quit_app", "Quit", priority=True),
        Binding("r", "refresh", "Refresh"),
        Binding("left", "prev_tab", "Prev tab", show=False, priority=True),
        Binding("right", "next_tab", "Next tab", show=False, priority=True),
        Binding("up", "prev_action", "Prev", show=False, priority=True),
        Binding("down", "next_action", "Next", show=False, priority=True),
        Binding("enter", "run_selected", "Run", show=False, priority=True),
        Binding("1", "quick_run('1')", "1", show=False),
        Binding("2", "quick_run('2')", "2", show=False),
        Binding("3", "quick_run('3')", "3", show=False),
        Binding("4", "quick_run('4')", "4", show=False),
        Binding("5", "quick_run('5')", "5", show=False),
        Binding("6", "quick_run('6')", "6", show=False),
        Binding("7", "quick_run('7')", "7", show=False),
        Binding("8", "quick_run('8')", "8", show=False),
        Binding("9", "quick_run('9')", "9", show=False),
    ]

    def __init__(self) -> None:
        super().__init__()
        self.state = load_state()
        self._current_actions: list[dict] = []
        self._building = False
        self._last_ctrl_c: float = 0.0

    def compose(self) -> ComposeResult:
        yield Label(f"✦ {t('title')}", id="title-bar", markup=True)
        yield Label(_param_bar_text(self.state), id="param-bar", markup=True)
        with TabbedContent(
            t("tab.home"), t("tab.isp"), t("tab.dev"), t("tab.info"),
            id="tabs",
        ):
            with TabPane(t("tab.home"), id="tab-home"):
                with Vertical(classes="tab-body"):
                    yield ContentPanel(id="content-home")
                    yield Rule()
                    yield ActionPanel(id="actions-home")
            with TabPane(t("tab.isp"), id="tab-isp"):
                with Vertical(classes="tab-body"):
                    yield ScrollableContentPanel(id="content-isp")
                    yield Rule()
                    yield ActionPanel(id="actions-isp")
            with TabPane(t("tab.dev"), id="tab-dev"):
                with Vertical(classes="tab-body"):
                    yield ScrollableContentPanel(id="content-dev")
                    yield Rule()
                    yield ActionPanel(id="actions-dev")
            with TabPane(t("tab.info"), id="tab-info"):
                with Vertical(classes="tab-body"):
                    yield ScrollableContentPanel(id="content-info")
                    yield Rule()
                    yield ActionPanel(id="actions-info")
        with Horizontal(id="build-progress-container"):
            yield Label("Building...", id="build-progress-label")
            yield ProgressBar(id="build-progress", total=100, show_eta=False, show_percentage=True)
        yield Label(
            t("status.full", tab=t("tab.home")),
            id="status-bar",
        )

    def on_mount(self) -> None:
        self._refresh_all()

    def _set_content_lines(self, content_id: str, lines: list[str]) -> None:
        panel = self.query_one(f"#{content_id}")
        panel.set_lines(lines)
        if isinstance(panel, ScrollableContentPanel):
            panel.scroll_home(animate=False)

    def _refresh_all(self) -> None:
        """Rebuild all tab content and actions, preserving active selection."""
        # Remember current selection index on the active panel
        active_panel = self._active_action_panel()
        saved_index = active_panel.index

        self.query_one("#title-bar", Label).update(f"✦ {t('title')}")
        self.query_one("#param-bar", Label).update(_param_bar_text(self.state))

        # Determine which panel is active so we can restore its index
        tc = self.query_one("#tabs", TabbedContent)
        active_tab = tc.active or "tab-home"
        active_panel_id = active_tab.replace("tab-", "actions-")

        # Home
        self._set_content_lines("content-home", _home_lines(self.state))
        ki = saved_index if active_panel_id == "actions-home" else None
        self.query_one("#actions-home", ActionPanel).set_actions(_home_actions(self.state), keep_index=ki)

        # ISP
        self._set_content_lines("content-isp", isp_lines(self.state))
        ki = saved_index if active_panel_id == "actions-isp" else None
        self.query_one("#actions-isp", ActionPanel).set_actions(_isp_actions(), keep_index=ki)

        # Dev
        self._set_content_lines("content-dev", _dev_lines(self.state))
        ki = saved_index if active_panel_id == "actions-dev" else None
        self.query_one("#actions-dev", ActionPanel).set_actions(_dev_actions(), keep_index=ki)

        # Info
        self._set_content_lines("content-info", _info_lines(self.state))
        ki = saved_index if active_panel_id == "actions-info" else None
        self.query_one("#actions-info", ActionPanel).set_actions(_info_actions(), keep_index=ki)

    def _rebuild_ui(self) -> None:
        """Full UI rebuild including tab labels (for language switch)."""
        self.query_one("#title-bar", Label).update(f"✦ {t('title')}")
        tab_map = {"tab-home": t("tab.home"), "tab-isp": t("tab.isp"),
                    "tab-dev": t("tab.dev"), "tab-info": t("tab.info")}
        tc = self.query_one("#tabs", TabbedContent)
        for tab in tc.query("Tab"):
            pane_id = str(getattr(tab, "id", "")).replace("--content-tab-", "")
            if pane_id in tab_map:
                tab.label = tab_map[pane_id]
        self._refresh_all()
        current_tab = tc.active or "tab-home"
        human = current_tab.replace("tab-", "").capitalize()
        self.query_one("#status-bar", Label).update(t("status.full", tab=human))

    def _active_action_panel(self) -> ActionPanel:
        """Return the ActionPanel for the currently active tab."""
        tc = self.query_one("#tabs", TabbedContent)
        tab_id = tc.active or "tab-home"
        panel_id = tab_id.replace("tab-", "actions-")
        return self.query_one(f"#{panel_id}", ActionPanel)

    def _active_actions(self) -> list[dict]:
        """Return the current tab's action list."""
        tc = self.query_one("#tabs", TabbedContent)
        tab_id = tc.active or "tab-home"
        if tab_id == "tab-home":
            return _home_actions(self.state)
        elif tab_id == "tab-isp":
            return _isp_actions()
        elif tab_id == "tab-dev":
            return _dev_actions()
        else:
            return _info_actions()

    # ── Tab switch handler ────────────────────────────────────────────────────
    def on_tabbed_content_tab_activated(self, event: TabbedContent.TabActivated) -> None:
        tab_name = event.pane.id or ""
        human = tab_name.replace("tab-", "").capitalize()
        self.query_one("#status-bar", Label).update(
            t("status.full", tab=human)
        )

    # ── Action execution ──────────────────────────────────────────────────────
    def on_list_view_selected(self, event: ListView.Selected) -> None:
        item = event.item
        if isinstance(item, ActionItem):
            self._run_action(item.action_data)

    def action_quit_app(self) -> None:
        now = time.monotonic()
        if now - self._last_ctrl_c < 1.5:
            save_state(self.state)
            self.exit()
        else:
            self._last_ctrl_c = now
            self.notify(t("quit_hint"), timeout=1.5)

    def action_refresh(self) -> None:
        self._refresh_all()
        self.notify("Refreshed", timeout=1)

    def action_quick_run(self, digit: str) -> None:
        idx = int(digit) - 1
        actions = self._active_actions()
        if 0 <= idx < len(actions):
            panel = self._active_action_panel()
            panel.index = idx
            self._run_action(actions[idx])

    # ── Arrow-key navigation ──────────────────────────────────────────────────
    def action_prev_tab(self) -> None:
        tc = self.query_one("#tabs", TabbedContent)
        tabs = list(tc.query("TabPane"))
        if not tabs:
            return
        ids = [t.id for t in tabs]
        cur = tc.active or ids[0]
        idx = ids.index(cur) if cur in ids else 0
        tc.active = ids[(idx - 1) % len(ids)]

    def action_next_tab(self) -> None:
        tc = self.query_one("#tabs", TabbedContent)
        tabs = list(tc.query("TabPane"))
        if not tabs:
            return
        ids = [t.id for t in tabs]
        cur = tc.active or ids[0]
        idx = ids.index(cur) if cur in ids else 0
        tc.active = ids[(idx + 1) % len(ids)]

    def action_prev_action(self) -> None:
        panel = self._active_action_panel()
        if panel.index is not None and panel.index > 0:
            panel.index -= 1
        elif panel._nodes:
            panel.index = len(panel._nodes) - 1

    def action_next_action(self) -> None:
        panel = self._active_action_panel()
        total = len(panel._nodes)
        if total == 0:
            return
        if panel.index is None:
            panel.index = 0
        elif panel.index < total - 1:
            panel.index += 1
        else:
            panel.index = 0

    def action_run_selected(self) -> None:
        panel = self._active_action_panel()
        if panel.index is not None and 0 <= panel.index < len(panel._nodes):
            item = panel._nodes[panel.index]
            if isinstance(item, ActionItem):
                self._run_action(item.action_data)

    def _run_action(self, action: dict) -> None:
        action_id = action["id"]
        handler = ACTION_DISPATCH.get(action_id)
        if handler:
            handler(self)

    # ── Suspend-and-run (for external commands) ───────────────────────────────
    def _suspend_and_run(self, cmd: list[str], cwd: Path = PROJECT_ROOT) -> None:
        """Suspend the TUI, run a command interactively, resume."""
        def _runner() -> None:
            print("\n\033[1;36m$\033[0m \033[1m" + " ".join(shlex.quote(p) for p in cmd) + "\033[0m")
            subprocess.run(cmd, cwd=str(cwd), check=False, shell=(os.name == "nt"))
            try:
                input("\n\033[2mPress Enter to return...\033[0m")
            except (EOFError, KeyboardInterrupt):
                print()
        with self.suspend():
            _runner()
        self._refresh_all()

    def _suspend_and_run_sequence(self, commands: list[tuple[list[str], Path]]) -> None:
        def _runner() -> None:
            for cmd, cwd in commands:
                print("\n\033[1;36m$\033[0m \033[1m" + " ".join(shlex.quote(p) for p in cmd) + "\033[0m")
                result = subprocess.run(cmd, cwd=str(cwd), check=False, shell=(os.name == "nt"))
                if result.returncode != 0:
                    print(f"\n\033[31mCommand failed with exit {result.returncode}.\033[0m")
                    break
            try:
                input("\n\033[2mPress Enter to return...\033[0m")
            except (EOFError, KeyboardInterrupt):
                print()
        with self.suspend():
            _runner()
        self._refresh_all()

    def _launch_in_terminal(self, cmd: list[str], cwd: Path = PROJECT_ROOT) -> None:
        """Launch a command in a new terminal window (non-blocking)."""
        cmd_str = subprocess.list2cmdline(cmd)
        if os.name == "nt":
            subprocess.Popen(
                f'start "" cmd /k "cd /d {cwd} && {cmd_str}"',
                shell=True,
            )
        else:
            subprocess.Popen(
                ["bash", "-c", cmd_str],
                cwd=str(cwd),
                start_new_session=True,
            )
        self.notify(t("launched_terminal", cmd=cmd_str), timeout=3)

    def _suspend_and_show(self, title: str, lines: list[str]) -> None:
        def _runner() -> None:
            print(f"\n\033[1;36m== {title} ==\033[0m")
            for line in lines:
                stripped = line.lstrip()
                if stripped.startswith("[OK]"):
                    print(f"\033[32m{line}\033[0m")
                elif stripped.startswith("[FAIL]"):
                    print(f"\033[31m{line}\033[0m")
                elif stripped.startswith("[WARN]"):
                    print(f"\033[33m{line}\033[0m")
                elif ": " in line and not line.startswith(" "):
                    key, value = line.split(": ", 1)
                    print(f"\033[36m{key}:\033[0m \033[1m{value}\033[0m")
                else:
                    print(line)
            try:
                input("\n\033[2mPress Enter to return...\033[0m")
            except (EOFError, KeyboardInterrupt):
                print()
        with self.suspend():
            _runner()

    def _prompt_input(self, prompt: str, default: str = "") -> str:
        result = {"value": ""}
        def _runner() -> None:
            shown = prompt
            if default:
                shown += f" [{default}]"
            shown += ": "
            try:
                val = input(shown).strip()
            except (EOFError, KeyboardInterrupt):
                val = ""
            result["value"] = val or default
        with self.suspend():
            _runner()
        return result["value"]

    # ── Build with progress bar ───────────────────────────────────────────────
    def _show_build_progress(self, label: str = "Building...") -> None:
        container = self.query_one("#build-progress-container")
        container.add_class("visible")
        self.query_one("#build-progress-label", Label).update(label)
        bar = self.query_one("#build-progress", ProgressBar)
        bar.update(progress=0)

    def _update_build_progress(self, progress: float, label: str | None = None) -> None:
        bar = self.query_one("#build-progress", ProgressBar)
        bar.update(progress=progress)
        if label:
            self.query_one("#build-progress-label", Label).update(label)

    def _hide_build_progress(self) -> None:
        self.query_one("#build-progress-container").remove_class("visible")

    def _show_build_result(self, lines: list[str], rc: int) -> None:
        """Show build output in the current tab's content panel."""
        tc = self.query_one("#tabs", TabbedContent)
        tab_id = tc.active or "tab-home"
        content_id = tab_id.replace("tab-", "content-")
        result_lines: list[str] = []
        for line in lines:
            result_lines.append(line)
        self._set_content_lines(content_id, result_lines)
        if rc == 0:
            self.notify(t("build.complete"), severity="information", timeout=3)
        else:
            self.notify(t("build.failed", rc=rc), severity="error", timeout=5)

    @work(thread=True)
    def _run_build_with_progress(self, cmd: list[str], cwd: Path = PROJECT_ROOT) -> None:
        """Run a build command in background thread with progress bar."""
        self.call_from_thread(self._show_build_progress, f"Building: {' '.join(cmd[-2:])}")

        process = subprocess.Popen(
            cmd, cwd=str(cwd),
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            bufsize=0,
        )

        output_lines: list[str] = []
        line_count = 0
        buf = b""

        while True:
            chunk = process.stdout.read(1)
            if not chunk:
                if buf:
                    line = buf.decode("utf-8", errors="replace").rstrip()
                    output_lines.append(line)
                break
            if chunk == b"\n":
                line = buf.decode("utf-8", errors="replace").rstrip()
                buf = b""
                output_lines.append(line)
                line_count += 1
                progress = min(95.0, line_count * 2.0)
                short_line = line.strip()[:60]
                self.call_from_thread(self._update_build_progress, progress, f"Building: {short_line}")
            elif chunk == b"\r":
                continue
            else:
                buf += chunk

        process.wait()
        rc = process.returncode

        if rc == 0:
            self.call_from_thread(self._update_build_progress, 100.0, t("build.complete"))
        else:
            self.call_from_thread(self._update_build_progress, 100.0, t("build.failed", rc=rc))

        time.sleep(0.8)
        self.call_from_thread(self._hide_build_progress)
        self.call_from_thread(self._refresh_all)

        # Show build output in content panel instead of suspending
        summary = output_lines[-30:] if len(output_lines) > 30 else output_lines
        summary.append("")
        summary.append(f"Exit code: {rc}")
        self.call_from_thread(self._show_build_result, summary, rc)

    # ── Directory chooser ─────────────────────────────────────────────────────
    def _choose_directory(self, default: str = "") -> str:
        result = {"value": ""}
        def _runner() -> None:
            if sys.platform.startswith("win"):
                val = _choose_directory_windows(default)
            elif sys.platform == "darwin":
                val = _choose_directory_macos(default)
            else:
                val = _choose_directory_linux(default)
            if not val:
                val = _choose_directory_tk(default)
            result["value"] = val.strip() if val else ""
        with self.suspend():
            _runner()
        return result["value"]


# ── Platform directory choosers (reused from old tui) ─────────────────────────

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
    try:
        r = subprocess.run([shell, "-NoProfile", "-STA", "-Command", "\n".join(script_lines)],
                           capture_output=True, text=True, check=False)
        return r.stdout if r.returncode == 0 else ""
    except FileNotFoundError:
        return ""


def _choose_directory_macos(default: str = "") -> str:
    script = 'POSIX path of (choose folder with prompt "Select MRS Toolchain root")'
    if default and Path(default).exists():
        escaped = default.replace("\\", "\\\\").replace('"', '\\"')
        script = (
            'POSIX path of (choose folder with prompt "Select MRS Toolchain root" '
            f'default location POSIX file "{escaped}")'
        )
    try:
        r = subprocess.run(["osascript", "-e", script],
                           capture_output=True, text=True, check=False)
        return r.stdout.strip() if r.returncode == 0 else ""
    except FileNotFoundError:
        return ""


def _choose_directory_linux(default: str = "") -> str:
    if not (os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")):
        return ""
    initial = default if default and Path(default).exists() else str(PROJECT_ROOT)
    zenity = shutil.which("zenity")
    if zenity:
        try:
            r = subprocess.run([zenity, "--file-selection", "--directory",
                                "--title=Select MRS Toolchain root",
                                f"--filename={initial.rstrip('/')}/"],
                               capture_output=True, text=True, check=False)
            if r.returncode == 0:
                return r.stdout.strip()
        except FileNotFoundError:
            pass
    kdialog = shutil.which("kdialog")
    if kdialog:
        try:
            r = subprocess.run([kdialog, "--getexistingdirectory", initial,
                                "--title", "Select MRS Toolchain root"],
                               capture_output=True, text=True, check=False)
            if r.returncode == 0:
                return r.stdout.strip()
        except FileNotFoundError:
            pass
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


# ── Action dispatch ───────────────────────────────────────────────────────────

def _act_toggle_target(app: BKConsoleApp) -> None:
    current = app.state["target"]
    if current not in TARGET_ORDER:
        current = TARGET_ORDER[0]
    nxt = (TARGET_ORDER.index(current) + 1) % len(TARGET_ORDER)
    app.state["target"] = TARGET_ORDER[nxt]
    _profile(app.state).normalize_state(app.state)
    save_state(app.state)
    app._refresh_all()


def _act_cycle_keyboard(app: BKConsoleApp) -> None:
    if _is_ch592(app.state):
        app.state["keyboard"] = "KNOB" if app.state["keyboard"] == "5KEY" else "5KEY"
    else:
        cur = app.state["keyboard"]
        if cur not in CH552_KEYBOARDS:
            cur = CH552_KEYBOARDS[0]
        nxt = (CH552_KEYBOARDS.index(cur) + 1) % len(CH552_KEYBOARDS)
        app.state["keyboard"] = CH552_KEYBOARDS[nxt]
    save_state(app.state)
    app._refresh_all()


def _act_toggle_build_type(app: BKConsoleApp) -> None:
    app.state["build_type"] = "debug" if app.state["build_type"] == "release" else "release"
    save_state(app.state)
    app._refresh_all()


def _act_configure_toolchain(app: BKConsoleApp) -> None:
    default = _effective_toolchain(app.state) or "/path/to/MRS_Toolchain/Toolchain"
    toolchain_root = app._choose_directory(default)
    if not toolchain_root:
        toolchain_root = app._prompt_input("MRS_TOOLCHAIN_ROOT", default)
    if not toolchain_root.strip():
        app.notify(t("toolchain.empty"), severity="warning")
        return
    gcc = _find_gcc_in_toolchain(toolchain_root)
    if gcc:
        app.state["toolchain_root"] = toolchain_root
        save_state(app.state)
        app._refresh_all()
        app.notify(f"{t('toolchain.saved')} ({Path(gcc).name})", severity="information")
    elif Path(toolchain_root).is_dir():
        app.notify(t("toolchain.no_gcc"), severity="error")
    else:
        app.notify(t("toolchain.path_missing"), severity="error")


def _act_reset_toolchain(app: BKConsoleApp) -> None:
    if app.state.get("toolchain_root"):
        app.state["toolchain_root"] = ""
        save_state(app.state)
        app._refresh_all()
        app.notify(t("toolchain.reset"), severity="information")
    else:
        app.notify(t("toolchain.reset_no_cache"), severity="warning")


def _act_build(app: BKConsoleApp) -> None:
    app._suspend_and_run(_build_cmd(app.state))


def _act_clean(app: BKConsoleApp) -> None:
    build_dir = _build_dir(app.state)
    if build_dir.is_dir():
        shutil.rmtree(build_dir)
        app._refresh_all()
        app.notify(t("clean.done", path=str(build_dir.name)), severity="information", timeout=3)
    else:
        app.notify(t("clean.nothing"), severity="warning", timeout=2)


def _act_flash(app: BKConsoleApp) -> None:
    app._suspend_and_run_sequence([
        (_build_cmd(app.state), PROJECT_ROOT),
        (_flash_cmd(app.state), PROJECT_ROOT),
    ])


def _act_show_commands(app: BKConsoleApp) -> None:
    lines = [
        f"{t('cmd.target')}: {app.state['target']}",
        f"{t('cmd.build_config')}: {_build_label(app.state)}",
        f"{t('cmd.artifact')}: {display_path(_artifact_path(app.state))}",
        "",
        t("cmd.build_label"),
        f"  {_build_cmd_display(app.state)}",
        "",
        t("cmd.flash_label"),
        f"  {_flash_cmd_display(app.state)}",
        "",
        t("cmd.verify_label"),
        f"  {_verify_cmd_display(app.state)}",
    ]
    app._suspend_and_show(t("action.show_commands"), lines)


def _act_generate_ide(app: BKConsoleApp) -> None:
    # Import generate_ide_config from old module since it has complex logic
    from tui_console import generate_ide_config
    lines = generate_ide_config(app.state, "all")
    app._suspend_and_show("IDE Config", lines)


def _act_install_wchisp(app: BKConsoleApp) -> None:
    app._suspend_and_run([sys.executable, str(SETUP_SCRIPT)])


def _act_probe(app: BKConsoleApp) -> None:
    app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "probe"])


def _act_show_isp_sheet(app: BKConsoleApp) -> None:
    lines = [
        t("isp_sheet.title"), "",
        t("isp_sheet.build_first"),
        f"  {_build_cmd_display(app.state)}", "",
        t("isp_sheet.isp"),
        "  python tools/scripts/flash.py info",
        "  python tools/scripts/flash.py probe",
        f"  {_flash_cmd_display(app.state)}",
        f"  {_verify_cmd_display(app.state)}",
        "  python tools/scripts/flash.py erase",
        "  python tools/scripts/flash.py reset",
        "  python tools/scripts/flash.py eeprom dump --out eeprom_dump.bin",
        "  python tools/scripts/flash.py eeprom erase",
        "  python tools/scripts/flash.py eeprom write --file eeprom_dump.bin",
        "  python tools/scripts/flash.py config info",
        "  python tools/scripts/flash.py config reset",
        "", t("isp_sheet.transport_flags"),
        "  -d 0",
        "  -s --port /dev/cu.usbmodemXXXX",
    ]
    app._suspend_and_show(t("tab.isp"), lines)


def _act_chip_menu(app: BKConsoleApp) -> None:
    choice = app._prompt_input(t("prompt.chip_command"), "probe").strip().lower()
    sub = {
        "probe": lambda: app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "probe"]),
        "info": lambda: app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "info"]),
        "verify": lambda: app._suspend_and_run(_verify_cmd(app.state)),
        "erase": lambda: app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "erase"]),
        "reset": lambda: app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "reset"]),
    }
    fn = sub.get(choice)
    if fn:
        fn()
    else:
        app.notify(t("prompt.unknown_command", cmd=choice), severity="error")


def _act_eeprom_menu(app: BKConsoleApp) -> None:
    choice = app._prompt_input(t("prompt.eeprom_command"), "dump").strip().lower()
    if choice == "dump":
        out = app._prompt_input(t("prompt.eeprom_dump_file"), "eeprom_dump.bin")
        app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "eeprom", "dump", "--out", out])
    elif choice == "write":
        f = app._prompt_input(t("prompt.eeprom_input_file"), "eeprom_dump.bin")
        if not f.strip():
            app.notify(t("prompt.file_empty"), severity="warning")
            return
        app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "eeprom", "write", "--file", f])
    elif choice == "erase":
        app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "eeprom", "erase"])
    else:
        app.notify(t("prompt.unknown_command", cmd=choice), severity="error")


def _act_config_menu(app: BKConsoleApp) -> None:
    choice = app._prompt_input(t("prompt.config_command"), "info").strip().lower()
    if choice == "info":
        app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "config", "info"])
    elif choice == "reset":
        app._suspend_and_run([sys.executable, str(FLASH_SCRIPT), "config", "reset"])
    else:
        app.notify(t("prompt.unknown_command", cmd=choice), severity="error")


def _act_studio_install(app: BKConsoleApp) -> None:
    app._suspend_and_run(["pnpm", "install"], cwd=STUDIO_DIR)


def _act_studio_dev(app: BKConsoleApp) -> None:
    app._launch_in_terminal(["pnpm", "run", "dev"], cwd=STUDIO_DIR)


def _act_studio_build(app: BKConsoleApp) -> None:
    app._suspend_and_run(["pnpm", "run", "build"], cwd=STUDIO_DIR)


def _act_docs_install(app: BKConsoleApp) -> None:
    app._suspend_and_run(["pnpm", "install"], cwd=DOCS_DIR)


def _act_docs_dev(app: BKConsoleApp) -> None:
    app._launch_in_terminal(["pnpm", "run", "dev"], cwd=DOCS_DIR)


def _act_docs_build(app: BKConsoleApp) -> None:
    app._suspend_and_run(["pnpm", "run", "build"], cwd=DOCS_DIR)


def _open_url(name: str, url: str, app: BKConsoleApp) -> None:
    if webbrowser.open(url):
        app.notify(t("url.opened", name=name, url=url), timeout=2)
    else:
        app.notify(t("url.failed"), severity="error")


def _act_toggle_lang(app: BKConsoleApp) -> None:
    new_lang = toggle_lang()
    app.state["lang"] = new_lang
    save_state(app.state)
    app._rebuild_ui()
    lang_label = t("lang.en") if new_lang == "en" else t("lang.zh")
    app.notify(f"Language: {lang_label}", timeout=1)


ACTION_DISPATCH: dict[str, callable] = {
    "toggle_target": _act_toggle_target,
    "cycle_keyboard": _act_cycle_keyboard,
    "toggle_build_type": _act_toggle_build_type,
    "configure_toolchain": _act_configure_toolchain,
    "reset_toolchain": _act_reset_toolchain,
    "build": _act_build,
    "clean": _act_clean,
    "flash": _act_flash,
    "show_commands": _act_show_commands,
    "generate_ide_config": _act_generate_ide,
    "install_wchisp": _act_install_wchisp,
    "probe": _act_probe,
    "show_isp_sheet": _act_show_isp_sheet,
    "chip_menu": _act_chip_menu,
    "eeprom_menu": _act_eeprom_menu,
    "config_menu": _act_config_menu,
    "studio_install": _act_studio_install,
    "studio_dev": _act_studio_dev,
    "studio_build": _act_studio_build,
    "docs_install": _act_docs_install,
    "docs_dev": _act_docs_dev,
    "docs_build": _act_docs_build,
    "toggle_lang": _act_toggle_lang,
    "open_mrs": lambda app: _open_url("MRS download", DOC_URLS["MRS download"], app),
    "open_wchisp": lambda app: _open_url("wchisp release", DOC_URLS["wchisp release"], app),
    "open_wch": lambda app: _open_url("WCH homepage", DOC_URLS["WCH homepage"], app),
    "open_repo": lambda app: _open_url("Project repo", DOC_URLS["Project repo"], app),
}


# ── Entry ─────────────────────────────────────────────────────────────────────

def run_textual() -> None:
    app = BKConsoleApp()
    app.run()


if __name__ == "__main__":
    run_textual()
