#!/usr/bin/env python3
import json, csv, pathlib, sys

def main():
    out = csv.writer(sys.stdout)
    out.writerow(['serial','pass','delta_g'])
    for p in pathlib.Path(sys.argv[1]).glob('*/result.json'):
        data = json.loads(p.read_text())
        out.writerow([data.get('serial'), data.get('pass'), data.get('delta_g')])

if __name__=='__main__':
    main()
