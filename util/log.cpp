#include "log.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>

namespace util {
namespace {
bool json_mode() {
  const char* e = std::getenv("LOG_JSON");
  return e && std::string(e) == "1";
}
std::string level_str(LogLevel l) {
  switch(l){
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
  }
  return "";
}
std::string timestamp() {
  using namespace std::chrono;
  auto now = system_clock::now();
  std::time_t t = system_clock::to_time_t(now);
  std::tm tm = *std::gmtime(&t);
  char buf[32];
  std::strftime(buf, sizeof(buf), "%FT%TZ", &tm);
  return buf;
}
}

void log(LogLevel lvl, const std::string& msg,
         const std::map<std::string,std::string>& kv) {
  if (json_mode()) {
    std::ostringstream oss;
    oss << "{\"ts\":\"" << timestamp() << "\",\"level\":\"" << level_str(lvl)
        << "\",\"msg\":\"" << msg << "\"";
    for (const auto& p : kv) {
      oss << ",\"" << p.first << "\":\"" << p.second << "\"";
    }
    oss << "}";
    std::cout << oss.str() << std::endl;
  } else {
    std::cerr << "[" << level_str(lvl) << "] " << msg;
    for (const auto& p:kv) std::cerr << ' ' << p.first << '=' << p.second;
    std::cerr << std::endl;
  }
}

} // namespace util
