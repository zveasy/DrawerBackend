#!/bin/bash
set -e
DATE=$(date +%Y%m%d)
OUTDIR=dist
mkdir -p "$OUTDIR"
ZIP="$OUTDIR/lab_packet_$DATE.zip"
TMP=$(mktemp -d)
cp -r docs/compliance "$TMP/"
cp -r assets/labels "$TMP/"
[ -f register_mvp ] && cp register_mvp "$TMP/firmware.bin"
echo "See docs/compliance/CompliancePlan.md" > "$TMP/README_FIRST.md"
(cd "$TMP" && zip -r "$OLDPWD/$ZIP" . >/dev/null)
rm -rf "$TMP"
echo "Created $ZIP"
