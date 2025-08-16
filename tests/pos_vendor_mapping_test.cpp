#include <gtest/gtest.h>
#include "../src/pos/vendors/vendor_a_adapter.hpp"
#include "../src/pos/vendors/vendor_b_adapter.hpp"

TEST(PosVendor, AdapterMappings) {
  pos::PurchaseRequest r1, r2;
  std::string bodyA = "{\"price_cents\":735,\"deposit_cents\":1000,\"order_id\":\"A-123\"}";
  ASSERT_TRUE(pos::vendors::parse_vendor_a(bodyA, "", r1));
  EXPECT_EQ(735, r1.price_cents);
  EXPECT_EQ(1000, r1.deposit_cents);
  std::string key = r1.idem_key;
  ASSERT_TRUE(pos::vendors::parse_vendor_a(bodyA, "", r2));
  EXPECT_EQ(key, r2.idem_key); // deterministic
  pos::PurchaseRequest rbad;
  EXPECT_FALSE(pos::vendors::parse_vendor_a("{\"price_cents\":-5,\"deposit_cents\":1}", "", rbad));

  pos::PurchaseRequest rb1, rb2;
  std::string bodyB = "{\"amount\":{\"price\":\"500\",\"deposit\":\"700\"},\"meta\":{\"ticket\":\"B-7\"}}";
  ASSERT_TRUE(pos::vendors::parse_vendor_b(bodyB, "", rb1));
  EXPECT_EQ(500, rb1.price_cents);
  EXPECT_EQ(700, rb1.deposit_cents);
  std::string keyb = rb1.idem_key;
  ASSERT_TRUE(pos::vendors::parse_vendor_b(bodyB, "", rb2));
  EXPECT_EQ(keyb, rb2.idem_key);
  pos::PurchaseRequest rb3;
  EXPECT_FALSE(pos::vendors::parse_vendor_b("{\"amount\":{\"price\":100001,\"deposit\":0}}", "", rb3));
}

