# POS Contracts (Sprint 18)

This document describes the finalised POS purchase contracts exposed by the
`register_mvp` project.  Two transport mechanisms are supported: HTTP and a
light‑weight serial line protocol.  Both make use of **idempotency keys** to
guard against double dispensing.

## HTTP Webhook

* Endpoint: `POST /pos/purchase`
* Headers:
  * `Content-Type: application/json`
  * `X-Idempotency-Key: <uuid>` – required.  Reuse the same value when retrying.
  * `X-Pos-Key` – optional shared secret configured on the device.
* Body shapes:
  * **VendorA** – `{"price_cents":735,"deposit_cents":1000,"order_id":"A-1"}`
  * **VendorB** – `{"amount":{"price":735,"deposit":1000},"meta":{"ticket":"B-1"}}`
* Success `200` – `{"status":"OK","txn_id":"...","change_cents":265,"coins":{"quarter":11,"dime":0,"nickel":0,"penny":0}}`
* Busy `409` – `{"error":"busy"}`
* Bad request `400` – `{"error":"bad_request"}`
* Idempotency mismatch `409` – `{"error":"idempotency_mismatch"}`
* Pending `202` – `{"status":"processing"}` when a duplicate arrives while the
  first request is still executing.

## Serial Line Protocol

Each request is a single ASCII line terminated by a newline:

```
REQ id=<key> price=<cents> deposit=<cents>
```

The device responds with one of:

```
OK id=<key> change=<cents> q=<n> d=<n> n=<n> p=<n>
BUSY id=<key>
PENDING id=<key>
BAD id=<key>
CONFLICT id=<key>
```

Whitespace between fields is flexible and fields may appear in any order.

## Retry Guidance

Clients should perform exponential backoff with jitter when receiving transient
errors (5xx or `202`).  A typical sequence is 200ms, 400ms, 800ms, … up to a
maximum delay of 5s and an overall retry window below 30s.  Always reuse the
same idempotency key when retrying.  A `409 idempotency_mismatch` indicates an
operator error and should not be retried.

