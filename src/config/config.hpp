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
  std::string source_path; // loaded from
  std::vector<std::string> warnings; // invalid keys/values
};

// Load with merge: missing fields take defaults; invalid values -> default + warning.
Config load();
// Expose sane built-in defaults
const Config& defaults();

} // namespace cfg
