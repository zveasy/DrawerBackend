# QuantEngine Publisher Bridge

This document describes the ZeroMQ publisher integration and the event schema emitted by the register.

## Transport

- ZMQ PUB socket, connected to configured endpoint `cfg.quant.endpoint` (e.g. `tcp://quant.local:5555`).
- Messages are sent as two frames: `[topic][payload]`.
  - `topic`: `cfg.quant.topic` (e.g. `register/stream`)
  - `payload`: JSON (UTF-8 encoded)

## Security

- Optional HMAC-SHA256 signature if `cfg.quant.hmac_key_hex` is set.
- Signature field `sig` is computed over the JSON string prior to adding `sig`, then inserted and re-serialized.

## Event: purchase

Example payload:
```json
{
  "type": "purchase",
  "version": 1,
  "client_id": "REG-CLIENT",
  "txn_id": "<uuid-or-mono-inc>",
  "idem_key": "<pos-idem-key>",
  "ts_ms": 1712712345678,
  "amount": {
    "price_cents": 1234,
    "deposit_cents": 2000,
    "change_cents": 766
  },
  "sig": "<optional-hex-hmac>"
}
```

### Field definitions

- `type`: Always `purchase` for completed transactions.
- `version`: Schema version. Currently `1`.
- `client_id`: From config `cfg.quant.client_id`.
- `txn_id`: Internal transaction id from `journal::Txn`.
- `idem_key`: POS-provided idempotency key.
- `ts_ms`: Unix epoch timestamp in milliseconds at publish time.
- `amount.price_cents`: Purchase total.
- `amount.deposit_cents`: Cash inserted.
- `amount.change_cents`: Change dispensed (can be `0`).
- `sig`: Optional HMAC-SHA256 hex over the JSON string without `sig`.

## Configuration

From `config/config.example.ini` `[quant]` section:

```
[quant]
enable=0
endpoint=tcp://quant.local:5555
client_id=REG-CLIENT
hmac_key_hex=
topic=register/stream
queue_dir=/var/lib/register-mvp/quantq
backoff_ms=200,400,800,1600,3000
heartbeat_seconds=15
reserve_floor_cents=2000
max_daily_outflow_cents=50000
min_step_change_cents=100
max_update_rate_hz=2
tolerance_cents=100
client_public=
client_secret=
server_public=
```

Notes:
- With `ENABLE_QUANT=OFF` at build time, the bridge uses a no-op publisher (messages are ignored).
- With `ENABLE_QUANT=ON`, a ZMQ publisher is created if `endpoint` and `topic` are non-empty.

## Implementation references

- Publisher interface: `src/quant/publisher.hpp`
- ZMQ/Null publisher: `src/quant/publisher.cpp`
- Wiring in runtime: `src/main.cpp`
- POS publish hook: `src/pos/router.cpp`
- Config parsing: `src/config/config.cpp`

## Open items

- Confirm whether additional fields are required by QuantEngine (e.g., ISO8601 timestamp, device serial, location, currency code).
- Confirm signature scheme and exact canonicalization expectations.
