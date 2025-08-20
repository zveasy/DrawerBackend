#include <gtest/gtest.h>
#include "cloud/tls_identity.hpp"
#include "config/config.hpp"
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <cstdlib>

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
  setenv("AWS_KEY_TEST", key.c_str(), 1);
  setenv("AWS_CERT_TEST", cert.c_str(), 1);
  c.aws.key.env = "AWS_KEY_TEST";
  c.aws.cert.env = "AWS_CERT_TEST";
  c.identity.use_tpm = false;
  EXPECT_NO_THROW(cloud::TlsIdentity::load_from_config(c));
  chmod(key.c_str(), 0644);
  EXPECT_THROW(cloud::TlsIdentity::load_from_config(c), std::runtime_error);
}
