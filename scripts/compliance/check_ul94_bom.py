#!/usr/bin/env python3
import csv, sys

if len(sys.argv)<2:
    print('usage: check_ul94_bom.py BOM.csv')
    sys.exit(1)

ok = True
with open(sys.argv[1]) as f:
    rdr = csv.DictReader(f)
    for row in rdr:
        if row.get('Plastic','').lower() != 'true':
            continue
        rating = row.get('UL94_Rating','')
        desc = row.get('Description','').lower()
        if 'cover' in desc and rating not in ['V-0','V-1','V-2']:
            print(f"cover {row['RefDes']} insufficient rating {rating}")
            ok = False
        if 'airflow' in desc and rating != 'V-0':
            print(f"airflow part {row['RefDes']} requires V-0")
            ok = False
if not ok:
    sys.exit(1)
