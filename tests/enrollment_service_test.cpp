#include <gtest/gtest.h>

#include <filesystem>

#include "cloud/control_plane/enrollment_service.hpp"

TEST(EnrollmentService, EnrollsOnceAndPersistsIdentity) {
  auto store = std::filesystem::temp_directory_path() / "enrollment_store.json";
  std::filesystem::remove(store);
  cloud::control_plane::EnrollmentService service(store.string());
  service.add_token({"TOKEN-123456789", "device-1", "merchant-1", "KE-NBO", "pilot", "pilot", 2000});

  auto ok = service.enroll({"device-1", "merchant-1", "KE-NBO", "pilot", "pilot", "TOKEN-123456789"}, 1000);
  ASSERT_TRUE(ok.ok) << ok.reason;
  EXPECT_EQ("merchant-1", ok.twin.merchant_id);
  EXPECT_TRUE(service.identity("device-1"));

  auto reused = service.enroll({"device-1", "merchant-1", "KE-NBO", "pilot", "pilot", "TOKEN-123456789"}, 1000);
  EXPECT_FALSE(reused.ok);
  EXPECT_EQ("reused_token", reused.reason);

  cloud::control_plane::EnrollmentService restarted(store.string());
  auto identity = restarted.identity("device-1");
  ASSERT_TRUE(identity);
  EXPECT_EQ("pilot", identity->deployment_channel);
}

TEST(EnrollmentService, RejectsExpiredMalformedRevokedAndMismatchedTokens) {
  cloud::control_plane::EnrollmentService service;
  service.add_token({"EXPIRED-123456", "device-1", "merchant", "R", "pilot", "pilot", 10});
  service.add_token({"REVOKED-123456", "device-1", "merchant", "R", "pilot", "pilot", 2000, false, true});
  service.add_token({"MISMATCH-123456", "device-2", "merchant", "R", "pilot", "pilot", 2000});

  EXPECT_EQ("expired_token",
            service.enroll({"device-1", "merchant", "R", "pilot", "pilot", "EXPIRED-123456"}, 100).reason);
  EXPECT_EQ("malformed_token",
            service.enroll({"device-1", "merchant", "R", "pilot", "pilot", "bad token"}, 100).reason);
  EXPECT_EQ("revoked_token",
            service.enroll({"device-1", "merchant", "R", "pilot", "pilot", "REVOKED-123456"}, 100).reason);
  EXPECT_EQ("device_mismatch",
            service.enroll({"device-1", "merchant", "R", "pilot", "pilot", "MISMATCH-123456"}, 100).reason);
}
