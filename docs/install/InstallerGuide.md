# Installer Guide

## Box Contents
- Register unit
- Power brick and single harness
- Spares kit

## Tools Required
- #2 Phillips screwdriver
- Ground bond tester
- Network cable

## Safety Notes
- Disconnect power before opening covers.
- Follow lock-out/tag-out procedures.

## Installation Steps
1. **Mount Stand:** Secure the stand using four M6 bolts.
2. **Connect Power:** Attach the power brick and route the single harness
   through the cabinet.
3. **Ground Bond:** Verify chassis ground using the tester.
4. **First Boot:**
   - Run `scripts/hardening/apply_hardening.sh`.
   - Copy configuration to `/etc/register-mvp/config.ini`.
   - Select **EOL** or **demo** mode in the config.
5. **Verification:**
   - `systemctl status register-mvp`
   - `curl 127.0.0.1:8080/status`

