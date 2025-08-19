# Quant Bridge Runbook

This runbook outlines basic operation of the QuantEngine bridge.

## Overview

The register may optionally stream balance and transaction updates to a
QuantEngine instance via ZeroMQ.  Messages are signed with an optional
HMAC key and include idempotency keys for safe replay.

## Testing

A local development server or paper gateway can be used to verify
connectivity.  Ensure the `[quant]` section is configured in
`config.ini` and enable the feature with `--quant 1`.
