#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <memory>

#include "cloud/iot_client.hpp"

class FakeMqttClient : public IMqttClient {
 public:
  bool online = true;
  std::function<void(const std::string&, const std::string&)> cb;
  std::vector<std::pair<std::string, std::string>> published;
  std::string sub_topic;

  bool connect() override { return online; }
  bool publish(const std::string& topic, const std::string& payload) override {
    if (!online) return false;
    published.emplace_back(topic, payload);
    return true;
  }
  bool subscribe(const std::string& topic) override {
    sub_topic = topic;
    return true;
  }
  void set_message_handler(std::function<void(const std::string&, const std::string&)> f) override {
    cb = std::move(f);
  }
  void emit(const std::string& topic, const std::string& payload) {
    if (cb) cb(topic, payload);
  }
};

static std::string tmpfile_path() {
  char path[] = "/tmp/iotqueueXXXXXX";
  int fd = mkstemp(path);
  if (fd >= 0) close(fd);
  std::string p = path;
  std::remove(p.c_str());
  return p;
}

TEST(IotClient, QueueAndFlushOnReconnect) {
  auto* fake = new FakeMqttClient();
  fake->online = false;
  std::unique_ptr<IMqttClient> ptr(fake);
  std::string qpath = tmpfile_path();
  IotClient client(std::move(ptr), qpath, "thing1");
  client.connect();
  nlohmann::json txn = {{"id", 1}};
  nlohmann::json health = {{"ok", true}};
  client.publish_telemetry(txn, health);
  EXPECT_EQ(fake->published.size(), 0u);
  std::ifstream f(qpath);
  std::string line;
  std::getline(f, line);
  EXPECT_NE(line.find("id"), std::string::npos);
  fake->online = true;
  client.connect();
  EXPECT_EQ(fake->published.size(), 1u);
}

TEST(IotClient, ShadowUpdateCallback) {
  auto* fake = new FakeMqttClient();
  std::unique_ptr<IMqttClient> ptr(fake);
  std::string qpath = tmpfile_path();
  IotClient client(std::move(ptr), qpath, "thing1");
  bool called = false;
  client.set_shadow_callback([&](const nlohmann::json& j) {
    called = true;
    EXPECT_EQ(j.at("pulses"), 10);
  });
  fake->emit("$aws/things/thing1/shadow/update/delta",
             "{\"state\":{\"pulses\":10}}");
  EXPECT_TRUE(called);
}

