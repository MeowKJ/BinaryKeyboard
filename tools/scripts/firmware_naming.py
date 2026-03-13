#!/usr/bin/env python3
"""Shared firmware naming helpers."""

from __future__ import annotations

from versioning import firmware_version


def ch592_version() -> str:
    return firmware_version("CH592F")


def ch552_version() -> str:
    return firmware_version("CH552G")


def normalize_keyboard_name(keyboard: str) -> str:
    value = keyboard.strip().upper()
    aliases = {
        "5KEYS": "5KEY",
    }
    value = aliases.get(value, value)
    if value not in {"BASIC", "KNOB", "5KEY"}:
        raise ValueError(f"Unsupported keyboard: {keyboard}")
    return value


def ch592_model_from_preset(preset: str) -> str:
    return "KNOB" if "knob" in preset.lower() else "5KEY"


def ch592_model_from_keyboard(keyboard: str) -> str:
    keyboard_upper = normalize_keyboard_name(keyboard)
    if keyboard_upper == "BASIC":
        raise ValueError("CH592F does not support BASIC keyboard")
    return keyboard_upper


def ch552_model_from_keyboard(keyboard: str) -> str:
    return normalize_keyboard_name(keyboard)


def firmware_filename(chip: str, model: str, version: str, ext: str) -> str:
    return f"{chip.upper()}-{model.upper()}-{version}.{ext.lstrip('.')}"


def ch592_filename_for_preset(preset: str, ext: str) -> str:
    return firmware_filename("CH592F", ch592_model_from_preset(preset), ch592_version(), ext)


def ch592_filename_for_keyboard(keyboard: str, ext: str) -> str:
    return firmware_filename("CH592F", ch592_model_from_keyboard(keyboard), ch592_version(), ext)


def ch552_filename_for_keyboard(keyboard: str, ext: str) -> str:
    return firmware_filename("CH552G", ch552_model_from_keyboard(keyboard), ch552_version(), ext)


def ch552_model_from_variant(variant: str) -> str:
    return ch552_model_from_keyboard(variant)


def ch552_filename_for_variant(variant: str, ext: str) -> str:
    return ch552_filename_for_keyboard(variant, ext)
