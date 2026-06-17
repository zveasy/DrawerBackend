#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace cloud::identity {

struct CertificateIdentity {
  std::string device_id;
  std::string fingerprint_sha256;
  std::string state{"active"};  // active, pending, expired, revoked
  std::string issued_at;
  std::string expires_at;
  std::string rotated_from_fingerprint;
};

void to_json(nlohmann::json& j, const CertificateIdentity& identity);
void from_json(const nlohmann::json& j, CertificateIdentity& identity);

class CertificateIdentityStore {
 public:
  explicit CertificateIdentityStore(std::string store_path = "");

  void upsert(CertificateIdentity identity);
  std::optional<CertificateIdentity> find(const std::string& fingerprint) const;
  bool allow(const std::string& fingerprint) const;
  void mark_state(const std::string& fingerprint, const std::string& state);
  void promote_pending(const std::string& pending_fingerprint);

 private:
  void load();
  void save() const;

  std::string store_path_;
  std::unordered_map<std::string, CertificateIdentity> identities_;
};

bool cert_state_allows_device(const CertificateIdentity& identity);

}  // namespace cloud::identity
