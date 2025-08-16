#pragma once
#include <string>
struct DispenseStats { bool ok; int requested; int dispensed; int pulses; int retries; std::string reason; int elapsed_ms; };
struct IDispenser {
  virtual ~IDispenser() = default;
  virtual DispenseStats dispenseCoins(int coins) = 0;
};
