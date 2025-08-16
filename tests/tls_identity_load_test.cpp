#include <gtest/gtest.h>
#include "cloud/tls_identity.hpp"
#include "config/config.hpp"
#include <filesystem>
#include <fstream>
#include <sys/stat.h>

TEST(TlsIdentity, LoadAndPerms) {
  cfg::Config c = cfg::defaults();
  auto dir = std::filesystem::temp_directory_path() / "tlsid";
  std::filesystem::create_directories(dir);
  std::string key = (dir / "key.pem").string();
  std::string cert = (dir / "cert.pem").string();
  {
    std::ofstream k(key); k << "k";
    std::ofstream ce(cert); ce << "c";
  }
  chmod(key.c_str(), 0600);
  c.aws.key = key;
  c.aws.cert = cert;
  c.identity.use_tpm = false;
  EXPECT_NO_THROW(cloud::TlsIdentity::load_from_config(c));
  chmod(key.c_str(), 0644);
  EXPECT_THROW(cloud::TlsIdentity::load_from_config(c), std::runtime_error);
}
