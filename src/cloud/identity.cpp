#include "cloud/identity.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <functional>

namespace cloud {
namespace {
std::string hash_str(const std::string& in) {
  std::size_t h = std::hash<std::string>{}(in);
  std::ostringstream oss; oss << std::hex << h;
  return oss.str();
}
}

std::string device_id(const cfg::Config& c) {
  static std::string id;
  if (!id.empty()) return id;
  std::string raw;
  if (c.identity.use_tpm) {
    raw = "tpm"; // placeholder
  } else {
    const char* cpu = std::getenv("RMVP_CPU_SERIAL");
    if (cpu) raw += cpu;
    const char* mid = std::getenv("RMVP_MACHINE_ID");
    if (mid) raw += mid;
    else {
      std::ifstream in("/etc/machine-id");
      if (in) { std::string line; std::getline(in, line); raw += line; }
    }
  }
  id = c.identity.device_id_prefix + hash_str(raw);
  return id;
}

} // namespace cloud
