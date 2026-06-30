#include "massert.h"

int main()
{
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
    massert_t instance;

    if (massert_init(&instance, &config) != MASSERT_STATUS_OK) {
        return 1;
    }

    massert_fire(&instance, MASSERT_SEVERITY_WARN, "cxx.cpp", 12u, "main", "ok", 0u, "hello");
    return massert_total_count(&instance) == 1u ? 0 : 2;
}
