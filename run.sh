#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
VENV_DIR="$PROJECT_ROOT/.venv"
REQUIREMENTS_FILE="$PROJECT_ROOT/tools/scripts/requirements-console.txt"
CONSOLE_SCRIPT="$PROJECT_ROOT/tools/scripts/console.py"

if command -v python3 >/dev/null 2>&1; then
  PYTHON_BIN="python3"
elif command -v python >/dev/null 2>&1; then
  PYTHON_BIN="python"
else
  echo "[run] Python not found. Please install python3 first." >&2
  exit 1
fi

cd "$PROJECT_ROOT"

if [ ! -x "$VENV_DIR/bin/python" ]; then
  echo "[run] Creating virtual environment: $VENV_DIR" >&2
  "$PYTHON_BIN" -m venv "$VENV_DIR"
fi

# shellcheck disable=SC1091
source "$VENV_DIR/bin/activate"

if ! python -c "import importlib.util, sys; sys.exit(0 if importlib.util.find_spec('textual') else 1)"; then
  echo "[run] Installing console dependencies..." >&2
  python -m pip install -r "$REQUIREMENTS_FILE"
fi

exec python "$CONSOLE_SCRIPT"
