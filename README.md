# DrawerBackend

DrawerBackend is a C++17 hardware backend, cloud-ready fleet platform,
cash-operations engine, and financial edge device platform. It controls local
drawer hardware, exposes POS and service APIs, reports telemetry, supports OTA
updates, models devices as cloud-managed twins, reconciles cash movement against
physical state, and provides a shared abstraction for drawers, safes, recyclers,
kiosks, teller stations, and ATM-like terminals.

## Architecture

The runtime is layered so hardware control remains isolated from fleet and cloud
features:

- **HAL and drivers:** GPIO abstraction, shutter stepper, parallel hopper, and
  HX711 scale drivers.
- **Application logic:** dispense control, transaction engine, change making,
  audit weighing, self-test, jam clearing, service mode, EOL, and burn-in flows.
- **Local APIs:** device HTTP API, POS HTTP/serial integrations, docs endpoint,
  metrics endpoints, and TUI mode.
- **Cloud and fleet:** AWS-style telemetry queueing, shadow updates, device twin
  models, fleet manager, health scoring, predictive maintenance, inventory
  forecasting, alerts, and aggregate fleet metrics.
- **Cash intelligence:** durable cash ledger, reconciliation engine,
  denomination inventory, deterministic cash forecasting, anomaly detection,
  cash health scores, and cash dashboard models.
- **Trust evidence:** VEIL client boundary, portable trust evidence records,
  append-only hash chains, deterministic bundle exports, policy gates, and
  local trust score summaries.
- **Financial edge platform:** generic device types, capabilities, mock vendor
  drivers, multi-compartment inventory, simulators, generic commands,
  health/risk summaries, and trust-evidenced high-risk operations.
- **Operations:** Docker Compose, systemd packaging, hardening scripts, CI,
  Kubernetes Helm chart, Terraform scaffolding, and support documentation.
- **Square backend:** a separate TypeScript/Express service under `backend/api`
  processes Square webhooks and records merchant/drawer ledger state.

See [docs/architecture.md](docs/architecture.md), [docs/device_twin.md](docs/device_twin.md),
and [docs/fleet_management.md](docs/fleet_management.md) for deeper design notes.

## Native Build

```bash
cmake -S . -B build -DUSE_MOCK_GPIO=ON
cmake --build build -j
./build/register_mvp --api 8080
```

Use `-DUSE_MOCK_GPIO=OFF` on Linux hardware with `libgpiod-dev` installed.

Common runtime modes:

```bash
./build/register_mvp --demo-shutter
./build/register_mvp --dispense 4
./build/register_mvp --pos-http 9090
./build/register_mvp --api 8080 --tui
```

## Pilot Cloud Architecture

DrawerBackend remains a local hardware daemon first: dispensing, shutter,
hopper, scale, POS, and emergency service behavior continue to run on the
appliance. The pilot cloud layer adds a control-plane abstraction around device
twins so a cloud service can become the source of truth while the appliance
keeps a durable local JSON cache for offline operation.

Pilot device twins now carry deployment metadata (`device_id`, `merchant_id`,
`region`, `environment`, and `deployment_channel`), sync state
(`sync_status`, `last_synced_at`, local/remote revisions, and conflict flags),
and international inventory metadata. Disabled or revoked devices are prevented
from submitting fleet updates, receiving local commands, or applying OTA
updates.

The production-integration layer adds an authenticated HTTP cloud contract for
twin push/pull, one-time enrollment token validation, certificate identity
lifecycle tracking, and OTA release eligibility APIs. The local JSON twin store
remains the offline cache and an offline sync queue preserves device updates
while the cloud endpoint is unavailable.

Generation 1 adds a centralized fleet-control plane for organizations,
merchants, branches, regions, device registry, heartbeats, remote commands,
alerts, metrics, audit events, and dashboard views. The APIs are tenant-aware
through `X-Org-Id` and protected by the same fail-closed API authentication as
the existing appliance and fleet routes.

Generation 2 adds cash intelligence and reconciliation: every cash-affecting
event can be recorded in a durable ledger, reconciled against observed drawer
state, analyzed for denomination drift and deterministic depletion forecasts,
scored for cash health, and surfaced through tenant-aware cash dashboards.
Anomaly detection raises fleet alerts when a fleet control plane is attached.

Generation 3 adds VEIL trust evidence integration. Device, cash, command, OTA,
enrollment, reconciliation, anomaly, and certificate events can be represented
as portable trust evidence, chained locally, verified deterministically, exported
as evidence bundles, policy-checked through a clean VEIL client interface, and
summarized as local trust scores for VEIL ingestion.

Generation 4 adds a generalized financial edge abstraction for cash drawers,
smart safes, cash recyclers, teller stations, kiosks, and ATM-like terminals.
Devices register capabilities, execute generic commands through mock vendor
drivers, track multi-compartment inventory, simulate faults and telemetry, feed
health/risk scoring, and emit VEIL evidence for high-risk operations.

See [docs/device_enrollment.md](docs/device_enrollment.md),
[docs/international_deployment.md](docs/international_deployment.md),
[docs/ota_safety_model.md](docs/ota_safety_model.md), and
[docs/production_gaps.md](docs/production_gaps.md). Production integration
contracts are documented in [docs/cloud_sync_contract.md](docs/cloud_sync_contract.md),
[docs/production_enrollment_flow.md](docs/production_enrollment_flow.md),
[docs/ota_release_lifecycle.md](docs/ota_release_lifecycle.md), and
[docs/mtls_lifecycle.md](docs/mtls_lifecycle.md). Generation 1 fleet operations
are documented in [docs/fleet_control_plane.md](docs/fleet_control_plane.md).
Generation 2 cash operations are documented in
[docs/cash_intelligence.md](docs/cash_intelligence.md). Generation 3 trust
evidence integration is documented in
[docs/veil_trust_integration.md](docs/veil_trust_integration.md). Generation 4
financial edge platform behavior is documented in
[docs/financial_edge_platform.md](docs/financial_edge_platform.md).

## Fleet API

The local HTTP server exposes read-only fleet intelligence endpoints seeded with
the local drawer twin in development, persisted to a local JSON twin cache, and
prepared to sync through the cloud control-plane interface:

- `GET /fleet/devices`
- `GET /fleet/device/{id}`
- `GET /fleet/device/{id}/health`
- `GET /fleet/device/{id}/history`
- `GET /fleet/device/{id}/inventory`
- `GET /fleet/metrics`

These endpoints are additive to the existing `/txn`, `/command`, `/status`,
`/metrics`, `/metrics.json`, and `/help` routes.

Centralized fleet-control endpoints are exposed under `/fleet-control/v1/*` for
organizations, merchants, branches, regions, devices, commands, alerts, metrics,
and dashboard views.

Cash intelligence endpoints are exposed under `/cash/v1/*` for ledger entries,
denomination inventory, reconciliation reports, forecasts, anomalies, cash
health scores, and cash dashboard views.

Trust evidence endpoints are exposed under `/trust/v1/*` for evidence
submission, evidence listing, chain verification, deterministic bundle exports,
policy decision previews, and trust score summaries.

Financial edge endpoints are exposed under `/edge/v1/*` for device type
registration, capabilities, driver metadata, simulator actions, generic command
execution, inventory views, health/risk summaries, and audit events.

Production and non-loopback API binds require authentication. Set
`REGISTER_MVP_API_TOKEN` or `pos.key` and send `Authorization: Bearer <token>`
for protected endpoints. `/healthz` remains unauthenticated for local
healthchecks.

## Docker Compose

Dev profile:

```bash
docker compose --profile dev up -d
docker compose --profile dev down -v
```

Production-like docs profile:

```bash
docker compose --profile prod up docs_build
docker compose --profile prod up -d docs_prod
```

The Makefile wraps common flows:

```bash
make build
make test
make dev-up
make smoke
make compose-dev
make docs-prod-test
```

## Tests

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DUSE_MOCK_GPIO=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

New fleet and cash tests cover health scoring, predictive maintenance, inventory
forecasting, alert generation, device history, fleet API responses, cloud sync
state, enrollment metadata, conflict handling, and international inventory
validation, cash ledger persistence, denomination math, reconciliation
statuses, anomaly alerts, cash health explainability, dashboard aggregation, and
cash API authorization. VEIL trust tests cover the local client boundary,
evidence creation, hash chaining, tamper/missing-link/duplicate/out-of-order
detection, deterministic bundles, policy unavailable behavior, fail-closed
production gates, local dev allow behavior, trust score explainability, and
trust API authorization. Edge platform tests cover drawer compatibility, device
type registration, capability validation, unsupported command rejection, mock
driver execution, simulator behavior, multi-compartment inventory,
policy-gated commands, trust evidence generation, health/risk scoring, and edge
API authorization.

HTTP integration tests bind ephemeral ports on `127.0.0.1`; restricted
sandboxes that block local socket binding must run those tests with permission to
open localhost listeners. The tests do not require privileged ports, external
network access, real GPIO hardware, or host firewall configuration.

Real `.env` files and key/certificate material are intentionally ignored. Use
`.env.example` and `backend/api/.env.example` as templates.

TypeScript dependency hygiene for the Square webhook backend:

```bash
cd backend/api
npm run audit:pilot
npm run sbom
```

## Documentation

Operational and manufacturing docs live under `docs/`, including POS contracts,
provisioning, security hardening, release process, SLOs, compliance procedures,
installer guides, service manuals, RMA process, runbooks, and training material.
