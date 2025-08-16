#include "change_maker.hpp"

ChangeMaker::ChangeMaker(const Inventory& inv) : inv_(inv) {}

std::vector<PlanItem> ChangeMaker::make_plan_cents(int change_cents, int& leftover_cents) const {
  std::vector<PlanItem> plan;
  leftover_cents = change_cents;
  const Denom order[] = {Denom::QUARTER, Denom::DIME, Denom::NICKEL, Denom::PENNY};
  for(Denom d : order){
    if(leftover_cents <= 0) break;
    if(!inv_.has(d)) continue;
    const auto& hs = inv_.at(d);
    int avail = hs.logical_count;
    if(avail <= 0) continue;
    int usable = avail;
    if(inv_.low(d)){
      usable = std::max(0, avail - hs.spec.low_stock_threshold);
    }
    if(usable <= 0) continue;
    int coin_val = cents(d);
    int need = leftover_cents / coin_val;
    if(need <= 0) continue;
    int use = std::min(need, usable);
    plan.push_back({d, use});
    leftover_cents -= use * coin_val;
  }
  return plan;
}

