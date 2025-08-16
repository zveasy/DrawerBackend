#include "cloud/iot_client.hpp"
#include "cloud/shadow.hpp"
#include <chrono>
#include <sstream>

using namespace std::chrono_literals;

namespace cloud {

IoTClient::IoTClient(IMqttClient& mqtt, const IoTOptions& opt)
    : mqtt_(mqtt), opt_(opt), queue_(opt.queue_dir, opt.max_queue_bytes) {}

IoTClient::~IoTClient() { stop(); }

bool IoTClient::start() {
  queue_.init();
  mqtt_.subscribe(shadow::topic_update_delta(opt_.thing_name), opt_.qos,
                  [this](const MqttMessage& m){ on_shadow_delta_(m); });
  bool ok = mqtt_.connect();
  running_ = true;
  bg_ = std::thread(&IoTClient::bg_flush_loop_, this);
  return ok;
}

void IoTClient::stop() {
  running_ = false;
  if (bg_.joinable()) bg_.join();
  mqtt_.disconnect();
}

bool IoTClient::publish_txn(const journal::Txn& t) {
  MqttMessage m;
  m.topic = opt_.topic_prefix + "/telemetry";
  m.qos = opt_.qos;
  m.payload = journal::to_json(t);
  if (mqtt_.is_connected() && mqtt_.publish(m)) return true;
  return queue_.enqueue(m);
}

bool IoTClient::publish_health(const std::string& json) {
  MqttMessage m{opt_.topic_prefix + "/telemetry", json, opt_.qos, false};
  if (mqtt_.is_connected() && mqtt_.publish(m)) return true;
  return queue_.enqueue(m);
}

void IoTClient::set_shadow_callback(std::function<void(const std::map<std::string,std::string>&)> cb) {
  cb_ = std::move(cb);
}

bool IoTClient::publish_reported(const std::map<std::string,std::string>& kv) {
  MqttMessage m{shadow::topic_update(opt_.thing_name), shadow::build_reported(kv), opt_.qos, false};
  if (mqtt_.is_connected() && mqtt_.publish(m)) return true;
  return queue_.enqueue(m);
}

void IoTClient::bg_flush_loop_() {
  while (running_) {
    if (!mqtt_.is_connected()) {
      mqtt_.connect();
    } else {
      queue_.try_flush(mqtt_);
    }
    std::this_thread::sleep_for(100ms);
  }
}

void IoTClient::on_shadow_delta_(const MqttMessage& m) {
  auto kv = shadow::parse_desired(m.payload);
  if (!kv.empty()) {
    if (cb_) cb_(kv);
    publish_reported(kv);
  }
}

} // namespace cloud

