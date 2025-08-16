#!/usr/bin/env python3
import argparse, json, os, csv, datetime

parser = argparse.ArgumentParser(description="Log Surge/EFT results")
parser.add_argument('--generator', required=True)
parser.add_argument('--coupling', required=True)
parser.add_argument('--level', required=True)
parser.add_argument('--result', required=True)
parser.add_argument('--note', default="")
args = parser.parse_args()

dirp = os.path.join('data','compliance')
os.makedirs(dirp, exist_ok=True)
rec = {
    'ts': datetime.datetime.utcnow().isoformat()+'Z',
    'generator': args.generator,
    'coupling': args.coupling,
    'level': args.level,
    'result': args.result,
    'note': args.note
}
jsonl_path = os.path.join(dirp,'surge_log.jsonl')
with open(jsonl_path,'a') as f:
    f.write(json.dumps(rec)+'\n')
csv_path = os.path.join(dirp,'surge_log.csv')
write_header = not os.path.exists(csv_path)
with open(csv_path,'a',newline='') as f:
    w = csv.DictWriter(f, fieldnames=['ts','generator','coupling','level','result','note'])
    if write_header: w.writeheader()
    w.writerow(rec)
print('logged', rec['generator'], rec['result'])
