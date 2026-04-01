#!/usr/bin/env python3
"""Compatibility wrapper for the CH592F build helper."""

from targets.ch592.build import *  # noqa: F401,F403
from targets.ch592.build import main


if __name__ == "__main__":
    raise SystemExit(main())
