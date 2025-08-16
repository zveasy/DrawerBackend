#pragma once
#include "connector.hpp"
#include "router.hpp"

namespace pos {

// Simplified serial connector used for tests.  Real implementation would
// manage a serial port; here we expose a handle_line helper that consumes a
// single request line and returns the device response.
class SerialConnector : public Connector {
public:
  SerialConnector(Router &router, const Options &opt = Options());
  bool start() override; // no-op
  void stop() override;  // no-op

  // Process a line of ASCII protocol, returning response line.
  std::string handle_line(const std::string &line);

private:
  Router &router_;
  Options opt_;
};

} // namespace pos
