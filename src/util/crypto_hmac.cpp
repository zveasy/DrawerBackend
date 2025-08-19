#include "crypto_hmac.hpp"
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <sstream>
#include <vector>
#include <iomanip>

namespace util {
namespace {
std::vector<unsigned char> hex_to_bytes(std::string_view hex) {
  std::vector<unsigned char> out;
  if (hex.size() % 2) return out;
  out.reserve(hex.size()/2);
  for (size_t i=0; i<hex.size(); i+=2) {
    unsigned int byte;
    std::stringstream ss;
    ss << std::hex << hex.substr(i,2);
    ss >> byte;
    out.push_back(static_cast<unsigned char>(byte));
  }
  return out;
}
} // namespace

std::string hmac_sha256_hex(std::string_view key_hex, std::string_view data) {
  if (key_hex.empty()) return {};
  auto key = hex_to_bytes(key_hex);
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int len = 0;
  HMAC(EVP_sha256(), key.data(), static_cast<int>(key.size()),
       reinterpret_cast<const unsigned char*>(data.data()), data.size(),
       result, &len);
  std::ostringstream oss;
  for (unsigned int i=0; i<len; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(result[i]);
  }
  return oss.str();
}

} // namespace util
