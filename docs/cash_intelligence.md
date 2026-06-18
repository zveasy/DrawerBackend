# Cash Intelligence And Reconciliation

Generation 2 adds a deterministic cash-operations layer for understanding cash
movement, reconciliation variance, shortage risk, and operational anomalies
across devices, branches, merchants, and regions.

## Ledger

The cash ledger records every cash-affecting event:

- transactions
- manual adjustments
- drawer open and close events
- cash in
- cash out
- starting and ending balances
- reconciliation events

Entries carry device, merchant, branch, region, currency, denomination
breakdowns, expected and observed balances, variance, optional operator
metadata, timestamps, and audit event IDs. Ledger data persists to the local
offline JSON cache.

## Reconciliation

The reconciliation engine compares expected cash against observed physical cash
for device, branch, merchant, or region scopes. Reports are persisted and can
produce these statuses:

- `balanced`
- `minor_variance`
- `major_variance`
- `unresolved`
- `suspected_loss`
- `suspected_fraud`

Findings include duplicate events, missing scoped events, stale inventory,
shortage, and unexplained drawer-open signals.

## Denominations And Forecasting

Inventory is tracked by currency and denomination. Currency codes are validated
as ISO-style three-letter uppercase codes, and configured denomination sets are
enforced where present. The service computes denomination drift and recommends a
greedy replenishment mix against configured denominations.

Forecasting uses deterministic hourly cash-movement baselines. It estimates
depletion timing, surplus timing, replenishment timing, shortage-risk windows,
and low/medium/high confidence labels without introducing ML dependencies.

## Anomalies And Alerts

Rule-based anomaly detection covers repeated small shortages, large one-time
variance, after-hours drawer activity, excessive manual adjustments, unusual
cash-out frequency, and suspicious cash movement. When attached to the fleet
control plane, detected anomalies create fleet alerts with warning or critical
severity.

## Health And Dashboards

Cash health scores include explainable factors for reconciliation variance,
stale inventory, anomaly severity, sync health, transaction failures, and
offline duration. Dashboard views expose current cash position, shortage risk,
surplus risk, variance trend, anomaly summary, and replenishment
recommendations.

## APIs

Cash APIs are exposed under `/cash/v1/*` and use the same fail-closed
authentication path as other protected appliance and fleet routes.

Key surfaces:

- `GET /cash/v1/ledger`
- `POST /cash/v1/ledger`
- `GET /cash/v1/inventory`
- `POST /cash/v1/inventory`
- `POST /cash/v1/reconcile`
- `GET /cash/v1/reconciliation`
- `POST /cash/v1/forecast`
- `GET /cash/v1/anomalies`
- `POST /cash/v1/anomalies/detect`
- `GET /cash/v1/health/{scope}/{id}`
- `GET /cash/v1/dashboard/{scope}/{id}`

Tenant filtering uses `X-Org-Id` or `organization_id` where device ownership is
available through the fleet registry.
