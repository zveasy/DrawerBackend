#pragma once
#include "idispenser.hpp"
#include "dispense_ctrl.hpp"

class DispenseAdapter : public IDispenser {
public:
  explicit DispenseAdapter(DispenseController& ctrl);
  DispenseStats dispenseCoins(int coins) override;
private:
  DispenseController& ctrl_;
};
