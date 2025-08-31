#!/usr/bin/env python3
"""Square POS adapter skeleton.

Translates DrawerBackend purchase intent into a Square Payment using the
Square Payments API. This is a minimal example intended for integration teams.

Env:
  - SQUARE_ACCESS_TOKEN (required)
  - SQUARE_LOCATION_ID (required)
  - SQUARE_ENV=sandbox|production (optional, default: sandbox)

Usage:
  python3 square_adapter.py --price 500 --deposit 0 --order-id demo-1

Notes:
  - Uses a stable on-disk idempotency key per order id.
  - Creates a CASH payment by default so no card nonce is required.
  - For cards, replace the CASH flow with a proper source_id (nonce) from
    Square's Web/POS SDK and remove cash_details.
"""

from __future__ import annotations

import argparse
import json
import os
import time
import uuid
from pathlib import Path
from typing import Optional

try:
    import requests
except ImportError:  # pragma: no cover
    requests = None


def stable_key(order_id: str) -> str:
    path = Path(f".idem-square-{order_id}")
    if path.exists():
        return path.read_text().strip()
    key = str(uuid.uuid4())
    path.write_text(key)
    return key


def square_base_url() -> str:
    env = os.getenv("SQUARE_ENV", "sandbox").lower()
    if env == "production":
        return "https://connect.squareup.com"
    return "https://connect.squareupsandbox.com"


def build_cash_payment_body(location_id: str, amount_cents: int, idempotency_key: str, note: str) -> dict:
    # CASH flow: source_id must be the literal string "CASH" and cash_details provided.
    return {
        "idempotency_key": idempotency_key,
        "location_id": location_id,
        "source_id": "CASH",
        "amount_money": {"amount": amount_cents, "currency": "USD"},
        "cash_details": {"buyer_supplied_money": {"amount": amount_cents, "currency": "USD"}},
        "note": note[:500],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--price", type=int, required=True, help="total price in cents")
    parser.add_argument("--deposit", type=int, default=0, help="deposit in cents (optional)")
    parser.add_argument("--order-id", required=True, help="unique order id")
    args = parser.parse_args()

    if requests is None:
        raise SystemExit("requests module not available; pip install requests")

    token = os.getenv("SQUARE_ACCESS_TOKEN")
    location_id = os.getenv("SQUARE_LOCATION_ID")
    if not token or not location_id:
        raise SystemExit("SQUARE_ACCESS_TOKEN and SQUARE_LOCATION_ID must be set in environment")

    # Map DrawerBackend request -> Square amount
    amount_cents = int(args.price)  # adjust if deposit should be subtracted
    idem_key = stable_key(args.order_id)

    body = build_cash_payment_body(location_id, amount_cents, idem_key, note=f"Order {args.order_id}")

    url = f"{square_base_url()}/v2/payments"
    headers = {
        "Content-Type": "application/json",
        "Accept": "application/json",
        "Authorization": f"Bearer {token}",
    }

    delay = 0.3
    for attempt in range(6):
        try:
            resp = requests.post(url, headers=headers, data=json.dumps(body), timeout=10)
        except Exception as e:  # pragma: no cover
            resp = None
        if resp is not None and resp.status_code in (200, 201):
            try:
                data = resp.json()
            except Exception:
                data = {"raw": resp.text}
            print(json.dumps(data))
            return
        # 400/401 are permanent (invalid request or auth), 409 could be idempotent conflict
        if resp is not None and resp.status_code in (400, 401):
            print(f"permanent failure {resp.status_code}: {resp.text}")
            return
        time.sleep(delay)
        delay = min(delay * 2, 5)

    print("giving up after retries")


if __name__ == "__main__":
    main()
