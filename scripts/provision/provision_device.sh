#!/usr/bin/env bash
set -euo pipefail

# Simple provisioning stub: generate key and self-signed cert if none
CFG=${1:-/etc/register-mvp/config.ini}
CERT_DIR=$(awk -F= '/^cert_dir/{print $2}' "$CFG" 2>/dev/null || echo '/etc/register-mvp/certs')
mkdir -p "$CERT_DIR"
KEY="$CERT_DIR/device.key"
CERT="$CERT_DIR/device.crt"
if [[ ! -f "$KEY" ]]; then
  umask 077
  openssl genpkey -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -out "$KEY"
fi
if [[ ! -f "$CERT" ]]; then
  openssl req -new -x509 -subj "/CN=$(hostname)" -key "$KEY" -out "$CERT" -days 365
fi
chmod 600 "$KEY"
chmod 644 "$CERT"
echo "device_id=$(hostname)" > /etc/opt/register-mvp/identity

ENV_FILE=/etc/register-mvp/secrets.env
cat > "$ENV_FILE" <<EOF
AWS_ROOT_CA=$CERT_DIR/AmazonRootCA1.pem
AWS_CERT=$CERT
AWS_KEY=$KEY
EOF
chmod 600 "$ENV_FILE"
