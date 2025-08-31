#pragma once
#include <memory>
#include <string>
#include <cstdint>

namespace journal { struct Txn; }

namespace cfg { struct Quant; }

namespace quant {

// Lightweight interface to publish purchase events to QuantEngine.
class Publisher {
public:
  struct Options {
    std::string endpoint;        // e.g. tcp://quant.local:5555
    std::string topic;           // e.g. register/stream
    std::string client_id;       // e.g. REG-CLIENT
    std::string hmac_key_hex;    // optional
    int heartbeat_seconds{15};
  };

  virtual ~Publisher() = default;

  // Factory that may return nullptr if configuration is incomplete.
  static std::shared_ptr<Publisher> create(const Options &opt);

  // Convenience: build Options from cfg::Quant (if available)
  static Options from_config(const cfg::Quant &q);

  // Publish a completed purchase event. Safe to call from POS thread; it should
  // be non-blocking or very short. Implementation may queue to disk/background.
  virtual void publish_purchase(const journal::Txn &t,
                                int price_cents,
                                int deposit_cents,
                                const std::string &idem_key) = 0;

  // Publish a drawer/account balance update so QuantEngine can act on available
  // funds. "account_id" identifies the merchant/account; "drawer_cents" is the
  // current drawer balance; "pending_change_cents" and "reserve_floor_cents" help
  // the engine compute investable funds; "delta_cents" is the latest applied
  // sweep/change to the drawer; "idem_key" provides idempotency for retries.
  // Implementations must be best-effort and non-blocking.
  virtual void publish_drawer_balance(const std::string &account_id,
                                      int64_t drawer_cents,
                                      int64_t pending_change_cents,
                                      int64_t reserve_floor_cents,
                                      int64_t delta_cents,
                                      const std::string &idem_key) = 0;
};

} // namespace quant

