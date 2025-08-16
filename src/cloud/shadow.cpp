#include "cloud/shadow.hpp"
#include <sstream>
#ifdef USE_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

namespace shadow {

std::string build_reported(const std::map<std::string,std::string>& kv) {
#ifdef USE_NLOHMANN_JSON
  nlohmann::json reported;
  for (const auto& [k,v] : kv) {
    nlohmann::json* cur = &reported;
    size_t pos=0; size_t next;
    while ((next = k.find('.', pos)) != std::string::npos) {
      std::string part = k.substr(pos, next-pos);
      cur = &((*cur)[part]);
      pos = next+1;
    }
    std::string last = k.substr(pos);
    if (v == "true" || v == "false") (*cur)[last] = (v=="true");
    else {
      char* end=nullptr; long val=strtol(v.c_str(), &end, 10);
      if (end && *end=='\0') (*cur)[last]=val; else (*cur)[last]=v;
    }
  }
  nlohmann::json root; root["state"]["reported"] = reported;
  return root.dump();
#else
  std::ostringstream oss; oss << "{"; bool first=true; for(auto& [k,v]:kv){ if(!first) oss<<","; first=false; oss<<"\""<<k<<"\":\""<<v<<"\""; } oss<<"}"; return oss.str();
#endif
}

std::map<std::string,std::string> parse_desired(const std::string& json) {
  std::map<std::string,std::string> out;
#ifdef USE_NLOHMANN_JSON
  try {
    auto j = nlohmann::json::parse(json);
    if (!j.contains("state")) return out;
    auto& st = j["state"];
    if (st.contains("hopper") && st["hopper"].contains("pulses_per_coin")) {
      out["hopper.pulses_per_coin"] = st["hopper"]["pulses_per_coin"].dump();
    }
    if (st.contains("presentation") && st["presentation"].contains("present_ms")) {
      out["presentation.present_ms"] = st["presentation"]["present_ms"].dump();
    }
  } catch (...) {}
#endif
  return out;
}

} // namespace shadow

