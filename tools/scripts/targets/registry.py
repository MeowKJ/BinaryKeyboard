from __future__ import annotations

from targets.ch552 import CH552_PROFILE
from targets.ch592 import CH592_PROFILE


TARGET_PROFILES = {
    CH592_PROFILE.key: CH592_PROFILE,
    CH552_PROFILE.key: CH552_PROFILE,
}

TARGET_ORDER = tuple(TARGET_PROFILES.keys())


def get_target_profile(key: str):
    return TARGET_PROFILES.get(key, CH592_PROFILE)
