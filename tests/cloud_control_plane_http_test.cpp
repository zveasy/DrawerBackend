#include <gtest/gtest.h>

#include <filesystem>
#include <thread>

#include <httplib.h>

#include "cloud/control_plane/http_device_twin_client.hpp"
#include "cloud/control_plane/twin_contract_server.hpp"
#include "cloud/fleet_manager/fleet_manager.hpp"

namespace {

struct RunningServer {
  httplib::Server server;
  std::thread thread;
  int port{0};

  ~RunningServer() {
    server.stop();
    if (thread.joinable()) thread.join();
  }

  void start() {
    port = server.bind_to_any_port("127.0.0.1");
    ASSERT_GT(port, 0);
    thread = std::thread([this] { server.listen_after_bind(); });
    server.wait_until_ready();
  }

  std::string url() const { return "http://127.0.0.1:" + std::to_string(port); }
};

}  // namespace

TEST(CloudControlPlaneHttp, AuthenticatedPushPullAndConflict) {
  cloud::control_plane::TwinContractServer contract;
  auto existing = cloud::fleet_manager::make_local_default_twin();
  existing.drawer_id = "drawer-1";
  existing.device_id = "device-1";
  existing.revision = 5;
  contract.upsert(existing);

  RunningServer cloud;
  contract.register_routes(cloud.server, "token");
  cloud.start();

  cloud::control_plane::HttpDeviceTwinClient client({cloud.url(), "token", "", 1000, 8});
  auto stale = existing;
  stale.revision = 5;
  auto conflict = client.push_twin(stale);
  EXPECT_FALSE(conflict.ok);
  EXPECT_TRUE(conflict.conflict);
  EXPECT_EQ("remote_revision_conflict", conflict.reason);

  auto updated = existing;
  updated.revision = 6;
  updated.health.jam_count = 2;
  auto pushed = client.push_twin(updated);
  EXPECT_TRUE(pushed.ok) << pushed.reason;

  auto pulled = client.fetch_twin("drawer-1");
  ASSERT_TRUE(pulled);
  EXPECT_EQ(6, pulled->revision);
  EXPECT_EQ("device-1", pulled->device_id);
}

TEST(CloudControlPlaneHttp, OfflineQueueSurvivesAndFlushes) {
  auto queue = std::filesystem::temp_directory_path() / "drawer_cloud_queue.json";
  std::filesystem::remove(queue);

  auto twin = cloud::fleet_manager::make_local_default_twin();
  twin.drawer_id = "drawer-offline";
  twin.device_id = "device-offline";
  twin.revision = 2;
  {
    cloud::control_plane::HttpDeviceTwinClient offline(
        {"http://127.0.0.1:9", "token", queue.string(), 100, 8});
    auto failed = offline.push_twin(twin);
    EXPECT_FALSE(failed.ok);
    EXPECT_EQ(1, offline.status().offline_queue_depth);
  }

  cloud::control_plane::TwinContractServer contract;
  RunningServer cloud;
  contract.register_routes(cloud.server, "token");
  cloud.start();

  cloud::control_plane::HttpDeviceTwinClient online({cloud.url(), "token", queue.string(), 1000, 8});
  EXPECT_EQ(1, online.status().offline_queue_depth);
  EXPECT_EQ(1, online.flush_offline_queue());
  EXPECT_EQ(0, online.status().offline_queue_depth);
  EXPECT_TRUE(online.fetch_twin("drawer-offline"));
}

TEST(CloudControlPlaneHttp, DisabledDeviceCannotPush) {
  cloud::control_plane::TwinContractServer contract;
  auto disabled = cloud::fleet_manager::make_local_default_twin();
  disabled.drawer_id = "drawer-disabled";
  disabled.device_id = "device-disabled";
  disabled.disabled = true;
  disabled.revision = 10;
  contract.upsert(disabled);

  RunningServer cloud;
  contract.register_routes(cloud.server, "token");
  cloud.start();

  cloud::control_plane::HttpDeviceTwinClient client({cloud.url(), "token", "", 1000, 8});
  EXPECT_TRUE(client.is_device_disabled("device-disabled"));
  auto attempt = disabled;
  attempt.disabled = false;
  attempt.revision = 11;
  auto pushed = client.push_twin(attempt);
  EXPECT_FALSE(pushed.ok);
  EXPECT_TRUE(pushed.disabled);
}
