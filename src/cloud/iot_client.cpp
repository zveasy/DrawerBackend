#include "cloud/iot_client.hpp"

#include <fstream>
#include <sstream>

#include <mqtt/async_client.h>

using json = nlohmann::json;

// ----- DiskQueue -----
DiskQueue::DiskQueue(const std::string& path) : path_(path) {
  std::ifstream f(path_);
  std::string line;
  while (std::getline(f, line)) {
    if (!line.empty()) q_.push_back(line);
  }
}

void DiskQueue::persist() const {
  std::ofstream f(path_, std::ios::trunc);
  for (const auto& s : q_) {
    f << s << '\n';
  }
}

void DiskQueue::push(const std::string& msg) {
  q_.push_back(msg);
  persist();
}

void DiskQueue::pop() {
  if (!q_.empty()) {
    q_.pop_front();
    persist();
  }
}

// ----- IotClient -----
IotClient::IotClient(std::unique_ptr<IMqttClient> mqtt,
                     const std::string& queue_path,
                     const std::string& thing_id)
    : mqtt_(std::move(mqtt)), queue_(queue_path), thing_id_(thing_id) {
  mqtt_->set_message_handler(
      [this](const std::string& topic, const std::string& payload) {
        on_message(topic, payload);
      });
}

bool IotClient::connect() {
  if (!mqtt_->connect()) return false;
  mqtt_->subscribe("$aws/things/" + thing_id_ + "/shadow/update/delta");
  flush_queue();
  return true;
}

void IotClient::publish_telemetry(const json& txn, const json& health) {
  json msg;
  msg["txn"] = txn;
  msg["health"] = health;
  std::string payload = msg.dump();
  std::string topic = "register/" + thing_id_ + "/telemetry";
  if (!mqtt_->publish(topic, payload)) {
    queue_.push(payload);
  }
}

void IotClient::update_shadow_reported(const json& reported) {
  json j;
  j["state"]["reported"] = reported;
  mqtt_->publish("$aws/things/" + thing_id_ + "/shadow/update", j.dump());
}

void IotClient::flush_queue() {
  while (!queue_.empty()) {
    std::string msg = queue_.front();
    if (mqtt_->publish("register/" + thing_id_ + "/telemetry", msg)) {
      queue_.pop();
    } else {
      break;
    }
  }
}

void IotClient::on_message(const std::string& topic,
                           const std::string& payload) {
  if (topic.find("/shadow/update/delta") != std::string::npos) {
    try {
      auto j = json::parse(payload);
      if (shadow_cb_ && j.contains("state")) shadow_cb_(j["state"]);
    } catch (...) {
    }
  }
}

// ----- Paho MQTT implementation -----
class PahoMqttClient : public IMqttClient, public virtual mqtt::callback {
  mqtt::async_client client_;
  mqtt::connect_options conn_;
  std::function<void(const std::string&, const std::string&)> cb_;

public:
  PahoMqttClient(const std::string& endpoint, const std::string& client_id,
                 const std::string& cert, const std::string& key,
                 const std::string& ca)
      : client_("ssl://" + endpoint + ":8883", client_id) {
    mqtt::ssl_options ssl;
    ssl.set_trust_store(ca);
    ssl.set_key_store(cert);
    ssl.set_private_key(key);
    conn_.set_clean_session(true);
    conn_.set_ssl(ssl);
    client_.set_callback(*this);
  }

  bool connect() override {
    try {
      client_.connect(conn_)->wait();
      return true;
    } catch (...) {
      return false;
    }
  }

  bool publish(const std::string& topic, const std::string& payload) override {
    try {
      auto msg = mqtt::make_message(topic, payload);
      client_.publish(msg)->wait();
      return true;
    } catch (...) {
      return false;
    }
  }

  bool subscribe(const std::string& topic) override {
    try {
      client_.subscribe(topic, 1)->wait();
      return true;
    } catch (...) {
      return false;
    }
  }

  void set_message_handler(
      std::function<void(const std::string&, const std::string&)> cb) override {
    cb_ = std::move(cb);
  }

  void message_arrived(mqtt::const_message_ptr msg) override {
    if (cb_) cb_(msg->get_topic(), msg->to_string());
  }

  void connection_lost(const std::string&) override {}
};

std::unique_ptr<IMqttClient> make_paho_client(const std::string& endpoint,
                                              const std::string& client_id,
                                              const std::string& cert,
                                              const std::string& key,
                                              const std::string& ca) {
  return std::make_unique<PahoMqttClient>(endpoint, client_id, cert, key, ca);
}

