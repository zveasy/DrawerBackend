#!/usr/bin/env python3
import argparse, json, os, csv, datetime

parser = argparse.ArgumentParser(description="Log ESD test results")
parser.add_argument('--point', required=True)
parser.add_argument('--level', required=True)
parser.add_argument('--polarity', required=True)
parser.add_argument('--result', required=True)
parser.add_argument('--note', default="")
args = parser.parse_args()

dirp = os.path.join('data','compliance')
os.makedirs(dirp, exist_ok=True)
rec = {
    'ts': datetime.datetime.utcnow().isoformat()+'Z',
    'point': args.point,
    'level': args.level,
    'polarity': args.polarity,
    'result': args.result,
    'note': args.note
}
jsonl_path = os.path.join(dirp,'esd_log.jsonl')
with open(jsonl_path,'a') as f:
    f.write(json.dumps(rec)+'\n')
csv_path = os.path.join(dirp,'esd_log.csv')
write_header = not os.path.exists(csv_path)
with open(csv_path,'a',newline='') as f:
    w = csv.DictWriter(f, fieldnames=['ts','point','level','polarity','result','note'])
    if write_header: w.writeheader()
    w.writerow(rec)
print('logged', rec['point'], rec['result'])
