# Device Twin

The Device Twin subsystem gives every physical drawer a persistent cloud
representation. It is implemented in `src/cloud/device_twin` and managed through
`src/cloud/fleet_manager`.

## DrawerTwin Model

A `DrawerTwin` contains:

- `drawer_id`
- `merchant_id`
- firmware version and target version
- hardware revision
- health score and health inputs
- denomination inventory levels
- fault history
- maintenance history
- connectivity status
- first-seen and last-telemetry timestamps
- complete audit timeline
- predictive maintenance assessment
- inventory forecast
- active alerts

Supporting models include `DrawerHealth`, `MaintenanceRecord`, `FaultEvent`,
`FirmwareState`, and `InventoryState`.

## Runtime Behavior

The local runtime seeds a `local-drawer` twin so development and support tooling
can query fleet endpoints without a remote cloud dependency. In production, the
same models can be populated from telemetry ingestion, Square/POS events, OTA
events, service mode actions, and support workflows.

## Audit Timeline

The twin history records transactions, maintenance actions, OTA updates, service
mode entries, faults, and inventory refills. Events use a stable `type`, a human
summary, a timestamp, and structured metadata for downstream analytics.
