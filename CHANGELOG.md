# Changelog

## [1.0.0] — 2026-03-20

### Added
- Four severity levels: WARN, ERROR, FATAL, HALT.
- Hook chain (up to 4 hooks, fire in order).
- Context capture: file, line, function, expression, message, error code, timestamp.
- Re-entrancy guard against recursive panics.
- Panic history ring buffer.
- Convenience macros: MASSERT, MASSERT_MSG, MASSERT_WARN, MPANIC, MPANIC_CODE.
- Global singleton + independent instances.
- 33 tests covering all severities, hooks, history, re-entrancy.
