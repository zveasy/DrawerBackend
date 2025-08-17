#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
if command -v markdownlint >/dev/null 2>&1; then
  markdownlint "$ROOT/docs" "$ROOT/mkdocs.yml"
else
  echo "markdownlint not installed" >&2
fi
