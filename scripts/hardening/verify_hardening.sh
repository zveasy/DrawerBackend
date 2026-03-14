#!/usr/bin/env bash
set -euo pipefail

fail() {
  echo "$1" >&2
  exit 1
}

unit_file="/lib/systemd/system/register-mvp.service"
sshd_hardening="/etc/ssh/sshd_config.d/register-mvp.conf"
nft_conf="/etc/nftables.conf"
audit_rules="/etc/audit/rules.d/register-mvp.rules"

# verify systemd unit is installed and pinned to the least-privileged user
[[ -f "${unit_file}" ]] || fail "missing systemd unit file: ${unit_file}"
grep -Eq '^User=registermvp$' "${unit_file}" || fail "register-mvp.service is not configured with User=registermvp"

# verify SSH hardening drop-in is installed and disables password auth
[[ -f "${sshd_hardening}" ]] || fail "missing ssh hardening file: ${sshd_hardening}"
grep -Eiq '^PasswordAuthentication[[:space:]]+no$' "${sshd_hardening}" || fail "SSH password authentication is not disabled in ${sshd_hardening}"

# verify nftables default-drop policy was installed
[[ -f "${nft_conf}" ]] || fail "missing nftables config: ${nft_conf}"
grep -Eq 'policy[[:space:]]+drop' "${nft_conf}" || fail "nftables default drop policy not found in ${nft_conf}"

# verify auditd rule file present
[[ -f "${audit_rules}" ]] || fail "auditd rule file ${audit_rules} is missing"

echo "Hardening verification passed."
