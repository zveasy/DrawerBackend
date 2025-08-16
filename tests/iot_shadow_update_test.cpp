#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>

#include "cloud/iot_client.hpp"
#include "cloud/mqtt_loopback.cpp"
#include "cloud/shadow.hpp"
#include "config/config.hpp"

using namespace std::chrono_literals;

static std::string tempdir() {
  auto base = std::filesystem::temp_directory_path();
  auto path = base / std::filesystem::path("iotq-XXXXXX");
  std::string s = path.string();
  mkdtemp(s.data());
  return s;
}

TEST(IoTShadow, UpdateDesired) {
  cloud::LoopbackMqttClient mqtt; mqtt.set_online(true);
  cloud::IoTOptions opt; opt.topic_prefix="register/REG-01"; opt.thing_name="REG-01"; opt.queue_dir=tempdir();
  cloud::IoTClient client(mqtt, opt); client.start();
  cfg::Config cfg; cfg.hopper.pulses_per_coin = 1; cfg.pres.present_ms = 2000;
  client.set_shadow_callback([&](const std::map<std::string,std::string>& kv){
    auto it = kv.find("hopper.pulses_per_coin");
    if(it!=kv.end()) cfg.hopper.pulses_per_coin = std::stoi(it->second);
    auto it2 = kv.find("presentation.present_ms");
    if(it2!=kv.end()) cfg.pres.present_ms = std::stoi(it2->second);
  });
  std::string delta = "{\"state\":{\"hopper\":{\"pulses_per_coin\":2},\"presentation\":{\"present_ms\":1500}}}";
  mqtt.publish({shadow::topic_update_delta("REG-01"), delta, 1, false});
  for(int i=0;i<10 && mqtt.published().size()<2; ++i) std::this_thread::sleep_for(50ms);
  EXPECT_EQ(cfg.hopper.pulses_per_coin, 2);
  EXPECT_EQ(cfg.pres.present_ms, 1500);
  ASSERT_GE(mqtt.published().size(), 2u);
  EXPECT_EQ(mqtt.published().back().topic, shadow::topic_update("REG-01"));
  client.stop();
}

