#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TS=$(date +%Y%m%d_%H%M%S)
TMP=$(mktemp -d)
OUT="support_bundle_${TS}.tar.gz"
journalctl -u register-mvp >"$TMP/journalctl.log" 2>/dev/null || true
cp /etc/register-mvp/config.ini "$TMP/" 2>/dev/null || true
cp data/health.json "$TMP/" 2>/dev/null || true
cp data/telemetry.jsonl "$TMP/" 2>/dev/null || true
ip addr >"$TMP/net.txt" 2>&1 || true
/opt/register_mvp/bin/register_mvp --version >"$TMP/version.txt" 2>/dev/null || true
tar -czf "$OUT" -C "$TMP" .
python3 "$SCRIPT_DIR/redact_bundle.py" "$OUT"
echo "$OUT"
