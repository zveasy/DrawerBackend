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

TEST(AuditFlags, SensorStuck) {
  FakeScale fs;
  fs.seq = {100,100,100,100};
  audit::Config cfg;
  cfg.samples_pre = 2;
  cfg.samples_post = 2;
  auto res = audit::run(&fs, cfg, 3);
  EXPECT_FALSE(res.ok);
  bool found=false;
  for(const auto& f:res.flags) if(f=="SENSOR_STUCK") found=true;
  EXPECT_TRUE(found);
}
