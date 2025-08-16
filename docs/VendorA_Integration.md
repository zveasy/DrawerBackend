# VendorA Integration Guide

VendorA uses a flat JSON object when invoking the register.  Field mapping:

| VendorA field | Description | Internal field |
|---------------|-------------|----------------|
| `price_cents` | Purchase price in cents | `price` |
| `deposit_cents` | Customer deposit in cents | `deposit` |
| `order_id` | Optional order identifier used to derive a default idempotency key | – |

### Example Request

```
POST /pos/purchase
X-Idempotency-Key: 123e4567-e89b-12d3-a456-426614174000
Content-Type: application/json

{"price_cents":735,"deposit_cents":1000,"order_id":"A-123"}
```

### Example Success Response

```
{"status":"OK","txn_id":"txn-1","change_cents":265,"coins":{"quarter":11,"dime":0,"nickel":0,"penny":0}}
```

Refer to `integrations/pos/sample_plugin.py` for sample client code with retry
logic and idempotency key handling.

