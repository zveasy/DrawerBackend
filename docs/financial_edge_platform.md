# Financial Edge Device Platform

Generation 4 adds a generalized financial edge platform for drawers, smart
safes, cash recyclers, teller stations, kiosks, and ATM-like terminals.

## Device Abstraction

The edge model supports:

- `cash_drawer`
- `smart_safe`
- `cash_recycler`
- `teller_station`
- `kiosk`
- `atm_like_terminal`

Shared capabilities include inventory, cash-in, cash-out, open/close,
lock-control, sensor telemetry, command execution, firmware update, identity,
and evidence export. The existing drawer remains the first concrete device
profile through `cash_drawer`.

## Capabilities And Drivers

Each device registers its supported capabilities. Generic commands are mapped to
required capabilities and unsupported commands are rejected fail-closed. Driver
scaffolding is provided through a vendor-neutral interface plus deterministic
mock drivers for each supported device type. No real vendor SDKs are required.

## Inventory And Simulation

Inventory supports compartments, cassettes, drawers, vault sections, recycler
bins, and terminal cash slots with multi-currency denomination breakdowns.
Simulator actions can move devices online/offline, generate faults, update
sensor telemetry, and mutate inventory in deterministic test scenarios.

## Commands, Policy, And Evidence

Generic commands include lock, unlock, open, close, dispense cash, accept cash,
count cash, reconcile device, self-test, key rotation, evidence export, and
maintenance-mode transitions. High-risk commands are policy-gated through the
VEIL trust service when configured and generate trust evidence.

## Health And Risk

Health summaries include sensor state, compartment status, lock health,
fault indicators, maintenance state, command failure rate, and evidence-chain
health. Scores are exported to metrics and can feed fleet, cash, and trust
summaries.

## APIs

Edge APIs are exposed under `/edge/v1/*` and use the same fail-closed
authentication path as other protected routes.

Key surfaces:

- `GET /edge/v1/devices`
- `POST /edge/v1/devices`
- `GET /edge/v1/devices/{id}/capabilities`
- `POST /edge/v1/devices/{id}/capabilities`
- `GET /edge/v1/drivers`
- `POST /edge/v1/devices/{id}/commands`
- `GET /edge/v1/devices/{id}/inventory`
- `POST /edge/v1/devices/{id}/inventory`
- `POST /edge/v1/devices/{id}/simulate`
- `GET /edge/v1/devices/{id}/health`
- `GET /edge/v1/events`
