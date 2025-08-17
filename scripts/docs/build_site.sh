#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
if command -v pandoc >/dev/null 2>&1; then
  mkdir -p "$ROOT/site"
  pandoc "$ROOT/docs/index.md" -o "$ROOT/site/index.html"
else
  echo "pandoc not installed; skipping" >&2
fi
