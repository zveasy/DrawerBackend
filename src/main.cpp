#include "config/config.hpp"
#include "app/selftest.hpp"
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
#include "server/http_server.hpp"
#include "ui/tui.hpp"
#include "cloud/iot_client.hpp"
#include "cloud/mqtt_loopback.cpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <sstream>
#include <thread>

int main(int argc, char** argv) {
  bool demo_shutter = false;
  bool use_scale = false;
  bool do_selftest = false;
  int dispense_n = -1;
  int purchase_cents = -1;
  int deposit_cents = -1;
  bool run_api = false;
  bool run_tui = false;
  int api_port = 8080;
  bool aws_flag = false;
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
    } else if (arg == "--selftest") {
      do_selftest = true;
    } else if (arg == "--api") {
      run_api = true;
      if (i + 1 < argc && argv[i+1][0] != '-') api_port = std::stoi(argv[++i]);
    } else if (arg == "--tui") {
      run_api = true; run_tui = true;
      if (i + 1 < argc && argv[i+1][0] != '-') api_port = std::stoi(argv[++i]);
    } else if (arg == "--aws" && i + 1 < argc) {
      aws_flag = std::stoi(argv[++i]) != 0;
    }
  }

  try {
    cfg::Config cfg = cfg::load();

    std::unique_ptr<IMqttClient> mqtt_client;
    std::unique_ptr<cloud::IoTClient> iot;
    bool use_aws = aws_flag || cfg.aws.enable;
    if (use_aws) {
      auto loop = std::make_unique<cloud::LoopbackMqttClient>();
      cloud::IoTOptions opt;
      opt.topic_prefix = cfg.aws.topic_prefix;
      opt.thing_name = cfg.aws.thing_name;
      opt.qos = cfg.aws.qos;
      opt.queue_dir = cfg.aws.queue_dir;
      opt.max_queue_bytes = cfg.aws.max_queue_bytes;
      iot = std::make_unique<cloud::IoTClient>(*loop, opt);
      iot->set_shadow_callback([&](const std::map<std::string,std::string>& kv){
        auto it = kv.find("hopper.pulses_per_coin");
        if(it!=kv.end()) cfg.hopper.pulses_per_coin = std::stoi(it->second);
        auto it2 = kv.find("presentation.present_ms");
        if(it2!=kv.end()) cfg.pres.present_ms = std::stoi(it2->second);
      });
      iot->start();
      mqtt_client = std::move(loop);
    }

    if (do_selftest) {
      auto res = selftest::run(cfg);
      if (iot) iot->publish_health(res.json);
      if (res.ok) {
        std::cout << "SELFTEST OK" << std::endl;
        return 0;
      } else {
        // extract reasons
        std::string reasons; auto add_reason=[&](const std::string& comp){
          auto pos = res.json.find("\""+comp+"\"");
          if(pos!=std::string::npos){
            auto okpos = res.json.find("\"ok\":", pos);
            if(okpos!=std::string::npos && res.json.compare(okpos+5,5,"false")==0){
              auto rpos = res.json.find("\"reason\":\"", pos);
              if(rpos!=std::string::npos){
                auto start=rpos+11; auto end=res.json.find("\"", start);
                if(end!=std::string::npos){
                  if(!reasons.empty()) reasons += "; ";
                  reasons += comp+":"+res.json.substr(start,end-start);
                }
              }
            }
          }
        };
        add_reason("shutter"); add_reason("hopper"); add_reason("scale");
        std::cout << "SELFTEST FAIL: " << reasons << std::endl;
        return 1;
      }
    }

    auto chip = hal::make_chip();

    if (demo_shutter) {
      Stepper step(*chip, cfg.pins.step, cfg.pins.dir, cfg.pins.enable,
                   cfg.pins.limit_open, cfg.pins.limit_closed,
                   cfg.mech.steps_per_mm, 400, 80);
      ShutterFSM fsm(step, 5, cfg.mech.max_mm);
      fsm.cmdHome();
      while (fsm.state() != ShutterState::CLOSED &&
             fsm.state() != ShutterState::FAULT) {
        fsm.tick();
      }
      return fsm.state() == ShutterState::CLOSED ? 0 : 1;
    }

    if (run_api) {
      Stepper step(*chip, cfg.pins.step, cfg.pins.dir, cfg.pins.enable,
                   cfg.pins.limit_open, cfg.pins.limit_closed,
                   cfg.mech.steps_per_mm, 400, 80);
      ShutterFSM fsm(step, 5, cfg.mech.max_mm);
      ShutterAdapter sh(fsm);
      HopperParallel hopper(*chip, cfg.pins.hopper_en, cfg.pins.hopper_pulse,
                            true, cfg.hopper.pulses_per_coin,
                            cfg.hopper.min_edge_interval_us);
      DispenseConfig dcfg;
      dcfg.jam_ms = cfg.disp.jam_ms;
      dcfg.settle_ms = cfg.disp.settle_ms;
      dcfg.max_ms_per_coin = cfg.disp.max_ms_per_coin;
      dcfg.hard_timeout_ms = cfg.disp.hard_timeout_ms;
      DispenseController dctrl(hopper, dcfg);
      dctrl.loadCalibration();
      DispenseAdapter disp(dctrl);
      TxnConfig tcfg;
      tcfg.open_mm = cfg.mech.open_mm;
      tcfg.present_ms = cfg.pres.present_ms;
      TxnEngine eng(sh, disp, tcfg);
      if (tcfg.resume_on_start) eng.resume_if_needed();
      HttpServer srv(eng, sh, disp);
      if (!srv.start("127.0.0.1", api_port)) return 1;
      std::cout << "API listening on 127.0.0.1:" << srv.port() << std::endl;
      if (run_tui) {
        tui::run(srv.port());
        srv.stop();
        return 0;
      }
      while (true) std::this_thread::sleep_for(std::chrono::seconds(60));
      return 0;
    }

    if (purchase_cents >= 0 && deposit_cents >= 0) {
      Stepper step(*chip, cfg.pins.step, cfg.pins.dir, cfg.pins.enable,
                   cfg.pins.limit_open, cfg.pins.limit_closed,
                   cfg.mech.steps_per_mm, 400, 80);
      ShutterFSM fsm(step, 5, cfg.mech.max_mm);
      ShutterAdapter sh(fsm);
      HopperParallel hopper(*chip, cfg.pins.hopper_en, cfg.pins.hopper_pulse,
                            true, cfg.hopper.pulses_per_coin,
                            cfg.hopper.min_edge_interval_us);
      DispenseConfig dcfg;
      dcfg.jam_ms = cfg.disp.jam_ms;
      dcfg.settle_ms = cfg.disp.settle_ms;
      dcfg.max_ms_per_coin = cfg.disp.max_ms_per_coin;
      dcfg.hard_timeout_ms = cfg.disp.hard_timeout_ms;
      DispenseController dctrl(hopper, dcfg);
      dctrl.loadCalibration();
      DispenseAdapter disp(dctrl);
      TxnConfig tcfg;
      tcfg.open_mm = cfg.mech.open_mm;
      tcfg.present_ms = cfg.pres.present_ms;
      TxnEngine eng(sh, disp, tcfg);
      if (tcfg.resume_on_start) eng.resume_if_needed();
      auto res = eng.run_purchase(purchase_cents, deposit_cents);
      if (iot) iot->publish_txn(res);
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
      HopperParallel hopper(*chip, cfg.pins.hopper_en, cfg.pins.hopper_pulse,
                            true, cfg.hopper.pulses_per_coin,
                            cfg.hopper.min_edge_interval_us);
      DispenseConfig dcfg;
      dcfg.jam_ms = cfg.disp.jam_ms;
      dcfg.settle_ms = cfg.disp.settle_ms;
      dcfg.max_ms_per_coin = cfg.disp.max_ms_per_coin;
      dcfg.hard_timeout_ms = cfg.disp.hard_timeout_ms;
      DispenseController ctrl(hopper, dcfg);
      ctrl.loadCalibration();

      std::unique_ptr<HX711> hx;
      IScale* scale = nullptr;
      if (use_scale) {
        try {
          hx.reset(new HX711(*chip, cfg.pins.hx_dt, cfg.pins.hx_sck));
          scale = hx.get();
        } catch (const std::exception& e) {
          LOG_WARN("scale_init", {{"err", e.what()}});
          scale = nullptr;
        }
      }
      audit::Config acfg;
      acfg.coin.mass_g = cfg.audit.coin_mass_g;
      acfg.coin.tol_per_coin_g = cfg.audit.tolerance_per_coin_g;
      acfg.calib.grams_per_raw = cfg.audit.grams_per_raw;
      acfg.calib.tare_raw = cfg.audit.tare_raw;
      acfg.samples_pre = cfg.audit.samples_pre;
      acfg.samples_post = cfg.audit.samples_post;
      acfg.settle_ms = cfg.audit.settle_ms;
      acfg.stuck_epsilon_raw = cfg.audit.stuck_epsilon_raw;
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
