#!/usr/bin/env python3
import sys, json, pathlib

def main():
    serial = sys.argv[1] if len(sys.argv) > 1 else "SERIAL"
    outdir = pathlib.Path('/var/lib/register-mvp/eol') / serial
    outdir.mkdir(parents=True, exist_ok=True)
    (outdir / 'label.png').write_bytes(b'placeholder')
    print(f'label for {serial} -> {outdir / "label.png"}')

if __name__ == '__main__':
    main()
