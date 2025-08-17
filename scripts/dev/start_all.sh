#!/usr/bin/env bash
set -euo pipefail

# Starts dev services for DrawerBackend
# - API on 8080
# - API with docs on 8082 (REGISTER_MVP_DOCS_PATH=./docs)
# - POS HTTP connector on 9090
# - TUI bound to API 8081

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="$ROOT_DIR/build/register_mvp"
LOG_DIR="$ROOT_DIR/logs"
PID_DIR="$ROOT_DIR/tmp/dev"

mkdir -p "$LOG_DIR" "$PID_DIR"

if [[ ! -x "$BIN" ]]; then
  echo "Binary not found at $BIN. Please build first (e.g., cmake --build build)" >&2
  exit 1
fi

start_one() {
  local name="$1"; shift
  local cmd=("$BIN" "$@")
  local log="$LOG_DIR/${name}.log"
  local pidfile="$PID_DIR/${name}.pid"

  if [[ -f "$pidfile" ]] && kill -0 "$(cat "$pidfile")" 2>/dev/null; then
    echo "$name already running with PID $(cat "$pidfile")"
    return 0
  fi

  echo "Starting $name -> $log"
  nohup "${cmd[@]}" >"$log" 2>&1 &
  echo $! >"$pidfile"
}

# API 8080
start_one "api_8080" --api 8080

# API 8082 with docs
REGISTER_MVP_DOCS_PATH="$ROOT_DIR/docs" start_one "api_docs_8082" --api 8082

# POS HTTP 9090
start_one "pos_9090" --pos-http 9090

# TUI bound to 8081
start_one "tui_8081" --tui 8081

echo "All services attempted. View logs in $LOG_DIR."
