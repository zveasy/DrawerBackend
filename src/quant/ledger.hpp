#pragma once
#include <cstdint>

namespace quant {

struct Snapshot {
  int64_t drawer_cents{0};
  int64_t pending_change_cents{0};
  int64_t reserve_floor_cents{0};
};

// Returns investable cents ensuring non-negative.
int64_t investable_cents(const Snapshot& s);

} // namespace quant
