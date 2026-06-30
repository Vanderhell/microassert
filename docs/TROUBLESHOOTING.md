# Troubleshooting

## Missing Initialization

Symptoms:

- `massert_add_hook()` or `massert_set_reset()` returns `MASSERT_STATUS_INVALID_STATE`
- counters and history queries stay at zero unexpectedly

Cause:

- `massert_init()` was not called for the instance, or the global singleton was queried before explicit configuration.

## Invalid Config or Storage

Symptoms:

- `massert_init()` returns `MASSERT_STATUS_INVALID_ARGUMENT`

Cause:

- `hook_capacity > 0` with `hook_slots == NULL`
- `history_capacity > 0` with `history_entries == NULL`

Zero capacity is valid and disables the corresponding storage.

## Nested Panic Behavior

- Nested WARN and ERROR are suppressed while a panic dispatch is active.
- Nested FATAL and HALT go to the emergency terminal path immediately.
- Hook mutation and reinitialization during dispatch return `MASSERT_STATUS_BUSY`.

## Reset Callback Returns Then Halt Path

`massert_panic_fatal()` does not assume reset is non-returning.

If reset returns:

1. halt callback is invoked
2. the terminal spin path is entered if halt returns

## History Truncation

- History text fields are bounded copies.
- `message_truncated`, `file_truncated`, `func_truncated`, and `expr_truncated` record truncation state.
- `format_failed` records message formatting failure.

## Zero Hook or History Capacity

- Zero hook capacity disables hook registration.
- Zero history capacity disables retained history.
- Both are valid configurations.

## Format-Check Compiler Differences

- GCC and Clang use `__attribute__((format(printf, ...)))`.
- Unsupported compilers do not receive the format-check attribute.
- Compile-fail format diagnostics are only expected where format checking is actually emitted.

## Compile-Fail Diagnostics

- GCC and Clang emit `file:line:column:`-style diagnostics.
- MSVC emits `file(line):`-style diagnostics.
- The compile-fail harness matches compiler-specific diagnostic shapes.

## CMake Install and `find_package()` Issues

- The external install-consumer test must use the same generator and relevant toolchain and flag context as the producer build.
- Sanitizer builds must propagate sanitizer compile and linker flags into the external consumer configure step.
- Cross-compiling builds must propagate the toolchain file into the external consumer configure step.

## Makefile Flag Injection Issues

The repository Makefiles preserve caller-provided:

- `CC`
- `CPPFLAGS`
- `CFLAGS`
- `LDFLAGS`
- `LDLIBS`

Project include paths are appended separately so injected `CPPFLAGS` do not erase required header paths.
