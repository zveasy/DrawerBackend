#include "cloud/disk_queue.hpp"
#include "util/fs.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

DiskQueue::DiskQueue(const std::string& dir, uint64_t max_bytes)
    : dir_(dir), max_bytes_(max_bytes) {}

bool DiskQueue::init() {
  if (!fsutil::ensure_dir(dir_)) return false;
  files_.clear();
  bytes_ = 0;
  for (auto& p : std::filesystem::directory_iterator(dir_)) {
    if (p.is_regular_file()) {
      files_.push_back(p.path().filename().string());
      bytes_ += p.file_size();
    }
  }
  std::sort(files_.begin(), files_.end());
  return true;
}

bool DiskQueue::write_file_(const std::string& path, const MqttMessage& m) {
  std::ofstream out(path, std::ios::binary);
  if (!out) return false;
  out << m.topic << '\t' << m.qos << '\t' << (m.retain?1:0) << '\n';
  out << m.payload << '\n';
  return out.good();
}

bool DiskQueue::enqueue(const MqttMessage& m) {
  std::string header = m.topic + "\t" + std::to_string(m.qos) + "\t" + (m.retain?"1":"0") + "\n";
  uint64_t need = header.size() + m.payload.size() + 1; // newline
  if (bytes_ + need > max_bytes_) return false;
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  std::ostringstream name;
  name << "q-" << ts << '-' << seq_++ << ".msg";
  std::string path = dir_ + "/" + name.str();
  if (!write_file_(path, m)) return false;
  files_.push_back(name.str());
  bytes_ += need;
  return true;
}

bool DiskQueue::read_file_(const std::string& path, MqttMessage& m) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::string header;
  std::getline(in, header);
  std::istringstream hs(header);
  std::string qos, retain;
  if (!std::getline(hs, m.topic, '\t')) return false;
  if (!std::getline(hs, qos, '\t')) return false;
  if (!std::getline(hs, retain, '\t')) return false;
  std::getline(in, m.payload);
  m.qos = std::stoi(qos);
  m.retain = std::stoi(retain) != 0;
  return true;
}

bool DiskQueue::try_flush(IMqttClient& client, int max_msgs) {
  int count = 0;
  while (!files_.empty() && count < max_msgs) {
    std::string fname = files_.front();
    std::string path = dir_ + "/" + fname;
    MqttMessage m;
    if (!read_file_(path, m)) { std::filesystem::remove(path); files_.pop_front(); continue; }
    if (!client.publish(m)) break;
    bytes_ -= std::filesystem::file_size(path);
    std::filesystem::remove(path);
    files_.pop_front();
    count++;
  }
  return files_.empty();
}

