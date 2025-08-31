# Square Webhooks → ZMQ Bridge

A minimal FastAPI service that receives Square webhooks and publishes `drawer_balance` events over ZMQ for QuantEngine.

## Endpoints

- `GET /health` – health probe
- `POST /square/webhooks` – Square webhook receiver (signature-verified unless disabled)

## Environment

- `SQUARE_ENV` – `sandbox` or `production` (informational)
- `SQUARE_WEBHOOK_SIGNATURE_KEY` – webhook signature key (required in prod)
- `SKIP_SIGNATURE_VERIFY` – set `1` to bypass signature verification in dev
- `ZMQ_ENDPOINT` – e.g. `tcp://localhost:5555`
- `ZMQ_TOPIC` – e.g. `register/stream`
- `CLIENT_ID` – client id included in payloads
- `HMAC_KEY_HEX` – optional HMAC secret to sign payloads
- `DEFAULT_RESERVE_FLOOR_CENTS` – defaults to `2000`
- `CURRENCY` – default currency code (`USD`)
- `LOG_LEVEL` – `INFO` (default), `DEBUG`, etc.
- `PORT` – container port (default via compose `${SQUARE_WEBHOOKS_PORT}`)

## Run (Docker Compose)

Ensure these variables exist in your `.env`:

```
SQUARE_WEBHOOKS_PORT=8085
ZMQ_ENDPOINT=tcp://quant.local:5555
ZMQ_TOPIC=register/stream
CLIENT_ID=REG-CLIENT
HMAC_KEY_HEX=
SQUARE_WEBHOOK_SIGNATURE_KEY=changeme
DEFAULT_RESERVE_FLOOR_CENTS=2000
LOG_LEVEL=INFO
```

Then:

```
docker compose up -d --build square_webhooks
```

## Payload handling

- Accepts Square `payments.created`/`updated` payloads.
- Uses `merchant_id` as `account_id`.
- Computes `delta_cents` as the payment `amount_money.amount`.
- Maintains an in-memory balance (replace with Redis/Postgres in production).
- Publishes a `drawer_balance` message as documented in `docs/api/QuantBridge.md`.

## Notes

- Replace in-memory `_BALANCES` with a persistent store for real deployments.
- Add OAuth flow and token storage to expand beyond webhooks.
