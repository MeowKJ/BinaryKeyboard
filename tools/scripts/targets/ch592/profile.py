from __future__ import annotations

import json
import os
import shutil
import sys
from pathlib import Path

from ch592f import DEFAULT_KEYBOARD, DEFAULT_PROFILE, preset_for, _resolve_preset
from firmware_naming import ch592_filename_for_keyboard
from targets.common import FLASH_SCRIPT, PROJECT_ROOT, SCRIPT_DIR, TargetActionSpec, TargetProfile


FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
CH592_USER_PRESETS = FIRMWARE_DIR / "CMakeUserPresets.json"
CH592_USER_PRESETS_EXAMPLE = FIRMWARE_DIR / "CMakeUserPresets.json.example"


def _base_preset(state: dict) -> str:
    return preset_for(state["keyboard"], state["build_type"])


def _resolved_preset_label(state: dict) -> str:
    return _resolve_preset(_base_preset(state))


def _normalize_state(state: dict) -> None:
    if state.get("keyboard") not in ("5KEY", "KNOB"):
        state["keyboard"] = DEFAULT_KEYBOARD
    if state.get("build_type") not in ("release", "debug"):
        state["build_type"] = DEFAULT_PROFILE


def _build_dir(state: dict) -> Path:
    return FIRMWARE_DIR / "build" / _resolved_preset_label(state)


def _artifact_path(state: dict) -> Path:
    build_dir = _build_dir(state)
    filename = ch592_filename_for_keyboard(state["keyboard"], "bin")
    named = build_dir / filename
    legacy = build_dir / "CH592F.bin"
    return named if named.is_file() or not legacy.is_file() else legacy


def _build_label(state: dict) -> str:
    return f"keyboard {state['keyboard']}  ·  profile {state['build_type']}"


def _build_command(state: dict) -> list[str]:
    return [sys.executable, str(SCRIPT_DIR / "ch592f.py"), "build", "--keyboard", state["keyboard"], "--profile", state["build_type"]]


def _flash_command(state: dict) -> list[str]:
    return [sys.executable, str(FLASH_SCRIPT), "flash", "--file", str(_artifact_path(state))]


def _verify_command(state: dict) -> list[str]:
    return [sys.executable, str(FLASH_SCRIPT), "verify", "--file", str(_artifact_path(state))]


def _build_command_display(state: dict) -> str:
    return f"python tools/scripts/ch592f.py build --keyboard {state['keyboard']} --profile {state['build_type']}"


def _flash_command_display(state: dict) -> str:
    return f"python tools/scripts/flash.py flash --file {_display_path(_artifact_path(state))}"


def _verify_command_display(state: dict) -> str:
    return f"python tools/scripts/flash.py verify --file {_display_path(_artifact_path(state))}"


def _effective_toolchain(state: dict) -> str:
    env = os.environ.get("MRS_TOOLCHAIN_ROOT", "").strip()
    return state.get("toolchain_root", "").strip() or env


def _find_gcc_in_toolchain(toolchain_root: str) -> str | None:
    root = Path(toolchain_root)
    if not root.is_dir():
        return None
    candidates = list(root.rglob("riscv-none-embed-gcc"))
    if not candidates:
        candidates = list(root.rglob("riscv-none-elf-gcc"))
    if os.name == "nt" and not candidates:
        candidates = list(root.rglob("riscv-none-embed-gcc.exe")) + list(root.rglob("riscv-none-elf-gcc.exe"))
    return str(candidates[0]) if candidates else None


def _target_details_lines(state: dict) -> list[str]:
    toolchain = _effective_toolchain(state) or "(unset)"
    preset_status = "present" if CH592_USER_PRESETS.is_file() else "missing — run Configure toolchain"
    return [
        f"Target: {state['target']}",
        f"Build dir: {_display_path(_build_dir(state))}",
        f"Toolchain: {toolchain}",
        f"CMakeUserPresets: {preset_status}",
        f"Keyboard: {state['keyboard']}",
        f"Profile: {state['build_type']}",
    ]


def _detect_tool_lines(state: dict) -> list[str]:
    toolchain = _effective_toolchain(state)
    lines = [
        f"ninja: {shutil.which('ninja') or 'missing'}",
        f"CMakeUserPresets: {'present' if CH592_USER_PRESETS.is_file() else 'missing'}",
        f"MRS_TOOLCHAIN_ROOT: {toolchain or 'unset'}",
    ]
    if toolchain:
        lines.append(f"  riscv-gcc: {_find_gcc_in_toolchain(toolchain) or 'not found in toolchain path'}")
    return lines


def _doctor_lines(state: dict) -> list[str]:
    lines: list[str] = []
    ninja_path = shutil.which("ninja")
    lines.append(f"[{'OK' if ninja_path else 'WARN'}] ninja: {ninja_path or 'missing'}")
    if CH592_USER_PRESETS.is_file():
        lines.append("[OK] CMakeUserPresets.json: present")
    else:
        lines.append("[WARN] CMakeUserPresets.json: missing — run 'Configure toolchain'")

    toolchain = _effective_toolchain(state)
    if toolchain:
        if Path(toolchain).is_dir():
            gcc = _find_gcc_in_toolchain(toolchain)
            if gcc:
                lines.append(f"[OK] toolchain: {toolchain}")
                lines.append(f"[OK] riscv-gcc: {Path(gcc).name}")
            else:
                lines.append(f"[WARN] toolchain: {toolchain} (no riscv-*-elf-gcc found)")
        else:
            lines.append(f"[WARN] toolchain: {toolchain} (path does not exist)")
    else:
        lines.append("[WARN] toolchain: not configured")
    return lines


def _home_actions(state: dict) -> list[TargetActionSpec]:
    return [
        TargetActionSpec("toggle_target", f"Toggle target  [{state['target']}]", "Switch between CH592F and CH552G workflows."),
        TargetActionSpec("cycle_keyboard", f"Cycle keyboard  [{state['keyboard']}]", "Switch between 5KEY and KNOB."),
        TargetActionSpec("toggle_build_type", f"Toggle profile  [{state['build_type']}]", "Switch between release and debug."),
        TargetActionSpec("configure_toolchain", "Configure toolchain", "Set MRS_TOOLCHAIN_ROOT and write CMakeUserPresets.json."),
        TargetActionSpec("build", "Build selected target", "Run tools/scripts/ch592f.py build."),
        TargetActionSpec("flash", "Flash selected target", "Build, then flash the resolved CH592F artifact."),
        TargetActionSpec("show_commands", "Show build commands", "Print the resolved build / flash / verify commands."),
        TargetActionSpec("install_wchisp", "Install or update wchisp", "Run tools/scripts/setup.py."),
        TargetActionSpec("probe", "Probe ISP devices", "List connected WCH ISP devices."),
    ]


def _display_path(path: Path) -> str:
    try:
        return str(path.relative_to(PROJECT_ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


CH592_PROFILE = TargetProfile(
    key="CH592F",
    display_name="CH592F",
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
