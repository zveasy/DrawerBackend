#!/usr/bin/env python3
import argparse, csv, sys

parser = argparse.ArgumentParser(description="Validate pre-scan CSV logs")
parser.add_argument('files', nargs='+')
args = parser.parse_args()

required = {'mode','frequency_MHz','peak_dBuV'}

ok = True
for path in args.files:
    with open(path) as f:
        rdr = csv.DictReader(f)
        if not required.issubset(rdr.fieldnames or []):
            print(f"missing columns in {path}", file=sys.stderr)
            ok = False
            continue
        modes = {row['mode'] for row in rdr}
        if not {'EMI_WORST','EMI_IDLE'}.issubset(modes):
            print(f"missing required modes in {path}", file=sys.stderr)
            ok = False

if not ok:
    sys.exit(1)
