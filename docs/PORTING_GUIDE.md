# Porting Guide

`microassert` is split into a public header and one implementation file.

- `include/massert.h`
- `src/massert.c`

The library requires a C99 compiler, uses `vsnprintf`, and is compatible with C++ consumers through `extern "C"`.

## Minimal Integration

```cmake
add_library(microassert STATIC src/massert.c)
target_include_directories(microassert PUBLIC include)
```

## Instance Setup

Provide caller-owned storage for hooks and retained history, then initialize a `massert_t` with `massert_init()`.

## Callback Mapping

- `clock_fn` should return a monotonically increasing millisecond counter if available.
- `reset_fn` should request a device or process reset.
- `halt_fn` should stop execution in a debugger-friendly way.
- Hook callbacks should be short and side-effect aware.

## Platform Examples

- STM32: `NVIC_SystemReset()`
- ESP32: `esp_restart()`
- Hosted test process: `exit(1)` or a test double

## Build Note

If you package the library, export a CMake config file and a namespaced target so external consumers can link `microassert::microassert`.
