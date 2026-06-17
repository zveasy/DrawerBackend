# Production Enrollment Flow

Production enrollment binds a physical drawer identity to merchant and regional
deployment metadata through a one-time token.

## Token Policy

Enrollment tokens are rejected when they are:

- malformed
- unknown
- already used
- expired
- revoked
- bound to a different `device_id`

Valid tokens produce an enrolled drawer twin and persist the identity binding.

## Endpoint

`POST /control-plane/v1/enroll`

```json
{
  "device_id": "REG-001",
  "merchant_id": "merchant-123",
  "region": "KE-NBO",
  "environment": "pilot",
  "deployment_channel": "pilot",
  "enrollment_token": "TOKEN-..."
}
```

The response includes `{ "ok": true, "twin": ... }` on success. Failed
enrollments increment `register_enrollment_failures_total{reason}`.

## Remaining Integration

The repo now contains the production enrollment backend scaffolding and local
persistence model. A commercial deployment still needs secure token issuance,
operator workflows, mTLS binding after enrollment, and cloud tenant/RBAC policy.
