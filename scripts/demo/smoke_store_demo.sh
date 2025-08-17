#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TELE="$SCRIPT_DIR/demo_telemetry.json"
rm -f "$TELE"
"$SCRIPT_DIR/../../build/register_mvp" --purchase 735 --deposit 1000 --json >"$TELE" 2>"$SCRIPT_DIR/demo.log" || { echo FAIL; exit 1; }
if grep -q 'OK' "$TELE"; then
  echo PASS
  exit 0
else
  echo FAIL
  exit 1
fi

