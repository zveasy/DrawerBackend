#!/usr/bin/env bash
set -euo pipefail

useradd -r -s /usr/sbin/nologin registermvp 2>/dev/null || true
install -m644 packaging/systemd/register-mvp.service /lib/systemd/system/register-mvp.service
install -m644 packaging/tmpfiles.d/register-mvp.conf /usr/lib/tmpfiles.d/register-mvp.conf
install -m644 packaging/firewall/nftables.conf /etc/nftables.conf
install -m644 packaging/sshd/sshd_hardening.conf /etc/ssh/sshd_config.d/register-mvp.conf
install -m644 packaging/auditd/audit.rules /etc/audit/rules.d/register-mvp.rules
systemctl daemon-reload || true
