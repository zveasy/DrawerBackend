#pragma once

#include <string>

#include "compliance/compliance_mode.hpp"

struct CliOptions {
  bool demo_shutter = false;
  bool use_scale = false;
  bool do_selftest = false;
  int dispense_n = -1;
  int purchase_cents = -1;
  int deposit_cents = -1;
  bool run_api = false;
  bool run_tui = false;
  int api_port = 8080;
  bool pos_http = false;
  int pos_port = -1;
  bool aws_flag = false;
  bool service_cli = false;
  std::string service_pin;
  compliance::ComplianceMode comp_mode = compliance::ComplianceMode::NONE;
  int prescan_cycles = 0;
  bool log_json = false;

  int parse(int argc, char** argv);
};
