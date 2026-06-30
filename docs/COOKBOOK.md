# Cookbook

## Minimal Initialization

```c
#include "massert.h"

int main(void)
{
    massert_hook_slot_t hooks[1];
    massert_history_entry_t history[1];
    massert_t instance;
    massert_config_t config = {
        hooks,
        1u,
        history,
        1u,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    if (massert_init(&instance, &config) != MASSERT_STATUS_OK) {
        return 1;
    }

    return 0;
}
```

## Caller-Owned Storage

`microassert` does not allocate hook or history storage. The caller owns both buffers and passes them through `massert_config_t`.

```c
static massert_hook_slot_t hooks[4];
static massert_history_entry_t history[8];
static massert_t instance;

static massert_config_t config = {
    hooks,
    4u,
    history,
    8u,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};
```

## Hook Registration

```c
static void warn_hook(const massert_info_t *info, void *ctx)
{
    (void)ctx;
    (void)info;
}

size_t slot = 0u;
if (massert_add_hook(&instance, warn_hook, NULL, &slot) != MASSERT_STATUS_OK) {
    return 1;
}
```

## Fatal, Reset, and Halt Callbacks

`massert_panic_fatal()` calls reset, then halt if reset returns, then enters the terminal spin path.

```c
static void board_reset(void *ctx)
{
    (void)ctx;
}

static void board_halt(void *ctx)
{
    (void)ctx;
    for (;;) {
    }
}

if (massert_set_reset(&instance, board_reset, NULL) != MASSERT_STATUS_OK) {
    return 1;
}
if (massert_set_halt(&instance, board_halt, NULL) != MASSERT_STATUS_OK) {
    return 1;
}
```

## History Buffer Access

```c
size_t i;
for (i = 0u; i < massert_history_count(&instance); i++) {
    const massert_history_entry_t *entry = massert_history_at(&instance, i);
    if (entry != NULL) {
        (void)entry->msg;
    }
}
```

History entries retain copied text. Stored `file`, `func`, `expr`, and `msg` fields do not borrow stack pointers from the original call site.

## Global Singleton Usage

Passing `NULL` uses the deterministic global singleton.

```c
massert_config_t config = {
    NULL,
    0u,
    NULL,
    0u,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

if (massert_init(NULL, &config) != MASSERT_STATUS_OK) {
    return 1;
}

massert_fire(NULL, MASSERT_SEVERITY_WARN, "app.c", 10u, "main", NULL, 0u, "global warn");
```

## C++ Consumer Note

- The public header is wrapped in `extern "C"` when included from C++.
- FATAL and HALT paths do not unwind C++ stack frames and do not run destructors.
- C++ consumers should treat terminal paths as non-returning process or firmware termination paths.

## Embedded and No-Heap Note

- No heap allocation is required.
- No OS or RTOS service is required.
- Provide caller-owned storage for all hooks and retained history.
- On bare-metal targets, reset and halt callbacks are the caller's integration point for board-specific terminal behavior.
