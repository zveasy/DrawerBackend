#include "multi_dispenser.hpp"

MultiDispenser::MultiDispenser(Inventory& inv) : inv_(inv) {}

void MultiDispenser::attach(const HopperHandle& h){
  hoppers_[h.spec.denom] = h;
}

MultiDispenseStats MultiDispenser::execute(const DispensePlan& plan){
  MultiDispenseStats st{true, "OK", 0, 0, plan, {}};
  for(const auto& item : plan){
    auto it = hoppers_.find(item.denom);
    if(it==hoppers_.end()){
      st.ok = false;
      st.reason = "NOHOP";
      st.fulfilled.push_back({item.denom,0});
      continue;
    }
    auto* drv = it->second.driver;
    if(!drv){
      st.ok = false;
      st.reason = "NOHOP";
      st.fulfilled.push_back({item.denom,0});
      continue;
    }
    auto res = drv->dispenseCoins(item.count);
    int dec = inv_.take(item.denom, res.dispensed);
    (void)dec; // dec same as res.dispensed but bounds checked
    st.pulses += res.pulses;
    st.coins  += res.dispensed;
    st.fulfilled.push_back({item.denom, res.dispensed});
    if(res.dispensed < item.count || !res.ok){
      st.ok = false;
      if(!res.reason.empty()) st.reason = res.reason;
      else st.reason = res.dispensed < item.count ? "PARTIAL" : "";
    }
  }
  if(st.ok) st.reason = "OK";
  else if(st.reason.empty()) st.reason = "PARTIAL";
  return st;
}

