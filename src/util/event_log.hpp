#pragma once
#include <map>
#include <string>

namespace eventlog {

class Logger {
public:
  explicit Logger(const std::string& path="data/service.log");
  bool append(const std::map<std::string,std::string>& kv);
private:
  std::string path_;
};

} // namespace eventlog

