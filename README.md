# microassert

[![CI](https://github.com/Vanderhell/microassert/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Vanderhell/microassert/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C99](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

`microassert` is a small assert and panic library for C and C++ consumers.
It has no third-party dependencies and formats messages with the standard
C library `vsnprintf`.

## Features

- explicit, status-returning initialization
- caller-owned hook storage and retained history storage
- WARN, ERROR, FATAL, and HALT severities
- copied history entries with bounded text buffers
- deterministic nested-panic handling and terminal callbacks
- C99 minimum, C11-compatible source
- C and C++ header compatibility

## Quick Start

```c
#include "massert.h"

static uint32_t clock_ms(void)
{
    return 1234u;
}

static void reset_cb(void *ctx)
{
    (void)ctx;
}

static void halt_cb(void *ctx)
{
    (void)ctx;
}

int main(void)
{
    massert_hook_slot_t hooks[2];
    massert_history_entry_t history[4];
    massert_t ma;
    massert_config_t config = {
        hooks,
        2u,
        history,
        4u,
        clock_ms,
        reset_cb,
        NULL,
        halt_cb,
        NULL
    };

    if (massert_init(&ma, &config) != MASSERT_STATUS_OK) {
        return 1;
    }

    MASSERT_WARN(1 == 1, "not reached");
    MASSERT_MSG(2 == 3, "mismatch: %d", 3);
    MPANIC("fatal condition");
}
```

## Build

Direct Makefile workflow:

```bash
mingw32-make test
```

CMake workflow:

```bash
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /tmp/microassert-install
```

## Public API

- `massert_init()` validates caller storage and copies callbacks by value.
- `massert_set_reset()` and `massert_set_halt()` reject mutation while a panic is active.
- `massert_fire()` routes WARN and ERROR back to the caller, while FATAL and HALT are terminal.
- `massert_panic_fatal()` calls reset, then halt if reset returns, then spins.
- `massert_panic_halt()` calls halt, then spins if halt returns.
- `massert_history_at()` returns retained copied history, valid until overwritten or reinit.

## Notes

- Hooks are best-effort and do not guarantee logging, persistence, or flushing.
- The global singleton is deterministic, but the library is not thread-safe.
- FATAL and HALT do not unwind C frames or run C++ destructors.
- Hardware fault handling, signals, and power-loss recovery are outside the library contract.

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Cookbook](docs/COOKBOOK.md)
- [Design notes](docs/DESIGN.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [Verification notes](docs/VERIFICATION.md)
- [Contributing](CONTRIBUTING.md)
- [Security policy](SECURITY.md)
- [Changelog](CHANGELOG.md)

## License

MIT, see [LICENSE](LICENSE).
