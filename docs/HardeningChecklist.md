# Hardening Checklist

- [ ] Service runs as `registermvp` user
- [ ] SSH disables passwords
- [ ] nftables default drop
- [ ] auditd logging enabled

After applying hardening, run `scripts/hardening/verify_hardening.sh` to validate these settings.
