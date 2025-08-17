#!/bin/bash
set -e
cd "$(dirname "$0")/.."
OUT=$(scripts/demo/smoke_store_demo.sh)
echo "$OUT"
grep -q PASS <<<"$OUT"
test -s scripts/demo/demo_telemetry.json

