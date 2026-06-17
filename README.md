# DrawerBackend

DrawerBackend is a C++17 hardware backend and cloud-ready fleet platform for a
cash/change dispensing appliance. It controls local drawer hardware, exposes POS
and service APIs, reports telemetry, supports OTA updates, and now models each
physical drawer as a cloud-managed device twin.

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

## Fleet API

The local HTTP server exposes read-only fleet intelligence endpoints seeded with
the local drawer twin in development:

- `GET /fleet/devices`
- `GET /fleet/device/{id}`
- `GET /fleet/device/{id}/health`
- `GET /fleet/device/{id}/history`
- `GET /fleet/device/{id}/inventory`
- `GET /fleet/metrics`

These endpoints are additive to the existing `/txn`, `/command`, `/status`,
`/metrics`, `/metrics.json`, and `/help` routes.

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

New fleet tests cover health scoring, predictive maintenance, inventory
forecasting, alert generation, device history, and fleet API responses.

HTTP integration tests bind ephemeral ports on `127.0.0.1`; restricted
sandboxes that block local socket binding must run those tests with permission to
open localhost listeners. The tests do not require privileged ports, external
network access, real GPIO hardware, or host firewall configuration.

## Documentation

Operational and manufacturing docs live under `docs/`, including POS contracts,
provisioning, security hardening, release process, SLOs, compliance procedures,
installer guides, service manuals, RMA process, runbooks, and training material.
