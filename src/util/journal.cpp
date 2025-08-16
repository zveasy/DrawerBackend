#include "journal.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <unistd.h>
#include <fcntl.h>

namespace journal {
static const char* kDir = "data";
static const char* kFile = "data/txn.log";

bool ensure_data_dir() {
  std::error_code ec;
  std::filesystem::create_directories(kDir, ec);
  return !ec;
}

std::string to_json(const Txn& t) {
  char host[64] = {};
  if (::gethostname(host, sizeof(host)) != 0) host[0] = '\0';
  auto now = std::chrono::system_clock::now();
  std::time_t tt = std::chrono::system_clock::to_time_t(now);
  std::tm tm = *std::gmtime(&tt);
  char ts[32];
  std::strftime(ts, sizeof(ts), "%FT%TZ", &tm);
  std::ostringstream oss;
  oss << "{\"ts\":\"" << ts << "\",\"id\":\"" << t.id
      << "\",\"price\":" << t.price
      << ",\"deposit\":" << t.deposit
      << ",\"change\":" << t.change
      << ",\"quarters\":" << t.quarters
      << ",\"phase\":\"" << t.phase
      << "\",\"dispensed\":" << t.dispensed
      << ",\"reason\":\"" << t.reason
      << "\",\"host\":\"" << host << "\"}";
  return oss.str();
}

bool append(const Txn& t) {
  if (!ensure_data_dir()) return false;
  std::string line = to_json(t);
  line.push_back('\n');
  int fd = ::open(kFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd < 0) return false;
  ssize_t w = ::write(fd, line.c_str(), line.size());
  if (w != (ssize_t)line.size()) { ::close(fd); return false; }
  ::fsync(fd);
  ::close(fd);
  return true;
}

static bool parse_line(const std::string& line, Txn& out) {
  auto get_int = [&](const std::string& key, int& dst) {
    auto pos = line.find("\"" + key + "\":");
    if (pos == std::string::npos) return false;
    pos += key.size() + 3;
    size_t end = line.find_first_of(",}", pos);
    dst = std::stoi(line.substr(pos, end - pos));
    return true;
  };
  auto get_str = [&](const std::string& key, std::string& dst) {
    auto pos = line.find("\"" + key + "\":\"");
    if (pos == std::string::npos) return false;
    pos += key.size() + 4;
    size_t end = line.find("\"", pos);
    dst = line.substr(pos, end - pos);
    return true;
  };
  get_str("id", out.id);
  get_int("price", out.price);
  get_int("deposit", out.deposit);
  get_int("change", out.change);
  get_int("quarters", out.quarters);
  get_int("dispensed", out.dispensed);
  get_str("phase", out.phase);
  get_str("reason", out.reason);
  return true;
}

bool load_last(Txn& out) {
  std::ifstream f(kFile);
  if (!f.is_open()) return false;
  std::string line, last;
  while (std::getline(f, line)) {
    if (!line.empty()) last = line;
  }
  if (last.empty()) return false;
  return parse_line(last, out);
}

} // namespace journal
