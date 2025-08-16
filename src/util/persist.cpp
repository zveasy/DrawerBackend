#include "persist.hpp"

#include <fstream>

namespace persist {

bool load_kv(const std::string& path, std::map<std::string, std::string>& out) {
  std::ifstream f(path);
  if (!f.is_open()) return false;
  std::string line;
  while (std::getline(f, line)) {
    auto pos = line.find('=');
    if (pos == std::string::npos) continue;
    std::string key = line.substr(0, pos);
    std::string val = line.substr(pos + 1);
    out[key] = val;
  }
  return true;
}

bool save_kv(const std::string& path, const std::map<std::string, std::string>& kv) {
  std::ofstream f(path);
  if (!f.is_open()) return false;
  for (const auto& p : kv) {
    f << p.first << '=' << p.second << '\n';
  }
  return true;
}

} // namespace persist

