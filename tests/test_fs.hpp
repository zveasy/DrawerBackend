#pragma once
#include <filesystem>
#include <string>
#include <chrono>
#include <random>
#include <unistd.h>
#include <gtest/gtest.h>
#include <cctype>

// RAII helper that switches CWD to a unique, per-test temp directory
// and cleans it up on destruction.
class TestCwd {
public:
  TestCwd() {
    namespace fs = std::filesystem;
    old_ = fs::current_path();

    auto* ut = ::testing::UnitTest::GetInstance();
    const ::testing::TestInfo* ti = ut ? ut->current_test_info() : nullptr;
    std::string test_name = ti ? (std::string(ti->test_suite_name()) + "." + ti->name()) : "unknown";

    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::random_device rd;
    auto pid = static_cast<unsigned long>(::getpid());

    root_ = fs::temp_directory_path() / "register_mvp-tests" /
            (sanitize(test_name) + "-" + std::to_string(pid) + "-" + std::to_string(now) + "-" + std::to_string(rd()));
    fs::create_directories(root_);
    fs::current_path(root_);
  }

  ~TestCwd() {
    namespace fs = std::filesystem;
    try {
      fs::current_path(old_);
    } catch (...) {}
    try {
      fs::remove_all(root_);
    } catch (...) {}
  }

  const std::filesystem::path& root() const { return root_; }

  std::filesystem::path path(const std::string& p) const { return root_ / p; }

private:
  static std::string sanitize(const std::string& in) {
    std::string out;
    out.reserve(in.size());
    for (char c : in) {
      if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') out.push_back(c);
      else out.push_back('_');
    }
    return out;
  }

  std::filesystem::path old_;
  std::filesystem::path root_;
};
