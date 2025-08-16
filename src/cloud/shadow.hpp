#pragma once
#include <map>
#include <string>

namespace shadow {
  inline std::string topic_update_delta(const std::string& thing){ return "$aws/things/"+thing+"/shadow/update/delta"; }
  inline std::string topic_update(const std::string& thing){ return "$aws/things/"+thing+"/shadow/update"; }
  inline std::string topic_get(const std::string& thing){ return "$aws/things/"+thing+"/shadow/get"; }
  inline std::string topic_get_accepted(const std::string& thing){ return "$aws/things/"+thing+"/shadow/get/accepted"; }
  std::string build_reported(const std::map<std::string,std::string>& kv);
  std::map<std::string,std::string> parse_desired(const std::string& json);
}

