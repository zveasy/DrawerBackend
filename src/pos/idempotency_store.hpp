#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace pos {

struct IdemRec {
  std::string key;
  std::string payload_hash;
  std::string result_json;
  std::string status; // PENDING, DONE, FAILED
  uint64_t first_ns{0};
  uint64_t last_ns{0};
};

class IdempotencyStore {
public:
  explicit IdempotencyStore(const std::string &dir, int ttl_hours = 24);
  bool open();

  enum class Check { NEW, PENDING, MATCH_DONE, MISMATCH };

  Check check(const std::string &key, const std::string &payload_hash,
              IdemRec *out = nullptr);
  void put_pending(const std::string &key, const std::string &payload_hash);
  void put_done(const std::string &key, const std::string &payload_hash,
                const std::string &result_json);
  void sweep_expired();

private:
  std::string dir_;
  std::string path_;
  int64_t ttl_ns_;
  std::unordered_map<std::string, IdemRec> map_;
  std::mutex mu_;
};

} // namespace pos

