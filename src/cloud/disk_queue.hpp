#pragma once
#include <deque>
#include <string>
#include <cstdint>
#include "cloud/mqtt_iface.hpp"

class DiskQueue {
public:
  explicit DiskQueue(const std::string& dir, uint64_t max_bytes = 8ull<<20);
  bool init();
  bool enqueue(const MqttMessage& m);
  bool try_flush(IMqttClient& client, int max_msgs = 100);
  uint64_t size_bytes() const { return bytes_; }
  size_t size_msgs() const { return files_.size(); }
private:
  std::string dir_;
  uint64_t max_bytes_;
  uint64_t bytes_{0};
  uint64_t seq_{0};
  std::deque<std::string> files_;
  bool write_file_(const std::string& path, const MqttMessage& m);
  bool read_file_(const std::string& path, MqttMessage& m);
};

