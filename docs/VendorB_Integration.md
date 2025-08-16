# VendorB Integration Guide

VendorB wraps monetary values in an `amount` object and passes additional
metadata via a `meta` object.  Fields may be provided as strings or numbers.

| VendorB path | Description | Internal field |
|--------------|-------------|----------------|
| `amount.price` | Purchase price in cents | `price` |
| `amount.deposit` | Customer deposit in cents | `deposit` |
| `meta.ticket` | Ticket identifier used to derive a default idempotency key | – |

### Example Request

```
POST /pos/purchase
X-Idempotency-Key: 123e4567-e89b-12d3-a456-426614174000
Content-Type: application/json

{"amount":{"price":735,"deposit":1000},"meta":{"ticket":"B-456"}}
```

### Example Success Response

```
{"status":"OK","txn_id":"txn-1","change_cents":265,"coins":{"quarter":11,"dime":0,"nickel":0,"penny":0}}
```

Refer to `integrations/pos/sample_plugin.py` for a small Python example client.

