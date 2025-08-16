#pragma once
#include <string>
#include "config/config.hpp"

namespace cloud {
struct TlsCreds { std::string cert_pem; std::string key_ref; bool key_is_engine{false}; };
class TlsIdentity {
public:
  static TlsCreds load_from_config(const cfg::Config& c);
  static bool validate_perms(const std::string& key_path);
  template<typename T> static void configure_openssl_ctx(T*, const TlsCreds&) {}
};
}
