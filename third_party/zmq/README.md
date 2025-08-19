# ZeroMQ Dependencies

This project uses ZeroMQ for the QuantEngine bridge.
Dependencies are managed via CMake `FetchContent`:

- [libzmq](https://github.com/zeromq/libzmq) – the core C library
- [cppzmq](https://github.com/zeromq/cppzmq) – header-only C++ bindings

Both are fetched at configure time when the QuantEngine bridge
is enabled (`ENABLE_QUANT` option).
