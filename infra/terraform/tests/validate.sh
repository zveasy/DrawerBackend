#!/usr/bin/env bash
set -euo pipefail
ROOT=$(git rev-parse --show-toplevel)/infra/terraform
terraform -chdir="$ROOT" init -backend=false >/dev/null
terraform -chdir="$ROOT" fmt -check
terraform -chdir="$ROOT" validate
for env in "$ROOT"/envs/*; do
  terraform -chdir="$env" init -backend=false >/dev/null
  terraform -chdir="$env" validate
done
