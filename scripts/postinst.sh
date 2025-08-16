#!/usr/bin/env bash
set -e
mkdir -p /var/log/register-mvp
chown root:root /var/log/register-mvp
chmod 755 /var/log/register-mvp
# copy config on first install
if [ ! -f /etc/register-mvp/config.ini ]; then
  mkdir -p /etc/register-mvp
  cp -n /opt/register_mvp/share/config.ini.example /etc/register-mvp/config.ini || true
fi
systemctl daemon-reload || true
systemctl enable register-mvp || true
