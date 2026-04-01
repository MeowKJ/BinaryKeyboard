#!/usr/bin/env python3
"""
setup.py — BinaryKeyboard development environment setup
Downloads wchisp from GitHub releases into tools/scripts/
"""

import json
import os
import platform
import shutil
import stat
import sys
import tarfile
import tempfile
import urllib.request
import zipfile
from pathlib import Path

REPO = "ch32-rs/wchisp"
API_URL = f"https://api.github.com/repos/{REPO}/releases/latest"
TOOLS_SCRIPTS = Path(__file__).parent

# ── Terminal colors ────────────────────────────────────────────────────────────
def _c(code, text): return f"\033[{code}m{text}\033[0m" if sys.stdout.isatty() else text
def info(msg):    print(_c("36", "[INFO]"), msg)
def ok(msg):      print(_c("32", "[ OK ]"), msg)
def warn(msg):    print(_c("33", "[WARN]"), msg)
def error(msg):   print(_c("31", "[ERR ]"), msg, file=sys.stderr)
def sep():        print(_c("2", "-" * 44))


# ── Platform detection ─────────────────────────────────────────────────────────
def detect_platform() -> tuple[str, str]:
    """Returns (os_tag, arch_tag) matching wchisp release naming."""
    system = platform.system().lower()
    machine = platform.machine().lower()

    os_map = {
        "darwin": "macos",
        "linux":  "linux",
        "windows": "win",
    }
    arch_map = {
        "x86_64":  "x64",
        "amd64":   "x64",
        "aarch64": "aarch64",
        "arm64":   "arm64",
    }

    os_tag = os_map.get(system)
    arch_tag = arch_map.get(machine)

    if not os_tag:
        raise RuntimeError(f"Unsupported OS: {platform.system()}")
    if not arch_tag:
        raise RuntimeError(f"Unsupported architecture: {platform.machine()}")

    # wchisp uses "arm64" for macOS Apple Silicon, "aarch64" for Linux ARM
    if os_tag == "macos" and arch_tag == "aarch64":
        arch_tag = "arm64"
    if os_tag == "linux" and arch_tag == "arm64":
        arch_tag = "aarch64"

    return os_tag, arch_tag


# ── GitHub release fetch ───────────────────────────────────────────────────────
def fetch_latest_release() -> dict:
    info(f"Fetching latest release from github.com/{REPO} ...")
    req = urllib.request.Request(API_URL, headers={"User-Agent": "setup.py"})
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.load(resp)


def find_asset(assets: list, os_tag: str, arch_tag: str) -> dict:
    pattern = f"{os_tag}-{arch_tag}"
    for asset in assets:
        if pattern in asset["name"]:
            return asset
    available = [a["name"] for a in assets]
    raise RuntimeError(
        f"No asset found for '{pattern}'.\nAvailable:\n" +
        "\n".join(f"  {n}" for n in available)
    )


# ── Download & extract ─────────────────────────────────────────────────────────
def download(url: str, dest: Path) -> None:
    info(f"Downloading {url.split('/')[-1]} ...")
    def _progress(count, block, total):
        pct = min(count * block / total * 100, 100) if total > 0 else 0
        bar = "█" * int(pct / 5) + "░" * (20 - int(pct / 5))
        print(f"\r  [{bar}] {pct:5.1f}%", end="", flush=True)
    urllib.request.urlretrieve(url, dest, reporthook=_progress)
    print()


def extract_binary(archive: Path, dest_dir: Path) -> Path:
    """Extract wchisp binary from archive, return path to extracted binary."""
    dest_dir.mkdir(parents=True, exist_ok=True)
    binary_name = "wchisp.exe" if platform.system() == "Windows" else "wchisp"

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)

        if archive.suffix == ".gz":
            with tarfile.open(archive, "r:gz") as tf:
                tf.extractall(tmp_path)
        elif archive.suffix == ".zip":
            with zipfile.ZipFile(archive) as zf:
                zf.extractall(tmp_path)
        else:
            raise RuntimeError(f"Unknown archive type: {archive.suffix}")

        # Find the binary in extracted files
        found = list(tmp_path.rglob(binary_name))
        if not found:
            raise RuntimeError(f"'{binary_name}' not found in archive.")

        dest = dest_dir / binary_name
        shutil.copy2(found[0], dest)

        # Make executable
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        return dest


# ── Version check ──────────────────────────────────────────────────────────────
def current_version(binary: Path):
    """Return version string of installed wchisp, or None."""
    import subprocess
    try:
        result = subprocess.run(
            [str(binary), "--version"],
            capture_output=True, text=True, timeout=5
        )
        return result.stdout.strip() or result.stderr.strip()
    except Exception:
        return None


# ── Main ───────────────────────────────────────────────────────────────────────
def main():
    sep()
    print(_c("1", "  BinaryKeyboard — Development Setup"))
    sep()

    # Check existing installation
    dest_binary = TOOLS_SCRIPTS / ("wchisp.exe" if platform.system() == "Windows" else "wchisp")

    if dest_binary.exists() and "--force" not in sys.argv:
        ver = current_version(dest_binary)
        ok(f"wchisp already installed: {ver or 'unknown version'}")
        info(f"Location: {dest_binary}")
        info("Use --force to reinstall.")
        sep()
        print_next_steps()
        return

    # Detect platform
    try:
        os_tag, arch_tag = detect_platform()
    except RuntimeError as e:
        error(str(e))
        sys.exit(1)
    info(f"Platform: {platform.system()} / {platform.machine()} → {os_tag}-{arch_tag}")

    # Fetch release info
    try:
        release = fetch_latest_release()
    except Exception as e:
        error(f"Failed to fetch release info: {e}")
        sys.exit(1)

    version = release["tag_name"]
    info(f"Latest version: {version}")

    # Find matching asset
    try:
        asset = find_asset(release["assets"], os_tag, arch_tag)
    except RuntimeError as e:
        error(str(e))
        sys.exit(1)

    # Download
    with tempfile.NamedTemporaryFile(suffix=Path(asset["name"]).suffix, delete=False) as tmp:
        tmp_path = Path(tmp.name)

    try:
        download(asset["browser_download_url"], tmp_path)

        # Extract
        info(f"Extracting to {TOOLS_SCRIPTS}/")
        binary = extract_binary(tmp_path, TOOLS_SCRIPTS)

        # Verify
        ver = current_version(binary)
        sep()
        ok(f"wchisp installed: {ver or version}")
        ok(f"Location: {binary}")
    finally:
        tmp_path.unlink(missing_ok=True)

    sep()
    print_next_steps()


def print_next_steps():
    print(_c("1", "  Next steps:"))
    print("  1. Build firmware:")
    print("     CH592F: python tools/scripts/ch592f.py build --keyboard 5KEY --profile release")
    print("     CH552G: python tools/scripts/ch552g.py build --keyboard BASIC")
    print("  2. Connect target via USB while holding BOOT button")
    print("  3. Flash artifact:")
    print("     python tools/scripts/flash.py flash --file <firmware-artifact.bin>")
    sep()


if __name__ == "__main__":
    main()
