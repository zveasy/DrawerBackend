#pragma once
#include "denom.hpp"
#include <map>
#include <optional>

struct HopperStock {
  DenomSpec spec;
  int logical_count{0};                 // decremented on successful dispense
  std::optional<double> grams_per_raw;  // optional per-hopper scale slope
  std::optional<long>   tare_raw;       // optional
  bool enabled{true};
};

class Inventory {
public:
  // Insert or update hopper by denom
  void upsert(const HopperStock& hs);
  bool has(Denom d) const;
  HopperStock& at(Denom d);
  const HopperStock& at(Denom d) const;
  // Decrement (bounds-checked); returns actually decremented amount
  int take(Denom d, int n);
  // Add back (used for compensation on partials)
  void give_back(Denom d, int n);
  // Low-stock check
  bool low(Denom d) const;
  // Snapshot counts
  std::map<Denom,int> counts() const;
private:
  std::map<Denom, HopperStock> m_;
};

