#pragma once
#include "cloud/mqtt_iface.hpp"
#include <map>
#include <mutex>

namespace cloud {

class LoopbackMqttClient : public IMqttClient {
 public:
  LoopbackMqttClient() = default;
  void set_online(bool on) { online_ = on; if(!on) connected_ = false; }

  bool connect() override {
    connected_ = online_;
    return connected_;
  }

  void disconnect() override { connected_ = false; }

  bool publish(const MqttMessage& m) override {
    if (!connected_) return false;
    published_.push_back(m);
    for (auto& [topic, sub] : subs_) {
      if (topic == m.topic && sub.cb) sub.cb(m);
    }
    return true;
  }

  bool subscribe(const std::string& topic_filter, int qos, MqttHandler cb) override {
    subs_[topic_filter] = {qos, std::move(cb)};
    return true;
  }

  bool is_connected() const override { return connected_; }

  const std::vector<MqttMessage>& published() const { return published_; }

 private:
  struct Sub { int qos; MqttHandler cb; };
  bool online_{false};
  bool connected_{false};
  std::map<std::string, Sub> subs_;
  std::vector<MqttMessage> published_;
};

} // namespace cloud

