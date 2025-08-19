#include "router.hpp"
#include <chrono>
#include "util/log.hpp"
#include "quant/publisher.hpp"

namespace pos {

Router::Router(TxnEngine &eng, IdempotencyStore &store, std::shared_ptr<quant::Publisher> pub)
    : eng_(eng), store_(store), publisher_(std::move(pub)) {}

std::pair<int, std::string> Router::handle(const PurchaseRequest &req) {
  auto hash = payload_hash(req.price_cents, req.deposit_cents);

  IdemRec rec;
  auto check = store_.check(req.idem_key, hash, &rec);
  switch (check) {
  case IdempotencyStore::Check::MISMATCH:
    return {409, mismatch_json()};
  case IdempotencyStore::Check::PENDING:
    return {202, pending_json()};
  case IdempotencyStore::Check::MATCH_DONE:
    return {200, rec.result_json};
  case IdempotencyStore::Check::NEW:
  default:
    break; // continue
  }

  // Global busy guard
  if (busy_.exchange(true)) {
    return {409, busy_json()};
  }

  store_.put_pending(req.idem_key, hash);
  auto txn = eng_.run_purchase(req.price_cents, req.deposit_cents);
  busy_.store(false);

  // Publish to QuantEngine (best-effort, non-blocking)
  if (publisher_) {
    try {
      publisher_->publish_purchase(txn, req.price_cents, req.deposit_cents, req.idem_key);
      LOG_INFO("quant_publish_ok", {{"idem", req.idem_key}});
    } catch (const std::exception &e) {
      LOG_WARN("quant_publish_err", {{"err", e.what()}});
    } catch (...) {
      LOG_WARN("quant_publish_err", {{"err", "unknown"}});
    }
  }

  auto json = success_json(txn);
  store_.put_done(req.idem_key, hash, json);
  return {200, json};
}

} // namespace pos

