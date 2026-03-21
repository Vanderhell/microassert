# microassert

[![CI](https://github.com/Vanderhell/microassert/actions/workflows/ci.yml/badge.svg?branch=master)](https://github.com/Vanderhell/microassert/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C99](https://img.shields.io/badge/C-C99-blue.svg)](https://en.wikipedia.org/wiki/C99)

Unified panic and assert system for embedded systems.

C99, zero dependencies, zero allocations, hook-chain architecture, portable.

## Why microassert?

Embedded projects often end up with fragmented failure handling:
`assert()`, `while(1)`, ad-hoc reset calls, and missing diagnostics.

`microassert` provides one panic pipeline:

1. Capture context (file, line, function, message, timestamp)
2. Execute hooks (log, dump, persist, flush)
3. Continue, reset, or halt based on severity

## Features

- Four severities: `WARN`, `ERROR`, `FATAL`, `HALT`
- Hook chain with configurable order
- Panic context capture and formatted messages
- Re-entrancy guard for nested panic scenarios
- Optional panic history ring buffer
- Global singleton plus instance-based API
- Compile-time switches for footprint control

## Quick Start

```c
#include "massert.h"

static uint32_t clock_ms(void) { return HAL_GetTick(); }

static void log_hook(const massert_info_t *info, void *ctx) {
    (void)ctx;
    // route info->msg / info->severity to your logger
}

int main(void) {
    massert_t *ma = massert_global();
    massert_init(ma, clock_ms);
    massert_add_hook(ma, log_hook, NULL);

    MASSERT(1 == 1);
    MASSERT_WARN(2 == 3, "non-fatal mismatch");
    MASSERT_MSG(4 == 5, "fatal mismatch: %d != %d", 4, 5);

    return 0;
}
```

## Build and Test

From repository root:

```bash
clang -std=c99 -Wall -Wextra -Wpedantic -Werror -Iinclude src/massert.c tests/test_all.c -o tests/test_all
./tests/test_all
```

On Linux with GCC:

```bash
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -Iinclude src/massert.c tests/test_all.c -o tests/test_all
./tests/test_all
```

## Configuration

| Macro | Default | Description |
|---|---:|---|
| `MASSERT_MAX_HOOKS` | `4` | Maximum hooks in chain |
| `MASSERT_MSG_SIZE` | `96` | Message buffer size |
| `MASSERT_ENABLE_LOCATION` | `1` | Capture file/line/function |
| `MASSERT_ENABLE_HISTORY` | `1` | Store panic history |
| `MASSERT_HISTORY_DEPTH` | `4` | History ring size |

## Documentation

- [API reference](docs/API_REFERENCE.md)
- [Design notes](docs/DESIGN.md)
- [Porting guide](docs/PORTING_GUIDE.md)
- [Contributing](CONTRIBUTING.md)
- [Changelog](CHANGELOG.md)

## Ecosystem Integrations

- [microlog](https://github.com/Vanderhell/microlog)
- [nvlog](https://github.com/Vanderhell/nvlog)
- [panicdump](https://github.com/Vanderhell/panicdump)
- [microboot](https://github.com/Vanderhell/microboot)

## License

MIT, see [LICENSE](LICENSE).
