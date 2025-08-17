# Local API

The device exposes a REST API on `http://127.0.0.1:8080`.

## Routes
- `POST /txn` – body: `{ "price":735, "deposit":1000 }`
  - Returns transaction result with coin counts.
- `GET /status` – current state and last transaction.
- `POST /command` – guarded; open/close/dispense commands.

All requests are idempotent; retry with exponential backoff when receiving
network errors. Errors return JSON `{ "error":"..." }` with HTTP codes.

