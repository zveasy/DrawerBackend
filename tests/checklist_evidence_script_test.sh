#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCRIPT="$ROOT_DIR/scripts/compliance/update_checklist_status.py"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

EOL_REPORT="$TMP/result.json"
EOL_CHECKLIST="$TMP/EOL_Checklist.md"
HARDENING_CHECKLIST="$TMP/HardeningChecklist.md"
UNIT_FILE="$TMP/register-mvp.service"
SSHD_HARDENING="$TMP/register-mvp-sshd.conf"
NFT_CONF="$TMP/nftables.conf"
AUDIT_RULES="$TMP/register-mvp.rules"
EVIDENCE_DIR="$TMP/evidence"

cat > "$EOL_REPORT" <<'JSON'
{
  "serial": "REG-TEST-0001",
  "device_id": "REG-TEST-0001",
  "version": "v1.2.3",
  "pass": true,
  "dispensed": 10,
  "expected_g": 5.0,
  "measured_g": 5.01,
  "delta_g": 0.01,
  "steps": [
    {"name":"Self-check","ok":true,"reason":"","value":0},
    {"name":"Home shutter","ok":true,"reason":"","value":0},
    {"name":"Dispense","ok":true,"reason":"","value":10},
    {"name":"Scale verify","ok":true,"reason":"","value":5.01},
    {"name":"Present","ok":true,"reason":"","value":0},
    {"name":"Telemetry seen","ok":true,"reason":"","value":0}
  ]
}
JSON

cat > "$EOL_CHECKLIST" <<'MD'
# EOL Checklist

- [ ] Self-check
- [ ] Home shutter
- [ ] Dispense coins
- [ ] Scale verify
- [ ] Present/close
- [ ] Telemetry seen
MD

cat > "$HARDENING_CHECKLIST" <<'MD'
# Hardening Checklist

- [ ] Service runs as `registermvp` user
- [ ] SSH disables passwords
- [ ] nftables default drop
- [ ] auditd logging enabled
MD

cat > "$UNIT_FILE" <<'UNIT'
[Service]
User=registermvp
UNIT

cat > "$SSHD_HARDENING" <<'SSH'
PasswordAuthentication no
SSH

cat > "$NFT_CONF" <<'NFT'
table inet filter {
  chain input { type filter hook input priority 0; policy drop; }
}
NFT

touch "$AUDIT_RULES"

python3 "$SCRIPT" \
  --eol-report "$EOL_REPORT" \
  --eol-checklist "$EOL_CHECKLIST" \
  --hardening-checklist "$HARDENING_CHECKLIST" \
  --evidence-dir "$EVIDENCE_DIR" \
  --unit-file "$UNIT_FILE" \
  --sshd-hardening "$SSHD_HARDENING" \
  --nft-conf "$NFT_CONF" \
  --audit-rules "$AUDIT_RULES"

rg -n '^\- \[x\] Self-check$' "$EOL_CHECKLIST"
rg -n '^\- \[x\] Dispense coins$' "$EOL_CHECKLIST"
rg -n '^\- \[x\] Present/close$' "$EOL_CHECKLIST"
rg -n '^\- \[x\] Service runs as `registermvp` user$' "$HARDENING_CHECKLIST"
rg -n '^\- \[x\] SSH disables passwords$' "$HARDENING_CHECKLIST"
rg -n '^\- \[x\] nftables default drop$' "$HARDENING_CHECKLIST"
rg -n '^\- \[x\] auditd logging enabled$' "$HARDENING_CHECKLIST"

test -s "$EVIDENCE_DIR/eol_evidence.json"
test -s "$EVIDENCE_DIR/hardening_evidence.json"
test -s "$EVIDENCE_DIR/checklist_status.json"
