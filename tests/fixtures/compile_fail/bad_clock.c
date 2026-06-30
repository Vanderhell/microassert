#include "massert.h"

static int bad_clock(void)
{
    return 1;
}

int main(void)
{
    massert_t instance;
    massert_hook_slot_t hooks[1];
    massert_history_entry_t history[1];
    massert_config_t config = {
        hooks,
        1u,
        history,
        1u,
        bad_clock,
        NULL,
        NULL,
        NULL,
        NULL
    };

    return (int)massert_init(&instance, &config);
}
