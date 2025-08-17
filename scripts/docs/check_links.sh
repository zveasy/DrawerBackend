#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BROKEN=0
while IFS= read -r file; do
  DIR=$(dirname "$file")
  grep -oE '\[[^]]*\]\([^)]*\)' "$file" | while read -r m; do
    link=$(echo "$m" | sed -E 's/.*\(([^)]*)\)/\1/')
    [[ -z "$link" ]] && continue
    if [[ "$link" =~ ^http ]]; then
      if command -v curl >/dev/null 2>&1; then
        curl -Lsf "$link" >/dev/null || echo "Broken external $link in $file"
      fi
    else
      [ -f "$DIR/$link" ] || { echo "Broken link $link in $file"; BROKEN=1; }
    fi
  done
done < <(find "$ROOT/docs" -name '*.md')
exit $BROKEN
