#include "cloud/identity/cert_identity.hpp"

#include <filesystem>
#include <fstream>

#include "obs/metrics.hpp"
#include "ops/operational_status.hpp"

namespace cloud::identity {

void to_json(nlohmann::json& j, const CertificateIdentity& identity) {
  j = {{"device_id", identity.device_id},
       {"fingerprint_sha256", identity.fingerprint_sha256},
       {"state", identity.state},
       {"issued_at", identity.issued_at},
       {"expires_at", identity.expires_at},
       {"rotated_from_fingerprint", identity.rotated_from_fingerprint}};
}

void from_json(const nlohmann::json& j, CertificateIdentity& identity) {
  identity.device_id = j.value("device_id", identity.device_id);
  identity.fingerprint_sha256 = j.value("fingerprint_sha256", identity.fingerprint_sha256);
  identity.state = j.value("state", identity.state);
  identity.issued_at = j.value("issued_at", identity.issued_at);
  identity.expires_at = j.value("expires_at", identity.expires_at);
  identity.rotated_from_fingerprint = j.value("rotated_from_fingerprint", identity.rotated_from_fingerprint);
}

bool cert_state_allows_device(const CertificateIdentity& identity) {
  return identity.state == "active" || identity.state == "pending";
}

CertificateIdentityStore::CertificateIdentityStore(std::string store_path)
    : store_path_(std::move(store_path)) {
  load();
}

void CertificateIdentityStore::upsert(CertificateIdentity identity) {
  identities_[identity.fingerprint_sha256] = std::move(identity);
  save();
}

std::optional<CertificateIdentity> CertificateIdentityStore::find(const std::string& fingerprint) const {
  auto it = identities_.find(fingerprint);
  if (it == identities_.end()) return std::nullopt;
  return it->second;
}

bool CertificateIdentityStore::allow(const std::string& fingerprint) const {
  auto found = find(fingerprint);
  bool ok = found && cert_state_allows_device(*found);
  ops::update_cert_identity(found ? found->state : "unknown");
  if (!ok) {
    obs::M().counter("register_cert_identity_rejects_total", "Certificate identity rejects",
                     {{"reason", found ? found->state : "unknown"}})
        .inc();
  }
  return ok;
}

void CertificateIdentityStore::mark_state(const std::string& fingerprint, const std::string& state) {
  auto it = identities_.find(fingerprint);
  if (it != identities_.end()) {
    it->second.state = state;
    save();
  }
}

void CertificateIdentityStore::promote_pending(const std::string& pending_fingerprint) {
  auto it = identities_.find(pending_fingerprint);
  if (it == identities_.end() || it->second.state != "pending") return;
  if (!it->second.rotated_from_fingerprint.empty()) mark_state(it->second.rotated_from_fingerprint, "expired");
  it->second.state = "active";
  save();
}

void CertificateIdentityStore::load() {
  if (store_path_.empty()) return;
  std::ifstream in(store_path_);
  if (!in) return;
  try {
    nlohmann::json j;
    in >> j;
    for (const auto& item : j.value("cert_identities", nlohmann::json::array())) {
      auto identity = item.get<CertificateIdentity>();
      identities_[identity.fingerprint_sha256] = identity;
    }
  } catch (...) {
  }
}

void CertificateIdentityStore::save() const {
  if (store_path_.empty()) return;
  auto parent = std::filesystem::path(store_path_).parent_path();
  if (!parent.empty()) std::filesystem::create_directories(parent);
  nlohmann::json identities = nlohmann::json::array();
  for (const auto& item : identities_) identities.push_back(item.second);
  std::ofstream(store_path_, std::ios::trunc)
      << nlohmann::json{{"schema", 1}, {"cert_identities", identities}}.dump(2) << "\n";
}

}  // namespace cloud::identity
