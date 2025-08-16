#include "audit.hpp"

#include "../util/persist.hpp"
#include "../hal/time.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>

namespace audit {

bool load_calib(const std::string& path, Calib& out) {
  std::map<std::string, std::string> kv;
  if (!persist::load_kv(path, kv)) return false;
  bool ok = false;
  auto it = kv.find("grams_per_raw");
  if (it != kv.end()) { out.grams_per_raw = std::stod(it->second); ok = true; }
  it = kv.find("tare_raw");
  if (it != kv.end()) { out.tare_raw = std::stol(it->second); ok = true; }
  return ok;
}

bool save_calib(const std::string& path, const Calib& in) {
  std::map<std::string, std::string> kv{{"grams_per_raw", std::to_string(in.grams_per_raw)},
                                        {"tare_raw", std::to_string(in.tare_raw)}};
  return persist::save_kv(path, kv);
}

Result run(IScale* scale, const Config& cfg, int coins_dispensed) {
  Result r;
  if (!scale) { r.skipped = true; return r; }
  try {
    std::vector<long> samples;
    samples.reserve(cfg.samples_pre + cfg.samples_post);
    long sum_pre = 0;
    for (int i = 0; i < cfg.samples_pre; ++i) {
      long v = scale->read_average(1);
      sum_pre += v;
      samples.push_back(v);
    }
    long baseline_raw = sum_pre / std::max(1, cfg.samples_pre);
    hal::sleep_us(cfg.settle_ms * 1000);
    long sum_post = 0;
    for (int i = 0; i < cfg.samples_post; ++i) {
      long v = scale->read_average(1);
      sum_post += v;
      samples.push_back(v);
    }
    long post_raw = sum_post / std::max(1, cfg.samples_post);
    auto [min_it, max_it] = std::minmax_element(samples.begin(), samples.end());
    if (samples.size() > 1 && (*max_it - *min_it) < cfg.stuck_epsilon_raw)
      r.flags.push_back("SENSOR_STUCK");

    long delta_raw = post_raw - baseline_raw;
    r.measured_g = static_cast<double>(delta_raw) * cfg.calib.grams_per_raw;
    r.expected_g = coins_dispensed * cfg.coin.mass_g;
    r.delta_g = r.measured_g - r.expected_g;
    double tol_g = coins_dispensed * cfg.coin.tol_per_coin_g;
    if (r.measured_g < r.expected_g - tol_g) r.flags.push_back("UNDERPAY");
    if (r.measured_g > r.expected_g + tol_g) r.flags.push_back("OVERPAY");
    r.ok = r.flags.empty();
  } catch (const std::exception&) {
    r.flags.clear();
    r.skipped = true;
  }
  return r;
}

} // namespace audit
