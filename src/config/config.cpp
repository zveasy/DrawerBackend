#include "config/config.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>

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

void set_int(int& target, const std::string& value, const std::string& fullkey, Config& cfg) {
  int def = target;
  try { target = std::stoi(value); }
  catch (...) { target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
}

void set_long(long& target, const std::string& value, const std::string& fullkey, Config& cfg) {
  long def = target;
  try { target = std::stol(value); }
  catch (...) { target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
}

void set_double(double& target, const std::string& value, const std::string& fullkey, Config& cfg) {
  double def = target;
  try { target = std::stod(value); }
  catch (...) { target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
}

void set_uint64(uint64_t& target, const std::string& value, const std::string& fullkey, Config& cfg) {
  uint64_t def = target;
  try { target = std::stoull(value); }
  catch (...) { target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + std::to_string(def)); }
}

void set_bool(bool& target, const std::string& value, const std::string& fullkey, Config& cfg) {
  bool def = target;
  try { target = std::stoi(value) != 0; }
  catch (...) { target = def; cfg.warnings.push_back(fullkey + " invalid, using default " + (def?"1":"0")); }
}

using Setter = void(*)(Config&, const std::string&, const std::string&);
using SectionParser = bool(*)(Config&, const std::string&, const std::string&, const std::string&);

bool parse_io_pins(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"step", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.step, v, fk, c); }},
    {"dir", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.dir, v, fk, c); }},
    {"enable", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.enable, v, fk, c); }},
    {"limit_open", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.limit_open, v, fk, c); }},
    {"limit_closed", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.limit_closed, v, fk, c); }},
    {"hopper_en", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.hopper_en, v, fk, c); }},
    {"hopper_pulse", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.hopper_pulse, v, fk, c); }},
    {"hx_dt", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.hx_dt, v, fk, c); }},
    {"hx_sck", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pins.hx_sck, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_mechanics(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"steps_per_mm", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.mech.steps_per_mm, v, fk, c); }},
    {"open_mm", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.mech.open_mm, v, fk, c); }},
    {"max_mm", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.mech.max_mm, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_hopper(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"pulses_per_coin", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.hopper.pulses_per_coin, v, fk, c); }},
    {"min_edge_interval_us", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.hopper.min_edge_interval_us, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_dispense(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"jam_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.disp.jam_ms, v, fk, c); }},
    {"settle_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.disp.settle_ms, v, fk, c); }},
    {"max_ms_per_coin", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.disp.max_ms_per_coin, v, fk, c); }},
    {"hard_timeout_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.disp.hard_timeout_ms, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_audit(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"coin_mass_g", [](Config& c, const std::string& v, const std::string& fk){ set_double(c.audit.coin_mass_g, v, fk, c); }},
    {"tolerance_per_coin_g", [](Config& c, const std::string& v, const std::string& fk){ set_double(c.audit.tolerance_per_coin_g, v, fk, c); }},
    {"grams_per_raw", [](Config& c, const std::string& v, const std::string& fk){ set_double(c.audit.grams_per_raw, v, fk, c); }},
    {"tare_raw", [](Config& c, const std::string& v, const std::string& fk){ set_long(c.audit.tare_raw, v, fk, c); }},
    {"samples_pre", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.audit.samples_pre, v, fk, c); }},
    {"samples_post", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.audit.samples_post, v, fk, c); }},
    {"settle_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.audit.settle_ms, v, fk, c); }},
    {"stuck_epsilon_raw", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.audit.stuck_epsilon_raw, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_presentation(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"present_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pres.present_ms, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_selftest(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable_coin_test", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.st.enable_coin_test, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_aws(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.aws.enable, v, fk, c); }},
    {"endpoint", [](Config& c, const std::string& v, const std::string&){ c.aws.endpoint = v; }},
    {"thing_name", [](Config& c, const std::string& v, const std::string&){ c.aws.thing_name = v; }},
    {"client_id", [](Config& c, const std::string& v, const std::string&){ c.aws.client_id = v; }},
    {"root_ca", [](Config& c, const std::string& v, const std::string&){ c.aws.root_ca = v; }},
    {"cert", [](Config& c, const std::string& v, const std::string&){ c.aws.cert = v; }},
    {"key", [](Config& c, const std::string& v, const std::string&){ c.aws.key = v; }},
    {"topic_prefix", [](Config& c, const std::string& v, const std::string&){ c.aws.topic_prefix = v; }},
    {"qos", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.aws.qos, v, fk, c); }},
    {"queue_dir", [](Config& c, const std::string& v, const std::string&){ c.aws.queue_dir = v; }},
    {"max_queue_bytes", [](Config& c, const std::string& v, const std::string& fk){ set_uint64(c.aws.max_queue_bytes, v, fk, c); }},
    {"use_tls_identity", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.aws.use_tls_identity, v, fk, c); }},
    {"rotation_check_minutes", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.aws.rotation_check_minutes, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_security(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable_ro_root", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.security.enable_ro_root, v, fk, c); }},
    {"ssh_keys_only", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.security.ssh_keys_only, v, fk, c); }},
    {"firewall_enable", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.security.firewall_enable, v, fk, c); }},
    {"allow_ssh_from", [](Config& c, const std::string& v, const std::string&){ c.security.allow_ssh_from = v; }},
    {"auditd_enable", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.security.auditd_enable, v, fk, c); }},
    {"service_user", [](Config& c, const std::string& v, const std::string&){ c.security.service_user = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_identity(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"use_tpm", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.identity.use_tpm, v, fk, c); }},
    {"tpm_parent_handle", [](Config& c, const std::string& v, const std::string&){ c.identity.tpm_parent_handle = v; }},
    {"ca_endpoint", [](Config& c, const std::string& v, const std::string&){ c.identity.ca_endpoint = v; }},
    {"cert_dir", [](Config& c, const std::string& v, const std::string&){ c.identity.cert_dir = v; }},
    {"device_id_prefix", [](Config& c, const std::string& v, const std::string&){ c.identity.device_id_prefix = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_safety(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"estop_pin", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.safety.estop_pin, v, fk, c); }},
    {"lid_tamper_pin", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.safety.lid_tamper_pin, v, fk, c); }},
    {"overcurrent_pin", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.safety.overcurrent_pin, v, fk, c); }},
    {"debounce_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.safety.debounce_ms, v, fk, c); }},
    {"active_high", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.safety.active_high, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_service(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.service.enable, v, fk, c); }},
    {"pin_code", [](Config& c, const std::string& v, const std::string&){ c.service.pin_code = v; }},
    {"jam_clear_shutter_mm", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.service.jam_clear_shutter_mm, v, fk, c); }},
    {"jam_clear_cycles", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.service.jam_clear_cycles, v, fk, c); }},
    {"hopper_nudge_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.service.hopper_nudge_ms, v, fk, c); }},
    {"hopper_max_retries", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.service.hopper_max_retries, v, fk, c); }},
    {"audit_path", [](Config& c, const std::string& v, const std::string&){ c.service.audit_path = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_pos(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable_http", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.pos.enable_http, v, fk, c); }},
    {"port", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.pos.port, v, fk, c); }},
    {"key", [](Config& c, const std::string& v, const std::string&){ c.pos.key = v; }},
    {"tls_cert", [](Config& c, const std::string& v, const std::string&){ c.pos.tls_cert = v; }},
    {"tls_key", [](Config& c, const std::string& v, const std::string&){ c.pos.tls_key = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_ota(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.ota.enable, v, fk, c); }},
    {"backend", [](Config& c, const std::string& v, const std::string&){ c.ota.backend = v; }},
    {"feed_url", [](Config& c, const std::string& v, const std::string&){ c.ota.feed_url = v; }},
    {"channel", [](Config& c, const std::string& v, const std::string&){ c.ota.channel = v; }},
    {"ring", [](Config& c, const std::string& v, const std::string&){ c.ota.ring = v; }},
    {"poll_seconds", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.ota.poll_seconds, v, fk, c); }},
    {"state_dir", [](Config& c, const std::string& v, const std::string&){ c.ota.state_dir = v; }},
    {"key_pub", [](Config& c, const std::string& v, const std::string&){ c.ota.key_pub = v; }},
    {"health_grace_seconds", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.ota.health_grace_seconds, v, fk, c); }},
    {"require_signed", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.ota.require_signed, v, fk, c); }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_manufacturing(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable_first_boot_eol", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.mfg.enable_first_boot_eol, v, fk, c); }},
    {"label_printer_uri", [](Config& c, const std::string& v, const std::string&){ c.mfg.label_printer_uri = v; }},
    {"label_template", [](Config& c, const std::string& v, const std::string&){ c.mfg.label_template = v; }},
    {"company_name", [](Config& c, const std::string& v, const std::string&){ c.mfg.company_name = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_eol(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"weigh_tolerance_g", [](Config& c, const std::string& v, const std::string& fk){ set_double(c.eol.weigh_tolerance_g, v, fk, c); }},
    {"coins_for_test", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.eol.coins_for_test, v, fk, c); }},
    {"open_mm", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.eol.open_mm, v, fk, c); }},
    {"present_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.eol.present_ms, v, fk, c); }},
    {"retries", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.eol.retries, v, fk, c); }},
    {"min_success_txn_visible_seconds", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.eol.min_success_txn_visible_seconds, v, fk, c); }},
    {"result_dir", [](Config& c, const std::string& v, const std::string&){ c.eol.result_dir = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_burnin(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"cycles", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.burnin.cycles, v, fk, c); }},
    {"rest_ms", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.burnin.rest_ms, v, fk, c); }},
    {"abort_on_fault", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.burnin.abort_on_fault, v, fk, c); }},
    {"log_path", [](Config& c, const std::string& v, const std::string&){ c.burnin.log_path = v; }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

bool parse_quant(Config& cfg, const std::string& key, const std::string& value, const std::string& fullkey) {
  static const std::unordered_map<std::string, Setter> map = {
    {"enable", [](Config& c, const std::string& v, const std::string& fk){ set_bool(c.quant.enable, v, fk, c); }},
    {"endpoint", [](Config& c, const std::string& v, const std::string&){ c.quant.endpoint = v; }},
    {"client_id", [](Config& c, const std::string& v, const std::string&){ c.quant.client_id = v; }},
    {"hmac_key_hex", [](Config& c, const std::string& v, const std::string&){ c.quant.hmac_key_hex = v; }},
    {"topic", [](Config& c, const std::string& v, const std::string&){ c.quant.topic = v; }},
    {"queue_dir", [](Config& c, const std::string& v, const std::string&){ c.quant.queue_dir = v; }},
    {"backoff_ms", [](Config& c, const std::string& v, const std::string&){ c.quant.backoff_ms = v; }},
    {"heartbeat_seconds", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.quant.heartbeat_seconds, v, fk, c); }},
    {"reserve_floor_cents", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.quant.reserve_floor_cents, v, fk, c); }},
    {"max_daily_outflow_cents", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.quant.max_daily_outflow_cents, v, fk, c); }},
    {"min_step_change_cents", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.quant.min_step_change_cents, v, fk, c); }},
    {"max_update_rate_hz", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.quant.max_update_rate_hz, v, fk, c); }},
    {"tolerance_cents", [](Config& c, const std::string& v, const std::string& fk){ set_int(c.quant.tolerance_cents, v, fk, c); }},
    {"client_public", [](Config&, const std::string&, const std::string&){ /* accepted but ignored */ }},
    {"client_secret", [](Config&, const std::string&, const std::string&){ /* accepted but ignored */ }},
    {"server_public", [](Config&, const std::string&, const std::string&){ /* accepted but ignored */ }},
  };
  auto it = map.find(key);
  if (it == map.end()) return false;
  it->second(cfg, value, fullkey);
  return true;
}

const std::unordered_map<std::string, SectionParser> section_parsers = {
  {"io.pins", parse_io_pins},
  {"mechanics", parse_mechanics},
  {"hopper", parse_hopper},
  {"dispense", parse_dispense},
  {"audit", parse_audit},
  {"presentation", parse_presentation},
  {"selftest", parse_selftest},
  {"aws", parse_aws},
  {"security", parse_security},
  {"identity", parse_identity},
  {"safety", parse_safety},
  {"service", parse_service},
  {"pos", parse_pos},
  {"ota", parse_ota},
  {"manufacturing", parse_manufacturing},
  {"eol", parse_eol},
  {"burnin", parse_burnin},
  {"quant", parse_quant},
};

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
        auto sit = section_parsers.find(section);
        if (sit != section_parsers.end()) {
          handled = sit->second(cfg, key, value, fullkey);
        }
      } catch (...) {
        cfg.warnings.push_back(fullkey + " invalid, using default");
        handled = true;
      }
      if (!handled) {
        cfg.warnings.push_back(fullkey + " unknown, ignoring");
      }
    }
  }

  return cfg;
}

} // namespace cfg
