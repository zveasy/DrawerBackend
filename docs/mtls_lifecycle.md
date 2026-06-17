# mTLS Lifecycle

Pilot production integration adds certificate identity lifecycle scaffolding.
The model tracks:

- `device_id`
- certificate SHA-256 fingerprint
- state: `active`, `pending`, `expired`, or `revoked`
- issue and expiry timestamps
- rotation source fingerprint

## Access Policy

`active` and `pending` identities are allowed. `expired`, `revoked`, and unknown
fingerprints are blocked and increment `register_cert_identity_rejects_total`.

## Rotation

During rotation the new certificate is stored as `pending` with
`rotated_from_fingerprint` pointing at the active certificate. Promotion makes
the pending certificate active and marks the previous fingerprint expired.

## Remaining Integration

The lifecycle store does not yet terminate TLS itself. Production deployment
should connect this state to the ingress/proxy or mTLS verifier that extracts the
peer certificate fingerprint.
