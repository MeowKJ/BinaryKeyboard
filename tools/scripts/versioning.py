#!/usr/bin/env python3
"""Component-scoped version helpers for BinaryKeyboard."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from datetime import datetime, timezone
from functools import lru_cache
from pathlib import Path
from typing import Any
import re
from urllib import error, request


SCRIPT_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = SCRIPT_DIR.parent.parent
VERSIONS_FILE = PROJECT_ROOT / "config" / "versions.json"

COMPONENT_ALIASES = {
    "studio": "studio",
    "meowisp": "meowisp",
    "ch552": "ch552",
    "ch552g": "ch552",
    "ch592": "ch592",
    "ch592f": "ch592",
}

CHIP_TO_COMPONENT = {
    "CH552G": "ch552",
    "CH592F": "ch592",
}

CH592_MODELS = ("5KEY", "KNOB")
CH552_MODELS = ("BASIC", "5KEY", "KNOB")
STUDIO_ASSET_RE = re.compile(
    r"^BinaryKeyboard(?:[ .-])Studio-(?P<version>\d+\.\d+\.\d+)-"
)
MEOWISP_ASSET_RE = re.compile(
    r"^MeowISP-(?:linux-(?:amd64|arm64)|macos-(?:apple-silicon|intel))-(?P<version>\d+\.\d+)-portable\.(?:tar\.gz|zip)$|^MeowISP-windows-amd64-(?P<win_version>\d+\.\d+)\.exe$"
)
CH592_RELEASE_ASSET_RE = re.compile(
    r"^CH592F-(?:BASIC|KNOB|5KEY)-(?P<version>\d+\.\d+\.\d+)(?:-(?:app|full|iap))?\.(?:bin|hex)$"
)
CH552_RELEASE_ASSET_RE = re.compile(
    r"^CH552G-(?:BASIC|KNOB|5KEY)-(?P<version>\d+\.\d+\.\d+)\.(?:bin|hex)$"
)


def _split_base_version(value: str) -> tuple[int, int]:
    parts = [int(part, 10) for part in value.strip().split(".")]
    if len(parts) != 2:
        raise ValueError(f"Expected base version 'x.y', got: {value}")
    return parts[0], parts[1]


def _version_parts(component: str) -> int:
    meta = component_meta(component)
    return int(meta.get("version_parts", 3))


def _git_output(args: list[str]) -> str:
    try:
        result = subprocess.run(
            args,
            capture_output=True,
            text=True,
            check=True,
            cwd=PROJECT_ROOT,
        )
        return result.stdout.strip()
    except subprocess.SubprocessError as exc:
        print(f"WARNING: git command failed: {' '.join(args)}: {exc}", file=sys.stderr)
        return ""


def _git_commit_count_for_paths(paths: list[str]) -> int:
    if not paths:
        return 0
    output = _git_output(["git", "rev-list", "--count", "HEAD", "--", *paths])
    try:
        return int(output)
    except ValueError:
        return 0


def _git_paths_changed_since(ref: str, paths: list[str]) -> bool:
    if not ref or not paths:
        return False
    try:
        result = subprocess.run(
            ["git", "diff", "--quiet", f"{ref}..HEAD", "--", *paths],
            cwd=PROJECT_ROOT,
            check=False,
        )
    except subprocess.SubprocessError as exc:
        print(f"WARNING: git diff failed for {ref}: {exc}", file=sys.stderr)
        return False
    if result.returncode == 0:
        return False
    if result.returncode == 1:
        return True
    print(f"WARNING: git diff exited with {result.returncode} for {ref}", file=sys.stderr)
    return False


def _git_short_sha() -> str:
    sha = _git_output(["git", "rev-parse", "--short", "HEAD"])
    return sha or "unknown"


def _github_headers() -> dict[str, str]:
    headers = {
        "Accept": "application/vnd.github+json",
        "User-Agent": "BinaryKeyboard-Versioning",
        "X-GitHub-Api-Version": "2022-11-28",
    }
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    return headers


def _fetch_json(url: str) -> dict[str, Any] | None:
    req = request.Request(url, headers=_github_headers())
    try:
        with request.urlopen(req, timeout=15) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except error.HTTPError as exc:
        print(f"WARNING: request failed {url}: HTTP {exc.code}", file=sys.stderr)
        return None
    except Exception as exc:
        print(f"WARNING: request failed {url}: {exc}", file=sys.stderr)
        return None


def _split_version(value: str) -> tuple[int, int, int]:
    parts = [int(part, 10) for part in value.strip().split(".")]
    if len(parts) != 3:
        raise ValueError(f"Expected version 'x.y.z', got: {value}")
    return parts[0], parts[1], parts[2]


@lru_cache(maxsize=1)
def _latest_release_payload() -> dict[str, Any] | None:
    settings = release_settings()
    repo = settings["repository"]
    return _fetch_json(f"https://api.github.com/repos/{repo}/releases/latest")


def _version_from_release_assets(component: str, assets: list[dict[str, Any]]) -> str | None:
    versions_found: set[str] = set()
    matcher = {
        "studio": STUDIO_ASSET_RE,
        "meowisp": MEOWISP_ASSET_RE,
        "ch592": CH592_RELEASE_ASSET_RE,
        "ch552": CH552_RELEASE_ASSET_RE,
    }[component]

    for asset in assets:
        name = str(asset.get("name", ""))
        match = matcher.match(name)
        if match:
            versions_found.add(match.group("version") or match.group("win_version"))

    if len(versions_found) == 1:
        return next(iter(versions_found))
    return None


@lru_cache(maxsize=1)
def _latest_release_versions() -> tuple[str, dict[str, str]] | None:
    payload = _latest_release_payload()
    if not payload:
        return None

    tag = str(payload.get("tag_name") or "").strip()
    assets = payload.get("assets") or []
    versions: dict[str, str] = {}

    for component in ("studio", "meowisp", "ch552", "ch592"):
        version = _version_from_release_assets(component, assets)
        if version:
            versions[component] = version

    if not tag or not versions:
        return None
    return tag, versions


def _latest_release_patch_for_base(versions: dict[str, str], major: int, minor: int) -> int | None:
    patches: list[int] = []
    for version in versions.values():
        try:
            rel_major, rel_minor, rel_patch = _split_version(version)
        except ValueError:
            continue
        if rel_major == major and rel_minor == minor:
            patches.append(rel_patch)
    if not patches:
        return None
    return max(patches)


def load_versions() -> dict[str, Any]:
    return json.loads(VERSIONS_FILE.read_text(encoding="utf-8"))


def _normalize_component(component: str) -> str:
    key = component.strip().lower()
    try:
        return COMPONENT_ALIASES[key]
    except KeyError as exc:
        raise KeyError(f"Unknown component: {component}") from exc


def component_meta(component: str) -> dict[str, Any]:
    component_key = _normalize_component(component)
    data = load_versions()
    try:
        return data["components"][component_key]
    except KeyError as exc:
        raise KeyError(f"Missing component metadata: {component}") from exc


def component_changed(component: str) -> bool:
    paths = list(component_meta(component).get("paths", []))
    latest_release = _latest_release_versions()
    if latest_release:
        release_tag, _released_versions = latest_release
        return _git_paths_changed_since(release_tag, paths)
    return _git_commit_count_for_paths(paths) > 0


def _is_local_build() -> bool:
    """True when running outside GitHub Actions (local dev environment)."""
    return not os.environ.get("CI")


def component_version(component: str, build_number: int | None = None) -> str:
    meta = component_meta(component)
    major, minor = _split_base_version(str(meta["base_version"]))
    version_parts = int(meta.get("version_parts", 3))
    if version_parts == 2:
        if _is_local_build():
            return "dev"
        return f"{major}.{minor}"

    if build_number is not None:
        return f"{major}.{minor}.{build_number}"

    # Explicit patch from environment (set by build tools for deterministic CI builds)
    env_number = os.environ.get("BK_BUILD_NUMBER")
    if env_number is not None:
        try:
            return f"{major}.{minor}.{int(env_number)}"
        except ValueError:
            pass

    if _is_local_build():
        return "dev"

    paths = list(meta.get("paths", []))
    latest_release = _latest_release_versions()
    if latest_release:
        release_tag, released_versions = latest_release
        released_version = released_versions.get(_normalize_component(component))
        if released_version:
            rel_major, rel_minor, rel_patch = _split_version(released_version)
            if rel_major == major and rel_minor == minor:
                patch = rel_patch + 1 if _git_paths_changed_since(release_tag, paths) else rel_patch
                return f"{major}.{minor}.{patch}"

        fallback_patch = _latest_release_patch_for_base(released_versions, major, minor)
        if fallback_patch is not None:
            patch = fallback_patch + 1 if _git_paths_changed_since(release_tag, paths) else fallback_patch
            return f"{major}.{minor}.{patch}"

    patch = _git_commit_count_for_paths(paths)
    return f"{major}.{minor}.{patch}"


def studio_version(build_number: int | None = None) -> str:
    return component_version("studio", build_number)


def meowisp_version(build_number: int | None = None) -> str:
    return component_version("meowisp", build_number)


def firmware_meta(chip: str) -> dict[str, Any]:
    chip_key = chip.strip().upper()
    try:
        component_key = CHIP_TO_COMPONENT[chip_key]
    except KeyError as exc:
        raise KeyError(f"Unknown firmware chip family: {chip}") from exc
    meta = dict(component_meta(component_key))
    meta["component"] = component_key
    meta["chip_family"] = meta["chip_family"].upper()
    return meta


def firmware_version(chip: str, build_number: int | None = None) -> str:
    chip_key = chip.strip().upper()
    try:
        component_key = CHIP_TO_COMPONENT[chip_key]
    except KeyError as exc:
        raise KeyError(f"Unknown firmware chip family: {chip}") from exc
    return component_version(component_key, build_number)


def protocol_family(chip: str) -> str:
    return str(firmware_meta(chip)["protocol_family"])


def current_component_versions(build_number: int | None = None) -> dict[str, str]:
    return {
        "studio": studio_version(build_number),
        "meowisp": meowisp_version(build_number),
        "ch552": component_version("ch552", build_number),
        "ch592": component_version("ch592", build_number),
    }


def release_settings() -> dict[str, str]:
    data = load_versions()
    return {
        "repository": str(data["release"]["repository"]),
        "manifest_url": str(data["release"]["manifest_url"]),
    }


def pages_base_url() -> str:
    manifest_url = release_settings()["manifest_url"].rstrip("/")
    suffix = "/api/release-manifest.json"
    if manifest_url.endswith(suffix):
        return manifest_url[: -len(suffix)]
    return manifest_url.rsplit("/", 1)[0]


def release_artifact_manifest(build_number: int | None = None) -> tuple[dict[str, Any], dict[str, str]]:
    versions = current_component_versions(build_number)
    base_url = pages_base_url().rstrip("/")

    def ch592_asset(model: str) -> dict[str, str]:
        version = versions["ch592"]
        return {
            "channel": "release",
            "version": version,
            "appBinUrl": f"{base_url}/firmware/ch592f/CH592F-{model}-{version}-app.bin",
            "fullHexUrl": f"{base_url}/firmware/ch592f/CH592F-{model}-{version}-full.hex",
        }

    def ch552_asset(model: str) -> dict[str, str]:
        version = versions["ch552"]
        return {
            "channel": "release",
            "version": version,
            "hexUrl": f"{base_url}/firmware/ch552g/CH552G-{model}-{version}.hex",
        }

    return ({
        "ch592": {model: ch592_asset(model) for model in CH592_MODELS},
        "ch552": {model: ch552_asset(model) for model in CH552_MODELS},
    }, versions)


def release_manifest_payload(build_number: int | None = None) -> dict[str, Any]:
    artifacts, versions = release_artifact_manifest(build_number)
    settings = release_settings()
    return {
        "generatedAt": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "commit": _git_short_sha(),
        "repository": settings["repository"],
        "versions": versions,
        "artifacts": artifacts,
    }


def emit_c_header(chip: str, out: Path, build_number: int | None = None) -> None:
    chip_key = chip.strip().upper()
    fw_ver = firmware_version(chip_key, build_number)
    is_dev = fw_ver == "dev"
    fw_major, fw_minor, fw_patch = (0, 0, 0) if is_dev else [int(p) for p in fw_ver.split(".")]
    channel_value = "BK_CHANNEL_DEV" if is_dev else "BK_CHANNEL_RELEASE"

    lines = [
        "/* Auto-generated by tools/scripts/versioning.py. Do not edit manually. */",
        "#ifndef BK_VERSION_CONFIG_H",
        "#define BK_VERSION_CONFIG_H",
        "",
        f"#define BK_CHIP_FAMILY \"{chip_key}\"",
        "",
        "/* Build channel */",
        "#define BK_CHANNEL_DEV      0u",
        "#define BK_CHANNEL_RELEASE  1u",
        f"#define BK_BUILD_CHANNEL    {channel_value}",
        "",
        "/* Firmware version (dev build: 0.0.0) */",
        f"#define BK_FIRMWARE_VERSION_MAJOR {fw_major}",
        f"#define BK_FIRMWARE_VERSION_MINOR {fw_minor}",
        f"#define BK_FIRMWARE_VERSION_PATCH {fw_patch}",
        "",
        "#endif",
        "",
    ]

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines), encoding="utf-8")


def emit_ts_module(out: Path, build_number: int | None = None) -> None:
    versions = current_component_versions(build_number)
    settings = release_settings()

    def firmware_meta_block(component_key: str) -> str:
        meta = component_meta(component_key)
        version = versions[component_key]
        is_dev = version == "dev"
        channel = "dev" if is_dev else "release"
        major, minor, patch = (0, 0, 0) if is_dev else [int(p) for p in version.split(".")]
        chip_key = str(meta["chip_family"]).upper()
        return (
            f"  {chip_key}: {{\n"
            f"    chipFamily: '{chip_key}',\n"
            f"    protocolFamily: '{meta['protocol_family']}',\n"
            f"    channel: '{channel}',\n"
            f"    firmwareVersion: {{ major: {major}, minor: {minor}, patch: {patch} }},\n"
            f"  }},"
        )

    local_manifest = {
        **release_manifest_payload(build_number),
        "manifestUrl": settings["manifest_url"],
    }

    lines = [
        "/* Auto-generated by tools/scripts/versioning.py. Do not edit manually. */",
        "export const FIRMWARE_VERSION_META = {",
        firmware_meta_block("ch592"),
        firmware_meta_block("ch552"),
        "} as const;",
        "",
        "export const RELEASE_FEED = " + json.dumps({
            "repository": settings["repository"],
            "manifestUrl": settings["manifest_url"],
        }, ensure_ascii=True) + " as const;",
        "",
        "export const LOCAL_RELEASE_MANIFEST = " + json.dumps(local_manifest, ensure_ascii=True) + " as const;",
        "",
        "export type FirmwareChipFamily = keyof typeof FIRMWARE_VERSION_META;",
        "",
    ]

    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text("\n".join(lines) + "\n", encoding="utf-8")


def emit_release_manifest(out: Path, build_number: int | None = None) -> None:
    payload = release_manifest_payload(build_number)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2, ensure_ascii=True) + "\n", encoding="utf-8")


# =============================================================================
# Naming helpers (formerly firmware_naming.py)
# =============================================================================

def ch592_version(build_number: int | None = None) -> str:
    return firmware_version("CH592F", build_number)


def ch552_version(build_number: int | None = None) -> str:
    return firmware_version("CH552G", build_number)


def studio_component_version(build_number: int | None = None) -> str:
    return studio_version(build_number)


def meowisp_component_version(build_number: int | None = None) -> str:
    return meowisp_version(build_number)


def normalize_keyboard_name(keyboard: str) -> str:
    value = keyboard.strip().upper()
    aliases = {
        "5KEYS": "5KEY",
    }
    value = aliases.get(value, value)
    if value not in {"BASIC", "KNOB", "5KEY"}:
        raise ValueError(f"Unsupported keyboard: {keyboard}")
    return value


def ch592_model_from_keyboard(keyboard: str) -> str:
    keyboard_upper = normalize_keyboard_name(keyboard)
    if keyboard_upper == "BASIC":
        raise ValueError("CH592F does not support BASIC keyboard")
    return keyboard_upper


def ch552_model_from_keyboard(keyboard: str) -> str:
    return normalize_keyboard_name(keyboard)


def firmware_stem(chip: str, model: str, version: str, suffix: str | None = None) -> str:
    stem = f"{chip.upper()}-{model.upper()}-{version}"
    if suffix:
        stem = f"{stem}-{suffix.lstrip('-')}"
    return stem


def firmware_filename(chip: str, model: str, version: str, ext: str, suffix: str | None = None) -> str:
    return f"{firmware_stem(chip, model, version, suffix)}.{ext.lstrip('.')}"


def ch592_base_stem_for_keyboard(keyboard: str) -> str:
    return firmware_stem("CH592F", ch592_model_from_keyboard(keyboard), ch592_version())


def ch592_filename_for_keyboard(keyboard: str, ext: str) -> str:
    return firmware_filename("CH592F", ch592_model_from_keyboard(keyboard), ch592_version(), ext, suffix="app")


def ch592_full_filenames_for_keyboard(keyboard: str, ext: str) -> tuple[str, str]:
    stem = ch592_base_stem_for_keyboard(keyboard)
    suffix = f".{ext.lstrip('.')}"
    return (f"{stem}-full{suffix}", f"{stem}.full{suffix}")


def ch592_iap_filename_for_keyboard(keyboard: str, ext: str) -> str:
    return firmware_filename("CH592F", ch592_model_from_keyboard(keyboard), ch592_version(), ext, suffix="iap")


def ch552_filename_for_keyboard(keyboard: str, ext: str) -> str:
    return firmware_filename("CH552G", ch552_model_from_keyboard(keyboard), ch552_version(), ext)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="tools/scripts/versioning.py")
    sub = parser.add_subparsers(dest="command", required=True)

    p_show = sub.add_parser("show", help="Print component version metadata")
    p_show.add_argument("--component", choices=("studio", "meowisp", "ch552", "ch592", "CH552G", "CH592F"))
    p_show.add_argument("--build-number", type=int, default=None,
                        help="Override patch number instead of git history")

    p_changed = sub.add_parser("changed", help="Check whether a component changed since the latest release")
    p_changed.add_argument("--component", required=True, choices=("studio", "meowisp", "ch552", "ch592", "CH552G", "CH592F"))

    p_emit = sub.add_parser("emit-c-header", help="Generate a C header for a chip family")
    p_emit.add_argument("--chip", required=True, choices=("CH592F", "CH552G"))
    p_emit.add_argument("--out", required=True)
    p_emit.add_argument("--build-number", type=int, default=None,
                        help="Override patch number instead of git history")

    p_emit_ts = sub.add_parser("emit-ts", help="Generate a TypeScript version module")
    p_emit_ts.add_argument("--out", required=True)
    p_emit_ts.add_argument("--build-number", type=int, default=None,
                           help="Override patch number instead of git history")

    p_emit_manifest = sub.add_parser("emit-release-manifest", help="Generate the release manifest JSON")
    p_emit_manifest.add_argument("--out", required=True)
    p_emit_manifest.add_argument("--build-number", type=int, default=None,
                                 help="Override patch number instead of git history")

    return parser


def main() -> int:
    args = build_parser().parse_args()

    if args.command == "show":
        build_number = getattr(args, "build_number", None)
        if args.component:
            key = args.component
            if key.upper() in CHIP_TO_COMPONENT:
                key = CHIP_TO_COMPONENT[key.upper()]
            meta = dict(component_meta(key))
            meta["version"] = component_version(key, build_number)
            print(json.dumps(meta, indent=2, ensure_ascii=True))
        else:
            data = load_versions()
            for component_key in data.get("components", {}):
                data["components"][component_key]["version"] = component_version(component_key, build_number)
            data["release_manifest"] = release_manifest_payload(build_number)
            print(json.dumps(data, indent=2, ensure_ascii=True))
        return 0

    if args.command == "changed":
        key = args.component
        if key.upper() in CHIP_TO_COMPONENT:
            key = CHIP_TO_COMPONENT[key.upper()]
        print("true" if component_changed(key) else "false")
        return 0

    if args.command == "emit-c-header":
        emit_c_header(args.chip, Path(args.out), args.build_number)
        return 0

    if args.command == "emit-ts":
        emit_ts_module(Path(args.out), args.build_number)
        return 0

    if args.command == "emit-release-manifest":
        emit_release_manifest(Path(args.out), args.build_number)
        return 0

    raise SystemExit(f"Unsupported command: {args.command}")


if __name__ == "__main__":
    raise SystemExit(main())
