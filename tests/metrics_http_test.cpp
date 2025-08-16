#include <gtest/gtest.h>
#include <httplib.h>
#include "obs/metrics_endpoint.hpp"
#include "obs/metrics.hpp"

using namespace obs;

TEST(MetricsHttp, Endpoints) {
  auto buckets = std::vector<double>{50,100,200,400,800,1600,3200,6400};
  M().gauge("register_device_up","",{}).set(1);
  M().histogram("register_dispense_duration_ms","",buckets).observe(100);
  MetricsEndpoint ep; ASSERT_TRUE(ep.start("127.0.0.1",0));
  httplib::Client cli("127.0.0.1", ep.port());
  auto res = cli.Get("/metrics");
  ASSERT_TRUE(res && res->status==200);
  EXPECT_NE(std::string::npos, res->body.find("register_device_up"));
  auto resj = cli.Get("/metrics.json");
  ASSERT_TRUE(resj && resj->status==200);
  EXPECT_NE(std::string::npos, resj->body.find("register_dispense_duration_ms_bucket"));
  ep.stop();
}
