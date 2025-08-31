# Square POS Integration (Adapter Skeleton)

This directory provides a lightweight adapter to integrate DrawerBackend with Square.
It demonstrates how to translate the register's purchase request into a Square
Payment using Square's REST API.

Status: skeleton for integration teams. Production deployments should add robust
error handling, logging, retries, and secrets management.

## Quick start (local test)

Requirements:
- Python 3.9+
- `requests` (`pip install requests`)
- Square access token and location ID

Environment:
- `SQUARE_ACCESS_TOKEN` (required)
- `SQUARE_LOCATION_ID` (required)
- `SQUARE_ENV` (optional: `sandbox`|`production`, default: `sandbox`)

Usage:
```
python3 square_adapter.py --price 500 --deposit 0 --order-id demo-1
```

This will create a Payment for $5.00 in Square (in sandbox by default) with a
stable idempotency key derived from the `order-id` and persist it so retries are
safe.

## Mapping

- DrawerBackend request (example):
  - Minimal schema aligned with `integrations/pos/sample_plugin.py` `make_body(mode="A")`:
    - `price_cents`: integer total price in cents
    - `deposit_cents`: integer deposit in cents (if applicable)
    - `order_id`: string unique order identifier
- Square Payment request:
  - `amount_money.amount`: `price_cents` (or price minus deposit if applicable)
  - `idempotency_key`: stable per `order_id`
  - `location_id`: from env

Notes:
- Taxes/fees/line items are out of scope for this skeleton and can be added by
  extending the adapter.
- This adapter uses Square's Payments API endpoint `/v2/payments`.

## Security

- Do NOT hardcode tokens. Use environment variables or your secret store.
- Consider rotating tokens and scoping minimum permissions in Square Developer Portal.

## Next steps

- Add application logging and structured error reporting
- Expand payload mapping to include items, taxes, customer data
- Add robust retry and backoff with rate-limit handling
- Add webhook handling for Square payment updates (optional)
