#!/usr/bin/env python3
import argparse, requests, time, pathlib, sys

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--host', default='127.0.0.1')
    p.add_argument('--start', action='store_true')
    p.add_argument('--wait', action='store_true')
    p.add_argument('--download-report', default='')
    args = p.parse_args()
    base = f'http://{args.host}:8080'
    if args.start:
      requests.post(base+'/eol/start')
    if args.wait:
      while True:
        r = requests.get(base+'/eol/status').json()
        if not r.get('busy'): break
        time.sleep(0.1)
    if args.download_report:
      r = requests.get(base+'/eol/result')
      if r.status_code==200:
        outdir = pathlib.Path(args.download_report)
        outdir.mkdir(parents=True, exist_ok=True)
        (outdir/'result.json').write_text(r.text)
        print('downloaded', outdir/'result.json')

if __name__=='__main__':
    main()
