#pragma once
#include "ishutter.hpp"
#include "idispenser.hpp"
#include "../util/journal.hpp"

struct TxnConfig {
  int open_mm = 40;
  int present_ms = 2000;
  int max_quarters_per_txn = 40;
  bool resume_on_start = true;
};

class TxnEngine {
public:
  TxnEngine(IShutter& shutter, IDispenser& dispenser, const TxnConfig& cfg);
  journal::Txn run_purchase(int price_cents, int deposit_cents);
  journal::Txn resume_if_needed();
private:
  journal::Txn continue_txn(journal::Txn t);
  IShutter& sh_;
  IDispenser& disp_;
  TxnConfig cfg_;
};
