# POS Connector

The POS connector exposes a simple webhook for driving purchases from a point of sale system.
It listens on an HTTP interface (default `127.0.0.1:9090`) and forwards transactions to the
internal `TxnEngine`.

## Endpoints

### `POST /purchase`
Request body:
```json
{"price":735,"deposit":1000,"id":"POS-123"}
```
Response:
```json
{"status":"OK","id":"txn-...","quarters":10,"change":265}
```
Errors:
- `409 {"error":"busy"}` – a transaction is already in progress.
- `400 {"error":"bad_request"}` – JSON was invalid or missing fields.
- `401 {"error":"unauthorized"}` – shared key header mismatch.

### `GET /ping`
Returns basic health information:
```json
{"pong":true,"version":"1.0-s12"}
```

## Security

If a shared secret is configured, clients must send `X-Pos-Key: <key>`.
Requests with a missing or wrong key are rejected with `401`.

## Example
```bash
curl -s -H 'Content-Type: application/json' \
     -d '{"price":735,"deposit":1000}' \
     http://127.0.0.1:9090/purchase
```

## Troubleshooting
- `409 busy` – another purchase is currently running.
- `400 bad_request` – the JSON payload could not be parsed.
- `401 unauthorized` – the `X-Pos-Key` header did not match the configured key.
