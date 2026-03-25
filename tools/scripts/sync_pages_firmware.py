#!/usr/bin/env python3
"""Sync latest firmware assets from GitHub Release into docs/public for Pages."""

from __future__ import annotations

import json
import re
import shutil
import tempfile
import time
import sys
import urllib.request
from urllib.error import URLError
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
VERSIONS_FILE = PROJECT_ROOT / "config" / "versions.json"

ASSET_RE = re.compile(
    r"^(?P<chip>CH592F|CH552G)-(?P<model>BASIC|KNOB|5KEY)-(?P<version>\d+\.\d+\.\d+)(?P<suffix>-full)?\.(?P<ext>bin|hex)$"
)


def release_repository() -> str:
    payload = json.loads(VERSIONS_FILE.read_text(encoding="utf-8"))
    return str(payload["release"]["repository"])


def fetch_latest_release(repo: str) -> dict:
    req = urllib.request.Request(
        f"https://api.github.com/repos/{repo}/releases/latest",
        headers={"User-Agent": "BinaryKeyboard Pages Sync"},
    )
    with urllib.request.urlopen(req, timeout=30) as resp:
        return json.load(resp)


def download(url: str, dst: Path) -> None:
    req = urllib.request.Request(url, headers={"User-Agent": "BinaryKeyboard Pages Sync"})
    last_error: Exception | None = None
    for attempt in range(1, 4):
        try:
            with urllib.request.urlopen(req, timeout=60) as resp:
                dst.write_bytes(resp.read())
            return
        except URLError as exc:
            last_error = exc
        except OSError as exc:
            last_error = exc
        if attempt < 3:
            time.sleep(attempt)
    if last_error is not None:
        raise last_error


def sync(out_dir: Path) -> int:
    repo = release_repository()
    release = fetch_latest_release(repo)
    assets = release.get("assets", [])

    with tempfile.TemporaryDirectory(prefix="pages-firmware-", dir=out_dir.parent if out_dir.parent.exists() else None) as tmp_root:
        staging_dir = Path(tmp_root) / "firmware"
        staging_dir.mkdir(parents=True, exist_ok=True)

        synced = 0
        for asset in assets:
            name = asset.get("name", "")
            match = ASSET_RE.match(name)
            if not match:
                continue

            chip_dir = staging_dir / match.group("chip").lower()
            chip_dir.mkdir(parents=True, exist_ok=True)
            dst = chip_dir / name
            download(str(asset["browser_download_url"]), dst)
            synced += 1
            print(f"[sync] {name} -> {dst}")

        if synced == 0:
            print("No firmware assets matched latest release.", file=sys.stderr)
            return 1

        if out_dir.exists():
            shutil.rmtree(out_dir)
        shutil.move(str(staging_dir), str(out_dir))

    print(f"Synced {synced} firmware assets from {release.get('tag_name', 'latest')}")
    return 0


def main() -> int:
    out_dir = PROJECT_ROOT / "docs" / "public" / "firmware"
    if len(sys.argv) > 1:
        out_dir = Path(sys.argv[1]).resolve()
    return sync(out_dir)


if __name__ == "__main__":
    raise SystemExit(main())
