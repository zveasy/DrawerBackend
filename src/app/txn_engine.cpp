#include "txn_engine.hpp"
#include "safety/faults.hpp"
#include <algorithm>
#include <chrono>
#include <thread>

TxnEngine::TxnEngine(IShutter& shutter, IDispenser& dispenser, const TxnConfig& cfg,
                     safety::FaultManager* fm)
    : sh_(shutter), disp_(dispenser), cfg_(cfg), fm_(fm) {}

static std::string make_id() {
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
  return "txn-" + std::to_string(ns);
}

journal::Txn TxnEngine::continue_txn(journal::Txn t) {
  int remaining = t.quarters - t.dispensed;
  if (remaining > 0) {
    if (fm_ && !fm_->check_and_block_motion()) {
      t.phase = "VOID"; t.reason = "FAULT"; journal::append(t); return t;
    }
    auto st = disp_.dispenseCoins(remaining);
    t.dispensed += st.dispensed;
    t.phase = "DISPENSING";
    t.reason = st.reason;
    journal::append(t);
    if (!st.ok || t.dispensed < t.quarters) {
      t.phase = "VOID";
      if (t.reason.empty()) t.reason = st.ok ? "PARTIAL" : st.reason;
      journal::append(t);
      return t;
    }
  }
  if (fm_ && !fm_->check_and_block_motion()) {
    t.phase = "VOID"; t.reason = "FAULT"; journal::append(t); return t;
  }
  if (!sh_.open_mm(cfg_.open_mm, &t.reason)) {
    t.phase = "VOID";
    journal::append(t);
    return t;
  }
  t.phase = "PRESENTING";
  journal::append(t);
  std::this_thread::sleep_for(std::chrono::milliseconds(cfg_.present_ms));
  if (fm_ && !fm_->check_and_block_motion()) {
    t.phase = "VOID"; t.reason = "FAULT"; journal::append(t); return t;
  }
  if (!sh_.close_mm(cfg_.open_mm, &t.reason)) {
    t.phase = "VOID";
    journal::append(t);
    return t;
  }
  t.phase = "CLOSED";
  journal::append(t);
  t.phase = "DONE";
  journal::append(t);
  return t;
}

journal::Txn TxnEngine::run_purchase(int price_cents, int deposit_cents) {
  journal::Txn t;
  t.id = make_id();
  t.price = price_cents;
  t.deposit = deposit_cents;
  t.change = std::max(0, deposit_cents - price_cents);
  t.quarters = std::min(cfg_.max_quarters_per_txn, t.change / 25);
  t.phase = "PENDING";
  journal::append(t);
  return continue_txn(t);
}

journal::Txn TxnEngine::resume_if_needed() {
  journal::Txn t;
  if (!journal::load_last(t)) return t;
  if (t.phase == "PENDING" || t.phase == "DISPENSING") {
    return continue_txn(t);
  }
  if (t.phase == "PRESENTING") {
    if (!sh_.close_mm(cfg_.open_mm, &t.reason)) {
      t.phase = "VOID";
      journal::append(t);
      return t;
    }
    t.phase = "CLOSED";
    journal::append(t);
    t.phase = "DONE";
    journal::append(t);
    return t;
  }
  return t;
}
