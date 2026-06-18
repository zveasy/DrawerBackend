# Predictive Maintenance

Predictive maintenance is implemented under `src/cloud/analytics`.

## Health Score

The health score is a 0-100 score derived from weighted operational signals:

- jam counts
- dispense failures
- scale drift
- self-test failures
- communication outages
- hopper depletion frequency
- uptime degradation

Scores below 70 are considered unhealthy for fleet aggregate metrics.

## Failure Probability

The maintenance engine estimates `predicted_failure_probability` from:

- motor wear indicators, represented by motor cycle count
- jam frequency
- scale calibration drift
- transaction latency
- uptime percentage

The output includes:

- `predicted_failure_probability`
- `recommended_service_date`
- `maintenance_priority`
- contributing `drivers`

Priority is `low`, `warning`, or `critical`.

## Inventory Intelligence

Inventory forecasting tracks denomination quantities, capacities, and
consumption rates. It estimates `hours_until_empty`, produces refill
recommendations, and records depletion trend strings for each denomination with a
known consumption rate.

Low inventory can generate warning or critical alerts based on the hours
remaining before depletion.
