#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <string>

#include <nlohmann/json.hpp>

// Simple MQTT client interface for dependency injection and testing
class IMqttClient {
public:
  virtual ~IMqttClient() = default;
  virtual bool connect() = 0;
  virtual bool publish(const std::string& topic, const std::string& payload) = 0;
  virtual bool subscribe(const std::string& topic) = 0;
  virtual void set_message_handler(
      std::function<void(const std::string&, const std::string&)> cb) = 0;
};

// Persistent disk-backed FIFO queue used for offline store-and-forward
class DiskQueue {
  std::string path_;
  std::deque<std::string> q_;
  void persist() const;

public:
  explicit DiskQueue(const std::string& path);
  void push(const std::string& msg);
  bool empty() const { return q_.empty(); }
  const std::string& front() const { return q_.front(); }
  void pop();
};

// High level AWS IoT Core client
class IotClient {
  std::unique_ptr<IMqttClient> mqtt_;
  DiskQueue queue_;
  std::string thing_id_;
  std::function<void(const nlohmann::json&)> shadow_cb_;

public:
  IotClient(std::unique_ptr<IMqttClient> mqtt, const std::string& queue_path,
            const std::string& thing_id);
  bool connect();
  void publish_telemetry(const nlohmann::json& txn,
                         const nlohmann::json& health);
  void update_shadow_reported(const nlohmann::json& reported);
  void set_shadow_callback(
      std::function<void(const nlohmann::json&)> cb) {
    shadow_cb_ = std::move(cb);
  }

private:
  void flush_queue();
  void on_message(const std::string& topic, const std::string& payload);
};

// Factory creating a Paho MQTT implementation
std::unique_ptr<IMqttClient> make_paho_client(
    const std::string& endpoint, const std::string& client_id,
    const std::string& cert, const std::string& key,
    const std::string& ca);

