# Support Playbooks

## JamRateHigh
- **Symptoms:** Frequent jam alerts.
- **Quick Triage:** Inspect hopper for debris.
- **Fix:** Run [Jam Clear](../service/JamClear.md).
- **Escalate:** If jams continue after two clears.

## DeviceOffline
- **Symptoms:** Device not sending heartbeats.
- **Quick Triage:** Check power and network cables.
- **Fix:** Reboot device.
- **Escalate:** If still offline.

## LowCoins
- Refill hoppers; verify via `/status`.

## ShutterLimitTimeout
- Check for mechanical obstruction; run service `open` then `close`.

## HopperNoPulses
- Ensure hopper motor cable seated; replace hopper if persistent.

## IdempotencyMismatch
- Clear `/var/lib/register-mvp/pos` and retry transaction.

## POS Busy
- POS endpoint returned 429; retry with backoff.

## ScaleDrift
- Tare scale via service menu.

## ConfigError
- Review config file for syntax; restore from backup.

## TelemetryNotFlowing
- Check network firewall; verify MQTT broker reachable.

