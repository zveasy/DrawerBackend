import os
import time
import hmac
import json
import hashlib
import zmq
from typing import Optional

class ZmqBalancePublisher:
    def __init__(self):
        self.endpoint = os.environ.get("ZMQ_ENDPOINT", "tcp://localhost:5555")
        self.topic = os.environ.get("ZMQ_TOPIC", "register/stream")
        self.client_id = os.environ.get("CLIENT_ID", "REG-CLIENT")
        self.hmac_key_hex = os.environ.get("HMAC_KEY_HEX", "").strip()
        self.currency_default = os.environ.get("CURRENCY", "USD")
        self.reserve_floor_cents = int(os.environ.get("DEFAULT_RESERVE_FLOOR_CENTS", "2000"))
        self._ctx = zmq.Context.instance()
        self._sock = self._ctx.socket(zmq.PUB)
        self._sock.connect(self.endpoint)

    def _hmac(self, payload: str) -> Optional[str]:
        if not self.hmac_key_hex:
            return None
        key = bytes.fromhex(self.hmac_key_hex)
        sig = hmac.new(key, payload.encode("utf-8"), hashlib.sha256).hexdigest()
        return sig

    def publish_drawer_balance(
        self,
        account_id: str,
        balance_cents: int,
        delta_cents: int,
        idem_key: str,
        pending_change_cents: int = 0,
        reserve_floor_cents: Optional[int] = None,
        currency: Optional[str] = None,
    ) -> None:
        j = {
            "type": "drawer_balance",
            "version": 1,
            "client_id": self.client_id,
            "account_id": account_id,
            "ts_ms": int(time.time() * 1000),
            "currency": currency or self.currency_default,
            "balance_cents": int(balance_cents),
            "delta_cents": int(delta_cents),
            "idem_key": idem_key,
            "amounts": {
                "pending_change_cents": int(pending_change_cents),
                "reserve_floor_cents": int(reserve_floor_cents if reserve_floor_cents is not None else self.reserve_floor_cents),
            },
        }
        payload = json.dumps(j, separators=(",", ":"))
        sig = self._hmac(payload)
        if sig:
            j["sig"] = sig
            payload = json.dumps(j, separators=(",", ":"))
        # send multipart [topic][payload]
        self._sock.send_multipart([self.topic.encode("utf-8"), payload.encode("utf-8")], flags=zmq.DONTWAIT)
