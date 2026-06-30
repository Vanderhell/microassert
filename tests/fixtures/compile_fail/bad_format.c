#include "massert.h"

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
    massert_fire(&instance, MASSERT_SEVERITY_WARN, "fmt.c", 1u, "main", NULL, 0u, "%d", "wrong");
    return 0;
}
