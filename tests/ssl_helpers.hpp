#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace test_ssl {

inline std::string shell_quote(const std::string& value) {
  std::string out = "'";
  for (char c : value) {
    if (c == '\'') {
      out += "'\\''";
    } else {
      out += c;
    }
  }
  out += "'";
  return out;
}

}  // namespace test_ssl

// Write a short-lived self-signed certificate and key to the given directory.
// Paths to the created files are returned via cert and key arguments.
inline void write_test_cert(const std::filesystem::path& dir, std::string& cert, std::string& key) {
  std::filesystem::create_directories(dir);
  cert = (dir / "cert.pem").string();
  key = (dir / "key.pem").string();
  auto serial = (dir / "serial.srl").string();
  std::filesystem::remove(cert);
  std::filesystem::remove(key);
  std::filesystem::remove(serial);

  std::string cmd =
      "openssl req -x509 -newkey rsa:2048 -sha256 -nodes -days 1 "
      "-subj /CN=localhost "
      "-keyout " +
      test_ssl::shell_quote(key) + " -out " + test_ssl::shell_quote(cert) + " >/dev/null 2>&1";
  if (std::system(cmd.c_str()) != 0) {
    throw std::runtime_error("failed to generate test TLS certificate");
  }

  std::ofstream(key, std::ios::app) << "";
  std::ofstream(cert, std::ios::app) << "";
}
