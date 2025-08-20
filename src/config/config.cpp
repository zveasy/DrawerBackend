#include "config/config.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <functional>

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
  c.quant = {};
  return c;
}

using json = nlohmann::json;
using Errors = std::vector<std::string>;
using JSetter = std::function<void(Config&, const json&, const std::string&, Errors&)>;

template<typename Map>
void apply_object(const json& obj, const std::string& prefix, const Map& map, Config& cfg, Errors& err) {
  if (!obj.is_object()) {
    err.push_back(prefix.substr(0, prefix.size()-1) + " must be object");
    return;
  }
  for (auto& [k, v] : obj.items()) {
    auto it = map.find(k);
    if (it == map.end()) {
      err.push_back(prefix + k + " unknown");
      continue;
    }
    it->second(cfg, v, prefix + k, err);
  }
}

#define INT_FIELD(sec, fld) {#fld, [](Config& c, const json& v, const std::string& fk, Errors& e){ if(!v.is_number_integer()) e.push_back(fk+" must be integer"); else c.sec.fld = v.get<int>(); }}
#define LONG_FIELD(sec, fld) {#fld, [](Config& c, const json& v, const std::string& fk, Errors& e){ if(!v.is_number_integer()) e.push_back(fk+" must be integer"); else c.sec.fld = v.get<long>(); }}
#define UINT64_FIELD(sec, fld) {#fld, [](Config& c, const json& v, const std::string& fk, Errors& e){ if(!(v.is_number_unsigned()||v.is_number_integer())) e.push_back(fk+" must be unsigned"); else c.sec.fld = v.get<uint64_t>(); }}
#define DOUBLE_FIELD(sec, fld) {#fld, [](Config& c, const json& v, const std::string& fk, Errors& e){ if(!v.is_number()) e.push_back(fk+" must be number"); else c.sec.fld = v.get<double>(); }}
#define BOOL_FIELD(sec, fld) {#fld, [](Config& c, const json& v, const std::string& fk, Errors& e){ bool val; if(v.is_boolean()) val = v.get<bool>(); else if(v.is_number_integer()) val = v.get<int>()!=0; else { e.push_back(fk+" must be bool"); return;} c.sec.fld = val; }}
#define STRING_FIELD(sec, fld) {#fld, [](Config& c, const json& v, const std::string& fk, Errors& e){ if(!v.is_string()) e.push_back(fk+" must be string"); else c.sec.fld = v.get<std::string>(); }}

void parse_io(const json& j, Config& cfg, Errors& err) {
  if (j.contains("pins")) {
    static const std::unordered_map<std::string,JSetter> map = {
      INT_FIELD(pins, step),
      INT_FIELD(pins, dir),
      INT_FIELD(pins, enable),
      INT_FIELD(pins, limit_open),
      INT_FIELD(pins, limit_closed),
      INT_FIELD(pins, hopper_en),
      INT_FIELD(pins, hopper_pulse),
      INT_FIELD(pins, hx_dt),
      INT_FIELD(pins, hx_sck),
    };
    apply_object(j.at("pins"), "io.pins.", map, cfg, err);
  }
  for (auto& [k, _] : j.items()) if (k != "pins") err.push_back("io."+k+" unknown");
}

void parse_mechanics(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    INT_FIELD(mech, steps_per_mm),
    INT_FIELD(mech, open_mm),
    INT_FIELD(mech, max_mm),
  };
  apply_object(j, "mechanics.", map, cfg, err);
}

void parse_hopper(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    INT_FIELD(hopper, pulses_per_coin),
    INT_FIELD(hopper, min_edge_interval_us),
  };
  apply_object(j, "hopper.", map, cfg, err);
}

void parse_dispense(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    INT_FIELD(disp, jam_ms),
    INT_FIELD(disp, settle_ms),
    INT_FIELD(disp, max_ms_per_coin),
    INT_FIELD(disp, hard_timeout_ms),
  };
  apply_object(j, "dispense.", map, cfg, err);
}

void parse_audit(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    DOUBLE_FIELD(audit, coin_mass_g),
    DOUBLE_FIELD(audit, tolerance_per_coin_g),
    DOUBLE_FIELD(audit, grams_per_raw),
    LONG_FIELD(audit, tare_raw),
    INT_FIELD(audit, samples_pre),
    INT_FIELD(audit, samples_post),
    INT_FIELD(audit, settle_ms),
    INT_FIELD(audit, stuck_epsilon_raw),
  };
  apply_object(j, "audit.", map, cfg, err);
}

void parse_presentation(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    INT_FIELD(pres, present_ms),
  };
  apply_object(j, "presentation.", map, cfg, err);
}

void parse_selftest(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(st, enable_coin_test),
  };
  apply_object(j, "selftest.", map, cfg, err);
}

void parse_aws(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(aws, enable),
    STRING_FIELD(aws, endpoint),
    STRING_FIELD(aws, thing_name),
    STRING_FIELD(aws, client_id),
    STRING_FIELD(aws, root_ca),
    STRING_FIELD(aws, cert),
    STRING_FIELD(aws, key),
    STRING_FIELD(aws, topic_prefix),
    INT_FIELD(aws, qos),
    STRING_FIELD(aws, queue_dir),
    UINT64_FIELD(aws, max_queue_bytes),
    BOOL_FIELD(aws, use_tls_identity),
    INT_FIELD(aws, rotation_check_minutes),
  };
  apply_object(j, "aws.", map, cfg, err);
}

void parse_security(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(security, enable_ro_root),
    BOOL_FIELD(security, ssh_keys_only),
    BOOL_FIELD(security, firewall_enable),
    STRING_FIELD(security, allow_ssh_from),
    BOOL_FIELD(security, auditd_enable),
    STRING_FIELD(security, service_user),
  };
  apply_object(j, "security.", map, cfg, err);
}

void parse_identity(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(identity, use_tpm),
    STRING_FIELD(identity, tpm_parent_handle),
    STRING_FIELD(identity, ca_endpoint),
    STRING_FIELD(identity, cert_dir),
    STRING_FIELD(identity, device_id_prefix),
  };
  apply_object(j, "identity.", map, cfg, err);
}

void parse_safety(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    INT_FIELD(safety, estop_pin),
    INT_FIELD(safety, lid_tamper_pin),
    INT_FIELD(safety, overcurrent_pin),
    BOOL_FIELD(safety, active_high),
    INT_FIELD(safety, debounce_ms),
  };
  apply_object(j, "safety.", map, cfg, err);
}

void parse_service(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(service, enable),
    STRING_FIELD(service, pin_code),
    INT_FIELD(service, jam_clear_shutter_mm),
    INT_FIELD(service, jam_clear_cycles),
    INT_FIELD(service, hopper_nudge_ms),
    INT_FIELD(service, hopper_max_retries),
    STRING_FIELD(service, audit_path),
  };
  apply_object(j, "service.", map, cfg, err);
}

void parse_pos(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(pos, enable_http),
    INT_FIELD(pos, port),
    STRING_FIELD(pos, key),
    STRING_FIELD(pos, tls_cert),
    STRING_FIELD(pos, tls_key),
  };
  apply_object(j, "pos.", map, cfg, err);
}

void parse_ota(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(ota, enable),
    STRING_FIELD(ota, backend),
    STRING_FIELD(ota, feed_url),
    STRING_FIELD(ota, channel),
    STRING_FIELD(ota, ring),
    INT_FIELD(ota, poll_seconds),
    STRING_FIELD(ota, state_dir),
    STRING_FIELD(ota, key_pub),
    INT_FIELD(ota, health_grace_seconds),
    BOOL_FIELD(ota, require_signed),
  };
  apply_object(j, "ota.", map, cfg, err);
}

void parse_manufacturing(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(mfg, enable_first_boot_eol),
    STRING_FIELD(mfg, label_printer_uri),
    STRING_FIELD(mfg, label_template),
    STRING_FIELD(mfg, company_name),
  };
  apply_object(j, "manufacturing.", map, cfg, err);
}

void parse_eol(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    DOUBLE_FIELD(eol, weigh_tolerance_g),
    INT_FIELD(eol, coins_for_test),
    INT_FIELD(eol, open_mm),
    INT_FIELD(eol, present_ms),
    INT_FIELD(eol, retries),
    INT_FIELD(eol, min_success_txn_visible_seconds),
    STRING_FIELD(eol, result_dir),
  };
  apply_object(j, "eol.", map, cfg, err);
}

void parse_burnin(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    INT_FIELD(burnin, cycles),
    INT_FIELD(burnin, rest_ms),
    BOOL_FIELD(burnin, abort_on_fault),
    STRING_FIELD(burnin, log_path),
  };
  apply_object(j, "burnin.", map, cfg, err);
}

void parse_quant(const json& j, Config& cfg, Errors& err) {
  static const std::unordered_map<std::string,JSetter> map = {
    BOOL_FIELD(quant, enable),
    STRING_FIELD(quant, endpoint),
    STRING_FIELD(quant, client_id),
    STRING_FIELD(quant, hmac_key_hex),
    STRING_FIELD(quant, topic),
    STRING_FIELD(quant, queue_dir),
    STRING_FIELD(quant, backoff_ms),
    INT_FIELD(quant, heartbeat_seconds),
    INT_FIELD(quant, reserve_floor_cents),
    INT_FIELD(quant, max_daily_outflow_cents),
    INT_FIELD(quant, min_step_change_cents),
    INT_FIELD(quant, max_update_rate_hz),
    INT_FIELD(quant, tolerance_cents),
  };
  apply_object(j, "quant.", map, cfg, err);
}

} // namespace

const Config& defaults() {
  static Config d = make_defaults();
  return d;
}

LoadResult load() {
  LoadResult res;
  res.config = defaults();
  const char* path = std::getenv("REGISTER_MVP_CONFIG");
  if (!path || std::string(path).empty()) {
    return res;
  }
  std::ifstream in(path);
  if (!in) {
    res.errors.push_back(std::string("unable to open config file ") + path);
    return res;
  }
  json j;
  try {
    in >> j;
  } catch (const std::exception& e) {
    res.errors.push_back(std::string("invalid json: ") + e.what());
    return res;
  }
  res.config.source_path = path;

  if (j.contains("io")) parse_io(j["io"], res.config, res.errors);
  if (j.contains("mechanics")) parse_mechanics(j["mechanics"], res.config, res.errors);
  if (j.contains("hopper")) parse_hopper(j["hopper"], res.config, res.errors);
  if (j.contains("dispense")) parse_dispense(j["dispense"], res.config, res.errors);
  if (j.contains("audit")) parse_audit(j["audit"], res.config, res.errors);
  if (j.contains("presentation")) parse_presentation(j["presentation"], res.config, res.errors);
  if (j.contains("selftest")) parse_selftest(j["selftest"], res.config, res.errors);
  if (j.contains("aws")) parse_aws(j["aws"], res.config, res.errors);
  if (j.contains("security")) parse_security(j["security"], res.config, res.errors);
  if (j.contains("identity")) parse_identity(j["identity"], res.config, res.errors);
  if (j.contains("safety")) parse_safety(j["safety"], res.config, res.errors);
  if (j.contains("service")) parse_service(j["service"], res.config, res.errors);
  if (j.contains("pos")) parse_pos(j["pos"], res.config, res.errors);
  if (j.contains("ota")) parse_ota(j["ota"], res.config, res.errors);
  if (j.contains("manufacturing")) parse_manufacturing(j["manufacturing"], res.config, res.errors);
  if (j.contains("eol")) parse_eol(j["eol"], res.config, res.errors);
  if (j.contains("burnin")) parse_burnin(j["burnin"], res.config, res.errors);
  if (j.contains("quant")) parse_quant(j["quant"], res.config, res.errors);

  static const std::unordered_set<std::string> known = {
    "io","mechanics","hopper","dispense","audit","presentation","selftest","aws","security","identity","safety","service","pos","ota","manufacturing","eol","burnin","quant"};
  for (auto& [k, _] : j.items()) {
    if (!known.count(k)) res.errors.push_back("unknown section " + k);
  }

  return res;
}

} // namespace cfg

