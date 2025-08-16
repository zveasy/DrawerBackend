#!/bin/sh
# Simplified provisioning flow
PLANT="A1"
PRINTER="file://stdout"
DRY=0
while [ $# -gt 0 ]; do
  case "$1" in
    --plant) PLANT="$2"; shift;;
    --printer) PRINTER="$2"; shift;;
    --dry-run) DRY=1;;
  esac
  shift
done
SERIAL=$(./register_mvp_serial "$PLANT")
if [ $DRY -eq 0 ]; then
  python3 scripts/provision/print_label.py "$SERIAL" "$PRINTER"
  touch /var/lib/register-mvp/provisioned 2>/dev/null || true
fi
echo "$SERIAL"
