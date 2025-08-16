#pragma once
#include <functional>
#include <string>
#include <vector>

struct MqttMessage {
  std::string topic;
  std::string payload;  // UTF-8 JSON
  int qos{1};
  bool retain{false};
};

// Generic callback for inbound MQTT messages
using MqttHandler = std::function<void(const MqttMessage&)>;

class IMqttClient {
public:
  virtual ~IMqttClient() = default;
  virtual bool connect() = 0;
  virtual void disconnect() = 0;
  virtual bool publish(const MqttMessage& m) = 0;
  virtual bool subscribe(const std::string& topic_filter, int qos, MqttHandler cb) = 0;
  virtual bool is_connected() const = 0;
};

