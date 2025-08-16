#include "util/version.hpp"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

namespace appver {
const char* version() { return APP_VERSION; }
const char* git() { return GIT_COMMIT; }
const char* build_time() {
  static const char* bt = __DATE__ " " __TIME__;
  return bt;
}
} // namespace appver

