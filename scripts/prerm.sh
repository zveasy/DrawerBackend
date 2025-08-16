#!/usr/bin/env bash
set -e
systemctl stop register-mvp || true
systemctl disable register-mvp || true
