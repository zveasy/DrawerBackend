#include "contracts.hpp"
#include <sstream>

namespace pos {

std::string payload_hash(int price_cents, int deposit_cents) {
  // Simple deterministic hash – adequate for tests.  Real implementation
  // would use SHA-256 of a canonical JSON representation.
  return std::to_string(price_cents) + ":" + std::to_string(deposit_cents);
}

std::string success_json(const journal::Txn &t) {
  std::ostringstream oss;
  oss << "{\"status\":\"OK\",\"txn_id\":\"" << t.id << "\""
      << ",\"change_cents\":" << t.change
      << ",\"coins\":{\"quarter\":" << t.quarters
      << ",\"dime\":0,\"nickel\":0,\"penny\":0}}";
  return oss.str();
}

std::string busy_json() { return "{\"error\":\"busy\"}"; }
std::string bad_request_json() { return "{\"error\":\"bad_request\"}"; }
std::string mismatch_json() {
  return "{\"error\":\"idempotency_mismatch\"}";
}
std::string pending_json() { return "{\"status\":\"processing\"}"; }

} // namespace pos

