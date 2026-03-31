#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
from pathlib import Path
from urllib import request


def github_request(url: str, token: str | None):
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "MeowISP/0.1",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    req = request.Request(url, headers=headers)
    return request.urlopen(req)


def download_file(url: str, out_path: Path, token: str | None) -> None:
    headers = {
        "Accept": "application/octet-stream",
        "User-Agent": "MeowISP/0.1",
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    req = request.Request(url, headers=headers)
    with request.urlopen(req) as resp:
        out_path.parent.mkdir(parents=True, exist_ok=True)
        with out_path.open("wb") as fh:
            shutil.copyfileobj(resp, fh)


def main() -> int:
    repo_root = Path(__file__).resolve().parents[3]
    default_out = repo_root / "tools" / "meowisp" / ".cache" / "windows-assets" / "CH375DLL64.dll"

    parser = argparse.ArgumentParser(description="Fetch CH375DLL64.dll from a GitHub release asset")
    parser.add_argument("--repo", default="MeowKJ/BinaryKeyboard")
    parser.add_argument("--tag", default="toolchain-linux")
    parser.add_argument("--asset", default="CH375DLL64.dll")
    parser.add_argument("--out", default=str(default_out))
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    out_path = Path(args.out).resolve()

    api_url = f"https://api.github.com/repos/{args.repo}/releases/tags/{args.tag}"
    with github_request(api_url, token) as resp:
        release = json.load(resp)

    assets = release.get("assets", [])
    asset = next((item for item in assets if item.get("name") == args.asset), None)
    if asset is None:
        names = ", ".join(sorted(item.get("name", "") for item in assets))
        raise SystemExit(
            f"asset not found in {args.repo}@{args.tag}: {args.asset}\n"
            f"available assets: {names}"
        )

    download_url = asset["browser_download_url"]
    download_file(download_url, out_path, token)
    print(out_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
