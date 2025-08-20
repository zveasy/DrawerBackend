#!/usr/bin/env bash
set -euo pipefail

# verify systemd unit runs as registermvp user
if ! systemctl show register-mvp.service -p User --value 2>/dev/null | grep -qx 'registermvp'; then
  echo "register-mvp service is not configured to run as registermvp user" >&2
  exit 1
fi

# verify SSH password logins are disabled
if ! sshd -T 2>/dev/null | grep -iq '^passwordauthentication no'; then
  echo "SSH password authentication is not disabled" >&2
  exit 1
fi

# verify nftables default policy drop
if ! nft list ruleset 2>/dev/null | grep -q 'policy drop'; then
  echo "nftables default policy is not set to drop" >&2
  exit 1
fi

# verify auditd rule file present
if [ ! -f /etc/audit/rules.d/register-mvp.rules ]; then
  echo "auditd rule file /etc/audit/rules.d/register-mvp.rules is missing" >&2
  exit 1
fi

echo "Hardening verification passed." 
