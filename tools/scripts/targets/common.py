from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Callable


SCRIPT_DIR = Path(__file__).resolve().parents[1]
PROJECT_ROOT = SCRIPT_DIR.parent.parent
FLASH_SCRIPT = SCRIPT_DIR / "flash.py"
SETUP_SCRIPT = SCRIPT_DIR / "setup.py"


@dataclass(frozen=True)
class TargetActionSpec:
    action_id: str
    label: str
    hint: str


@dataclass(frozen=True)
class TargetProfile:
    key: str
    display_name: str
    normalize_state: Callable[[dict], None]
    build_label: Callable[[dict], str]
    artifact_path: Callable[[dict], Path]
    build_dir: Callable[[dict], Path]
    build_command: Callable[[dict], list[str]]
    flash_command: Callable[[dict], list[str]]
    verify_command: Callable[[dict], list[str]]
    build_command_display: Callable[[dict], str]
    flash_command_display: Callable[[dict], str]
    verify_command_display: Callable[[dict], str]
    target_details_lines: Callable[[dict], list[str]]
    detect_tool_lines: Callable[[dict], list[str]]
    doctor_lines: Callable[[dict], list[str]]
    home_actions: Callable[[dict], list[TargetActionSpec]]
