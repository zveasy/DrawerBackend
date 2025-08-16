#pragma once
#include <string>
namespace safety { class FaultManager; }
namespace eventlog { class Logger; }

namespace compliance {

enum class ComplianceMode { NONE, EMI_WORST, EMI_IDLE, ESD_SAFE };

void init(safety::FaultManager* fm, eventlog::Logger* elog);
void set_mode(ComplianceMode m);
ComplianceMode mode();

// short bounded cycle for pre-scan worst-case pattern
void run_emi_worst_pattern_once();

std::string to_string(ComplianceMode m);

} // namespace compliance
