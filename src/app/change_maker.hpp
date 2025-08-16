#pragma once
#include "denom.hpp"
#include "inventory.hpp"
#include "multi_dispenser.hpp"
#include <vector>

// Compute an order-sensitive plan using greedy quarters->dimes->nickels->pennies,
// respecting inventory availability and low-stock thresholds.
class ChangeMaker {
public:
  explicit ChangeMaker(const Inventory& inv);
  // returns a plan limited by available coins; leftover_cents contains any remainder not coverable
  std::vector<PlanItem> make_plan_cents(int change_cents, int& leftover_cents) const;
private:
  const Inventory& inv_;
};

