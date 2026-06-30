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
