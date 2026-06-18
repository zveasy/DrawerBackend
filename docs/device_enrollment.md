# Device Enrollment Flow

DrawerBackend now separates local device identity from fleet membership. A
drawer twin includes:

- `device_id`
- `drawer_id`
- `merchant_id`
- `region`
- `environment`
- `deployment_channel`
- enrollment state
- disabled/revocation state

## Pilot Flow

1. Manufacturing or support provisions the local drawer with a stable
   `device_id` and hardware identity.
2. The operator assigns `merchant_id`, `region`, `environment`, and
   `deployment_channel`.
3. The appliance submits an enrollment request to the control-plane interface
   with an enrollment token or signed request.
4. The remote control plane returns the authoritative drawer twin metadata and
   initial revision.
5. The appliance stores that twin in the durable local JSON cache.

The current implementation provides the interface and local enforcement hooks.
The production enrollment backend, mTLS/device certificate binding, and token
issuance service remain external integration work.

## Revocation

The control plane can mark a device disabled. Disabled devices are not allowed
to submit fleet updates, receive local `/txn` or `/command` actions, or apply
OTA updates when the device is listed in an update manifest revocation set.

## Security Expectations

Production enrollment should use short-lived enrollment tokens or signed
requests tied to manufacturing records. Tokens must not be logged or persisted
in plaintext. Device certificates should become the long-lived identity after
enrollment.
