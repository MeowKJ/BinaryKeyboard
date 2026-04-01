#!/usr/bin/env python3
"""Compatibility wrapper for the CH552G build helper."""

from targets.ch552.build import *  # noqa: F401,F403
from targets.ch552.build import main


if __name__ == "__main__":
    raise SystemExit(main())
