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
           "register/REG-01", 1, "data/awsq", 8ull<<20, 0, 60};
  c.security = {1,1,1,"192.0.2.0/24",1,"registermvp"};
  c.identity = {0,"0x81000001","http://ca.local/sign","/etc/register-mvp/certs","REG"};
  c.safety = {-1,-1,-1,true,10};
  c.service = {true,"1234",3,3,150,2,"data/service.log"};
  c.pos = {true,9090,"","",""};
  c.ota = {false, "local", "", "stable", "default", 300, "/var/lib/register-mvp/ota", "", 180, true};
  c.mfg = {1, "file://stdout", "assets/labels/Device_Label.svg", "Acme Retail"};
  c.eol = {0.35,3,40,1500,1,10,"/var/lib/register-mvp/eol"};
  c.burnin = {500,250,true,"/var/log/register-mvp/burnin.jsonl"};
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
          } else if (key == "use_tls_identity") {
            bool def = cfg.aws.use_tls_identity;
            try { cfg.aws.use_tls_identity = std::stoi(value) != 0; }
            catch (...) { cfg.aws.use_tls_identity = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "rotation_check_minutes") {
            int def = cfg.aws.rotation_check_minutes;
            try { cfg.aws.rotation_check_minutes = std::stoi(value); }
            catch (...) { cfg.aws.rotation_check_minutes = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else {
            handled = false;
          }
        } else if (section == "security") {
          handled = true;
          if (key == "enable_ro_root") {
            bool def = cfg.security.enable_ro_root;
            try { cfg.security.enable_ro_root = std::stoi(value) != 0; }
            catch (...) { cfg.security.enable_ro_root = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "ssh_keys_only") {
            bool def = cfg.security.ssh_keys_only;
            try { cfg.security.ssh_keys_only = std::stoi(value) != 0; }
            catch (...) { cfg.security.ssh_keys_only = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "firewall_enable") {
            bool def = cfg.security.firewall_enable;
            try { cfg.security.firewall_enable = std::stoi(value) != 0; }
            catch (...) { cfg.security.firewall_enable = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "allow_ssh_from") {
            cfg.security.allow_ssh_from = value;
          } else if (key == "auditd_enable") {
            bool def = cfg.security.auditd_enable;
            try { cfg.security.auditd_enable = std::stoi(value) != 0; }
            catch (...) { cfg.security.auditd_enable = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "service_user") {
            cfg.security.service_user = value;
          } else {
            handled = false;
          }
        } else if (section == "identity") {
          handled = true;
          if (key == "use_tpm") {
            bool def = cfg.identity.use_tpm;
            try { cfg.identity.use_tpm = std::stoi(value) != 0; }
            catch (...) { cfg.identity.use_tpm = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "tpm_parent_handle") {
            cfg.identity.tpm_parent_handle = value;
          } else if (key == "ca_endpoint") {
            cfg.identity.ca_endpoint = value;
          } else if (key == "cert_dir") {
            cfg.identity.cert_dir = value;
          } else if (key == "device_id_prefix") {
            cfg.identity.device_id_prefix = value;
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
          } else if (key == "tls_cert") {
            cfg.pos.tls_cert = value;
          } else if (key == "tls_key") {
            cfg.pos.tls_key = value;
          } else {
            handled = false;
          }
        } else if (section == "ota") {
          handled = true;
          if (key == "enable") {
            bool def = cfg.ota.enable;
            try { cfg.ota.enable = std::stoi(value) != 0; }
            catch (...) { cfg.ota.enable = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "backend") {
            cfg.ota.backend = value;
          } else if (key == "feed_url") {
            cfg.ota.feed_url = value;
          } else if (key == "channel") {
            cfg.ota.channel = value;
          } else if (key == "ring") {
            cfg.ota.ring = value;
          } else if (key == "poll_seconds") {
            int def = cfg.ota.poll_seconds;
            try { cfg.ota.poll_seconds = std::stoi(value); }
            catch (...) { cfg.ota.poll_seconds = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "state_dir") {
            cfg.ota.state_dir = value;
          } else if (key == "key_pub") {
            cfg.ota.key_pub = value;
          } else if (key == "health_grace_seconds") {
            int def = cfg.ota.health_grace_seconds;
            try { cfg.ota.health_grace_seconds = std::stoi(value); }
            catch (...) { cfg.ota.health_grace_seconds = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
          } else if (key == "require_signed") {
            bool def = cfg.ota.require_signed;
            try { cfg.ota.require_signed = std::stoi(value) != 0; }
            catch (...) { cfg.ota.require_signed = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else {
            handled = false;
          }
        } else if (section == "manufacturing") {
          handled = true;
          if (key == "enable_first_boot_eol") {
            bool def = cfg.mfg.enable_first_boot_eol;
            try { cfg.mfg.enable_first_boot_eol = std::stoi(value) != 0; }
            catch(...) { cfg.mfg.enable_first_boot_eol = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
          } else if (key == "label_printer_uri") {
            cfg.mfg.label_printer_uri = value;
          } else if (key == "label_template") {
            cfg.mfg.label_template = value;
          } else if (key == "company_name") {
            cfg.mfg.company_name = value;
          } else {
            handled = false;
          }
        } else if (section == "eol") {
          handled = true;
          if (key == "weigh_tolerance_g") {
            double def = cfg.eol.weigh_tolerance_g;
            try { cfg.eol.weigh_tolerance_g = std::stod(value); }
            catch(...) { cfg.eol.weigh_tolerance_g = def; cfg.warnings.push_back(fullkey + " invalid, using default "+std::to_string(def)); }
          } else if (key == "coins_for_test") {
            int def = cfg.eol.coins_for_test;
            try { cfg.eol.coins_for_test = std::stoi(value); }
            catch(...) { cfg.eol.coins_for_test = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "open_mm") {
            int def = cfg.eol.open_mm;
            try { cfg.eol.open_mm = std::stoi(value); }
            catch(...) { cfg.eol.open_mm = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "present_ms") {
            int def = cfg.eol.present_ms;
            try { cfg.eol.present_ms = std::stoi(value); }
            catch(...) { cfg.eol.present_ms = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "retries") {
            int def = cfg.eol.retries;
            try { cfg.eol.retries = std::stoi(value); }
            catch(...) { cfg.eol.retries = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "min_success_txn_visible_seconds") {
            int def = cfg.eol.min_success_txn_visible_seconds;
            try { cfg.eol.min_success_txn_visible_seconds = std::stoi(value); }
            catch(...) { cfg.eol.min_success_txn_visible_seconds = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "result_dir") {
            cfg.eol.result_dir = value;
          } else {
            handled = false;
          }
        } else if (section == "burnin") {
          handled = true;
          if (key == "cycles") {
            int def = cfg.burnin.cycles;
            try { cfg.burnin.cycles = std::stoi(value); }
            catch(...) { cfg.burnin.cycles = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "rest_ms") {
            int def = cfg.burnin.rest_ms;
            try { cfg.burnin.rest_ms = std::stoi(value); }
            catch(...) { cfg.burnin.rest_ms = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "abort_on_fault") {
            bool def = cfg.burnin.abort_on_fault;
            try { cfg.burnin.abort_on_fault = std::stoi(value) != 0; }
            catch(...) { cfg.burnin.abort_on_fault = def; cfg.warnings.push_back(fullkey+" invalid, using default "+(def?"1":"0")); }
          } else if (key == "log_path") {
            cfg.burnin.log_path = value;
          } else {
            handled = false;
          }
        } else if (section == "quant") {
          handled = true;
          if (key == "enable") {
            bool def = cfg.quant.enable;
            try { cfg.quant.enable = std::stoi(value) != 0; }
            catch(...) { cfg.quant.enable = def; cfg.warnings.push_back(fullkey+" invalid, using default "+(def?"1":"0")); }
          } else if (key == "endpoint") {
            cfg.quant.endpoint = value;
          } else if (key == "client_id") {
            cfg.quant.client_id = value;
          } else if (key == "hmac_key_hex") {
            cfg.quant.hmac_key_hex = value;
          } else if (key == "topic") {
            cfg.quant.topic = value;
          } else if (key == "queue_dir") {
            cfg.quant.queue_dir = value;
          } else if (key == "backoff_ms") {
            cfg.quant.backoff_ms = value;
          } else if (key == "heartbeat_seconds") {
            int def = cfg.quant.heartbeat_seconds;
            try { cfg.quant.heartbeat_seconds = std::stoi(value); }
            catch(...) { cfg.quant.heartbeat_seconds = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "reserve_floor_cents") {
            int def = cfg.quant.reserve_floor_cents;
            try { cfg.quant.reserve_floor_cents = std::stoi(value); }
            catch(...) { cfg.quant.reserve_floor_cents = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "max_daily_outflow_cents") {
            int def = cfg.quant.max_daily_outflow_cents;
            try { cfg.quant.max_daily_outflow_cents = std::stoi(value); }
            catch(...) { cfg.quant.max_daily_outflow_cents = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "min_step_change_cents") {
            int def = cfg.quant.min_step_change_cents;
            try { cfg.quant.min_step_change_cents = std::stoi(value); }
            catch(...) { cfg.quant.min_step_change_cents = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "max_update_rate_hz") {
            int def = cfg.quant.max_update_rate_hz;
            try { cfg.quant.max_update_rate_hz = std::stoi(value); }
            catch(...) { cfg.quant.max_update_rate_hz = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "tolerance_cents") {
            int def = cfg.quant.tolerance_cents;
            try { cfg.quant.tolerance_cents = std::stoi(value); }
            catch(...) { cfg.quant.tolerance_cents = def; cfg.warnings.push_back(fullkey+" invalid, using default "+std::to_string(def)); }
          } else if (key == "client_public" || key == "client_secret" || key == "server_public") {
            // accepted but not stored; avoid unknown warning
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
