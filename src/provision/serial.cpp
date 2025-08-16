#include "provision/serial.hpp"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace provision {

std::string make_serial(const cfg::Config& cfg, std::time_t now) {
  static int seq = 0;
  std::tm tm = *std::gmtime(&now);
  int year = tm.tm_year % 100;
  int week = tm.tm_yday / 7 + 1;
  const char* plant_env = std::getenv("PLANT_CODE");
  std::string plant = plant_env ? plant_env : "PL";
  std::ostringstream oss;
  oss << cfg.identity.device_id_prefix << '-' << std::setw(2) << std::setfill('0') << year
      << std::setw(2) << std::setfill('0') << week
      << '-' << plant << '-' << std::setw(4) << std::setfill('0') << (++seq);
  return oss.str();
}

bool write_serial_file(const std::string& path, const std::string& serial) {
  std::ofstream f(path);
  if (!f) return false;
  f << serial;
  return true;
}

} // namespace provision
