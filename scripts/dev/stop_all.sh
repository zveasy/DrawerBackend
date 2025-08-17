#!/usr/bin/env bash
set -euo pipefail

# Stops dev services started by scripts/dev/start_all.sh

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PID_DIR="$ROOT_DIR/tmp/dev"

stop_one() {
  local name="$1"
  local pidfile="$PID_DIR/${name}.pid"
  if [[ ! -f "$pidfile" ]]; then
    echo "$name: no pidfile"
    return 0
  fi
  local pid
  pid="$(cat "$pidfile")"
  if ! kill -0 "$pid" 2>/dev/null; then
    echo "$name: process $pid not running"
    rm -f "$pidfile"
    return 0
  fi
  echo "Stopping $name (PID $pid)"
  kill "$pid" 2>/dev/null || true
  for i in {1..20}; do
    if kill -0 "$pid" 2>/dev/null; then
      sleep 0.1
    else
      break
    fi
  done
  if kill -0 "$pid" 2>/dev/null; then
    echo "$name: still running, sending SIGKILL"
    kill -9 "$pid" 2>/dev/null || true
  fi
  rm -f "$pidfile"
}

stop_one api_8080
stop_one api_docs_8082
stop_one pos_9090
stop_one tui_8081

echo "All stop attempts completed."
