#pragma once
#include <map>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include "config/config.hpp"
#include "hal/gpio.hpp"
#include "util/event_log.hpp"

namespace safety {

enum class FaultCode {
  NONE=0, ESTOP, LID_TAMPER, OVERCURRENT,
  JAM_SHUTTER, JAM_HOPPER, LIMIT_TIMEOUT, SENSOR_FAIL, CONFIG_ERROR
};

struct Fault {
  FaultCode code{FaultCode::NONE};
  std::string reason;
  uint64_t ts_ns{0};
  bool latched{false};
};

class FaultManager {
public:
  explicit FaultManager(const cfg::Safety& s, eventlog::Logger* elog=nullptr);
  ~FaultManager();
  void start();
  void stop();
  void raise(FaultCode code, const std::string& reason, bool latch=true);
  void clear(FaultCode code);
  bool any_active() const;
  std::vector<Fault> active() const;
  bool safety_blocking() const;
  bool check_and_block_motion() const; // true if motion allowed
private:
  void poll_thread();
  bool read_line(hal::Line* ln) const;
  cfg::Safety cfg_;
  eventlog::Logger* elog_;
  std::unique_ptr<hal::Chip> chip_;
  hal::Line *estop_, *lid_, *overcurrent_;
  mutable std::mutex mu_;
  std::map<FaultCode, Fault> faults_;
  bool running_{false};
  std::thread th_;
};

std::string fault_code_str(FaultCode c);

} // namespace safety

