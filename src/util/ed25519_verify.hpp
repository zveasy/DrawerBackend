#pragma once
#include <string>

namespace ed25519 {
// Verify base64 signature against data using Ed25519 public key in PEM format.
bool verify_pem(const std::string& pubkey_pem, const std::string& data, const std::string& sig_b64);
}

