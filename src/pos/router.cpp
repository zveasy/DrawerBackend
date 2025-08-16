#include "router.hpp"
#include <chrono>

namespace pos {

Router::Router(TxnEngine &eng, IdempotencyStore &store)
    : eng_(eng), store_(store) {}

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

  auto json = success_json(txn);
  store_.put_done(req.idem_key, hash, json);
  return {200, json};
}

} // namespace pos

