#!/usr/bin/env bash
set -euo pipefail
helm template test ../charts/register-ops >/tmp/ops.yaml
grep -q "kind: Deployment" /tmp/ops.yaml
grep -q "kind: Service" /tmp/ops.yaml
