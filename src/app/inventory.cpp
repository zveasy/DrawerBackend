#include "inventory.hpp"

void Inventory::upsert(const HopperStock& hs){
  m_[hs.spec.denom] = hs;
}

bool Inventory::has(Denom d) const{
  return m_.find(d) != m_.end();
}

HopperStock& Inventory::at(Denom d){
  return m_.at(d);
}

const HopperStock& Inventory::at(Denom d) const{
  return m_.at(d);
}

int Inventory::take(Denom d, int n){
  auto& hs = m_.at(d);
  int dec = n;
  if(dec > hs.logical_count) dec = hs.logical_count;
  hs.logical_count -= dec;
  return dec;
}

void Inventory::give_back(Denom d, int n){
  m_.at(d).logical_count += n;
}

bool Inventory::low(Denom d) const{
  auto it = m_.find(d);
  if(it==m_.end()) return true;
  return it->second.logical_count <= it->second.spec.low_stock_threshold;
}

std::map<Denom,int> Inventory::counts() const{
  std::map<Denom,int> out;
  for(const auto& kv : m_) out[kv.first] = kv.second.logical_count;
  return out;
}

