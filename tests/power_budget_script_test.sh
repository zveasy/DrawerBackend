#!/bin/bash
set -e
DIR="$(cd "$(dirname "$0")" && pwd)"
CSV="$DIR/sample_power.csv"
cat > "$CSV" <<CSV
rail,volts,amps,duty
5V,5,2,1
12V,12,1,0.5
CSV
OUT=$(python3 "$DIR/../scripts/compliance/power_budget.py" "$CSV")
echo "$OUT"
echo "$OUT" | grep -q "Recommended fuse: 3.1A"
