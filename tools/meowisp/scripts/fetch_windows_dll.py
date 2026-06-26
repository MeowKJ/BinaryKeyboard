#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import shutil
from pathlib import Path
from urllib import request

USER_AGENT = "MeowISP/0.1"


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


def direct_release_asset_url(repo: str, tag: str, asset: str) -> str:
    return f"https://github.com/{repo}/releases/download/{tag}/{asset}"


def download_file(url: str, out_path: Path) -> None:
    req = request.Request(
        apply_mirror(url),
        headers={
            "Accept": "application/octet-stream",
            "User-Agent": USER_AGENT,
        },
    )
    with request.urlopen(req, timeout=60) as resp:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with out_path.open("wb") as fh:
            shutil.copyfileobj(resp, fh)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[3]
    default_out = repo_root / "tools" / "meowisp" / ".cache" / "windows-assets" / "CH375DLL64.dll"

    parser = argparse.ArgumentParser(description="Fetch CH375DLL64.dll from a release asset")
    parser.add_argument("--repo", default="MeowKJ/BinaryKeyboard")
    parser.add_argument("--tag", default="toolchain-linux")
    parser.add_argument("--asset", default="CH375DLL64.dll")
    parser.add_argument("--url", default=os.environ.get("BINARYKEYBOARD_CH375DLL_URL", ""))
    parser.add_argument("--out", default=str(default_out))
    args = parser.parse_args()

    out_path = Path(args.out).resolve()
    download_url = args.url or direct_release_asset_url(args.repo, args.tag, args.asset)
    download_file(download_url, out_path)
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
