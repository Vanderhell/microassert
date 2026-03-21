# API Reference

> **Header:** `#include "massert.h"` · **Version:** 1.0.0

## Macros
- `MASSERT(expr)` — fatal if false
- `MASSERT_MSG(expr, fmt, ...)` — fatal with message
- `MASSERT_WARN(expr, fmt, ...)` — warn + continue
- `MPANIC(fmt, ...)` — unconditional fatal
- `MPANIC_CODE(code, fmt, ...)` — fatal with error code

## Severities: WARN → ERROR → FATAL → HALT
- WARN: hooks fire, execution continues
- ERROR: hooks fire, execution continues
- FATAL: hooks fire, reset_fn called, fallback halt
- HALT: hooks fire, infinite loop (for debugger)

## Thread safety
Not thread-safe. The re-entrancy guard protects against recursive panics from hooks, but not concurrent panics from different threads.
