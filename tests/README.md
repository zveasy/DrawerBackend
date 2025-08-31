# Tests README

This directory contains C++ unit tests for DrawerBackend.

Key conventions for stable parallel runs:

- Per-test temp cwd: Always isolate filesystem state using `TestCwd` from `tests/test_fs.hpp`.
  - Include: `#include "test_fs.hpp"`
  - Instantiate at test start: `TestCwd cwd;`
  - This switches the working directory to a unique temp dir and auto-cleans on scope exit.

- Avoid shared fixed paths: Do not read/write under fixed directories like `data/`, `idemdata/`, etc.
  - Use relative paths within the per-test cwd or `std::filesystem::temp_directory_path()` under `cwd`.

- Env vars in tests: If a component relies on an env var (e.g. `REGISTER_MVP_DOCS_PATH`), set it to a path under the test's temp cwd.

- Network ports: Prefer binding to `127.0.0.1` and port `0` (auto-assign). Capture the selected port from the server after start.

- Parallel-safe CI: Tests are executed with `ctest -j` in CI. Ensure new tests follow the above to avoid flakiness.

Example snippet:

```cpp
#include <gtest/gtest.h>
#include "test_fs.hpp"

TEST(MyFeature, WorksIsolated) {
  TestCwd cwd; // unique temp dir per test
  // create files under current working directory safely
}
```
