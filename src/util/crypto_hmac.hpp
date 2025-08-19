#pragma once
#include <string>
#include <string_view>

namespace util {

// Computes HMAC-SHA256 over `data` using `key_hex` (hex encoded key).
// Returns lowercase hex encoded digest.
std::string hmac_sha256_hex(std::string_view key_hex, std::string_view data);

} // namespace util
