from __future__ import annotations

import sys
from pathlib import Path

from ch552g import DEFAULT_KEYBOARD, VALID_KEYBOARDS, build_dir_for
from common import display_path as _display_path, find_sdcc
from versioning import ch552_filename_for_keyboard
from i18n import t
from targets.common import FLASH_SCRIPT, PROJECT_ROOT, SCRIPT_DIR, TargetActionSpec, TargetProfile


FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH552G"
VALID_KEYBOARD_SET = set(VALID_KEYBOARDS)


def _normalize_state(state: dict) -> None:
    if state.get("keyboard") not in VALID_KEYBOARD_SET:
        state["keyboard"] = DEFAULT_KEYBOARD


def _build_dir(state: dict) -> Path:
    return build_dir_for(state["keyboard"])


def _artifact_path(state: dict) -> Path:
    build_dir = _build_dir(state)
    filename = ch552_filename_for_keyboard(state["keyboard"], "bin")
    named = build_dir / filename
    legacy = build_dir / "CH552G.bin"
    return named if named.is_file() or not legacy.is_file() else legacy


def _build_label(state: dict) -> str:
    return f"keyboard {state['keyboard']}"


def _build_command(state: dict) -> list[str]:
    return [sys.executable, str(SCRIPT_DIR / "ch552g.py"), "build", "--keyboard", state["keyboard"]]


def _flash_command(state: dict) -> list[str]:
    return [sys.executable, str(FLASH_SCRIPT), "flash", "--file", str(_artifact_path(state))]


def _verify_command(state: dict) -> list[str]:
    return [sys.executable, str(FLASH_SCRIPT), "verify", "--file", str(_artifact_path(state))]


def _build_command_display(state: dict) -> str:
    return f"python tools/scripts/ch552g.py build --keyboard {state['keyboard']}"


def _flash_command_display(state: dict) -> str:
    return f"python tools/scripts/flash.py flash --file {_display_path(_artifact_path(state))}"


def _verify_command_display(state: dict) -> str:
    return f"python tools/scripts/flash.py verify --file {_display_path(_artifact_path(state))}"


def _target_details_lines(state: dict) -> list[str]:
    return [
        f"{t('detail.build_dir')}: {_display_path(_build_dir(state))}",
    ]


def _detect_tool_lines(_state: dict) -> list[str]:
    sdcc = find_sdcc()
    return [f"sdcc: {sdcc or 'missing'}"]


def _doctor_lines(_state: dict) -> list[str]:
    sdcc_path = find_sdcc()
    return [f"[{'OK' if sdcc_path else 'FAIL'}] sdcc: {sdcc_path or 'missing'}"]


def _home_actions(state: dict) -> list[TargetActionSpec]:
    return [
        TargetActionSpec("build", t("action.build"), t("hint.build_552")),
        TargetActionSpec("flash", t("action.flash"), t("hint.flash_552")),
        TargetActionSpec("clean", t("action.clean"), t("hint.clean")),
        TargetActionSpec("toggle_target", t("action.toggle_target", target=state["target"]), t("hint.toggle_target")),
        TargetActionSpec("cycle_keyboard", t("action.cycle_keyboard", keyboard=state["keyboard"]), t("hint.cycle_keyboard_552")),
        TargetActionSpec("show_commands", t("action.show_commands"), t("hint.show_commands")),
        TargetActionSpec("generate_ide_config", t("action.generate_ide"), t("hint.generate_ide")),
    ]


CH552_PROFILE = TargetProfile(
    key="CH552G",
    display_name="CH552G",
    normalize_state=_normalize_state,
    build_label=_build_label,
    artifact_path=_artifact_path,
    build_dir=_build_dir,
    build_command=_build_command,
    flash_command=_flash_command,
    verify_command=_verify_command,
    build_command_display=_build_command_display,
    flash_command_display=_flash_command_display,
    verify_command_display=_verify_command_display,
    target_details_lines=_target_details_lines,
    detect_tool_lines=_detect_tool_lines,
    doctor_lines=_doctor_lines,
    home_actions=_home_actions,
)
