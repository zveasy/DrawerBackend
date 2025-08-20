#include "util/cli_options.hpp"
#include "util/cxxopts.hpp"

#include <cstdlib>
#include <vector>

int CliOptions::parse(int argc, char** argv) {
  std::string subcmd;
  if (argc > 1 && argv[1][0] != '-') {
    subcmd = argv[1];
    --argc;
    ++argv;
  }

  cxxopts::Options options("register_mvp", "Register MVP command line");
  options.allow_unrecognised_options();
  options.add_options()(
      "json", "Enable JSON log output",
      cxxopts::value<bool>(log_json)->default_value("false")->implicit_value("true"))(
      "demo-shutter", "Demo shutter",
      cxxopts::value<bool>(demo_shutter)->default_value("false")->implicit_value("true"))(
      "with-scale", "Use scale when dispensing",
      cxxopts::value<bool>(use_scale)->default_value("false")->implicit_value("true"))(
      "dispense", "Dispense N coins", cxxopts::value<int>(dispense_n))(
      "purchase", "Purchase amount in cents", cxxopts::value<int>(purchase_cents))(
      "deposit", "Deposit amount in cents", cxxopts::value<int>(deposit_cents))(
      "selftest", "Run self-test",
      cxxopts::value<bool>(do_selftest)->default_value("false")->implicit_value("true"))(
      "service", "Enter service mode",
      cxxopts::value<bool>(service_cli)->default_value("false")->implicit_value("true"))(
      "pin", "Service PIN", cxxopts::value<std::string>(service_pin))(
      "api", "Run API server on optional port",
      cxxopts::value<int>(api_port)->default_value("8080")->implicit_value("8080"))(
      "tui", "Run API server with TUI", cxxopts::value<int>(api_port)->implicit_value("8080"))(
      "aws", "Enable AWS (0 or 1)", cxxopts::value<int>()->default_value("0"))(
      "pos-http", "Run POS HTTP on optional port",
      cxxopts::value<int>(pos_port)->default_value("-1")->implicit_value("-1"))(
      "compliance-mode", "Compliance mode", cxxopts::value<std::string>())(
      "prescan-cycle", "Prescan cycles", cxxopts::value<int>(prescan_cycles)->default_value("0"));

  auto result = options.parse(argc, argv);

  if (subcmd == "selftest") {
    do_selftest = true;
  } else if (subcmd == "api") {
    run_api = true;
    auto rest = result.unmatched();
    if (!rest.empty()) api_port = std::stoi(rest[0]);
  } else if (subcmd == "dispense") {
    auto rest = result.unmatched();
    if (!rest.empty()) dispense_n = std::stoi(rest[0]);
  }

  if (result.count("dispense")) dispense_n = result["dispense"].as<int>();
  if (result.count("purchase")) purchase_cents = result["purchase"].as<int>();
  if (result.count("deposit")) deposit_cents = result["deposit"].as<int>();
  if (result.count("api")) {
    run_api = true;
    api_port = result["api"].as<int>();
  }
  if (result.count("tui")) {
    run_api = true;
    run_tui = true;
    api_port = result["tui"].as<int>();
  }
  if (result.count("pos-http")) {
    pos_http = true;
    pos_port = result["pos-http"].as<int>();
  }
  if (result.count("aws")) aws_flag = result["aws"].as<int>() != 0;
  if (result.count("compliance-mode")) {
    auto m = result["compliance-mode"].as<std::string>();
    if (m == "emi_worst")
      comp_mode = compliance::ComplianceMode::EMI_WORST;
    else if (m == "emi_idle")
      comp_mode = compliance::ComplianceMode::EMI_IDLE;
    else if (m == "esd_safe")
      comp_mode = compliance::ComplianceMode::ESD_SAFE;
  }

  if (log_json) setenv("LOG_JSON", "1", 1);

  return 0;
}
