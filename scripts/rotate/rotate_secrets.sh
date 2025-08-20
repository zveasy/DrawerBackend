#!/usr/bin/env bash
set -euo pipefail

CFG=${1:-/etc/register-mvp/config.ini}
CERT_DIR=$(awk -F= '/^cert_dir/{print $2}' "$CFG" 2>/dev/null || echo '/etc/register-mvp/certs')
KEY="$CERT_DIR/device.key"
CERT="$CERT_DIR/device.crt"
TMP_CERT="$CERT.new"

openssl req -new -x509 -subj "/CN=$(hostname)" -key "$KEY" -out "$TMP_CERT" -days 365
mv "$TMP_CERT" "$CERT"
chmod 644 "$CERT"
ENV_FILE=/etc/register-mvp/secrets.env
if [[ -f "$ENV_FILE" ]]; then
  sed -i "s|^AWS_CERT=.*|AWS_CERT=$CERT|" "$ENV_FILE"
else
  cat > "$ENV_FILE" <<EOF
AWS_CERT=$CERT
AWS_KEY=$KEY
AWS_ROOT_CA=$CERT_DIR/AmazonRootCA1.pem
EOF
  chmod 600 "$ENV_FILE"
fi
# Signal service to reload
if command -v systemctl >/dev/null 2>&1; then
  systemctl kill -s HUP register-mvp || true
fi
