#include "massert.h"

int microassert_multi_tu_a(void);
int microassert_multi_tu_b(void);

int main(void)
{
    massert_t *global = massert_global();
    massert_hook_slot_t hooks[2];
    massert_history_entry_t history[2];
    massert_config_t config = {
        hooks,
        2u,
        history,
        2u,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    };

    if (massert_init(global, &config) != MASSERT_STATUS_OK) {
        return 1;
    }

    massert_fire(global, MASSERT_SEVERITY_WARN, "multi_a.c", 1u, "main", NULL, 0u, "multi tu a=%d", microassert_multi_tu_a());
    massert_fire(global, MASSERT_SEVERITY_WARN, "multi_b.c", 2u, "main", NULL, 0u, "multi tu b=%d", microassert_multi_tu_b());
    return (massert_total_count(global) == 2u) ? 0 : 2;
}
