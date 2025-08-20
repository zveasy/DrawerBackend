#include "cloud/tls_identity.hpp"
#include "util/log.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <stdexcept>

namespace cloud {

bool TlsIdentity::validate_perms(const std::string& key_path) {
  struct stat st{};
  if (stat(key_path.c_str(), &st) != 0) return false;
  // Refuse group/world readable
  return (st.st_mode & 0077) == 0;
}

TlsCreds TlsIdentity::load_from_config(const cfg::Config& c) {
  TlsCreds tc;
  auto cert_path = c.aws.cert.load();
  std::ifstream cert_in(cert_path);
  if (!cert_in) throw std::runtime_error("missing cert");
  std::stringstream ss; ss << cert_in.rdbuf();
  tc.cert_pem = ss.str();
  tc.key_ref = c.aws.key.load();
  tc.key_is_engine = c.identity.use_tpm;
  if (!tc.key_is_engine) {
    auto key_path = c.aws.key.load();
    if (!validate_perms(key_path)) {
      LOG_ERROR("weak key permissions", {{"key", key_path}});
      throw std::runtime_error("weak key permissions");
    }
  }
  return tc;
}

} // namespace cloud
