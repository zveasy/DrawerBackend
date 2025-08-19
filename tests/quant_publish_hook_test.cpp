#include <gtest/gtest.h>
#include <filesystem>
#include "../src/pos/router.hpp"
#include "../src/pos/idempotency_store.hpp"
#include "../src/app/txn_engine.hpp"
#include "../src/quant/publisher.hpp"

struct NoopShutter : IShutter { bool home(int, std::string*) override { return true; } bool open_mm(int, std::string*) override { return true; } bool close_mm(int, std::string*) override { return true; } };
struct InstantDisp : IDispenser { DispenseStats dispenseCoins(int c) override { return DispenseStats{true, c, c, 0, 0, "", 0}; } };

class TestPublisher : public quant::Publisher {
public:
  void publish_purchase(const journal::Txn &t, int price_cents, int deposit_cents, const std::string &idem_key) override {
    called = true;
    last_txn_id = t.id;
    last_price = price_cents;
    last_deposit = deposit_cents;
    last_idem = idem_key;
  }
  static std::shared_ptr<TestPublisher> make() { return std::make_shared<TestPublisher>(); }

  bool called{false};
  std::string last_txn_id;
  int last_price{0};
  int last_deposit{0};
  std::string last_idem;
};

TEST(QuantPublishHook, PublishesOnSuccess) {
  std::filesystem::remove_all("idemdata");
  NoopShutter sh; InstantDisp disp; TxnConfig cfg; TxnEngine eng(sh, disp, cfg);
  pos::IdempotencyStore store("idemdata"); ASSERT_TRUE(store.open());

  auto pub = TestPublisher::make();
  pos::Router router(eng, store, pub);

  pos::PurchaseRequest req; req.price_cents=123; req.deposit_cents=200; req.idem_key="idem-1";
  auto out = router.handle(req);
  EXPECT_EQ(200, out.first);
  EXPECT_TRUE(pub->called);
  EXPECT_EQ(123, pub->last_price);
  EXPECT_EQ(200, pub->last_deposit);
  EXPECT_EQ("idem-1", pub->last_idem);
}
