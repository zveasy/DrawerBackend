#pragma once
#include <atomic>
#include <utility>
#include "contracts.hpp"
#include "idempotency_store.hpp"
#include "../app/txn_engine.hpp"

namespace pos {

// Router glues adapters/frontends to the TxnEngine while enforcing
// idempotency semantics and the global busy guard from earlier sprints.
class Router {
public:
  Router(TxnEngine &eng, IdempotencyStore &store);

  // Process a normalized purchase request.  Returns pair of HTTP-like status
  // code and JSON body.
  std::pair<int, std::string> handle(const PurchaseRequest &req);

private:
  TxnEngine &eng_;
  IdempotencyStore &store_;
  std::atomic<bool> busy_{false};
};

} // namespace pos

