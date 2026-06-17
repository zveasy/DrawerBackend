# Fleet Management

Fleet Management promotes DrawerBackend from a single appliance backend to a
cloud-managed fleet platform.

## Fleet Manager

`FleetManager` owns the current set of drawer twins and produces aggregate
metrics:

- total drawers
- online drawers
- unhealthy drawers
- firmware distribution
- average health score
- active alarms

The manager enriches every upserted twin with health scoring, predictive
maintenance, inventory forecasting, and alert generation before storing it.

## HTTP API

The existing device HTTP server exposes read-only fleet endpoints:

```text
GET /fleet/devices
GET /fleet/device/{id}
GET /fleet/device/{id}/health
GET /fleet/device/{id}/history
GET /fleet/device/{id}/inventory
GET /fleet/metrics
```

Unknown drawer IDs return `404` with `{"error":"device_not_found"}`.

## Alerts

Alerts are generated as `warning` or `critical` for:

- repeated jams
- communication failures
- low inventory
- firmware mismatch
- self-test failures

The fleet metrics endpoint counts active warning and critical alerts across all
drawers.

## Metrics

Calling `/fleet/metrics` also updates observability gauges:

- `register_fleet_total_drawers`
- `register_fleet_online_drawers`
- `register_fleet_unhealthy_drawers`
- `register_fleet_average_health_score`
- `register_fleet_active_alarms`
