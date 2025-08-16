#include "serial_pos.hpp"

namespace pos {

SerialConnector::SerialConnector(TxnEngine& eng, const Options& opt)
    : eng_(eng), opt_(opt) {}

bool SerialConnector::start() { return true; }
void SerialConnector::stop() {}

} // namespace pos
