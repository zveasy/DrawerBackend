# Cloud Sync Contract

The pilot production integration adds an authenticated HTTP contract for device
twin synchronization. The appliance can keep using the local JSON twin cache
while `HttpDeviceTwinClient` pushes and pulls authoritative state from a cloud
control plane.

## Endpoints

- `POST /control-plane/v1/twins/{drawer_id}`
- `GET /control-plane/v1/twins/{drawer_id}`
- `GET /control-plane/v1/devices/{device_id}/status`

Requests use `Authorization: Bearer <token>`. Missing or invalid credentials are
rejected with `401`.

## Push Contract

Push requests send:

```json
{
  "revision": 6,
  "twin": {}
}
```

The server accepts newer revisions, returns `409` with the remote twin on
revision conflict, and returns `423` for disabled devices. Conflicts are not
auto-merged in the appliance. They are marked on the local twin for operator or
cloud-side resolution.

## Offline Queue

When the cloud endpoint is unavailable or returns a transient sync failure, the
HTTP client stores the twin update in a durable offline queue. `flush_offline_queue`
replays queued updates in order once connectivity returns.

## Metrics

- `register_cloud_sync_success_total`
- `register_cloud_sync_failures_total{reason}`
- `register_cloud_sync_conflicts_total`
- `register_revoked_device_attempts_total`
