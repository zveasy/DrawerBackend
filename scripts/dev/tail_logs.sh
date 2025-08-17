#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
LOG_DIR="$ROOT_DIR/logs"
mkdir -p "$LOG_DIR"

files=(
  "$LOG_DIR/api_8080.log"
  "$LOG_DIR/api_docs_8082.log"
  "$LOG_DIR/pos_9090.log"
  "$LOG_DIR/tui_8081.log"
)

echo "Tailing logs (press Ctrl-C to exit):"
ls -1 "${files[@]}" 2>/dev/null || true

tail -n 200 -F "${files[@]}"
