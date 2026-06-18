# OTA Safety Model

The OTA agent keeps the existing local install backend and adds production
control-plane scaffolding through a manifest fetch interface.

## Manifest Requirements

Pilot update manifests include:

- `channel`: `dev`, `pilot`, or `stable`
- `version`
- `artifact_url`
- `sha256`
- optional staged rollout percentage
- optional revoked device list
- optional rollback protection metadata
- optional signature

When `require_signed=true`, the agent fails closed if the verification key is
missing or the manifest signature is invalid. Unsigned pilot or stable updates
must not be accepted in production.

## Channel and Rollout Rules

The appliance only accepts manifests for its configured channel. Unsupported
channels, wrong-channel manifests, stale versions, missing hashes, failed hash
checks, revoked devices, and staged rollout exclusions are rejected before
installation.

## Remaining Integration Work

The current fetch interface is ready for HTTPS or mTLS-backed update services,
but the production backend is not implemented in this repo. A commercial pilot
must connect this interface to the cloud release service, enforce artifact
signing, persist rollout decisions server-side, and define rollback policy for
failed boots.
