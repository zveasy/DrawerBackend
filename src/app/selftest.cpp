#include "app/selftest.hpp"
#include "app/shutter_adapter.hpp"
#include "app/dispense_adapter.hpp"
#include "app/dispense_ctrl.hpp"
#include "drivers/stepper.hpp"
#include "drivers/hopper_parallel.hpp"
#include "drivers/hx711.hpp"
#include "hal/gpio.hpp"
#include "util/log.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unistd.h>

namespace selftest {

Result run(IShutter& sh, IDispenser& disp, IScale* scale, const cfg::Config& c) {
  struct Comp { bool ok=true; std::string reason; };
  Comp shutter, hopper, scl{false, "missing or not ready"};
  bool overall_ok = true;

  std::string reason;
  if (!sh.home(c.mech.max_mm, &reason)) {
    shutter.ok = false;
    shutter.reason = reason;
    overall_ok = false;
  }

  if (c.st.enable_coin_test) {
    auto stats = disp.dispenseCoins(1);
    if (!stats.ok || stats.dispensed != 1) {
      hopper.ok = false;
      hopper.reason = stats.reason;
      overall_ok = false;
    }
  } else {
    hopper.ok = true;
    hopper.reason = "coin test disabled";
  }

  long baseline = 0;
  bool scale_present = scale != nullptr;
  if (scale_present) {
    try {
      baseline = scale->read_average(c.audit.samples_pre);
      scl.ok = true;
      scl.reason.clear();
    } catch (const std::exception& e) {
      scl.ok = false;
      scl.reason = e.what();
    }
  }

  if (scale_present) {
    overall_ok = overall_ok && scl.ok;
  }

  auto now = std::chrono::system_clock::now();
  std::time_t tt = std::chrono::system_clock::to_time_t(now);
  char tsbuf[64];
  std::strftime(tsbuf, sizeof(tsbuf), "%FT%TZ", std::gmtime(&tt));
  char host[128];
  gethostname(host, sizeof(host));
  host[sizeof(host)-1] = '\0';

  std::ostringstream oss;
  oss << "{\"ts\":\"" << tsbuf << "\",\"ok\":" << (overall_ok?"true":"false")
      << ",\"host\":\"" << host << "\"";
  oss << ",\"config\":{\"source\":\"" << c.source_path << "\",\"warnings\":[";
  for (size_t i=0;i<c.warnings.size();++i) {
    if (i) oss << ',';
    oss << "\"" << c.warnings[i] << "\"";
  }
  oss << "]}";
  oss << ",\"components\":{";
  oss << "\"shutter\":{\"ok\":" << (shutter.ok?"true":"false") << ",\"reason\":\"" << shutter.reason << "\"}";
  oss << ",\"hopper\":{\"ok\":" << (hopper.ok?"true":"false") << ",\"reason\":\"" << hopper.reason << "\",\"pulses_per_coin\":" << c.hopper.pulses_per_coin << "}";
  oss << ",\"scale\":{\"ok\":" << (scl.ok?"true":"false") << ",\"reason\":\"" << scl.reason << "\",\"baseline_raw\":" << baseline << "}}";
  oss << "}";
  std::string json = oss.str();

  std::filesystem::create_directories("data");
  std::ofstream("data/health.json") << json;

  return {overall_ok, json};
}

Result run(const cfg::Config& c) {
  auto chip = hal::make_chip();

  Stepper step(*chip, c.pins.step, c.pins.dir, c.pins.enable, c.pins.limit_open,
               c.pins.limit_closed, c.mech.steps_per_mm, 400, 80);
  ShutterFSM fsm(step, 5, c.mech.max_mm);
  ShutterAdapter sh(fsm);

  HopperParallel hopper(*chip, c.pins.hopper_en, c.pins.hopper_pulse, true,
                        c.hopper.pulses_per_coin, c.hopper.min_edge_interval_us);
  DispenseConfig dcfg;
  dcfg.jam_ms = c.disp.jam_ms;
  dcfg.settle_ms = c.disp.settle_ms;
  dcfg.max_ms_per_coin = c.disp.max_ms_per_coin;
  dcfg.hard_timeout_ms = c.disp.hard_timeout_ms;
  DispenseController dctrl(hopper, dcfg);
  DispenseAdapter disp(dctrl);

  std::unique_ptr<HX711> hx;
  IScale* scale = nullptr;
  try {
    hx.reset(new HX711(*chip, c.pins.hx_dt, c.pins.hx_sck));
    scale = hx.get();
  } catch (...) {
    scale = nullptr;
  }

  return run(sh, disp, scale, c);
}

} // namespace selftest
