from __future__ import annotations

import sys
from pathlib import Path

from ch552g import DEFAULT_KEYBOARD, VALID_KEYBOARDS, build_dir_for, find_sdcc
from firmware_naming import ch552_filename_for_keyboard
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
        f"Target: {state['target']}",
        f"Build dir: {_display_path(_build_dir(state))}",
        f"Compiler: {find_sdcc() or 'missing'}",
        f"Keyboard: {state['keyboard']}",
        "Keymap: multi-layer remap + FUNC-hold layer switching",
        "RGB: key RGB with layer flash feedback (no indicator RGB)",
    ]


def _detect_tool_lines(_state: dict) -> list[str]:
    return [f"sdcc: {find_sdcc() or 'missing'}"]


def _doctor_lines(_state: dict) -> list[str]:
    sdcc_path = find_sdcc()
    return [f"[{'OK' if sdcc_path else 'WARN'}] sdcc: {sdcc_path or 'missing'}"]


def _home_actions(state: dict) -> list[TargetActionSpec]:
    return [
        TargetActionSpec("toggle_target", f"Toggle target  [{state['target']}]", "Switch between CH592F and CH552G workflows."),
        TargetActionSpec("cycle_keyboard", f"Cycle keyboard  [{state['keyboard']}]", "Cycle BASIC / KNOB / 5KEY."),
        TargetActionSpec("build", "Build selected target", "Run tools/scripts/ch552g.py build."),
        TargetActionSpec("flash", "Flash selected target", "Build, then flash the resolved CH552G artifact."),
        TargetActionSpec("show_commands", "Show build commands", "Print the resolved build / flash / verify commands."),
        TargetActionSpec("generate_ide_config", "Generate IDE config", "Write VSCode C/C++ settings and a root compile_commands.json."),
        TargetActionSpec("install_wchisp", "Install or update wchisp", "Run tools/scripts/setup.py."),
        TargetActionSpec("probe", "Probe ISP devices", "List connected WCH ISP devices."),
    ]


def _display_path(path: Path) -> str:
    try:
        return str(path.relative_to(PROJECT_ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


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
