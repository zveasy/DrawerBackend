# Remaining Production Gaps

This repo is ready for limited pilot hardening exercises, not unattended
commercial scale.

## Control Plane

The device twin store is durable locally and now has an authenticated HTTP
control-plane contract with conflict detection and an offline queue. Production
still needs a hosted remote source of truth, tenant isolation, fleet RBAC, cloud
audit retention, and operator tooling for conflict resolution.

## Identity

Enrollment and revocation scaffolding is present, including one-time token
validation and identity persistence. Production still needs manufacturing-backed
identity issuance, secure token delivery, and compromised-device recovery.

## OTA

The OTA agent fails closed for signed mode and rejects unsafe manifests. The OTA
release service now models promotion, staged rollout, rollback protection, and
eligibility audit decisions. Production still needs artifact hosting, signed
release pipeline automation, rollback reporting, and regional release approvals.

## International Operations

The data model supports multi-currency metadata, but country-specific compliance
rules, cash reconciliation, reporting, KYC/agent banking workflows, and service
provider procedures must be validated per market.

## Dependency Hygiene

The Square webhook backend should keep `npm run audit:pilot` at zero findings
before pilot releases. Any future exception must be documented with advisory,
affected package, exploitability assessment, mitigation, owner, and expiry.
