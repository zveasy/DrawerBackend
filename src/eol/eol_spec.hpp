#pragma once
#include <string>
#include <vector>

namespace eol {

struct EolStepResult {
  std::string name;
  bool ok;
  std::string reason;
  double value{0};
};

struct EolResult {
  bool pass{false};
  std::string device_id;
  std::string serial;
  std::string version;
  std::vector<EolStepResult> steps;
  int dispensed{0};
  double expected_g{0};
  double measured_g{0};
  double delta_g{0};
  std::string report_path;
};

} // namespace eol
