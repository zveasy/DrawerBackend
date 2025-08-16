#include "drivers/hopper_parallel.hpp"
#include "drivers/stepper.hpp"
#include "app/shutter_fsm.hpp"
#include "app/dispense_ctrl.hpp"
#include "app/txn_engine.hpp"
#include "app/shutter_adapter.hpp"
#include "app/dispense_adapter.hpp"
#include "app/audit.hpp"
#include "drivers/hx711.hpp"
#include "util/log.hpp"
#include "util/journal.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <sstream>

// --- PIN MAP ---
static constexpr int PIN_STEP = 23;
static constexpr int PIN_DIR  = 24;
static constexpr int PIN_EN   = 25;
static constexpr int PIN_OPEN = 26;
static constexpr int PIN_CLOSED = 27;

static constexpr int PIN_HOPPER_EN = 5;
static constexpr int PIN_HOPPER_P  = 6;

int main(int argc, char** argv) {
  bool demo_shutter = false;
  bool use_scale = false;
  int dispense_n = -1;
  int purchase_cents = -1;
  int deposit_cents = -1;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--json") {
      setenv("LOG_JSON", "1", 1);
    } else if (arg == "--demo-shutter") {
      demo_shutter = true;
    } else if (arg == "--with-scale") {
      use_scale = true;
    } else if (arg == "--dispense" && i + 1 < argc) {
      dispense_n = std::stoi(argv[++i]);
    } else if (arg == "--purchase" && i + 1 < argc) {
      purchase_cents = std::stoi(argv[++i]);
    } else if (arg == "--deposit" && i + 1 < argc) {
      deposit_cents = std::stoi(argv[++i]);
    }
  }

  try {
    auto chip = hal::make_chip();

    if (demo_shutter) {
      Stepper step(*chip, PIN_STEP, PIN_DIR, PIN_EN, PIN_OPEN, PIN_CLOSED,
                   /*steps_per_mm=*/40, /*pulse_us=*/400, /*rpm=*/80);
      ShutterFSM fsm(step, 5, 80);
      fsm.cmdHome();
      while (fsm.state() != ShutterState::CLOSED &&
             fsm.state() != ShutterState::FAULT) {
        fsm.tick();
      }
      return fsm.state() == ShutterState::CLOSED ? 0 : 1;
    }

    if (purchase_cents >= 0 && deposit_cents >= 0) {
      Stepper step(*chip, PIN_STEP, PIN_DIR, PIN_EN, PIN_OPEN, PIN_CLOSED,
                   /*steps_per_mm=*/40, /*pulse_us=*/400, /*rpm=*/80);
      ShutterFSM fsm(step, 5, 80);
      ShutterAdapter sh(fsm);
      HopperParallel hopper(*chip, PIN_HOPPER_EN, PIN_HOPPER_P);
      DispenseConfig dcfg;
      DispenseController dctrl(hopper, dcfg);
      dctrl.loadCalibration();
      DispenseAdapter disp(dctrl);
      TxnConfig tcfg;
      TxnEngine eng(sh, disp, tcfg);
      if (tcfg.resume_on_start) eng.resume_if_needed();
      auto res = eng.run_purchase(purchase_cents, deposit_cents);
      bool json = std::getenv("LOG_JSON") && std::string(std::getenv("LOG_JSON")) == "1";
      if (json) {
        std::cout << journal::to_json(res) << std::endl;
      } else {
        if (res.phase == "DONE") {
          std::cout << "OK quarters=" << res.quarters
                    << " open=" << tcfg.present_ms << "ms" << std::endl;
        } else {
          std::cout << "VOID reason=" << res.reason << std::endl;
        }
      }
      return res.phase == "DONE" ? 0 : 1;
    }

    if (dispense_n >= 0) {
      HopperParallel hopper(*chip, PIN_HOPPER_EN, PIN_HOPPER_P);
      DispenseConfig cfg;
      DispenseController ctrl(hopper, cfg);
      ctrl.loadCalibration();

      std::unique_ptr<HX711> hx;
      IScale* scale = nullptr;
      if (use_scale) {
        try {
          static constexpr int PIN_SCALE_DT = 16;
          static constexpr int PIN_SCALE_SCK = 17;
          hx.reset(new HX711(*chip, PIN_SCALE_DT, PIN_SCALE_SCK));
          scale = hx.get();
        } catch (const std::exception& e) {
          LOG_WARN("scale_init", {{"err", e.what()}});
          scale = nullptr;
        }
      }
      audit::Config acfg;
      audit::load_calib("data/scale_calib.txt", acfg.calib);

      auto res = ctrl.dispenseCoins(dispense_n);
      auto ares = audit::run(scale, acfg, res.dispensed);

      LOG_INFO("dispense",
               {{"requested", std::to_string(res.requested)},
                {"dispensed", std::to_string(res.dispensed)},
                {"pulses", std::to_string(res.pulses)},
                {"retries", std::to_string(res.retries)},
                {"reason", res.reason},
                {"ms", std::to_string(res.elapsed_ms)}});

      bool json = std::getenv("LOG_JSON") && std::string(std::getenv("LOG_JSON")) == "1";
      if (json) {
        std::ostringstream oss;
        oss << "{\"audit\":{\"expected_g\":" << ares.expected_g
            << ",\"measured_g\":" << ares.measured_g
            << ",\"delta_g\":" << ares.delta_g
            << ",\"ok\":" << (ares.ok ? "true" : "false")
            << ",\"skipped\":" << (ares.skipped ? "true" : "false");
        if (!ares.flags.empty()) {
          oss << ",\"flags\":[";
          for (size_t i = 0; i < ares.flags.size(); ++i) {
            if (i) oss << ",";
            oss << "\"" << ares.flags[i] << "\"";
          }
          oss << "]";
        }
        oss << "}}";
        std::cout << oss.str() << std::endl;
      } else {
        std::map<std::string, std::string> kv{{"expected_g", std::to_string(ares.expected_g)},
                                              {"measured_g", std::to_string(ares.measured_g)},
                                              {"delta_g", std::to_string(ares.delta_g)},
                                              {"ok", ares.ok ? "1" : "0"},
                                              {"skipped", ares.skipped ? "1" : "0"}};
        if (!ares.flags.empty()) {
          std::string fl;
          for (size_t i = 0; i < ares.flags.size(); ++i) {
            if (i) fl += ',';
            fl += ares.flags[i];
          }
          kv["flags"] = fl;
        }
        LOG_INFO("audit", kv);
      }

      if (res.ok) {
        std::cout << res.dispensed << " coins dispensed" << std::endl;
        return 0;
      } else {
        std::cout << "Dispense failed: " << res.reason << std::endl;
        return 1;
      }
    }

    std::cout << "No action specified. Use --demo-shutter or --dispense N" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    LOG_ERROR("main", {{"err", e.what()}});
    std::cerr << e.what() << std::endl;
    return 1;
  }
}

