#!/usr/bin/env python3
"""BinaryKeyboard development environment setup.

Downloads wchisp into tools/scripts/ without relying on the GitHub Releases API
for the common path. GitHub release assets redirect to release-assets CDN, which
avoids the low anonymous API rate limit that can break first-run setup.
"""

from __future__ import annotations

import json
import os
import platform
import shutil
import stat
import sys
import tarfile
import tempfile
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

REPO = "ch32-rs/wchisp"
GITHUB_BASE = f"https://github.com/{REPO}"
LATEST_URL = f"{GITHUB_BASE}/releases/latest"
API_URL = f"https://api.github.com/repos/{REPO}/releases/latest"
TOOLS_SCRIPTS = Path(__file__).parent
USER_AGENT = "BinaryKeyboard-setup/1.0"


def _c(code: str, text: str) -> str:
    return f"\033[{code}m{text}\033[0m" if sys.stdout.isatty() else text


def info(msg: str) -> None:
    print(_c("36", "[INFO]"), msg)


def ok(msg: str) -> None:
    print(_c("32", "[ OK ]"), msg)


def warn(msg: str) -> None:
    print(_c("33", "[WARN]"), msg)


def error(msg: str) -> None:
    print(_c("31", "[ERR ]"), msg, file=sys.stderr)


def sep() -> None:
    print(_c("2", "-" * 44))


def detect_platform() -> tuple[str, str]:
    """Return (os_tag, arch_tag) matching wchisp release asset names."""
    system = platform.system().lower()
    machine = platform.machine().lower()

    os_map = {
        "darwin": "macos",
        "linux": "linux",
        "windows": "win",
    }
    arch_map = {
        "x86_64": "x64",
        "amd64": "x64",
        "aarch64": "aarch64",
        "arm64": "arm64",
    }

    os_tag = os_map.get(system)
    arch_tag = arch_map.get(machine)

    if not os_tag:
        raise RuntimeError(f"Unsupported OS: {platform.system()}")
    if not arch_tag:
        raise RuntimeError(f"Unsupported architecture: {platform.machine()}")

    # wchisp uses arm64 for macOS Apple Silicon, and aarch64 for Linux ARM64.
    if os_tag == "macos" and arch_tag == "aarch64":
        arch_tag = "arm64"
    if os_tag == "linux" and arch_tag == "arm64":
        arch_tag = "aarch64"

    return os_tag, arch_tag


def request_url(url: str, *, method: str = "GET", timeout: int = 30):
    req = urllib.request.Request(
        apply_mirror(url),
        headers={"User-Agent": USER_AGENT},
        method=method,
    )
    return urllib.request.urlopen(req, timeout=timeout)


def apply_mirror(url: str) -> str:
    """Optionally route GitHub downloads through a user-provided mirror.

    Examples:
      BINARYKEYBOARD_GITHUB_MIRROR=https://gh-proxy.example/
      BINARYKEYBOARD_GITHUB_MIRROR=https://mirror.example/{url}
    """
    mirror = (
        os.environ.get("BINARYKEYBOARD_GITHUB_MIRROR")
        or os.environ.get("BK_GITHUB_MIRROR")
        or ""
    ).strip()
    if not mirror:
        return url
    if "{url}" in mirror:
        return mirror.replace("{url}", url)
    return mirror.rstrip("/") + "/" + url


def fetch_latest_tag_from_redirect() -> str:
    """Resolve /releases/latest without using api.github.com."""
    info(f"Resolving latest release from {LATEST_URL} ...")
    with request_url(LATEST_URL, method="HEAD", timeout=15) as resp:
        final_url = resp.geturl().rstrip("/")

    marker = "/releases/tag/"
    if marker not in final_url:
        raise RuntimeError(f"Unexpected latest release redirect: {final_url}")
    return final_url.rsplit("/", 1)[-1]


def release_asset_name(tag: str, os_tag: str, arch_tag: str) -> str:
    extension = ".zip" if os_tag == "win" else ".tar.gz"
    return f"wchisp-{tag}-{os_tag}-{arch_tag}{extension}"


def release_asset_url(tag: str, asset_name: str) -> str:
    return f"{GITHUB_BASE}/releases/download/{tag}/{asset_name}"


def fetch_latest_release_from_api() -> dict:
    info(f"Fetching latest release from api.github.com/repos/{REPO} ...")
    req = urllib.request.Request(API_URL, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.load(resp)


def find_asset(assets: list, os_tag: str, arch_tag: str) -> dict:
    pattern = f"{os_tag}-{arch_tag}"
    for asset in assets:
        if pattern in asset["name"]:
            return asset
    available = [a["name"] for a in assets]
    raise RuntimeError(
        f"No asset found for '{pattern}'.\nAvailable:\n"
        + "\n".join(f"  {name}" for name in available)
    )


def resolve_download(os_tag: str, arch_tag: str) -> tuple[str, str, str]:
    """Return (version, asset_name, url), preferring CDN-backed direct assets."""
    try:
        tag = fetch_latest_tag_from_redirect()
        asset_name = release_asset_name(tag, os_tag, arch_tag)
        url = release_asset_url(tag, asset_name)
        return tag, asset_name, url
    except Exception as exc:
        warn(f"Latest-release redirect failed, falling back to GitHub API: {exc}")

    release = fetch_latest_release_from_api()
    version = release["tag_name"]
    asset = find_asset(release["assets"], os_tag, arch_tag)
    return version, asset["name"], asset["browser_download_url"]


def temp_suffix(asset_name: str) -> str:
    if asset_name.endswith(".tar.gz"):
        return ".tar.gz"
    return Path(asset_name).suffix or ".download"


def download(url: str, dest: Path, asset_name: str) -> None:
    info(f"Downloading {asset_name} ...")
    with request_url(url, timeout=120) as resp, dest.open("wb") as fh:
        total = int(resp.headers.get("Content-Length", "0") or "0")
        done = 0
        while True:
            chunk = resp.read(1024 * 128)
            if not chunk:
                break
            fh.write(chunk)
            done += len(chunk)
            if total > 0:
                pct = min(done / total * 100, 100)
                filled = int(pct / 5)
                bar = "#" * filled + "-" * (20 - filled)
                print(f"\r  [{bar}] {pct:5.1f}%", end="", flush=True)
    print()


def extract_binary(archive: Path, dest_dir: Path) -> Path:
    """Extract the wchisp binary from an archive and return its path."""
    dest_dir.mkdir(parents=True, exist_ok=True)
    binary_name = "wchisp.exe" if platform.system() == "Windows" else "wchisp"

    with tempfile.TemporaryDirectory() as tmp:
        tmp_path = Path(tmp)

        if archive.name.endswith(".tar.gz"):
            with tarfile.open(archive, "r:gz") as tf:
                tf.extractall(tmp_path)
        elif archive.suffix == ".zip":
            with zipfile.ZipFile(archive) as zf:
                zf.extractall(tmp_path)
        else:
            raise RuntimeError(f"Unknown archive type: {archive.name}")

        found = list(tmp_path.rglob(binary_name))
        if not found:
            raise RuntimeError(f"{binary_name!r} not found in archive.")

        dest = dest_dir / binary_name
        shutil.copy2(found[0], dest)
        dest.chmod(dest.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
        return dest


def current_version(binary: Path) -> str | None:
    import subprocess

    try:
        result = subprocess.run(
            [str(binary), "--version"],
            capture_output=True,
            text=True,
            timeout=5,
            check=False,
        )
        return result.stdout.strip() or result.stderr.strip()
    except Exception:
        return None


def main() -> None:
    sep()
    print(_c("1", "  BinaryKeyboard Development Setup"))
    sep()

    binary_name = "wchisp.exe" if platform.system() == "Windows" else "wchisp"
    dest_binary = TOOLS_SCRIPTS / binary_name

    if dest_binary.exists() and "--force" not in sys.argv:
        ver = current_version(dest_binary)
        ok(f"wchisp already installed: {ver or 'unknown version'}")
        info(f"Location: {dest_binary}")
        info("Use --force to reinstall.")
        sep()
        print_next_steps()
        return

    try:
        os_tag, arch_tag = detect_platform()
    except RuntimeError as exc:
        error(str(exc))
        sys.exit(1)
    info(f"Platform: {platform.system()} / {platform.machine()} -> {os_tag}-{arch_tag}")

    try:
        version, asset_name, url = resolve_download(os_tag, arch_tag)
    except Exception as exc:
        error(f"Failed to resolve wchisp release asset: {exc}")
        sys.exit(1)

    info(f"Selected version: {version}")

    with tempfile.NamedTemporaryFile(suffix=temp_suffix(asset_name), delete=False) as tmp:
        tmp_path = Path(tmp.name)

    try:
        download(url, tmp_path, asset_name)
        info(f"Extracting to {TOOLS_SCRIPTS}/")
        binary = extract_binary(tmp_path, TOOLS_SCRIPTS)

        ver = current_version(binary)
        sep()
        ok(f"wchisp installed: {ver or version}")
        ok(f"Location: {binary}")
    except urllib.error.HTTPError as exc:
        error(f"Download failed: HTTP {exc.code} {exc.reason}")
        sys.exit(1)
    finally:
        tmp_path.unlink(missing_ok=True)

    sep()
    print_next_steps()


def print_next_steps() -> None:
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
