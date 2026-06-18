# Fleet Operations and Remote Command Control

## Fleet model

Generation 5 adds a durable, tenant-scoped operations registry under
`src/cloud/fleet_operations/`. A fleet contains device groups. Each enrolled
device has a location-aware assignment containing fleet, group, location,
branch, merchant, and region metadata. The local JSON store remains available
offline through `REGISTER_MVP_FLEET_OPERATIONS_STORE`.

Every `/fleet/v1/*` request requires the normal API authentication and an
`X-Org-Id` tenant header. Cross-tenant records are returned as not found, and
payload tenant IDs must exactly match the authenticated tenant scope.

## Lifecycle states

Supported states are:

- `provisioned`
- `active`
- `maintenance`
- `quarantined`
- `retired`

Normal transitions are `provisioned -> active`, `active -> maintenance`, and
`maintenance -> active`. Any non-retired device can be quarantined or retired.
Only the recovery flow can transition `quarantined -> active`. Retirement is
irreversible.

## Command queue lifecycle

Commands use these durable states:

`queued -> policy_checking -> approved -> dispatched -> acknowledged -> completed`

Terminal alternatives are `failed`, `expired`, and `cancelled`. Commands are
tenant scoped, checked against the Generation 4 capability registry, checked by
VEIL, and rejected when the device is offline, stale, quarantined, retired, or
has unknown capabilities.

High-risk commands require both a successful VEIL decision and non-empty
`approval_id` and `approved_by` fields. Without approval metadata they remain in
`policy_checking` with `approval_required` and may be cancelled. Expiration is
processed by command reads and by the service expiration operation.

Example:

```http
POST /fleet/v1/commands
Authorization: Bearer <token>
X-Org-Id: org-1
Content-Type: application/json

{
  "tenant_id": "org-1",
  "device_id": "drawer-22",
  "command": "unlock",
  "approval_id": "approval-482",
  "approved_by": "supervisor-7",
  "actor_id": "operator-3",
  "expires_at": 1770000120,
  "parameters": {}
}
```

## Heartbeat freshness

Heartbeats carry device and tenant identity, firmware and driver versions,
connectivity state, inventory summary, error codes, health metrics, timestamp,
and a signature placeholder. The default freshness threshold is 90 seconds.

Heartbeat ingestion fails closed and quarantines on stale timestamps, tenant
mismatch, unknown firmware, unknown driver, unknown capability state, or
inventory contradictions. Quarantined devices may still submit a fresh
heartbeat so recovery evidence can be evaluated, but the heartbeat response
remains rejected until recovery succeeds.

## Risk scoring

Risk is deterministic and explainable. Points are assigned for stale
heartbeats, repeated command failures, high command failure rate, inventory
drift, VEIL policy violations, unstable drivers, unusual cash movement, missing
trust evidence, and quarantine state.

Levels are:

- `low`: 0-24
- `medium`: 25-49
- `high`: 50-74
- `critical`: 75-100

Device and location responses include every factor, point value, and
explanation. Changes in risk level append a `risk_changed` event.

## Quarantine and recovery

Automatic quarantine triggers include stale heartbeats, tenant boundary
violations, inventory contradictions, VEIL denial, unknown firmware or driver,
suspicious command results, and three or more failed commands.

Recovery requires:

1. A fresh online heartbeat.
2. A successful self-test in heartbeat health metrics.
3. Known firmware, driver, and capability state.
4. No unresolved critical risk after excluding quarantine itself.
5. VEIL approval for `device_recovery`.

Recovery and quarantine create both fleet audit events and VEIL trust evidence.

## Event replay

Fleet operation events are append-only logical records with a global sequence,
previous hash, and deterministic SHA-256 event hash. Enrollment, lifecycle,
heartbeat, command, quarantine, recovery, and risk changes are persisted.
Replay verifies sequence continuity, previous-hash links, and event hashes
before returning the tenant-filtered event stream and hash summary.

## API summary

Registry:

- `POST /fleet/v1/devices/enroll`
- `GET /fleet/v1/devices`
- `GET /fleet/v1/devices/{device_id}`
- `PATCH /fleet/v1/devices/{device_id}`
- `POST /fleet/v1/devices/{device_id}/retire`

Commands:

- `POST /fleet/v1/commands`
- `GET /fleet/v1/commands`
- `GET /fleet/v1/commands/{command_id}`
- `POST /fleet/v1/commands/{command_id}/cancel`
- `POST /fleet/v1/commands/{command_id}/ack`
- `POST /fleet/v1/commands/{command_id}/complete`

Presence, risk, and quarantine:

- `POST /fleet/v1/heartbeats`
- `GET /fleet/v1/devices/{device_id}/health`
- `GET /fleet/v1/devices/{device_id}/freshness`
- `GET /fleet/v1/risk`
- `GET /fleet/v1/devices/{device_id}/risk`
- `GET /fleet/v1/locations/{location_id}/risk`
- `POST /fleet/v1/devices/{device_id}/quarantine`
- `POST /fleet/v1/devices/{device_id}/recover`
- `GET /fleet/v1/quarantine`

The `/fleet/v1` surface is additive. Existing `/fleet/*`,
`/fleet-control/v1/*`, `/edge/v1/*`, `/cash/v1/*`, and `/trust/v1/*` contracts
remain unchanged.
