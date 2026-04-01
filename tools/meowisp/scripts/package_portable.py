#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import plistlib
import shutil
import tarfile
import tempfile
import zipfile
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[3]
MEOWISP_ROOT = REPO_ROOT / "tools" / "meowisp"
UDEV_RULES = MEOWISP_ROOT / "assets" / "50-wchisp.rules"


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def common_readme(platform_name: str, arch: str) -> str:
    return f"""MeowISP portable package

Platform: {platform_name}
Architecture: {arch}

Run:
- GUI: launch the bundled MeowISP executable
- Doctor: run `meowisp --doctor`
- Probe: run `meowisp --probe`

MeowISP reads online firmware directly from GitHub Releases.
"""


def linux_readme(arch: str) -> str:
    return common_readme("Linux", arch) + """
Linux note:
- `50-wchisp.rules` is bundled in this package.
- Install it with:
  sudo cp 50-wchisp.rules /etc/udev/rules.d/50-wchisp.rules
  sudo udevadm control --reload-rules
  sudo udevadm trigger
"""


def macos_readme(arch: str) -> str:
    return common_readme("macOS", arch) + """
macOS note:
- Open `MeowISP.app` directly, or run the binary inside `MeowISP.app/Contents/MacOS/`.
- If Gatekeeper warns on first launch, right click the app and choose Open.
"""


def create_zip(source_dir: Path, archive_base: Path) -> Path:
    archive_path = archive_base.with_suffix(".zip")
    with zipfile.ZipFile(archive_path, "w", compression=zipfile.ZIP_DEFLATED) as zf:
        for item in sorted(source_dir.rglob("*")):
            zf.write(item, item.relative_to(source_dir.parent))
    return archive_path


def create_targz(source_dir: Path, archive_base: Path) -> Path:
    archive_path = archive_base.with_suffix(".tar.gz")
    with tarfile.open(archive_path, "w:gz") as tf:
        tf.add(source_dir, arcname=source_dir.name)
    return archive_path


def package_linux(binary: Path, out_dir: Path, arch: str, version: str) -> Path:
    package_name = f"MeowISP-linux-{arch}-{version}-portable"
    with tempfile.TemporaryDirectory(prefix="meowisp-linux-") as temp_dir:
        root = Path(temp_dir) / package_name
        root.mkdir(parents=True, exist_ok=True)
        target_binary = root / "meowisp"
        shutil.copy2(binary, target_binary)
        target_binary.chmod(target_binary.stat().st_mode | 0o111)
        shutil.copy2(UDEV_RULES, root / "50-wchisp.rules")
        write_text(root / "README.txt", linux_readme(arch))
        return create_targz(root, out_dir / package_name)


def package_windows(binary: Path, out_dir: Path, arch: str, version: str) -> Path:
    target = out_dir / f"MeowISP-windows-{arch}-{version}.exe"
    shutil.copy2(binary, target)
    return target


def package_macos(binary: Path, out_dir: Path, arch: str, version: str) -> Path:
    package_name = f"MeowISP-macos-{arch}-{version}-portable"
    with tempfile.TemporaryDirectory(prefix="meowisp-macos-") as temp_dir:
        root = Path(temp_dir) / package_name
        app_dir = root / "MeowISP.app" / "Contents"
        macos_dir = app_dir / "MacOS"
        macos_dir.mkdir(parents=True, exist_ok=True)
        target_binary = macos_dir / "MeowISP"
        shutil.copy2(binary, target_binary)
        target_binary.chmod(target_binary.stat().st_mode | 0o111)
        plist = {
            "CFBundleName": "MeowISP",
            "CFBundleDisplayName": "MeowISP",
            "CFBundleExecutable": "MeowISP",
            "CFBundleIdentifier": "io.binarykeyboard.meowisp",
            "CFBundlePackageType": "APPL",
            "CFBundleShortVersionString": version,
            "CFBundleVersion": os.environ.get("GITHUB_SHA", "dev")[:7] or "dev",
            "LSMinimumSystemVersion": "12.0",
            "NSHighResolutionCapable": True,
        }
        with (app_dir / "Info.plist").open("wb") as fp:
            plistlib.dump(plist, fp)
        write_text(root / "README.txt", macos_readme(arch))
        return create_zip(root, out_dir / package_name)


def main() -> int:
    parser = argparse.ArgumentParser(description="Package MeowISP portable app archives")
    parser.add_argument("--platform", required=True, choices=("linux", "macos", "windows"))
    parser.add_argument("--arch", required=True)
    parser.add_argument("--version", required=True)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--out-dir", required=True)
    args = parser.parse_args()

    binary = Path(args.binary).resolve()
    out_dir = Path(args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    if not binary.is_file():
        raise SystemExit(f"binary not found: {binary}")

    if args.platform == "linux":
        archive = package_linux(binary, out_dir, args.arch, args.version)
    elif args.platform == "macos":
        archive = package_macos(binary, out_dir, args.arch, args.version)
    else:
        archive = package_windows(binary, out_dir, args.arch, args.version)

    print(archive)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
