import os
import sys
from pathlib import Path
import importlib
import types

from fastapi.testclient import TestClient


def setup_module(module):
    # Ensure local service root is importable as a package
    service_root = Path(__file__).resolve().parents[1]
    if str(service_root) not in sys.path:
        sys.path.insert(0, str(service_root))
    # Ensure signature verification is disabled for tests
    os.environ["SKIP_SIGNATURE_VERIFY"] = "1"
    # Default envs for publisher
    os.environ.setdefault("CLIENT_ID", "TEST-CLIENT")
    os.environ.setdefault("ZMQ_ENDPOINT", "tcp://localhost:5555")
    os.environ.setdefault("ZMQ_TOPIC", "register/stream")


def test_payment_created_publishes_drawer_balance(monkeypatch):
    # Import app fresh to reset globals
    main = importlib.import_module("app.main")
    importlib.reload(main)

    # Attach a dummy publisher
    events = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            events.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())

    client = TestClient(main.app)

    payload = {
        "merchant_id": "M123",
        "type": "payments.created",
        "event_id": "evt_001",
        "data": {
            "object": {
                "payment": {
                    "id": "pay_abc",
                    "amount_money": {"amount": 1500, "currency": "USD"},
                }
            }
        },
    }

    resp = client.post("/square/webhooks", json=payload)
    assert resp.status_code == 200
    assert resp.json().get("ok") is True

    # Verify one event was published
    assert len(events) == 1
    evt = events[0]
    assert evt["account_id"] == "M123"
    assert evt["delta_cents"] == 1500
    assert evt["balance_cents"] == 1500
    assert evt["idem_key"] == "evt_001"
    assert evt["currency"] == "USD"


def test_ignored_event_without_amount(monkeypatch):
    main = importlib.import_module("app.main")
    importlib.reload(main)

    # Dummy publisher to capture any unintended publishes
    events = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            events.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())

    client = TestClient(main.app)

    payload = {
        "merchant_id": "M123",
        "type": "some.other.event",
        "event_id": "evt_002",
        "data": {"object": {"foo": "bar"}},
    }

    resp = client.post("/square/webhooks", json=payload)
    assert resp.status_code == 200
    assert resp.json().get("ok") is True
    # Should not publish any event
    assert events == []


def test_sweep_percent_applied(monkeypatch):
    # Ensure sweep is 50%
    os.environ["SWEEP_PERCENT"] = "50"
    os.environ["SKIP_SIGNATURE_VERIFY"] = "1"

    main = importlib.import_module("app.main")
    importlib.reload(main)

    events = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            events.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())

    client = TestClient(main.app)
    payload = {
        "merchant_id": "M456",
        "type": "payments.created",
        "event_id": "evt_003",
        "data": {
            "object": {
                "payment": {
                    "id": "pay_half",
                    "amount_money": {"amount": 1000, "currency": "USD"},
                }
            }
        },
    }
    resp = client.post("/square/webhooks", json=payload)
    assert resp.status_code == 200
    evt = events[0]
    assert evt["delta_cents"] == 500
    assert evt["balance_cents"] == 500


def test_signature_verification_passes(monkeypatch):
    # Enable signature verification
    os.environ["SKIP_SIGNATURE_VERIFY"] = "0"
    os.environ["SQUARE_WEBHOOK_SIGNATURE_KEY"] = "secretkey"

    main = importlib.import_module("app.main")
    importlib.reload(main)

    events = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            events.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())
    client = TestClient(main.app)

    # Build exact body and signature per implementation: HMAC over URL + body
    import hmac, hashlib, base64, json

    base = str(client.base_url).rstrip("/")
    url = f"{base}/square/webhooks"
    body_dict = {
        "merchant_id": "M789",
        "type": "payments.created",
        "event_id": "evt_004",
        "data": {"object": {"payment": {"id": "pay_sig", "amount_money": {"amount": 250, "currency": "USD"}}}},
    }
    body = json.dumps(body_dict, separators=(",", ":"))
    mac = hmac.new(b"secretkey", (url + body).encode("utf-8"), hashlib.sha256)
    sig = base64.b64encode(mac.digest()).decode("utf-8")

    resp = client.post(
        "/square/webhooks",
        data=body,
        headers={
            "Content-Type": "application/json",
            "x-square-hmacsha256-signature": sig,
        },
    )
    assert resp.status_code == 200
    assert events, "Expected event to be published"


def test_signature_verification_fails(monkeypatch):
    # Enable signature verification and ensure no fallbacks
    os.environ["SKIP_SIGNATURE_VERIFY"] = "0"
    os.environ["SQUARE_WEBHOOK_SIGNATURE_KEY"] = "secretkey"
    os.environ["ALLOW_SIGNATURE_FALLBACKS"] = "0"

    main = importlib.import_module("app.main")
    importlib.reload(main)

    events = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            events.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())
    client = TestClient(main.app)

    # Wrong signature
    import hmac, hashlib, base64, json
    base = str(client.base_url).rstrip("/")
    url = f"{base}/square/webhooks"
    body_dict = {
        "merchant_id": "M789",
        "type": "payments.created",
        "event_id": "evt_bad",
        "data": {"object": {"payment": {"id": "pay_sig2", "amount_money": {"amount": 100, "currency": "USD"}}}},
    }
    body = json.dumps(body_dict, separators=(",", ":"))
    mac = hmac.new(b"wrongkey", (url + body).encode("utf-8"), hashlib.sha256)
    sig = base64.b64encode(mac.digest()).decode("utf-8")

    resp = client.post(
        "/square/webhooks",
        data=body,
        headers={
            "Content-Type": "application/json",
            "x-square-hmacsha256-signature": sig,
        },
    )
    assert resp.status_code == 401
    assert events == []


def test_idempotency_dedup_in_memory(monkeypatch):
    # In-memory path
    os.environ["REDIS_URL"] = ""
    os.environ["SKIP_SIGNATURE_VERIFY"] = "1"

    main = importlib.import_module("app.main")
    importlib.reload(main)

    published = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            published.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())
    client = TestClient(main.app)

    payload = {
        "merchant_id": "MIDEMP",
        "type": "payments.created",
        "event_id": "evt_same",  # same event id twice
        "data": {
            "object": {
                "payment": {
                    "id": "pay_same",
                    "amount_money": {"amount": 200, "currency": "USD"},
                }
            }
        },
    }
    r1 = client.post("/square/webhooks", json=payload)
    r2 = client.post("/square/webhooks", json=payload)
    assert r1.status_code == 200 and r2.status_code == 200
    # Should only publish once
    assert len(published) == 1


def test_redis_path_monkeypatched(monkeypatch):
    # Force Redis path usage
    os.environ["REDIS_URL"] = "redis://dummy"  # non-empty to trigger path
    os.environ["SKIP_SIGNATURE_VERIFY"] = "1"
    os.environ["SWEEP_PERCENT"] = "100"

    main = importlib.import_module("app.main")
    importlib.reload(main)

    # Dummy async redis client
    balances = {}

    class DummyRedis:
        async def incrby(self, key, amount):
            balances[key] = balances.get(key, 0) + int(amount)
            return balances[key]

    monkeypatch.setattr(main, "redis_client", DummyRedis())

    events = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            events.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())

    client = TestClient(main.app)
    payload = {
        "merchant_id": "MREDIS",
        "type": "payments.created",
        "event_id": "evt_005",
        "data": {"object": {"payment": {"id": "pay1", "amount_money": {"amount": 300, "currency": "USD"}}}},
    }
    resp = client.post("/square/webhooks", json=payload)
    assert resp.status_code == 200
    # Ensure redis was used (balance stored in dummy balances)
    assert balances.get("balances:MREDIS") == 300
    assert events and events[0]["balance_cents"] == 300


def test_idempotency_dedup_with_redis(monkeypatch):
    # Use Redis path with mocked set() semantics for NX
    os.environ["REDIS_URL"] = "redis://dummy"
    os.environ["SKIP_SIGNATURE_VERIFY"] = "1"

    main = importlib.import_module("app.main")
    importlib.reload(main)

    class DummyRedis:
        def __init__(self):
            self.store = set()

        async def set(self, key, value, nx=False, ex=None):  # noqa: ARG002
            if nx:
                if key in self.store:
                    return False
                self.store.add(key)
                return True
            self.store.add(key)
            return True

        async def incrby(self, key, amount):
            # not used in this test
            return 0

    dummy = DummyRedis()
    monkeypatch.setattr(main, "redis_client", dummy)

    published = []

    class DummyPublisher:
        def publish_drawer_balance(self, **kwargs):
            published.append(kwargs)

    monkeypatch.setattr(main, "publisher", DummyPublisher())

    client = TestClient(main.app)
    payload = {
        "merchant_id": "MIDEMP2",
        "type": "payments.created",
        "event_id": "evt_same_redis",
        "data": {"object": {"payment": {"id": "pay_r", "amount_money": {"amount": 120, "currency": "USD"}}}},
    }

    r1 = client.post("/square/webhooks", json=payload)
    r2 = client.post("/square/webhooks", json=payload)
    assert r1.status_code == 200 and r2.status_code == 200
    # Should publish only once due to Redis NX
    assert len(published) == 1
