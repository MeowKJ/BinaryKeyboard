#!/usr/bin/env python3
"""Primary BinaryKeyboard TUI entry point."""

import os
from pathlib import Path
import subprocess
import sys
import venv

from common import find_wchisp


SCRIPT_PATH = Path(__file__).resolve()
SCRIPT_DIR = SCRIPT_PATH.parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
VENV_DIR = PROJECT_ROOT / ".venv"
VENV_BIN_DIR = VENV_DIR / ("Scripts" if os.name == "nt" else "bin")
CONSOLE_REQUIREMENTS = SCRIPT_DIR / "requirements-console.txt"
SETUP_SCRIPT = SCRIPT_DIR / "setup.py"
FETCH_WINDOWS_DLL_SCRIPT = PROJECT_ROOT / "tools" / "meowisp" / "scripts" / "fetch_windows_dll.py"


def _venv_python() -> Path:
    candidates = ["python.exe"] if os.name == "nt" else ["python3", "python"]
    for name in candidates:
        candidate = VENV_BIN_DIR / name
        if candidate.is_file():
            return candidate
    return VENV_BIN_DIR / candidates[0]


def _same_python(path: Path) -> bool:
    try:
        return Path(sys.executable).resolve() == path.resolve()
    except FileNotFoundError:
        return False


def _check_dependency(python_exe: Path, module_name: str) -> bool:
    result = subprocess.run(
        [
            str(python_exe),
            "-c",
            (
                "import importlib.util, sys; "
                f"sys.exit(0 if importlib.util.find_spec({module_name!r}) else 1)"
            ),
        ],
        check=False,
    )
    return result.returncode == 0


def _create_venv() -> None:
    print(f"[console] Creating virtual environment: {VENV_DIR}", file=sys.stderr)
    venv.EnvBuilder(with_pip=True).create(str(VENV_DIR))


def _install_requirements(python_exe: Path) -> None:
    print("[console] Installing console dependencies...", file=sys.stderr)
    try:
        subprocess.run(
            [str(python_exe), "-m", "pip", "install", "-r", str(CONSOLE_REQUIREMENTS)],
            check=True,
        )
    except subprocess.CalledProcessError as exc:
        raise SystemExit(
            "Failed to install console dependencies into `.venv`. "
            f"Try running `{python_exe} -m pip install -r {CONSOLE_REQUIREMENTS}` manually."
        ) from exc


def _venv_env() -> dict[str, str]:
    env = os.environ.copy()
    env["VIRTUAL_ENV"] = str(VENV_DIR)
    env["PATH"] = str(VENV_BIN_DIR) + os.pathsep + env.get("PATH", "")
    return env


def _ensure_venv_ready() -> None:
    python_exe = _venv_python()
    if not python_exe.is_file():
        _create_venv()
        python_exe = _venv_python()
    if not _check_dependency(python_exe, "textual"):
        _install_requirements(python_exe)


def _reexec_in_venv(python_exe: Path) -> None:
    print(f"[console] Restarting with {python_exe}", file=sys.stderr)
    argv = [str(python_exe), str(SCRIPT_PATH), *sys.argv[1:]]

    if os.name == "nt":
        completed = subprocess.run(argv, env=_venv_env(), check=False)
        raise SystemExit(completed.returncode)

    os.execve(str(python_exe), argv, _venv_env())


def _ensure_wchisp_ready(python_exe: Path) -> None:
    if find_wchisp():
        return

    print("[console] wchisp not found, running setup.py...", file=sys.stderr)
    result = subprocess.run(
        [str(python_exe), str(SETUP_SCRIPT)],
        env=_venv_env(),
        check=False,
    )
    if result.returncode != 0:
        print(
            "[console] Auto-install of wchisp failed. "
            "The console will continue and show ISP tools as unavailable.",
            file=sys.stderr,
        )


def _ensure_windows_isp_assets_ready(python_exe: Path) -> None:
    if os.name != "nt":
        return
    if not FETCH_WINDOWS_DLL_SCRIPT.is_file():
        print(
            f"[console] Windows ISP DLL fetch script not found: {FETCH_WINDOWS_DLL_SCRIPT}",
            file=sys.stderr,
        )
        return
    wchisp = find_wchisp()
    if not wchisp:
        print("[console] wchisp not found, skip CH375 DLL check.", file=sys.stderr)
        return
    dll_path = wchisp.parent / "CH375DLL64.dll"
    if dll_path.is_file():
        return

    print(
        f"[console] CH375DLL64.dll not found next to wchisp, fetching to {dll_path}...",
        file=sys.stderr,
    )
    result = subprocess.run(
        [str(python_exe), str(FETCH_WINDOWS_DLL_SCRIPT), "--out", str(dll_path)],
        env=_venv_env(),
        check=False,
    )
    if result.returncode != 0:
        print(
            "[console] Auto-fetch of CH375DLL64.dll failed. "
            "Windows ISP may be unavailable until the DLL is downloaded.",
            file=sys.stderr,
        )


def main() -> None:
    if len(sys.argv) > 1:
        raise SystemExit("Legacy console flags were removed. Run `python tools/scripts/console.py` without arguments.")

    _ensure_venv_ready()
    python_exe = _venv_python()
    if not _same_python(python_exe):
        _reexec_in_venv(python_exe)
    os.environ.update(_venv_env())
    _ensure_wchisp_ready(python_exe)
    _ensure_windows_isp_assets_ready(python_exe)

    try:
        from tui_textual import run_textual
    except ImportError as exc:
        missing = getattr(exc, "name", "") or ""
        if missing == "textual" or missing.startswith("textual."):
            raise SystemExit(
                "Missing dependency `textual` inside `.venv`. "
                f"Try `{python_exe} -m pip install -r {CONSOLE_REQUIREMENTS}`."
            ) from exc
        raise

    try:
        run_textual()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
