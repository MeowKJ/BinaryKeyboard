#!/usr/bin/env python3
"""Persistent tool path cache shared by BinaryKeyboard scripts."""

from __future__ import annotations

import json
import os
import shutil
from pathlib import Path
from typing import Iterable, Optional


SCRIPT_DIR = Path(__file__).resolve().parent
STATE_FILE = SCRIPT_DIR / ".binarykeyboard_console_state.json"
TOOL_CACHE_KEY = "tool_cache"


def _read_state_data() -> dict:
    if not STATE_FILE.is_file():
        return {}
    try:
        data = json.loads(STATE_FILE.read_text())
    except Exception:
        return {}
    return data if isinstance(data, dict) else {}


def _write_state_data(data: dict) -> None:
    STATE_FILE.write_text(json.dumps(data, indent=2, ensure_ascii=True) + "\n")


def _read_tool_cache() -> dict[str, str]:
    data = _read_state_data()
    cache = data.get(TOOL_CACHE_KEY)
    return dict(cache) if isinstance(cache, dict) else {}


def _write_tool_cache(cache: dict[str, str]) -> None:
    data = _read_state_data()
    data[TOOL_CACHE_KEY] = cache
    _write_state_data(data)


def read_tool_cache() -> dict[str, str]:
    return _read_tool_cache()


def get_cached_tool_path(cache_key: str) -> Optional[Path]:
    cached = _read_tool_cache().get(cache_key, "")
    if not cached:
        return None
    candidate = Path(cached)
    if candidate.exists():
        return candidate.resolve()
    _update_cached_tool(cache_key, None)
    return None


def _normalize_path(value: Path | str, binary_name: str) -> Optional[Path]:
    candidate = Path(value)
    if candidate.is_file():
        return candidate.resolve()
    if candidate.is_dir():
        binary = candidate / binary_name
        if binary.is_file():
            return binary.resolve()
    return None


def _update_cached_tool(cache_key: str, path: Optional[Path]) -> None:
    cache = _read_tool_cache()
    if path is None:
        if cache.pop(cache_key, None) is not None:
            _write_tool_cache(cache)
        return
    normalized = str(path.resolve())
    if cache.get(cache_key) != normalized:
        cache[cache_key] = normalized
        _write_tool_cache(cache)


def resolve_tool_path(
    cache_key: str,
    binary_name: str,
    *,
    env_name: str | None = None,
    preferred_candidates: Iterable[Path] = (),
    candidates: Iterable[Path] = (),
) -> Optional[Path]:
    env_path = os.environ.get(env_name, "") if env_name else ""
    if env_path:
        resolved = _normalize_path(env_path, binary_name)
        if resolved:
            _update_cached_tool(cache_key, resolved)
            return resolved

    cached = _read_tool_cache().get(cache_key, "")
    if cached:
        resolved = _normalize_path(cached, binary_name)
        if resolved:
            return resolved
        _update_cached_tool(cache_key, None)

    for candidate in preferred_candidates:
        resolved = _normalize_path(candidate, binary_name)
        if resolved:
            _update_cached_tool(cache_key, resolved)
            return resolved

    found = shutil.which(binary_name)
    if found:
        resolved = Path(found).resolve()
        _update_cached_tool(cache_key, resolved)
        return resolved

    for candidate in candidates:
        resolved = _normalize_path(candidate, binary_name)
        if resolved:
            _update_cached_tool(cache_key, resolved)
            return resolved

    return None
