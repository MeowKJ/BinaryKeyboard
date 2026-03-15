#!/usr/bin/env python3
"""Primary BinaryKeyboard TUI entry point."""

import argparse
import sys


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="tools/scripts/console.py",
        description="Unified TUI for BinaryKeyboard project workflows.",
    )
    parser.add_argument(
        "--text",
        action="store_true",
        help="Force the plain text fallback menu instead of the TUI.",
    )
    parser.add_argument(
        "--legacy",
        action="store_true",
        help="Use the legacy curses-based TUI instead of Textual.",
    )
    parser.add_argument(
        "--ide-config",
        choices=("vscode", "clion", "all"),
        help="Generate local IDE support files.",
    )
    parser.add_argument("--target", help="Override target for --ide-config.")
    parser.add_argument("--keyboard", help="Override keyboard for --ide-config.")
    parser.add_argument("--profile", choices=("release", "debug"), help="Override profile for --ide-config.")

    args = parser.parse_args()

    if args.ide_config or args.legacy or args.text:
        from tui_console import main as legacy_main
        # Forward the sys.argv so legacy parser picks up same flags
        legacy_main()
        return

    try:
        from tui_textual import run_textual
        run_textual()
    except ImportError:
        print("textual not installed. Install with: pip install textual", file=sys.stderr)
        print("Falling back to legacy TUI...", file=sys.stderr)
        from tui_console import main as legacy_main
        legacy_main()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
