#include "ledger.hpp"
#include <algorithm>

namespace quant {

int64_t investable_cents(const Snapshot& s) {
  return std::max<int64_t>(0, s.drawer_cents - s.pending_change_cents - s.reserve_floor_cents);
}

} // namespace quant
