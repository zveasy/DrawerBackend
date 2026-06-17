# Remaining Production Gaps

This repo is ready for limited pilot hardening exercises, not unattended
commercial scale.

## Control Plane

The device twin store is durable locally and has a cloud-control-plane
interface, but the cloud service remains to be built. A production deployment
needs a remote source of truth, conflict-resolution policy, tenant isolation,
fleet RBAC, and cloud audit retention.

## Identity

Enrollment and revocation scaffolding is present. Production still needs
manufacturing-backed identity issuance, mTLS certificate lifecycle management,
secure token delivery, and compromised-device recovery.

## OTA

The OTA agent fails closed for signed mode and rejects unsafe manifests. It
still needs a production update backend, signed release pipeline, staged rollout
orchestration, rollback reporting, and regional release approvals.

## International Operations

The data model supports multi-currency metadata, but country-specific compliance
rules, cash reconciliation, reporting, KYC/agent banking workflows, and service
provider procedures must be validated per market.

## Dependency Hygiene

The Square webhook backend should keep `npm run audit:pilot` at zero findings
before pilot releases. Any future exception must be documented with advisory,
affected package, exploitability assessment, mitigation, owner, and expiry.
