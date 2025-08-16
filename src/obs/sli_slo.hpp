#pragma once
#include <chrono>
#include <deque>
#include <string>
#include <vector>

namespace obs {

struct SLO {
  std::string name;
  double target;
  std::string expr;
};

struct SLIResult {
  std::string name;
  double value{0};
  bool met{false};
};

std::vector<SLO> default_slos();

class WindowSLI {
 public:
  explicit WindowSLI(std::chrono::minutes win = std::chrono::minutes(30));
  void sample(uint64_t ok_txn, uint64_t total_txn, uint64_t jam_cnt);
  SLIResult success_rate(double target) const;
  SLIResult jam_rate_per_1k(double target_per_1k) const;
 private:
  struct Sample { uint64_t ok; uint64_t total; uint64_t jams; std::chrono::steady_clock::time_point t; };
  std::chrono::minutes window_;
  std::deque<Sample> samples_;
};

} // namespace obs

