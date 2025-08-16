#include "compliance/compliance_mode.hpp"
#include "safety/faults.hpp"
#include "util/event_log.hpp"
#include "util/fs.hpp"
#include <chrono>
#include <thread>

namespace compliance {

static ComplianceMode g_mode = ComplianceMode::NONE;
static safety::FaultManager* g_fm = nullptr;
static eventlog::Logger* g_elog = nullptr;

void init(safety::FaultManager* fm, eventlog::Logger* elog) {
  g_fm = fm;
  g_elog = elog;
  fsutil::ensure_dir("data/compliance");
}

std::string to_string(ComplianceMode m) {
  switch (m) {
    case ComplianceMode::EMI_WORST: return "emi_worst";
    case ComplianceMode::EMI_IDLE: return "emi_idle";
    case ComplianceMode::ESD_SAFE: return "esd_safe";
    default: return "none";
  }
}

void set_mode(ComplianceMode m) {
  g_mode = m;
  if (g_elog) g_elog->append({{"event","compliance"},{"mode",to_string(m)}});
  if (m == ComplianceMode::ESD_SAFE && g_fm) {
    g_fm->raise(safety::FaultCode::ESTOP, "ESD_SAFE", false);
  }
}

ComplianceMode mode() { return g_mode; }

void run_emi_worst_pattern_once() {
  if (g_fm && !g_fm->check_and_block_motion()) return;
  if (g_elog) g_elog->append({{"event","compliance_cycle"},{"mode","emi_worst"}});
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

} // namespace compliance
