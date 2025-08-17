#!/usr/bin/env python3
import sys, re, tarfile, tempfile, io, os

SECRET_RE = re.compile(r"(-----BEGIN [^-]+-----.*?-----END [^-]+-----|token\s*=\s*\S+|[A-Za-z0-9+/=]{32,})", re.DOTALL)

def redact(data: bytes) -> bytes:
    try:
        text = data.decode('utf-8')
    except UnicodeDecodeError:
        return data
    return SECRET_RE.sub('[REDACTED]', text).encode('utf-8')

path = sys.argv[1]
tmp = tempfile.NamedTemporaryFile(delete=False)
with tarfile.open(path, 'r:gz') as tar, tarfile.open(tmp.name, 'w:gz') as out:
    for m in tar.getmembers():
        f = tar.extractfile(m)
        content = f.read() if f else b''
        red = redact(content)
        m.size = len(red)
        out.addfile(m, io.BytesIO(red))
os.replace(tmp.name, path)

