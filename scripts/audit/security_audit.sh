#!/usr/bin/env bash
set -euo pipefail

echo "Checking nftables rules..."
nft list ruleset >/dev/null 2>&1 && echo ok || echo missing

echo "Checking service user..."
ps -o user= -C register_mvp 2>/dev/null || echo "service not running"

echo "Checking cert permissions..."
ls -l /etc/register-mvp/certs 2>/dev/null || true
