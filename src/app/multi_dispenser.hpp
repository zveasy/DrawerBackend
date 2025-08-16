#pragma once
#include <map>
#include <vector>
#include <string>
#include "denom.hpp"
#include "inventory.hpp"
#include "idispenser.hpp"

// Concrete handle for one physical hopper
struct HopperHandle {
  DenomSpec spec;
  IDispenser* driver{nullptr};  // wraps single-hopper controller
};

struct PlanItem { Denom denom; int count; };
using DispensePlan = std::vector<PlanItem>; // ordered high->low denom

struct MultiDispenseStats {
  bool ok;
  std::string reason; // "OK","PARTIAL","JAM","TIMEOUT"
  int pulses{0};
  int coins{0};
  std::vector<PlanItem> requested;
  std::vector<PlanItem> fulfilled;
};

class MultiDispenser {
public:
  explicit MultiDispenser(Inventory& inv);
  void attach(const HopperHandle& h); // attach per denom; spec.denom must be unique
  // Execute a multi-denom plan; update inventory on success; partials allowed
  MultiDispenseStats execute(const DispensePlan& plan);
private:
  std::map<Denom, HopperHandle> hoppers_;
  Inventory& inv_;
};

