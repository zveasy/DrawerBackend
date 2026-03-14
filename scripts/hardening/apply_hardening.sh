#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -eq 0 ]]; then
  SUDO=()
else
  SUDO=(sudo)
fi

if ! id -u registermvp >/dev/null 2>&1; then
  "${SUDO[@]}" useradd -r -s /usr/sbin/nologin registermvp
fi

"${SUDO[@]}" install -d -m755 /lib/systemd/system
"${SUDO[@]}" install -d -m755 /usr/lib/tmpfiles.d
"${SUDO[@]}" install -d -m755 /etc/ssh/sshd_config.d
"${SUDO[@]}" install -d -m755 /etc/audit/rules.d

"${SUDO[@]}" install -m644 packaging/systemd/register-mvp.service /lib/systemd/system/register-mvp.service
"${SUDO[@]}" install -m644 packaging/tmpfiles.d/register-mvp.conf /usr/lib/tmpfiles.d/register-mvp.conf
"${SUDO[@]}" install -m644 packaging/firewall/nftables.conf /etc/nftables.conf
"${SUDO[@]}" install -m644 packaging/sshd/sshd_hardening.conf /etc/ssh/sshd_config.d/register-mvp.conf
"${SUDO[@]}" install -m644 packaging/auditd/audit.rules /etc/audit/rules.d/register-mvp.rules

if command -v systemctl >/dev/null 2>&1; then
  "${SUDO[@]}" systemctl daemon-reload || true
fi
