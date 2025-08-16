#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace cfg {

struct Pins { int step,dir,enable,limit_open,limit_closed,hopper_en,hopper_pulse,hx_dt,hx_sck; };
struct Mechanics { int steps_per_mm, open_mm, max_mm; };
struct Hopper { int pulses_per_coin, min_edge_interval_us; };
struct Dispense { int jam_ms, settle_ms, max_ms_per_coin, hard_timeout_ms; };
struct Audit { double coin_mass_g, tolerance_per_coin_g, grams_per_raw; long tare_raw; int samples_pre, samples_post, settle_ms, stuck_epsilon_raw; };
struct Presentation { int present_ms; };
struct SelfTest { bool enable_coin_test; };
struct Aws {
  bool enable{false};
  std::string endpoint;
  std::string thing_name;
  std::string client_id;
  std::string root_ca;
  std::string cert;
  std::string key;
  std::string topic_prefix;
  int qos{1};
  std::string queue_dir{"data/awsq"};
  uint64_t max_queue_bytes{8ull<<20};
  bool use_tls_identity{0};
  int rotation_check_minutes{60};
};

struct Safety {
  int estop_pin{-1};
  int lid_tamper_pin{-1};
  int overcurrent_pin{-1};
  bool active_high{true};
  int debounce_ms{10};
};

struct Service {
  bool enable{true};
  std::string pin_code{"1234"};
  int jam_clear_shutter_mm{3};
  int jam_clear_cycles{3};
  int hopper_nudge_ms{150};
  int hopper_max_retries{2};
  std::string audit_path{"data/service.log"};
};

struct Pos {
  bool enable_http{true};
  int port{9090};
  std::string key;
};

struct Security {
  bool enable_ro_root{0};
  bool ssh_keys_only{1};
  bool firewall_enable{1};
  std::string allow_ssh_from{""};
  bool auditd_enable{1};
  std::string service_user{"registermvp"};
};

struct Identity {
  bool use_tpm{0};
  std::string tpm_parent_handle{"0x81000001"};
  std::string ca_endpoint{"http://ca.local/sign"};
  std::string cert_dir{"/etc/register-mvp/certs"};
  std::string device_id_prefix{"REG"};
};

struct Config {
  Pins pins{};
  Mechanics mech{};
  Hopper hopper{};
  Dispense disp{};
  Audit audit{};
  Presentation pres{};
  SelfTest st{};
  Aws aws{};
  Security security{};
  Identity identity{};
  Safety safety{};
  Service service{};
  Pos pos{};
  std::string source_path; // loaded from
  std::vector<std::string> warnings; // invalid keys/values
};

// Load with merge: missing fields take defaults; invalid values -> default + warning.
Config load();
// Expose sane built-in defaults
const Config& defaults();

} // namespace cfg
