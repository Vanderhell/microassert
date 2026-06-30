#include "massert.h"

static int bad_reset(void)
{
    return 0;
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
        NULL,
        bad_reset,
        NULL,
        NULL,
        NULL
    };

    return (int)massert_init(&instance, &config);
}
