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
  std::ifstream cert_in(c.aws.cert);
  if (!cert_in) throw std::runtime_error("missing cert");
  std::stringstream ss; ss << cert_in.rdbuf();
  tc.cert_pem = ss.str();
  tc.key_ref = c.aws.key;
  tc.key_is_engine = c.identity.use_tpm;
  if (!tc.key_is_engine) {
    if (!validate_perms(c.aws.key)) {
      LOG_ERROR("weak key permissions", {{"key", c.aws.key}});
      throw std::runtime_error("weak key permissions");
    }
  }
  return tc;
}

} // namespace cloud
