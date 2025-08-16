#pragma once
#include "connector.hpp"

namespace pos {

// Optional serial POS connector stub. To implement:
//  - Open /dev/ttyAMA0
//  - Speak a simple line protocol: PRICE=<cents>;DEPOSIT=<cents>\n
//  - Parse response and invoke TxnEngine
class SerialConnector : public Connector {
public:
  SerialConnector(TxnEngine& eng, const Options& opt = Options());
  bool start() override; // no-op
  void stop() override; // no-op
private:
  TxnEngine& eng_;
  Options opt_;
};

} // namespace pos
