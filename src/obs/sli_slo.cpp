#include "obs/sli_slo.hpp"

namespace obs {

std::vector<SLO> default_slos() {
  return {
    {"success_rate_30m", 0.995, "increase(register_txn_total{status=\"OK\"}[30m]) / increase(register_txn_total[30m])"},
    {"jam_rate_per_1k_1h", 0.001, "increase(register_jam_total[1h]) / increase(register_txn_total[1h])"}
  };
}

WindowSLI::WindowSLI(std::chrono::minutes win) : window_(win) {}

void WindowSLI::sample(uint64_t ok, uint64_t total, uint64_t jams) {
  auto now = std::chrono::steady_clock::now();
  samples_.push_back({ok,total,jams,now});
  while(!samples_.empty() && now - samples_.front().t > window_) samples_.pop_front();
}

SLIResult WindowSLI::success_rate(double target) const {
  if (samples_.size() < 2) return {"success_rate", 0.0, false};
  const auto& first = samples_.front();
  const auto& last = samples_.back();
  double ok = static_cast<double>(last.ok - first.ok);
  double total = static_cast<double>(last.total - first.total);
  double val = total>0 ? ok/total : 0.0;
  return {"success_rate", val, val >= target};
}

SLIResult WindowSLI::jam_rate_per_1k(double target_per_1k) const {
  if (samples_.size() < 2) return {"jam_rate_per_1k", 0.0, true};
  const auto& first = samples_.front();
  const auto& last = samples_.back();
  double jams = static_cast<double>(last.jams - first.jams);
  double total = static_cast<double>(last.total - first.total);
  double rate = total>0 ? (jams/total)*1000.0 : 0.0;
  return {"jam_rate_per_1k", rate, rate <= target_per_1k};
}

} // namespace obs

