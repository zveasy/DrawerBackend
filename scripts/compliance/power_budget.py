#!/usr/bin/env python3
import csv, sys, math

if len(sys.argv) < 2:
    print('usage: power_budget.py loads.csv')
    sys.exit(1)

continuous_current = 0.0
peak_current = 0.0
continuous_power = 0.0
peak_power = 0.0

with open(sys.argv[1]) as f:
    rdr = csv.DictReader(f)
    for row in rdr:
        v = float(row['volts'])
        a = float(row['amps'])
        d = float(row.get('duty',1))
        continuous_current += a*d
        peak_current += a
        continuous_power += v*a*d
        peak_power += v*a

fuse = round(continuous_current*1.25,1)
print(f"Continuous power: {continuous_power:.1f}W")
print(f"Peak power: {peak_power:.1f}W")
print(f"Recommended fuse: {fuse:.1f}A")
print(f"TVS/MOV should handle >= {peak_power:.1f}W surges")
