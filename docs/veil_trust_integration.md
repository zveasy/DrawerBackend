# VEIL Trust Evidence Integration

Generation 3 adds a VEIL integration boundary for turning appliance, fleet,
cash, command, OTA, enrollment, reconciliation, anomaly, and certificate events
into portable trust evidence.

## Client Boundary

The `src/integrations/veil/` module defines a clean client interface:

- `submit_trust_event`
- `submit_evidence_bundle`
- `verify_policy`
- `request_restore_authorization`
- `fetch_trust_score`

The local implementation is deterministic and does not require a live VEIL
server. Production deployments can replace the client behind this interface.

## Evidence Model

Trust evidence records include tenant, device, merchant, branch, region,
event type, source module, timestamp, payload hash, previous hash,
local-signature placeholder, policy context, classification labels,
sensitivity labels, audit event ID, correlation ID, and source payload.

Supported source event classes include enrollment, heartbeat, disabled/revoked
attempts, command lifecycle events, transaction events, cash ledger entries,
reconciliation reports, variance detections, anomaly detections, OTA decisions
and install attempts, and certificate lifecycle events.

## Evidence Chain

Evidence is persisted locally in an append-only JSON store. The verifier checks:

- duplicate evidence IDs
- payload tampering
- missing hash links
- out-of-order append events

Evidence bundles are deterministic exports that include records, chain
verification, hash summary, time range, scope, policy decisions, and related
audit event IDs.

## Policy Gates

The policy layer supports high-risk gates for disabling/enabling devices,
pausing/resuming transactions, OTA promotion and install eligibility,
certificate revocation, reconciliation overrides/manual adjustments, and command
execution. If VEIL policy is unavailable, production mode fails closed for
configured high-risk actions. Development/test mode may use local allow policy.

## Trust Score Bridge

The local trust score adapter combines fleet health, cash health, certificate
status, OTA compliance, anomaly severity, reconciliation variance, and
offline/sync behavior into a VEIL-ingestable score with explainability factors.

## APIs

Trust APIs are exposed under `/trust/v1/*` and use the same fail-closed
authentication path as other protected appliance and fleet routes.

Key surfaces:

- `POST /trust/v1/evidence`
- `GET /trust/v1/evidence`
- `POST /trust/v1/evidence/{id}/submit`
- `GET /trust/v1/verify`
- `GET /trust/v1/export/{scope}/{id}`
- `POST /trust/v1/policy/preview`
- `GET /trust/v1/score/{device_id}`

Tenant filtering uses `X-Org-Id`, `tenant_id`, or `organization_id`.
