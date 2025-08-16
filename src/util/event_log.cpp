#include "util/event_log.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <unistd.h>

namespace eventlog {

Logger::Logger(const std::string& path) : path_(path) {
  std::filesystem::create_directories(std::filesystem::path(path_).parent_path());
}

bool Logger::append(const std::map<std::string,std::string>& kv) {
  std::ofstream f(path_, std::ios::app);
  if(!f) return false;
  f << "{";
  bool first=true;
  for(const auto& p: kv){
    if(!first) f << ",";
    first=false;
    f << "\""<<p.first<<"\":\""<<p.second<<"\"";
  }
  f << "}\n";
  f.flush();
  return true;
}

} // namespace eventlog

