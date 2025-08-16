#pragma once
#include <string>
#include "../util/journal.hpp"

namespace pos {

// Normalized purchase request used internally by Router and adapters
struct PurchaseRequest {
  int price_cents{0};
  int deposit_cents{0};
  std::string idem_key; // caller supplied or derived
};

// Compute a stable hash for idempotency comparison.  For the purposes of
// testing we do not require a cryptographically strong hash – a simple
// concatenation is sufficient and deterministic.
std::string payload_hash(int price_cents, int deposit_cents);

// Build the canonical JSON response for a completed transaction.  The
// structure intentionally matches the contract documented in Sprint 18.
std::string success_json(const journal::Txn &t);

// Common error JSON helpers used by the HTTP and serial frontends.
std::string busy_json();
std::string bad_request_json();
std::string mismatch_json();
std::string pending_json();

} // namespace pos

