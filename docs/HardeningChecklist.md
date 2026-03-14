# Hardening Checklist

- [ ] Service runs as `registermvp` user
- [ ] SSH disables passwords
- [ ] nftables default drop
- [ ] auditd logging enabled

After applying hardening, run `scripts/hardening/verify_hardening.sh` to validate these settings.
To refresh checklist marks and generate evidence files, run:
`python3 scripts/compliance/update_checklist_status.py --eol-report /path/to/result.json`
