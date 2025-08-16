#pragma once
#include <string>
#include <atomic>
#include "../app/txn_engine.hpp"

namespace pos {
  struct Options {
    std::string bind = "127.0.0.1";
    int port = 9090;
    std::string shared_key; // optional
  };

  class Connector {
  public:
    virtual ~Connector() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
  };
}
