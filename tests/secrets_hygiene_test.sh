#!/usr/bin/env bash
set -euo pipefail

tracked="$(git ls-files)"
if printf '%s\n' "$tracked" | grep -E '(^|/)\.env$' >/dev/null; then
  echo "tracked real .env file detected" >&2
  exit 1
fi

if printf '%s\n' "$tracked" | grep -E '\.(pem|key|crt|p12|pfx)$' >/dev/null; then
  echo "tracked private key/certificate material detected" >&2
  exit 1
fi

if git grep -n -I -E '-----BEGIN (RSA |EC |OPENSSH |)PRIVATE KEY-----|AWS_SECRET_ACCESS_KEY=|SQUARE_ACCESS_TOKEN=sq0|SQUARE_WEBHOOK_SIGNATURE_KEY=[A-Za-z0-9+/=]{20,}' -- ':!*.example' ':!tests/secrets_hygiene_test.sh' >/tmp/register_mvp_secret_hits.txt; then
  cat /tmp/register_mvp_secret_hits.txt >&2
  exit 1
fi
