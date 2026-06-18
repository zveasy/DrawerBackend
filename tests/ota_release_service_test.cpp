#include <gtest/gtest.h>

#include "ota/release_service.hpp"

TEST(OtaReleaseService, PromotesDevPilotStableAndEvaluatesRollout) {
  ota::ReleaseService service;
  ASSERT_TRUE(service.publish({"dev", "1.2.0", "file://artifact", "abc", "sig", 100, "", "", {}}));
  EXPECT_TRUE(service.promote("1.2.0", "dev", "pilot"));
  EXPECT_TRUE(service.promote("1.2.0", "pilot", "stable"));

  auto decision = service.eligible("device-1", "stable", "1.1.0");
  EXPECT_TRUE(decision.eligible) << decision.reason;
  EXPECT_EQ("1.2.0", decision.manifest.version);
  EXPECT_FALSE(service.audit_log().empty());
}

TEST(OtaReleaseService, RejectsRevokedStaleRollbackAndRollout) {
  ota::ReleaseService service;
  ASSERT_TRUE(service.publish({"pilot", "2.0.0", "file://artifact", "abc", "sig", 0, "1.5.0",
                               "no_rollback_below_min", {"revoked-device"}}));

  EXPECT_EQ("revoked", service.eligible("revoked-device", "pilot", "1.6.0").reason);
  EXPECT_EQ("stale_version", service.eligible("device-1", "pilot", "2.0.0").reason);
  EXPECT_EQ("rollback_protection", service.eligible("device-1", "pilot", "1.0.0").reason);
  EXPECT_EQ("rollout", service.eligible("device-1", "pilot", "1.6.0").reason);
}
