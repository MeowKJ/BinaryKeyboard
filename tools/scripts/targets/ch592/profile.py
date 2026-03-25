from __future__ import annotations

import os
import shutil
import sys
from pathlib import Path

from ch592f import DEFAULT_KEYBOARD, DEFAULT_PROFILE, artifact_paths as _artifact_paths, preset_for
from common import display_path as _display_path
from firmware_naming import ch592_filename_for_keyboard
from i18n import t
from targets.common import FLASH_SCRIPT, PROJECT_ROOT, SCRIPT_DIR, TargetActionSpec, TargetProfile
from tool_cache import get_cached_tool_path


FIRMWARE_DIR = PROJECT_ROOT / "firmware" / "CH592F"
_MRS_TOOLCHAIN_BIN_DIRS = ("RISC-V Embedded GCC/bin", "RISC-V Embedded GCC12/bin")
_RISCV_GCC_PREFIXES = ("riscv-none-embed-", "riscv-wch-elf-", "riscv-none-elf-")


def _base_preset(state: dict) -> str:
    return preset_for(state["keyboard"], state["build_type"])


def _normalize_state(state: dict) -> None:
    if state.get("keyboard") not in ("5KEY", "KNOB"):
        state["keyboard"] = DEFAULT_KEYBOARD
    if state.get("build_type") not in ("release", "debug"):
        state["build_type"] = DEFAULT_PROFILE


def _build_dir(state: dict) -> Path:
    return FIRMWARE_DIR / "build" / _base_preset(state)


def _artifact_path(state: dict) -> Path:
    """Return the -full.hex (JumpIAP + app + IAP merged) for ISP flashing."""
    build_dir = _build_dir(state)
    arts = _artifact_paths(build_dir, state["keyboard"])
    full = arts["full_hex"]
    if full.is_file():
        return full
    # Fallback: app .bin (before bootloader integration)
    app_bin = arts["bin"]
    legacy = build_dir / "CH592F.bin"
    return app_bin if app_bin.is_file() or not legacy.is_file() else legacy


def _build_label(state: dict) -> str:
    return f"keyboard {state['keyboard']}  |  profile {state['build_type']}"


def _build_command(state: dict) -> list[str]:
    return [sys.executable, str(SCRIPT_DIR / "ch592f.py"), "build-full", "--keyboard", state["keyboard"], "--profile", state["build_type"]]


def _flash_command(state: dict) -> list[str]:
    return [sys.executable, str(FLASH_SCRIPT), "flash", "--file", str(_artifact_path(state))]


def _verify_command(state: dict) -> list[str]:
    return [sys.executable, str(FLASH_SCRIPT), "verify", "--file", str(_artifact_path(state))]


def _build_command_display(state: dict) -> str:
    return f"python tools/scripts/ch592f.py build-full --keyboard {state['keyboard']} --profile {state['build_type']}"


def _flash_command_display(state: dict) -> str:
    return f"python tools/scripts/flash.py flash --file {_display_path(_artifact_path(state))}"


def _verify_command_display(state: dict) -> str:
    return f"python tools/scripts/flash.py verify --file {_display_path(_artifact_path(state))}"


def _gcc_binary_names() -> list[str]:
    suffix = ".exe" if os.name == "nt" else ""
    return [f"{prefix}gcc{suffix}" for prefix in _RISCV_GCC_PREFIXES]


def _find_gcc_in_toolchain_dir(toolchain_dir: Path) -> str | None:
    if not toolchain_dir.is_dir():
        return None
    for binary in _gcc_binary_names():
        candidate = toolchain_dir / binary
        if candidate.is_file():
            return str(candidate.resolve())
    return None


def _find_gcc_in_toolchain(toolchain_root: str) -> str | None:
    toolchain_dir = _toolchain_dir_from_root(toolchain_root)
    if toolchain_dir is None:
        return None
    return _find_gcc_in_toolchain_dir(toolchain_dir)


def _toolchain_dir_from_root(toolchain_root: str) -> Path | None:
    root = Path(toolchain_root)
    if not root.is_dir():
        return None
    for subdir in _MRS_TOOLCHAIN_BIN_DIRS:
        candidate = root / subdir
        if candidate.is_dir():
            return candidate.resolve()
    return None


def _find_gcc_on_path() -> str | None:
    for binary in _gcc_binary_names():
        resolved = shutil.which(binary)
        if resolved:
            return str(Path(resolved).resolve())
    return None


def _resolve_toolchain(state: dict) -> dict[str, object]:
    toolchain_root = state.get("toolchain_root", "").strip()
    env_root = os.environ.get("MRS_TOOLCHAIN_ROOT", "").strip()
    env_toolchain_dir = (
        os.environ.get("RISCV_TOOLCHAIN_DIR", "").strip()
        or os.environ.get("TOOLCHAIN_DIR", "").strip()
    )
    warnings: list[str] = []

    if env_toolchain_dir:
        toolchain_dir = Path(env_toolchain_dir)
        gcc = _find_gcc_in_toolchain_dir(toolchain_dir)
        if gcc:
            return {
                "source": "env RISCV_TOOLCHAIN_DIR/TOOLCHAIN_DIR",
                "toolchain_dir": str(toolchain_dir.resolve()),
                "gcc": gcc,
                "warnings": warnings,
            }
        warnings.append(f"env RISCV_TOOLCHAIN_DIR/TOOLCHAIN_DIR has no riscv gcc: {env_toolchain_dir}")

    explicit_root = env_root or toolchain_root
    explicit_root_source = "env MRS_TOOLCHAIN_ROOT" if env_root else "console cache toolchain_root"
    if explicit_root:
        resolved_dir = _toolchain_dir_from_root(explicit_root)
        if resolved_dir is None:
            warnings.append(f"{explicit_root_source} has no supported GCC bin dir: {explicit_root}")
        else:
            gcc = _find_gcc_in_toolchain_dir(resolved_dir)
            if gcc:
                return {
                    "source": explicit_root_source,
                    "toolchain_dir": str(resolved_dir),
                    "gcc": gcc,
                    "warnings": warnings,
                }
            warnings.append(f"{explicit_root_source} has no riscv gcc under: {resolved_dir}")

    cached_gcc = get_cached_tool_path("riscv_gcc")
    if cached_gcc and cached_gcc.is_file():
        return {
            "source": "tool cache riscv_gcc",
            "toolchain_dir": str(cached_gcc.parent.resolve()),
            "gcc": str(cached_gcc.resolve()),
            "warnings": warnings,
        }

    path_gcc = _find_gcc_on_path()
    if path_gcc:
        return {
            "source": "system PATH",
            "toolchain_dir": str(Path(path_gcc).resolve().parent),
            "gcc": path_gcc,
            "warnings": warnings,
        }

    return {
        "source": "unresolved",
        "toolchain_dir": "",
        "gcc": "",
        "warnings": warnings,
    }


def _target_details_lines(state: dict) -> list[str]:
    return [
        f"{t('detail.build_dir')}: {_display_path(_build_dir(state))}",
    ]


def _detect_tool_lines(state: dict) -> list[str]:
    resolved = _resolve_toolchain(state)
    lines = [
        f"ninja: {shutil.which('ninja') or 'missing'}",
        f"toolchain source: {resolved['source']}",
        "toolchain dir:",
        f"  {resolved['toolchain_dir'] or 'unresolved'}",
    ]
    if resolved["gcc"]:
        lines.append("riscv-gcc:")
        lines.append(f"  {resolved['gcc']}")
    for warning in resolved["warnings"]:
        lines.append(f"note: {warning}")
    return lines


def _doctor_lines(state: dict) -> list[str]:
    lines: list[str] = []
    ninja_path = shutil.which("ninja")
    lines.append(f"[{'OK' if ninja_path else 'FAIL'}] ninja: {ninja_path or 'missing'}")
    resolved = _resolve_toolchain(state)
    for warning in resolved["warnings"]:
        lines.append(f"[FAIL] {warning}")
    if resolved["gcc"]:
        lines.append(f"[OK] toolchain source: {resolved['source']}")
        lines.append(f"[OK] toolchain dir:")
        lines.append(f"      {resolved['toolchain_dir']}")
        lines.append(f"[OK] riscv-gcc: {Path(str(resolved['gcc'])).name}")
    else:
        lines.append("[FAIL] riscv-gcc: not found via env/tool cache/PATH")
    return lines


def _home_actions(state: dict) -> list[TargetActionSpec]:
    return [
        TargetActionSpec("build", t("action.build"), t("hint.build_592")),
        TargetActionSpec("flash", t("action.flash"), t("hint.flash_592")),
        TargetActionSpec("clean", t("action.clean"), t("hint.clean")),
        TargetActionSpec("toggle_target", t("action.toggle_target", target=state["target"]), t("hint.toggle_target")),
        TargetActionSpec("cycle_keyboard", t("action.cycle_keyboard", keyboard=state["keyboard"]), t("hint.cycle_keyboard_592")),
        TargetActionSpec("toggle_build_type", t("action.toggle_profile", profile=state["build_type"]), t("hint.toggle_profile")),
        TargetActionSpec("show_commands", t("action.show_commands"), t("hint.show_commands")),
        TargetActionSpec("configure_toolchain", t("action.configure_toolchain"), t("hint.configure_toolchain")),
        TargetActionSpec("generate_ide_config", t("action.generate_ide"), t("hint.generate_ide")),
    ]


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
