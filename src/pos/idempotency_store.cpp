#include "idempotency_store.hpp"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>

namespace pos {

using namespace std::chrono;

IdempotencyStore::IdempotencyStore(const std::string &dir, int ttl_hours)
    : dir_(dir), ttl_ns_(hours(ttl_hours).count() * 1000000000LL) {
  path_ = dir_ + "/idem.jsonl";
}

bool IdempotencyStore::open() {
  std::error_code ec;
  std::filesystem::create_directories(dir_, ec);
  std::ifstream in(path_);
  if (in.is_open()) {
    std::string line;
    auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
    while (std::getline(in, line)) {
      if (line.empty()) continue;
      try {
        auto j = nlohmann::json::parse(line);
        IdemRec r;
        r.key = j["key"].get<std::string>();
        r.payload_hash = j["payload_hash"].get<std::string>();
        r.result_json = j.value("result", "");
        r.status = j["status"].get<std::string>();
        r.first_ns = j.value("first_ns", 0ULL);
        r.last_ns = j.value("last_ns", 0ULL);
        if (ttl_ns_ > 0 && now - (int64_t)r.last_ns > ttl_ns_) continue; // expired
        map_[r.key] = r;
      } catch (...) {
        // ignore malformed lines
      }
    }
  }
  sweep_expired();
  return true;
}

IdempotencyStore::Check IdempotencyStore::check(const std::string &key,
                                                const std::string &payload_hash,
                                                IdemRec *out) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = map_.find(key);
  if (it == map_.end()) return Check::NEW;
  if (out) *out = it->second;
  if (it->second.payload_hash != payload_hash) return Check::MISMATCH;
  if (it->second.status == "PENDING") return Check::PENDING;
  if (it->second.status == "DONE") return Check::MATCH_DONE;
  return Check::NEW;
}

void IdempotencyStore::put_pending(const std::string &key,
                                   const std::string &payload_hash) {
  auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  IdemRec r{key, payload_hash, "", "PENDING", (uint64_t)now, (uint64_t)now};
  {
    std::lock_guard<std::mutex> lk(mu_);
    map_[key] = r;
  }
  nlohmann::json j = {{"key", key},
                      {"payload_hash", payload_hash},
                      {"status", "PENDING"},
                      {"first_ns", r.first_ns},
                      {"last_ns", r.last_ns}};
  std::ofstream out(path_, std::ios::app);
  out << j.dump() << "\n";
}

void IdempotencyStore::put_done(const std::string &key,
                                const std::string &payload_hash,
                                const std::string &result_json) {
  auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  IdemRec r{key, payload_hash, result_json, "DONE", (uint64_t)now,
            (uint64_t)now};
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto &slot = map_[key];
    if (slot.first_ns != 0) r.first_ns = slot.first_ns; // preserve
    slot = r;
  }
  nlohmann::json j = {{"key", key},
                      {"payload_hash", payload_hash},
                      {"status", "DONE"},
                      {"result", result_json},
                      {"first_ns", r.first_ns},
                      {"last_ns", r.last_ns}};
  std::ofstream out(path_, std::ios::app);
  out << j.dump() << "\n";
}

void IdempotencyStore::sweep_expired() {
  if (ttl_ns_ <= 0) return;
  auto now = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
  std::lock_guard<std::mutex> lk(mu_);
  for (auto it = map_.begin(); it != map_.end();) {
    if (now - (int64_t)it->second.last_ns > ttl_ns_)
      it = map_.erase(it);
    else
      ++it;
  }
}

} // namespace pos

