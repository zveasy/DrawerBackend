# Contributing to DrawerBackend

Thanks for your interest in contributing! This guide summarizes our testing and CI expectations to keep the codebase stable and fast.

## Test design principles

- **Isolate filesystem state**
  - Include `tests/test_fs.hpp` and instantiate `TestCwd` at the start of each test:
    ```cpp
    #include "tests/test_fs.hpp"
    TEST(MySuite, Case) {
      TestCwd cwd; // unique temp dir; auto-cleaned
      // ...
    }
    ```
  - Never write to repo-root or shared fixed paths like `data/` or `idemdata/`.
  - If a component needs an env var path (e.g., `REGISTER_MVP_DOCS_PATH`), point it inside the per-test temp dir.

- **Avoid port conflicts**
  - Bind servers to `127.0.0.1` and port `0` (ephemeral) whenever possible.
  - If you must pre-pick a port, prefer the helper in `tests/test_net.hpp` and start immediately after selection.

- **Parallel-safe by default**
  - CI runs tests with `ctest -j`. Do not rely on serialized execution.
  - Prefer deterministic timing and avoid sleeps where possible.

- **Structured test output**
  - CI collects JUnit from `ctest --output-junit`. Keep test names stable and clear.

See `tests/README.md` for more details and examples.

## CI expectations

- **Compilers**: GCC and Clang
- **Configurations**: Regular + sanitizers matrix (ASan+UBSan, TSAN with Clang)
- **Build type**: `RelWithDebInfo` (fast and debuggable)
- **Caching**: `ccache` enabled in CI
- **Retry policy**: Temporary light retry for flake triage. We aim to remove it; please fix flakiness at the source.

## Code quality tips

- Enable warnings locally (e.g., `-Wall -Wextra -Wpedantic`) and keep your build clean.
- Prefer RAII and narrow ownership. Avoid globals in tests.
- Keep tests small and focused; one behavior per test is ideal.

## Submitting changes

1. Create a feature branch.
2. Add tests covering your change.
3. Ensure `ctest -j` passes locally.
4. Open a PR against `main` and link any relevant issues.

Thank you for helping keep DrawerBackend robust and fast!
