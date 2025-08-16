#include <gtest/gtest.h>

#include "../src/app/audit.hpp"
#include <vector>

class FakeScale : public IScale {
public:
  std::vector<long> seq;
  size_t idx = 0;
  long read_average(int samples = 8) override {
    long sum = 0;
    for (int i = 0; i < samples; ++i) {
      if (idx < seq.size()) sum += seq[idx++];
      else sum += seq.back();
    }
    return sum / samples;
  }
};

TEST(AuditTolerance, WithinTolerance) {
  FakeScale fs;
  fs.seq = {0,0,3,3};
  audit::Config cfg;
  cfg.samples_pre = 2;
  cfg.samples_post = 2;
  cfg.calib.grams_per_raw = 5.67; // so delta_raw=3 -> 17.01g
  auto res = audit::run(&fs, cfg, 3);
  EXPECT_TRUE(res.ok);
  EXPECT_TRUE(res.flags.empty());
  EXPECT_NEAR(res.delta_g, 0.0, 1e-6);
  EXPECT_FALSE(res.skipped);
}
