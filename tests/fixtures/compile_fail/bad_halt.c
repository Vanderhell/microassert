#include "massert.h"

static int bad_halt(void)
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
        NULL,
        NULL,
        bad_halt,
        NULL
    };

    return (int)massert_init(&instance, &config);
}
