#include "quant/publisher.hpp"
#include "util/crypto_hmac.hpp"
#include "util/journal.hpp"
#include "config/config.hpp"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <chrono>

#if defined(ENABLE_QUANT)
#include <zmq.hpp>
#endif

namespace quant {

using nlohmann::json;

struct NullPublisher : public Publisher {
  void publish_purchase(const journal::Txn&, int, int, const std::string&) override {}
};

#if defined(ENABLE_QUANT)
class ZmqPublisher : public Publisher {
public:
  explicit ZmqPublisher(const Options &opt)
    : opt_(opt), ctx_(1), sock_(ctx_, zmq::socket_type::pub) {
    sock_.connect(opt_.endpoint);
  }

  void publish_purchase(const journal::Txn &t, int price_cents, int deposit_cents,
                        const std::string &idem_key) override {
    json j;
    j["type"] = "purchase";
    j["version"] = 1;
    j["client_id"] = opt_.client_id;
    j["txn_id"] = t.id;
    j["idem_key"] = idem_key;
    // epoch milliseconds
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
    j["ts_ms"] = now_ms;
    j["amount"] = {
      {"price_cents", price_cents},
      {"deposit_cents", deposit_cents},
      {"change_cents", t.change}
    };

    std::string payload = j.dump();
    if (!opt_.hmac_key_hex.empty()) {
      j["sig"] = util::hmac_sha256_hex(opt_.hmac_key_hex, payload);
      payload = j.dump();
    }

    // Send as [topic][payload]
    zmq::message_t topic(opt_.topic.begin(), opt_.topic.end());
    zmq::message_t body(payload.begin(), payload.end());
    (void)sock_.send(topic, zmq::send_flags::sndmore | zmq::send_flags::dontwait);
    (void)sock_.send(body, zmq::send_flags::dontwait);
  }

private:
  Options opt_;
  zmq::context_t ctx_;
  zmq::socket_t sock_;
};
#endif

std::shared_ptr<Publisher> Publisher::create(const Options &opt) {
#if defined(ENABLE_QUANT)
  if (!opt.endpoint.empty() && !opt.topic.empty()) {
    try { return std::make_shared<ZmqPublisher>(opt); }
    catch (...) { /* fallthrough to null */ }
  }
#endif
  return std::make_shared<NullPublisher>();
}

Publisher::Options Publisher::from_config(const cfg::Quant &q) {
  Options o;
  o.endpoint = q.endpoint;
  o.topic = q.topic;
  o.client_id = q.client_id;
  o.hmac_key_hex = q.hmac_key_hex;
  o.heartbeat_seconds = q.heartbeat_seconds;
  return o;
}

} // namespace quant
