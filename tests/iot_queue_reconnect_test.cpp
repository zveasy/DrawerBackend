#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>

#include "cloud/iot_client.hpp"
#include "cloud/mqtt_loopback.cpp"
#include "util/journal.hpp"

using namespace std::chrono_literals;

static std::string tempdir() {
  auto base = std::filesystem::temp_directory_path();
  auto path = base / std::filesystem::path("iotq-XXXXXX");
  std::string s = path.string();
  mkdtemp(s.data());
  return s;
}

TEST(IoTQueue, ReconnectFlush) {
  cloud::LoopbackMqttClient mqtt; // starts offline
  cloud::IoTOptions opt; opt.topic_prefix="register/REG-01"; opt.thing_name="REG-01"; opt.queue_dir=tempdir();
  cloud::IoTClient client(mqtt, opt);
  client.start();
  journal::Txn t; t.id="1"; t.phase="DONE";
  for(int i=0;i<3;i++){ t.id = std::to_string(i); client.publish_txn(t); }
  EXPECT_EQ(mqtt.published().size(), 0u);
  mqtt.set_online(true);
  for(int i=0;i<20 && mqtt.published().size()<3; ++i) std::this_thread::sleep_for(50ms);
  EXPECT_EQ(mqtt.published().size(), 3u);
  std::filesystem::path qdir(opt.queue_dir);
  size_t remaining = std::distance(std::filesystem::directory_iterator(qdir), std::filesystem::directory_iterator());
  EXPECT_EQ(remaining, 0u);
  client.stop();
}

