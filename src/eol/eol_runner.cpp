#include "eol/eol_runner.hpp"
#include "cloud/identity.hpp"
#include "util/version.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cmath>

namespace eol {

Runner::Runner(IShutter& sh, IDispenser& disp, IScale* sc, std::string telemetry_path)
    : shutter_(sh), dispenser_(disp), scale_(sc), telemetry_path_(std::move(telemetry_path)) {}

EolResult Runner::run_once(const cfg::Config& cfg) {
  current_steps_.clear();
  EolResult r;
  r.device_id = cloud::device_id(cfg);
  r.serial = r.device_id; // simple default
  r.version = appver::version();

  std::filesystem::path dir = std::filesystem::path(cfg.eol.result_dir) / r.serial;
  std::filesystem::create_directories(dir);

  auto add_step = [&](const std::string& name, bool ok, const std::string& reason="", double val=0.0){
    r.steps.push_back({name, ok, reason, val});
    current_steps_ = r.steps;
  };

  add_step("Self-check", true);

  if (!shutter_.home(cfg.mech.max_mm)) {
    add_step("Home shutter", false, "HOME_FAIL");
  } else {
    add_step("Home shutter", true);
  }

  auto d = dispenser_.dispenseCoins(cfg.eol.coins_for_test);
  r.dispensed = d.dispensed;
  add_step("Dispense", d.ok, d.ok?"":d.reason, d.dispensed);

  if (scale_) {
    long pre = scale_->read_average(cfg.audit.samples_pre);
    long post = scale_->read_average(cfg.audit.samples_post);
    double measured = (post - pre) * cfg.audit.grams_per_raw;
    double expected = cfg.eol.coins_for_test * cfg.audit.coin_mass_g;
    double delta = measured - expected;
    r.expected_g = expected;
    r.measured_g = measured;
    r.delta_g = delta;
    bool ok = std::fabs(delta) <= cfg.eol.weigh_tolerance_g;
    std::string reason;
    if (!ok) reason = delta < 0 ? "UNDERPAY" : "OVERPAY";
    add_step("Scale verify", ok, reason, measured);
  } else {
    add_step("Scale verify", true, "NOSCALE");
  }

  bool present_ok = shutter_.open_mm(cfg.eol.open_mm) && shutter_.close_mm(cfg.eol.open_mm);
  add_step("Present", present_ok, present_ok?"":"PRESENT_FAIL");

  bool tele_ok = false;
  {
    std::ifstream t(telemetry_path_);
    if (t) {
      std::stringstream buf; buf << t.rdbuf();
      auto s = buf.str();
      tele_ok = s.find("status=OK") != std::string::npos;
    }
  }
  add_step("Telemetry seen", tele_ok, tele_ok?"":"MISSING");

  r.pass = true;
  for (const auto& s : r.steps) if (!s.ok) r.pass = false;

  r.report_path = (dir / "result.json").string();
  {
    std::ofstream jf(r.report_path);
    jf << "{\n  \"pass\":" << (r.pass?"true":"false")
       << ",\n  \"serial\":\"" << r.serial << "\"\n}";
  }
  {
    std::ofstream cf(dir / "result.csv");
    cf << "name,ok,reason,value\n";
    for (const auto& s : r.steps) {
      cf << s.name << ',' << (s.ok?"1":"0") << ',' << s.reason << ',' << s.value << "\n";
    }
  }
  return r;
}

bool Runner::stamp_pass_fail(const EolResult& r) {
  std::filesystem::path dir = std::filesystem::path(r.report_path).parent_path();
  std::filesystem::path p = dir / (r.pass ? "PASS" : "FAIL");
  std::ofstream f(p);
  return static_cast<bool>(f);
}

} // namespace eol
