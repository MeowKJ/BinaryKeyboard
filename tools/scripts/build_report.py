#!/usr/bin/env python3
"""
Shared colorful build report helpers for BinaryKeyboard firmware scripts.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Callable, Iterable


ColorizeFn = Callable[[str, str], str]


@dataclass(frozen=True)
class UsageRow:
    name: str
    used: int
    total: int
    free: int
    pct: float


def strip_ansi(s: str) -> str:
    return re.sub(r"\033\[[^m]*m", "", s)


def visible_len(s: str) -> int:
    return len(strip_ansi(s))


def rpad(s: str, width: int) -> str:
    return s + " " * max(0, width - visible_len(s))


def pct_color_code(pct: float) -> str:
    return "31" if pct >= 90 else "33" if pct >= 70 else "32"


def _can_encode(ch: str) -> bool:
    import sys
    enc = getattr(sys.stdout, "encoding", "utf-8") or "utf-8"
    try:
        ch.encode(enc)
        return True
    except (UnicodeEncodeError, LookupError):
        return False


_BAR_FILLED = "█" if _can_encode("█") else "#"
_BAR_EMPTY  = "░" if _can_encode("░") else "-"


def usage_bar(pct: float, colorize: ColorizeFn, width: int = 15) -> str:
    filled = round(pct / 100 * width)
    bar = _BAR_FILLED * filled + _BAR_EMPTY * (width - filled)
    return colorize(pct_color_code(pct), bar)


def fmt_bytes(n: int) -> str:
    return f"{n / 1024:.1f} KB" if n >= 1024 else f"{n} B"


def render_usage_report(
    *,
    title: str,
    subtitle: str,
    rows: Iterable[UsageRow],
    detail_line: str = "",
    colorize: ColorizeFn,
    use_color: bool,
    width: int = 68,
    accent_code: str = "1;36",
) -> None:
    rows = list(rows)
    if not rows and not detail_line:
      return

    def hline(left: str, right: str, fill: str = "─") -> str:
        line = left + fill * width + right
        return colorize(accent_code, line) if use_color else line

    def brow(cells: str) -> str:
        inner = rpad(cells, width)
        if use_color:
            return colorize(accent_code, "│") + inner + colorize(accent_code, "│")
        return "│" + inner + "│"

    print()
    print(hline("┌", "┐"))
    print(brow("  " + colorize("1", title) + colorize("2", f"  ·  {subtitle}")))
    print(hline("├", "┤"))

    if rows:
        header = "  " + rpad(colorize("2", "Region"), 12)
        header += rpad(colorize("2", "Used / Total"), 22)
        header += rpad(colorize("2", "Free"), 12)
        header += colorize("2", "Usage")
        print(brow(header))
        print(hline("├", "┤"))

        for row in rows:
            cc = pct_color_code(row.pct)
            left = "  " + rpad(colorize("1", row.name), 11) + " "
            used = rpad(f"{fmt_bytes(row.used)} / {fmt_bytes(row.total)}", 22)
            free = rpad(colorize(cc, fmt_bytes(row.free)), 12)
            usage = usage_bar(row.pct, colorize) + "  " + colorize(cc, f"{row.pct:5.1f}%")
            print(brow(left + used + free + usage))

    if detail_line:
        print(hline("├", "┤"))
        print(brow("  " + detail_line))

    print(hline("└", "┘"))
