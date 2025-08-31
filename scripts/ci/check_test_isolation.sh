#!/usr/bin/env bash
set -euo pipefail

# Lightweight guardrail to discourage shared fixed paths in tests.
# Emits warnings if suspicious patterns are detected. Does not fail the job (exit 0).
# Patterns to flag: hardcoded shared dirs like "data/" or "idemdata" without TestCwd usage.

ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT_DIR/tests"

warn=0

# Find test files with suspicious path literals
mapfile -t files < <(grep -RIl --exclude-dir build --include "*.cpp" -e '"data/' -e "\"idemdata\"" || true)

for f in "${files[@]}"; do
  # Skip if file appears to use TestCwd helper
  if grep -q "#include \"test_fs.hpp\"" "$f" && grep -q "TestCwd" "$f"; then
    continue
  fi
  echo "[isolation-warning] $f uses a shared path literal (e.g., data/ or idemdata) without TestCwd." >&2
  warn=1
done

# Also flag direct std::filesystem::remove_all("data") usage
mapfile -t files_rm < <(grep -RIl --exclude-dir build --include "*.cpp" -e 'std::filesystem::remove_all\("data"\)' || true)
for f in "${files_rm[@]}"; do
  if grep -q "#include \"test_fs.hpp\"" "$f" && grep -q "TestCwd" "$f"; then
    continue
  fi
  echo "[isolation-warning] $f calls remove_all(\"data\") without TestCwd." >&2
  warn=1
done

if [[ $warn -eq 1 ]]; then
  echo "[isolation-summary] One or more tests may not be isolated. Please migrate to TestCwd." >&2
fi

exit $warn
