#include <gtest/gtest.h>
#include "obs/metrics.hpp"

using namespace obs;

TEST(MetricsHistogram, Buckets) {
  auto buckets = std::vector<double>{50,100,200,400,800,1600,3200,6400};
  auto& h = M().histogram("register_purchase_duration_ms","",buckets);
  h.observe(80);
  h.observe(500);
  EXPECT_EQ(2, h.count());
  EXPECT_DOUBLE_EQ(580, h.sum());
  auto counts = h.bucket_counts();
  // cumulative counts
  EXPECT_EQ(0, counts[0]);
  EXPECT_EQ(1, counts[1]);
  EXPECT_EQ(1, counts[2]);
  EXPECT_EQ(1, counts[3]);
  EXPECT_EQ(2, counts[4]);
  EXPECT_EQ(2, counts[5]);
  EXPECT_EQ(2, counts[6]);
  EXPECT_EQ(2, counts[7]);
  EXPECT_EQ(2, counts[8]);
}
