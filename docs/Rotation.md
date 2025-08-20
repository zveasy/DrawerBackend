# Secret Rotation

Use `scripts/rotate/rotate_secrets.sh` to renew the device certificate. The service handles SIGHUP to reload credentials without restart.

The configuration value `aws.rotation_check_minutes` controls how often the loader
re-reads credential environment variables. When combined with an external secret
store updating those variables, credentials are refreshed automatically without a
process restart.
