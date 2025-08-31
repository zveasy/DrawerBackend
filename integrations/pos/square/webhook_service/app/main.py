import os
import hmac
import base64
import hashlib
import logging
from typing import Any, Dict, Optional

from fastapi import FastAPI, Request, Header, HTTPException, Response
from contextlib import asynccontextmanager
from pydantic import BaseModel

from .publisher import ZmqBalancePublisher
try:
    from prometheus_client import Counter, generate_latest, CONTENT_TYPE_LATEST  # type: ignore
except Exception:  # pragma: no cover - optional
    Counter = None  # type: ignore
    generate_latest = None  # type: ignore
    CONTENT_TYPE_LATEST = "text/plain"  # type: ignore

# Metrics
_metrics_enabled = Counter is not None
if _metrics_enabled:
    METRICS_PROCESSED = Counter("webhook_processed_total", "Processed webhook events")
    METRICS_IGNORED = Counter("webhook_ignored_total", "Ignored webhook events")
    METRICS_DUPLICATE = Counter("webhook_duplicate_total", "Duplicate webhook events")
    METRICS_PUBLISH_ERRORS = Counter("webhook_publish_error_total", "Publish errors")

LOG_LEVEL = os.environ.get("LOG_LEVEL", "INFO").upper()
logging.basicConfig(level=getattr(logging, LOG_LEVEL, logging.INFO))
logger = logging.getLogger("square_webhooks")

@asynccontextmanager
async def lifespan(app: FastAPI):
    # Startup
    global redis_client
    if REDIS_URL:
        try:
            import redis.asyncio as redis  # type: ignore

            redis_client = redis.from_url(REDIS_URL, decode_responses=True)
            logger.info("redis_enabled", extra={"url": REDIS_URL})
        except Exception as e:
            logger.warning("redis_init_failed", extra={"err": str(e)})
    yield
    # Shutdown: nothing special for now


app = FastAPI(title="Square Webhooks → ZMQ Bridge", lifespan=lifespan)

publisher = ZmqBalancePublisher()

# Optional Redis client for persistent balances; fallback to in-memory
REDIS_URL = os.environ.get("REDIS_URL", "").strip()
redis_client = None  # type: ignore

# Minimal, ephemeral balance store (replace with Redis/Postgres if available)
_BALANCES: Dict[str, int] = {}
_PROCESSED: set[str] = set()



class WebhookAck(BaseModel):
    ok: bool


def verify_square_signature(signature_key: str, request_url: str, raw_body: bytes, provided_sig: str) -> bool:
    """
    Square recommends computing an HMAC-SHA256 using the webhook signature key over the request URL + raw body,
    then base64-encoding the result. Consult Square docs for your API version for exact details.
    This implementation uses URL + body; set SKIP_SIGNATURE_VERIFY=1 to bypass in dev.
    """
    try:
        body_str = raw_body.decode("utf-8")
        key = signature_key.encode("utf-8")

        # Primary: URL + body
        mac1 = hmac.new(key, (request_url + body_str).encode("utf-8"), hashlib.sha256)
        expected1 = base64.b64encode(mac1.digest()).decode("utf-8")
        if hmac.compare_digest(expected1, provided_sig):
            return True

        allow_fallbacks = os.environ.get("ALLOW_SIGNATURE_FALLBACKS", "0") == "1"
        if allow_fallbacks:
            # Fallback: path + body (in case host/port normalization differs in proxy/test envs)
            try:
                from urllib.parse import urlparse

                path = urlparse(request_url).path
            except Exception:
                path = request_url
            mac2 = hmac.new(key, (path + body_str).encode("utf-8"), hashlib.sha256)
            expected2 = base64.b64encode(mac2.digest()).decode("utf-8")
            if hmac.compare_digest(expected2, provided_sig):
                return True

            # Last resort: body only (dev/testing convenience)
            mac3 = hmac.new(key, body_str.encode("utf-8"), hashlib.sha256)
            expected3 = base64.b64encode(mac3.digest()).decode("utf-8")
            if hmac.compare_digest(expected3, provided_sig):
                return True

        return False
    except Exception:
        return False


@app.get("/health", response_model=WebhookAck)
async def health() -> WebhookAck:
    return WebhookAck(ok=True)


@app.post("/square/webhooks", response_model=WebhookAck)
async def square_webhooks(
    request: Request,
    x_square_hmacsha256_signature: Optional[str] = Header(default=None),
) -> WebhookAck:
    # Read config
    signature_key = os.environ.get("SQUARE_WEBHOOK_SIGNATURE_KEY", "")
    skip_verify = os.environ.get("SKIP_SIGNATURE_VERIFY", "0") == "1"

    # Read raw body and JSON
    raw_body = await request.body()
    try:
        body = await request.json()
    except Exception as e:
        logger.warning("invalid_json", extra={"err": str(e)})
        raise HTTPException(status_code=400, detail="invalid json")

    # Verify signature when enabled
    if signature_key and not skip_verify:
        full_url = str(request.url)
        if not x_square_hmacsha256_signature or not verify_square_signature(signature_key, full_url, raw_body, x_square_hmacsha256_signature):
            logger.warning("bad_signature")
            raise HTTPException(status_code=401, detail="bad signature")

    # Extract merchant (account) id and amount
    merchant_id = body.get("merchant_id") or body.get("merchantId")
    if not merchant_id:
        logger.warning("missing_merchant_id")
        raise HTTPException(status_code=400, detail="missing merchant_id")

    data: Dict[str, Any] = body.get("data", {})
    obj: Dict[str, Any] = data.get("object", {})

    # Square payloads vary; payments are usually under object.payment
    payment = obj.get("payment") or obj.get("object") or obj
    amount_money = (payment or {}).get("amount_money") or (payment or {}).get("total_money") or {}

    amount = amount_money.get("amount")
    currency = amount_money.get("currency") or os.environ.get("CURRENCY", "USD")

    if amount is None:
        # If not a payment event we recognize, ack to avoid retries
        logger.info("ignored_event", extra={"topic": body.get("type")})
        if _metrics_enabled:
            METRICS_IGNORED.inc()
        return WebhookAck(ok=True)

    # Apply sweep rule (percentage of amount). Defaults to 100%.
    try:
        sweep_percent = int(os.environ.get("SWEEP_PERCENT", "100"))
    except ValueError:
        sweep_percent = 100
    if sweep_percent < 0:
        sweep_percent = 0
    if sweep_percent > 100:
        sweep_percent = 100
    delta_cents = int(int(amount) * sweep_percent / 100)

    # Update balance (Redis if configured, else in-memory)
    new_balance: int
    if REDIS_URL and redis_client is not None:
        try:
            # INCRBY returns the new value
            new_balance = int(await redis_client.incrby(f"balances:{merchant_id}", delta_cents))  # type: ignore[attr-defined]
        except Exception as e:
            logger.warning("redis_incr_failed", extra={"err": str(e)})
            current = _BALANCES.get(merchant_id, 0)
            new_balance = current + delta_cents
            _BALANCES[merchant_id] = new_balance
    else:
        current = _BALANCES.get(merchant_id, 0)
        new_balance = current + delta_cents
        _BALANCES[merchant_id] = new_balance

    # Idempotency: prefer event_id; fallback to payment id or hash
    idem_key = body.get("event_id") or body.get("eventId") or (payment or {}).get("id") or f"evt-{hash(raw_body)}"

    # Dedup before publishing
    if REDIS_URL and redis_client is not None:
        try:
            # Set a key only if it does not exist; expire after 1 day
            # Returns True if set, None/False if already exists
            set_ok = await redis_client.set(f"events:{idem_key}", "1", nx=True, ex=86400)  # type: ignore[attr-defined]
            if not set_ok:
                logger.info("duplicate_event", extra={"idem_key": idem_key})
                if _metrics_enabled:
                    METRICS_DUPLICATE.inc()
                return WebhookAck(ok=True)
        except Exception as e:
            logger.warning("redis_idem_failed", extra={"err": str(e)})
            if idem_key in _PROCESSED:
                logger.info("duplicate_event_mem", extra={"idem_key": idem_key})
                if _metrics_enabled:
                    METRICS_DUPLICATE.inc()
                return WebhookAck(ok=True)
            _PROCESSED.add(idem_key)
    else:
        if idem_key in _PROCESSED:
            logger.info("duplicate_event_mem", extra={"idem_key": idem_key})
            if _metrics_enabled:
                METRICS_DUPLICATE.inc()
            return WebhookAck(ok=True)
        _PROCESSED.add(idem_key)

    try:
        publisher.publish_drawer_balance(
            account_id=merchant_id,
            balance_cents=new_balance,
            delta_cents=delta_cents,
            idem_key=idem_key,
            pending_change_cents=0,
            reserve_floor_cents=None,
            currency=currency,
        )
        logger.info("published_drawer_balance", extra={"merchant_id": merchant_id, "delta": delta_cents, "balance": new_balance})
        if _metrics_enabled:
            METRICS_PROCESSED.inc()
    except Exception as e:
        # Best effort: log but still ack to avoid webhook retries storm
        logger.exception("publish_error", extra={"err": str(e)})
        if _metrics_enabled:
            METRICS_PUBLISH_ERRORS.inc()

    return WebhookAck(ok=True)


@app.get("/metrics")
async def metrics():  # type: ignore[no-untyped-def]
    if not _metrics_enabled:
        raise HTTPException(status_code=404, detail="metrics disabled")
    data = generate_latest()  # type: ignore[call-arg]
    return Response(content=data, media_type=CONTENT_TYPE_LATEST)
