#!/usr/bin/env python3
"""Sample POS integration plugin demonstrating idempotent retries.

This script is intentionally lightweight and meant for integration teams as a
starting point.  It posts purchase requests to the local register HTTP POS
endpoint and retries with exponential backoff and jitter while reusing the same
idempotency key.
"""

import argparse
import json
import os
import random
import time
import uuid
from pathlib import Path

try:
    import requests
except ImportError:  # pragma: no cover - best effort, used only for docs
    requests = None


def stable_key(order_id: str) -> str:
    """Return a stable UUID for an order stored on disk."""
    path = Path(f".idem-{order_id}")
    if path.exists():
        return path.read_text().strip()
    key = str(uuid.uuid4())
    path.write_text(key)
    return key


def make_body(mode: str, price: int, deposit: int, ticket: str):
    if mode == "B":
        return {
            "amount": {"price": price, "deposit": deposit},
            "meta": {"ticket": ticket},
        }
    return {"price_cents": price, "deposit_cents": deposit, "order_id": ticket}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("price", type=int)
    parser.add_argument("deposit", type=int)
    parser.add_argument("--mode", choices=["A", "B"], default="A")
    parser.add_argument("--ticket", default="demo-1")
    args = parser.parse_args()

    if requests is None:
        raise SystemExit("requests module not available")

    key = stable_key(args.ticket)
    body = make_body(args.mode, args.price, args.deposit, args.ticket)

    url = "http://127.0.0.1:9090/pos/purchase"
    headers = {"X-Idempotency-Key": key, "Content-Type": "application/json"}

    delay = 0.2
    for attempt in range(6):
        try:
            resp = requests.post(url, headers=headers, json=body, timeout=5)
        except Exception:
            resp = None
        if resp is not None and resp.status_code == 200:
            print(resp.json())
            return
        if resp is not None and resp.status_code in (400, 401, 409):
            print("permanent failure", resp.status_code, resp.text)
            return
        time.sleep(delay + random.uniform(0, 0.1))
        delay = min(delay * 2, 5)

    print("giving up after retries")


if __name__ == "__main__":
    main()

