#include "massert.h"

static void bad_hook(const massert_info_t *info)
{
    (void)info;
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
        NULL,
        NULL
    };

    (void)massert_init(&instance, &config);
    return (int)massert_add_hook(&instance, bad_hook, NULL, NULL);
}
