#!/usr/bin/env bash
set -euo pipefail

base_api="http://127.0.0.1:8080"
docs_api="http://127.0.0.1:8082"
pos_api="http://127.0.0.1:9090"

pass() { echo "[OK] $1"; }
fail() { echo "[FAIL] $1"; exit 1; }

# API basic endpoints
curl -sfS "$base_api/version" > /dev/null && pass "/version" || fail "/version"
curl -sfS "$base_api/status" > /dev/null && pass "/status" || fail "/status"

# Docs index and index.html fallback
REGISTER_MVP_DOCS_PATH="$(cd "$(dirname "$0")/../../docs" && pwd)" \
  curl -sfS "$docs_api/help" > /dev/null && pass "/help" || fail "/help"
REGISTER_MVP_DOCS_PATH="$(cd "$(dirname "$0")/../../docs" && pwd)" \
  curl -sfS "$docs_api/help/index.html" > /dev/null && pass "/help/index.html (fallback)" || fail "/help/index.html"

# POS ping
curl -sfS "$pos_api/ping" > /dev/null && pass "POS /ping" || fail "POS /ping"

echo "Smoke tests completed successfully."
