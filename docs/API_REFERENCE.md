# API Reference

> **Header:** `#include "massert.h"`

## Core Types

- `massert_status_t` reports initialization and mutation errors.
- `massert_severity_t` classifies events as `WARN`, `ERROR`, `FATAL`, or `HALT`.
- `massert_info_t` describes one dispatched event, including location, severity, timestamp, and formatting flags.
- `massert_history_entry_t` stores a copied, retained snapshot of the event.
- `massert_t` is the instance object passed to `massert_fire()`.

## Macros

- `MASSERT(expr)` fires `FATAL` if `expr` is false.
- `MASSERT_MSG(expr, fmt, ...)` fires `FATAL` with a formatted message.
- `MASSERT_WARN(expr, fmt, ...)` fires `WARN` and continues.
- `MPANIC(fmt, ...)` fires `FATAL` unconditionally.
- `MPANIC_CODE(code, fmt, ...)` fires `FATAL` with an explicit code.
- `MPANIC_HALT(fmt, ...)` fires `HALT` unconditionally.

## Terminal Behavior

- `WARN` and `ERROR` dispatch hooks and return to the caller.
- `FATAL` dispatches hooks, calls the configured reset callback, then falls back to halt if reset returns.
- `HALT` dispatches hooks, calls the configured halt callback, then spins if halt returns.
- Nested `WARN` and `ERROR` events are suppressed while a panic is active.
- Nested `FATAL` or `HALT` events trigger the terminal halt path.

## Thread Safety

The library is not thread-safe. The re-entrancy guard protects against recursive panics from hooks, but not concurrent panics from different threads.
