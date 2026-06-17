# Fleet Control Plane

Generation 1 introduces a centralized fleet-control module for operating large
fleets of financial edge devices while preserving local/offline appliance
behavior.

## Hierarchy

The model supports:

- organizations
- merchants
- branches
- regions
- device ownership links
- tags
- labels

Hierarchy, registry, command, alert, and audit state can be persisted to a local
JSON store for offline-first operation or pilot deployments.

## Device Registry

Registry records track device ownership, environment, deployment channel,
firmware version, enrollment status, certificate identity status, heartbeat
timestamps, connectivity state, and health status.

Search supports tenant, merchant, branch, region, environment, channel, health,
connectivity, tag, label, and text filters.

## Presence

Devices send heartbeat messages with sent and received timestamps. The control
plane records latency, marks devices online, emits `heartbeat_received` and
`device_online` events, and marks stale devices offline after the configured
heartbeat timeout. Offline transitions create critical alerts.

## Commands

Supported command types:

- `restart`
- `refresh_config`
- `sync_inventory`
- `pause_transactions`
- `resume_transactions`
- `disable_device`
- `enable_device`

Commands have unique IDs, delivery status, acknowledgments, retries, expirations,
and audit events. Acknowledged command IDs are replay-protected to prevent
duplicate execution.

## Alerts And Metrics

Alerts include severity, timestamps, and acknowledgment state. Built-in metrics
cover online/offline device counts, command success/failure rate, average
heartbeat latency, fleet availability, alert counts, and sync counts.

## APIs

Fleet-control APIs are exposed under `/fleet-control/v1/*` and use the same
server authentication path as existing protected endpoints. Tenant isolation is
enforced with `X-Org-Id` or `organization_id` query parameters.

Key surfaces:

- `/fleet-control/v1/organizations`
- `/fleet-control/v1/merchants`
- `/fleet-control/v1/branches`
- `/fleet-control/v1/regions`
- `/fleet-control/v1/devices`
- `/fleet-control/v1/devices/{id}/heartbeat`
- `/fleet-control/v1/devices/{id}/commands`
- `/fleet-control/v1/commands`
- `/fleet-control/v1/alerts`
- `/fleet-control/v1/metrics`
- `/fleet-control/v1/dashboard/{scope}/{id}`
