#include "config/config.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace cfg {

namespace {

Config make_defaults() {
  Config c;
  c.pins = {23,24,25,26,27,5,6,19,13};
  c.mech = {40,40,80};
  c.hopper = {1,2000};
  c.disp = {600,120,300,5000};
  c.audit = {5.670,0.35,0.001,0,8,8,200,3};
  c.pres = {2000};
  c.st = {true};
  c.aws = {false, "", "REG-01", "REG-01",
           "/etc/register-mvp/certs/AmazonRootCA1.pem",
           "/etc/register-mvp/certs/device.pem.crt",
           "/etc/register-mvp/certs/private.pem.key",
           "register/REG-01", 1, "data/awsq", 8ull<<20};
  c.safety = {-1,-1,-1,true,10};
  c.service = {true,"1234",3,3,150,2,"data/service.log"};
  c.pos = {true,9090,""};
  return c;
}

std::string trim(const std::string& s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

std::string lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
  return s;
}

} // namespace

const Config& defaults() {
  static Config d = make_defaults();
  return d;
}

Config load() {
  Config cfg = defaults();
  cfg.source_path.clear();
  cfg.warnings.clear();

  std::string path;
  const char* env = std::getenv("REGISTER_MVP_CONFIG");
  if (env && std::filesystem::exists(env)) {
    path = env;
  } else if (std::filesystem::exists("config/config.ini")) {
    path = "config/config.ini";
  }

  if (!path.empty()) {
    cfg.source_path = path;
    std::ifstream in(path);
    std::string line, section;
    int lineno = 0;
    while (std::getline(in, line)) {
      lineno++;
      auto hash = line.find('#');
      if (hash != std::string::npos) line = line.substr(0, hash);
      line = trim(line);
      if (line.empty()) continue;
      if (line.front() == '[' && line.back() == ']') {
        section = lower(trim(line.substr(1, line.size()-2)));
        continue;
      }
      auto eq = line.find('=');
      if (eq == std::string::npos) {
        cfg.warnings.push_back("line " + std::to_string(lineno) + " invalid");
        continue;
      }
      std::string key = lower(trim(line.substr(0, eq)));
      std::string value = trim(line.substr(eq+1));
      auto fullkey = section + "." + key;
      bool handled = false;
      try {
        if (section == "io.pins") {
          handled = true;
          int* target = nullptr;
          if (key == "step") target = &cfg.pins.step;
          else if (key == "dir") target = &cfg.pins.dir;
          else if (key == "enable") target = &cfg.pins.enable;
          else if (key == "limit_open") target = &cfg.pins.limit_open;
          else if (key == "limit_closed") target = &cfg.pins.limit_closed;
          else if (key == "hopper_en") target = &cfg.pins.hopper_en;
          else if (key == "hopper_pulse") target = &cfg.pins.hopper_pulse;
          else if (key == "hx_dt") target = &cfg.pins.hx_dt;
          else if (key == "hx_sck") target = &cfg.pins.hx_sck;
          else handled = false;
          if (target) {
            int def = *target;
            try { *target = std::stoi(value); }
            catch (...) { *target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          }
        } else if (section == "mechanics") {
          handled = true;
          int* target = nullptr;
          if (key == "steps_per_mm") target = &cfg.mech.steps_per_mm;
          else if (key == "open_mm") target = &cfg.mech.open_mm;
          else if (key == "max_mm") target = &cfg.mech.max_mm;
          else handled = false;
          if (target) {
            int def = *target;
            try { *target = std::stoi(value); }
            catch (...) { *target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          }
        } else if (section == "hopper") {
          handled = true;
          int* target = nullptr;
          if (key == "pulses_per_coin") target = &cfg.hopper.pulses_per_coin;
          else if (key == "min_edge_interval_us") target = &cfg.hopper.min_edge_interval_us;
          else handled = false;
          if (target) {
            int def = *target;
            try { *target = std::stoi(value); }
            catch (...) { *target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          }
        } else if (section == "dispense") {
          handled = true;
          int* target = nullptr;
          if (key == "jam_ms") target = &cfg.disp.jam_ms;
          else if (key == "settle_ms") target = &cfg.disp.settle_ms;
          else if (key == "max_ms_per_coin") target = &cfg.disp.max_ms_per_coin;
          else if (key == "hard_timeout_ms") target = &cfg.disp.hard_timeout_ms;
          else handled = false;
          if (target) {
            int def = *target;
            try { *target = std::stoi(value); }
            catch (...) { *target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          }
        } else if (section == "audit") {
          handled = true;
          if (key == "coin_mass_g") {
            double def = cfg.audit.coin_mass_g;
            try { cfg.audit.coin_mass_g = std::stod(value); }
            catch (...) { cfg.audit.coin_mass_g = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "tolerance_per_coin_g") {
            double def = cfg.audit.tolerance_per_coin_g;
            try { cfg.audit.tolerance_per_coin_g = std::stod(value); }
            catch (...) { cfg.audit.tolerance_per_coin_g = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "grams_per_raw") {
            double def = cfg.audit.grams_per_raw;
            try { cfg.audit.grams_per_raw = std::stod(value); }
            catch (...) { cfg.audit.grams_per_raw = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "tare_raw") {
            long def = cfg.audit.tare_raw;
            try { cfg.audit.tare_raw = std::stol(value); }
            catch (...) { cfg.audit.tare_raw = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "samples_pre") {
            int def = cfg.audit.samples_pre;
            try { cfg.audit.samples_pre = std::stoi(value); }
            catch (...) { cfg.audit.samples_pre = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "samples_post") {
            int def = cfg.audit.samples_post;
            try { cfg.audit.samples_post = std::stoi(value); }
            catch (...) { cfg.audit.samples_post = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "settle_ms") {
            int def = cfg.audit.settle_ms;
            try { cfg.audit.settle_ms = std::stoi(value); }
            catch (...) { cfg.audit.settle_ms = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "stuck_epsilon_raw") {
            int def = cfg.audit.stuck_epsilon_raw;
            try { cfg.audit.stuck_epsilon_raw = std::stoi(value); }
            catch (...) { cfg.audit.stuck_epsilon_raw = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else {
            handled = false;
          }
        } else if (section == "presentation") {
          handled = true;
          if (key == "present_ms") {
            int def = cfg.pres.present_ms;
            try { cfg.pres.present_ms = std::stoi(value); }
            catch (...) { cfg.pres.present_ms = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else {
            handled = false;
          }
        } else if (section == "selftest") {
          handled = true;
          if (key == "enable_coin_test") {
            bool def = cfg.st.enable_coin_test;
            try { cfg.st.enable_coin_test = std::stoi(value) != 0; }
            catch (...) { cfg.st.enable_coin_test = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else {
            handled = false;
          }
        } else if (section == "aws") {
          handled = true;
          if (key == "enable") {
            bool def = cfg.aws.enable;
            try { cfg.aws.enable = std::stoi(value) != 0; }
            catch (...) { cfg.aws.enable = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "endpoint") {
            cfg.aws.endpoint = value;
          } else if (key == "thing_name") {
            cfg.aws.thing_name = value;
          } else if (key == "client_id") {
            cfg.aws.client_id = value;
          } else if (key == "root_ca") {
            cfg.aws.root_ca = value;
          } else if (key == "cert") {
            cfg.aws.cert = value;
          } else if (key == "key") {
            cfg.aws.key = value;
          } else if (key == "topic_prefix") {
            cfg.aws.topic_prefix = value;
          } else if (key == "qos") {
            int def = cfg.aws.qos;
            try { cfg.aws.qos = std::stoi(value); }
            catch (...) { cfg.aws.qos = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "queue_dir") {
            cfg.aws.queue_dir = value;
          } else if (key == "max_queue_bytes") {
            uint64_t def = cfg.aws.max_queue_bytes;
            try { cfg.aws.max_queue_bytes = std::stoull(value); }
            catch (...) { cfg.aws.max_queue_bytes = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else {
            handled = false;
          }
        } else if (section == "safety") {
          handled = true;
          int* target = nullptr;
          if (key == "estop_pin") target = &cfg.safety.estop_pin;
          else if (key == "lid_tamper_pin") target = &cfg.safety.lid_tamper_pin;
          else if (key == "overcurrent_pin") target = &cfg.safety.overcurrent_pin;
          else if (key == "debounce_ms") target = &cfg.safety.debounce_ms;
          if (key == "active_high") {
            bool def = cfg.safety.active_high;
            try { cfg.safety.active_high = std::stoi(value) != 0; }
            catch (...) { cfg.safety.active_high = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (target) {
            int def = *target;
            try { *target = std::stoi(value); }
            catch (...) { *target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else {
            handled = false;
          }
        } else if (section == "service") {
          handled = true;
          if (key == "enable") {
            bool def = cfg.service.enable;
            try { cfg.service.enable = std::stoi(value) != 0; }
            catch (...) { cfg.service.enable = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "pin_code") {
            cfg.service.pin_code = value;
          } else if (key == "jam_clear_shutter_mm") {
            int def = cfg.service.jam_clear_shutter_mm;
            try { cfg.service.jam_clear_shutter_mm = std::stoi(value); }
            catch (...) { cfg.service.jam_clear_shutter_mm = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "jam_clear_cycles") {
            int def = cfg.service.jam_clear_cycles;
            try { cfg.service.jam_clear_cycles = std::stoi(value); }
            catch (...) { cfg.service.jam_clear_cycles = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "hopper_nudge_ms") {
            int def = cfg.service.hopper_nudge_ms;
            try { cfg.service.hopper_nudge_ms = std::stoi(value); }
            catch (...) { cfg.service.hopper_nudge_ms = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "hopper_max_retries") {
            int def = cfg.service.hopper_max_retries;
            try { cfg.service.hopper_max_retries = std::stoi(value); }
            catch (...) { cfg.service.hopper_max_retries = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "audit_path") {
            cfg.service.audit_path = value;
          } else {
            handled = false;
          }
        } else if (section == "pos") {
          handled = true;
          if (key == "enable_http") {
            bool def = cfg.pos.enable_http;
            try { cfg.pos.enable_http = std::stoi(value) != 0; }
            catch (...) { cfg.pos.enable_http = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "port") {
            int def = cfg.pos.port;
            try { cfg.pos.port = std::stoi(value); }
            catch (...) { cfg.pos.port = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "key") {
            cfg.pos.key = value;
          } else {
            handled = false;
          }
        }
      } catch (...) {
        // Should not reach, but guard anyway
        cfg.warnings.push_back(fullkey + " invalid, using default");
      }
      if (!handled) {
        cfg.warnings.push_back(fullkey + " unknown, ignoring");
      }
    }
  }

  return cfg;
}

} // namespace cfg
