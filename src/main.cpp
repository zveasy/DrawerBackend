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
#include "app/multi_dispenser.hpp"
#include "app/change_maker.hpp"
#include "app/inventory.hpp"
#include "app/denom.hpp"
#include "drivers/hx711.hpp"
#include "util/log.hpp"
#include "util/journal.hpp"
#include "server/http_server.hpp"
#include "pos/http_pos.hpp"
#include "pos/router.hpp"
#include "pos/idempotency_store.hpp"
#include "ui/tui.hpp"
#include "cloud/iot_client.hpp"
#include "cloud/mqtt_loopback.cpp"
#include "safety/faults.hpp"
#include "app/service_mode.hpp"
#include "util/event_log.hpp"
#include "obs/metrics.hpp"
#include "compliance/compliance_mode.hpp"

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
  bool pos_http = false;
  int pos_port = -1;
  bool aws_flag = false;
  bool service_cli = false;
  std::string service_pin;
  compliance::ComplianceMode comp_mode = compliance::ComplianceMode::NONE;
  int prescan_cycles = 0;
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
    } else if (arg == "--service") {
      service_cli = true;
    } else if (arg == "--pin" && i + 1 < argc) {
      service_pin = argv[++i];
    } else if (arg == "--api") {
      run_api = true;
      if (i + 1 < argc && argv[i+1][0] != '-') api_port = std::stoi(argv[++i]);
    } else if (arg == "--tui") {
      run_api = true; run_tui = true;
      if (i + 1 < argc && argv[i+1][0] != '-') api_port = std::stoi(argv[++i]);
    } else if (arg == "--aws" && i + 1 < argc) {
      aws_flag = std::stoi(argv[++i]) != 0;
    } else if (arg == "--pos-http") {
      pos_http = true;
      if (i + 1 < argc && argv[i+1][0] != '-') pos_port = std::stoi(argv[++i]);
    } else if (arg == "--compliance-mode" && i + 1 < argc) {
      std::string m = argv[++i];
      if (m == "emi_worst") comp_mode = compliance::ComplianceMode::EMI_WORST;
      else if (m == "emi_idle") comp_mode = compliance::ComplianceMode::EMI_IDLE;
      else if (m == "esd_safe") comp_mode = compliance::ComplianceMode::ESD_SAFE;
    } else if (arg == "--prescan-cycle" && i + 1 < argc) {
      prescan_cycles = std::stoi(argv[++i]);
    }
  }

  try {
    cfg::Config cfg = cfg::load();
    obs::M().gauge("register_device_up", "Device up").set(1);
    obs::M().gauge("register_build_info", "Build info", {{"version","1.0.0"},{"git","unknown"}}).set(1);
    eventlog::Logger elog(cfg.service.audit_path);
    safety::FaultManager faults(cfg.safety, &elog);
    faults.start();
    compliance::init(&faults, &elog);
    if (comp_mode != compliance::ComplianceMode::NONE) {
      compliance::set_mode(comp_mode);
      if (prescan_cycles > 0) {
        for (int i = 0; i < prescan_cycles; ++i) {
          compliance::run_emi_worst_pattern_once();
        }
        return 0;
      }
    }
    ServiceMode svc(cfg.service, faults, elog);
    if(service_cli && svc.active()==false){
      svc.enter(service_pin.empty()?cfg.service.pin_code:service_pin);
    }

    std::unique_ptr<IMqttClient> mqtt_client;
    std::unique_ptr<cloud::IoTClient> iot;
    bool use_aws = aws_flag || cfg.aws.enable;

    bool any_action = demo_shutter || use_scale || do_selftest || dispense_n >= 0 ||
                      purchase_cents >= 0 || run_api || run_tui || service_cli;
    if (!pos_http && !any_action && cfg.pos.enable_http) {
      pos_http = true;
    }
    if (pos_port < 0) pos_port = cfg.pos.port;
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
      TxnEngine eng(sh, disp, tcfg, &faults);
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

    if (pos_http) {
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
      TxnEngine eng(sh, disp, tcfg, &faults);
      if (tcfg.resume_on_start) eng.resume_if_needed();
      pos::Options popt;
      popt.port = pos_port;
      popt.shared_key = cfg.pos.key;
      pos::IdempotencyStore store("/var/lib/register-mvp/pos");
      store.open();
      pos::Router router(eng, store);
      pos::HttpConnector conn(router, popt);
      if (!conn.start()) return 1;
      std::cout << "POS listening on " << popt.bind << ":" << conn.port() << std::endl;
      while (true) std::this_thread::sleep_for(std::chrono::seconds(60));
      return 0;
    }

      if (purchase_cents >= 0 && deposit_cents >= 0) {
        Inventory inv;
        for (const auto& spec : default_us_specs()) {
          HopperStock hs; hs.spec = spec; hs.logical_count = 100;
          inv.upsert(hs);
        }
        struct LocalDisp : IDispenser {
          DispenseStats dispenseCoins(int coins) override {
            return DispenseStats{true, coins, coins, coins, 0, "", 0};
          }
        };
        std::vector<std::unique_ptr<LocalDisp>> holders;
        MultiDispenser multi(inv);
        for (const auto& spec : default_us_specs()) {
          holders.emplace_back(new LocalDisp());
          multi.attach(HopperHandle{spec, holders.back().get()});
        }
        ChangeMaker cm(inv);
        int change = deposit_cents - purchase_cents;
        if (change < 0) change = 0;
        change %= 100;
        int leftover = 0;
        auto plan = cm.make_plan_cents(change, leftover);
        if (leftover > 0) {
          std::cout << "VOID reason=NOCHANGE" << std::endl;
          return 1;
        }
        auto st = multi.execute(plan);
        if (!st.ok) {
          std::cout << "VOID reason=" << st.reason << std::endl;
          return 1;
        }
        int q=0,d=0,n=0,p=0;
        for(const auto& pi : plan){
          if(pi.denom==Denom::QUARTER) q=pi.count;
          else if(pi.denom==Denom::DIME) d=pi.count;
          else if(pi.denom==Denom::NICKEL) n=pi.count;
          else if(pi.denom==Denom::PENNY) p=pi.count;
        }
        std::cout << "OK quarters="<<q<<" dimes="<<d<<" nickels="<<n<<" pennies="<<p<< std::endl;
        return 0;
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
