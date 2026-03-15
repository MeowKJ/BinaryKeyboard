#!/usr/bin/env python3
"""Render firmware size JSON reports as a markdown table for release notes."""

import json
import sys
from pathlib import Path


def fmt(region: dict) -> str:
    if not region:
        return "-"
    used = region["used"]
    total = region["total"]
    u_kb = used / 1024
    t_kb = total / 1024
    pct = used / total * 100 if total else 0
    return f"{u_kb:.1f} / {t_kb:.1f} KB ({pct:.1f}%)"


def main() -> None:
    files = [Path(f) for f in sys.argv[1:] if Path(f).is_file()]
    if not files:
        return

    reports = [json.loads(f.read_text()) for f in files]
    reports.sort(key=lambda r: (r["chip"], r["keyboard"]))

    print("### 📊 Flash / RAM 占用")
    print()
    print("| 固件 | Flash | RAM |")
    print("|------|-------|-----|")
    for r in reports:
        name = f'{r["chip"]}-{r["keyboard"]}'
        flash = r["regions"].get("FLASH", {})
        ram = r["regions"].get("RAM", r["regions"].get("XRAM", {}))
        print(f"| `{name}` | {fmt(flash)} | {fmt(ram)} |")
    print()


if __name__ == "__main__":
    main()
