#pragma once

#include "../drivers/iscale.hpp"
#include <string>
#include <vector>

namespace audit {

struct CoinSpec {
  std::string name = "quarter";
  double mass_g = 5.670;
  double tol_per_coin_g = 0.35;
};

struct Calib {
  double grams_per_raw = 0.001;
  long tare_raw = 0;
};

struct Config {
  CoinSpec coin{};
  Calib    calib{};
  int      samples_pre  = 8;
  int      samples_post = 8;
  int      settle_ms    = 200;
  long     stuck_epsilon_raw = 3;
};

struct Result {
  bool   ok = false;
  bool   skipped = false;
  double expected_g = 0.0;
  double measured_g = 0.0;
  double delta_g = 0.0;
  std::vector<std::string> flags;
};

bool load_calib(const std::string& path, Calib& out);
bool save_calib(const std::string& path, const Calib& in);
Result run(IScale* scale, const Config& cfg, int coins_dispensed);

} // namespace audit
