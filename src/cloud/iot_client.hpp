#pragma once
#include <atomic>
#include <thread>
#include <map>
#include <functional>
#include "../app/txn_engine.hpp"
#include "../config/config.hpp"
#include "cloud/mqtt_iface.hpp"
#include "cloud/disk_queue.hpp"

namespace cloud {
  struct IoTOptions {
    std::string topic_prefix;   // e.g., register/REG-01
    std::string thing_name;     // shadow thing
    int qos{1};
    std::string queue_dir{"data/awsq"};
    uint64_t max_queue_bytes{8ull<<20};
  };

  class IoTClient {
  public:
    IoTClient(IMqttClient& mqtt, const IoTOptions& opt);
    ~IoTClient();
    bool start();
    void stop();
    bool publish_txn(const journal::Txn& t);
    bool publish_health(const std::string& json);
    void set_shadow_callback(std::function<void(const std::map<std::string,std::string>&)> cb);
    bool publish_reported(const std::map<std::string,std::string>& kv);
  private:
    void bg_flush_loop_();
    void on_shadow_delta_(const MqttMessage& m);
    IMqttClient& mqtt_;
    IoTOptions opt_;
    DiskQueue queue_;
    std::atomic<bool> running_{false};
    std::thread bg_;
    std::function<void(const std::map<std::string,std::string>&)> cb_;
  };
}

