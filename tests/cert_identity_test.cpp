#include <gtest/gtest.h>

#include <filesystem>

#include "cloud/identity/cert_identity.hpp"

TEST(CertIdentity, BlocksExpiredRevokedAndUnknown) {
  cloud::identity::CertificateIdentityStore store;
  store.upsert({"device-1", "fp-active", "active", "", "", ""});
  store.upsert({"device-1", "fp-expired", "expired", "", "", ""});
  store.upsert({"device-1", "fp-revoked", "revoked", "", "", ""});

  EXPECT_TRUE(store.allow("fp-active"));
  EXPECT_FALSE(store.allow("fp-expired"));
  EXPECT_FALSE(store.allow("fp-revoked"));
  EXPECT_FALSE(store.allow("missing"));
}

TEST(CertIdentity, PromotesPendingRotationAndPersists) {
  auto path = std::filesystem::temp_directory_path() / "cert_identity_store.json";
  std::filesystem::remove(path);
  cloud::identity::CertificateIdentityStore store(path.string());
  store.upsert({"device-1", "old-fp", "active", "", "", ""});
  store.upsert({"device-1", "new-fp", "pending", "", "", "old-fp"});
  EXPECT_TRUE(store.allow("new-fp"));
  store.promote_pending("new-fp");

  cloud::identity::CertificateIdentityStore restarted(path.string());
  auto old_id = restarted.find("old-fp");
  auto new_id = restarted.find("new-fp");
  ASSERT_TRUE(old_id);
  ASSERT_TRUE(new_id);
  EXPECT_EQ("expired", old_id->state);
  EXPECT_EQ("active", new_id->state);
}
