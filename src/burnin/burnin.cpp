#include "burnin/burnin.hpp"
#include <fstream>
#include <thread>

namespace burnin {

int run(const cfg::Config& cfg, std::function<bool(int)> fault_cb, int cycles_override) {
  int cycles = cycles_override > 0 ? cycles_override : cfg.burnin.cycles;
  std::ofstream log(cfg.burnin.log_path, std::ios::app);
  for (int i = 0; i < cycles; ++i) {
    bool fault = fault_cb && fault_cb(i);
    log << "{\"cycle\":" << i << ",\"fault\":" << (fault?"true":"false") << "}\n";
    if (fault && cfg.burnin.abort_on_fault) {
      return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(cfg.burnin.rest_ms));
  }
  return 0;
}

} // namespace burnin
