# DeviceOffline

**Who:** On-call technician

**What:** Prometheus cannot scrape /metrics for 5m.

**When:** Page immediately after alert.

**How:**
1. Check power and network cables.
2. Verify `systemctl status register-mvp`.
3. Collect logs from `/var/log/register-mvp`.
4. Reboot device if unresponsive.
