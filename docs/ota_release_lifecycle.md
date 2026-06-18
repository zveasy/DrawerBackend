# OTA Release Lifecycle

The OTA release service scaffolding models release metadata and eligibility
decisions separately from the appliance install agent.

## Release Metadata

Release manifests include:

- channel: `dev`, `pilot`, or `stable`
- version
- artifact URL
- SHA-256 hash
- Ed25519 signature metadata
- staged rollout percentage
- minimum allowed current version
- rollback protection metadata
- revoked device list

## Promotion

Promotion is intentionally linear:

1. `dev`
2. `pilot`
3. `stable`

The service rejects invalid jumps such as `dev -> stable`.

## Eligibility

Eligibility rejects revoked devices, stale versions, rollback-protected devices
below the minimum version, and devices outside the staged rollout percentage.
Every decision is appended to an in-memory audit log and reject decisions
increment `register_ota_rejects_total{reason}`.

## Appliance Behavior

The existing OTA agent still fails closed for missing signature keys, invalid
channels, stale versions, revoked devices, missing hashes, bad hashes, bad
signatures, and rollout exclusion.
