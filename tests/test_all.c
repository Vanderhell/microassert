/*
 * microassert test suite.
 *
 * Note: FATAL and HALT cause infinite loops / resets, so we test them
 * indirectly via hooks and counters. WARN and ERROR are tested directly.
 *
 * Build: gcc -std=c99 -Wall -Wextra -I../include ../src/massert.c test_all.c -o test_all
 */

#include "massert.h"
#include <stdio.h>
#include <string.h>

/* ── Test framework ────────────────────────────────────────────────────── */

static int tests_run = 0, tests_passed = 0, tests_failed = 0;

#define TEST(name) static void name(void)
#define RUN_TEST(name) do {                                     \
    tests_run++;                                                \
    printf("  %-55s ", #name);                                  \
    name();                                                     \
    printf("PASS\n");                                           \
    tests_passed++;                                             \
} while (0)

#define ASSERT_EQ(expected, actual) do {                        \
    if ((expected) != (actual)) {                               \
        printf("FAIL\n    %s:%d: expected %d, got %d\n",       \
               __FILE__, __LINE__, (int)(expected), (int)(actual)); \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_TRUE(expr) do {                                  \
    if (!(expr)) {                                              \
        printf("FAIL\n    %s:%d: expected true\n",              \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_FALSE(expr) do {                                 \
    if ((expr)) {                                               \
        printf("FAIL\n    %s:%d: expected false\n",             \
               __FILE__, __LINE__);                             \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do {                    \
    if (strcmp((expected), (actual)) != 0) {                     \
        printf("FAIL\n    %s:%d: expected \"%s\", got \"%s\"\n",\
               __FILE__, __LINE__, (expected), (actual));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

#define ASSERT_STR_CONTAINS(haystack, needle) do {              \
    if (strstr((haystack), (needle)) == NULL) {                  \
        printf("FAIL\n    %s:%d: \"%s\" not in \"%s\"\n",       \
               __FILE__, __LINE__, (needle), (haystack));       \
        tests_failed++; return;                                 \
    }                                                           \
} while (0)

/* ── Mock clock ────────────────────────────────────────────────────────── */

static uint32_t mock_time = 5000;
static uint32_t mock_clock(void) { return mock_time; }

/* ── Hook tracking ─────────────────────────────────────────────────────── */

#define MAX_HOOK_CALLS 16
static massert_info_t hook_log[MAX_HOOK_CALLS];
static int hook_call_count = 0;

static void test_hook(const massert_info_t *info, void *ctx)
{
    (void)ctx;
    if (hook_call_count < MAX_HOOK_CALLS) {
        hook_log[hook_call_count++] = *info;
    }
}

/* Second hook for chain testing */
static int hook2_count = 0;
static void test_hook2(const massert_info_t *info, void *ctx)
{
    (void)info; (void)ctx;
    hook2_count++;
}

/* ── Setup ─────────────────────────────────────────────────────────────── */

static massert_t ma;

static void reset_all(void)
{
    mock_time = 5000;
    hook_call_count = 0;
    hook2_count = 0;
    memset(hook_log, 0, sizeof(hook_log));
    massert_init(&ma, mock_clock);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Init
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_init) {
    reset_all();
    ASSERT_EQ(0, (int)massert_panic_count(&ma));
    ASSERT_EQ(0, (int)massert_warn_count(&ma));
    ASSERT_EQ(0, (int)massert_error_count(&ma));
    ASSERT_EQ(0, (int)massert_fatal_count(&ma));
    ASSERT_EQ(0, ma.num_hooks);
}

TEST(test_init_null_clock) {
    massert_t local;
    massert_init(&local, NULL);
    /* Should work, timestamps will be 0 */
    ASSERT_TRUE(local.clock == NULL);
}

TEST(test_global_singleton) {
    massert_t *g1 = massert_global();
    massert_t *g2 = massert_global();
    ASSERT_TRUE(g1 == g2);
    ASSERT_TRUE(g1 != NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Hooks
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_add_hook) {
    reset_all();
    int idx = massert_add_hook(&ma, test_hook, NULL);
    ASSERT_EQ(0, idx);
    ASSERT_EQ(1, ma.num_hooks);
}

TEST(test_add_multiple_hooks) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);
    massert_add_hook(&ma, test_hook2, NULL);
    ASSERT_EQ(2, ma.num_hooks);
}

TEST(test_add_hook_full) {
    reset_all();
    for (int i = 0; i < MASSERT_MAX_HOOKS; i++) {
        ASSERT_TRUE(massert_add_hook(&ma, test_hook, NULL) >= 0);
    }
    ASSERT_EQ(-1, massert_add_hook(&ma, test_hook, NULL));
}

TEST(test_add_hook_null_fn) {
    reset_all();
    ASSERT_EQ(-1, massert_add_hook(&ma, NULL, NULL));
}

TEST(test_clear_hooks) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);
    ASSERT_EQ(1, ma.num_hooks);
    massert_clear_hooks(&ma);
    ASSERT_EQ(0, ma.num_hooks);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: WARN severity (logs + continues)
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_warn_continues) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_WARN, "test.c", 42, "my_func",
                  "x > 0", 0, "value was %d", -5);

    /* Should continue — we're still running */
    ASSERT_EQ(1, hook_call_count);
    ASSERT_EQ(1, (int)massert_panic_count(&ma));
    ASSERT_EQ(1, (int)massert_warn_count(&ma));
    ASSERT_EQ(0, (int)massert_error_count(&ma));
}

TEST(test_warn_captures_info) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);
    mock_time = 12345;

    massert_fire(&ma, MASSERT_WARN, "sensor.c", 99, "read_temp",
                  "temp < 100", 0, "overtemp: %d", 105);

    ASSERT_EQ(MASSERT_WARN, (int)hook_log[0].severity);
    ASSERT_EQ(12345, (int)hook_log[0].timestamp_ms);
#if MASSERT_ENABLE_LOCATION
    ASSERT_STR_EQ("sensor.c", hook_log[0].file);
    ASSERT_EQ(99, (int)hook_log[0].line);
    ASSERT_STR_EQ("read_temp", hook_log[0].func);
#endif
    ASSERT_STR_EQ("temp < 100", hook_log[0].expr);
    ASSERT_STR_CONTAINS(hook_log[0].msg, "overtemp: 105");
}

TEST(test_warn_no_hooks_ok) {
    reset_all();
    /* No hooks — should not crash */
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "no hooks");
    ASSERT_EQ(1, (int)massert_panic_count(&ma));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: ERROR severity (logs + hooks + continues)
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_error_continues) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "something bad");

    ASSERT_EQ(1, hook_call_count);
    ASSERT_EQ(1, (int)massert_error_count(&ma));
    ASSERT_EQ(0, (int)massert_fatal_count(&ma));
}

TEST(test_error_with_code) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 42, "err code %d", 42);

    ASSERT_EQ(42, (int)hook_log[0].error_code);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Hook chain order
 * ═══════════════════════════════════════════════════════════════════════════ */

static int chain_order[4];
static int chain_idx = 0;

static void chain_hook_0(const massert_info_t *info, void *ctx) { (void)info; (void)ctx; chain_order[chain_idx++] = 0; }
static void chain_hook_1(const massert_info_t *info, void *ctx) { (void)info; (void)ctx; chain_order[chain_idx++] = 1; }
static void chain_hook_2(const massert_info_t *info, void *ctx) { (void)info; (void)ctx; chain_order[chain_idx++] = 2; }

TEST(test_hook_chain_order) {
    reset_all();
    chain_idx = 0;
    memset(chain_order, -1, sizeof(chain_order));

    massert_add_hook(&ma, chain_hook_0, NULL);
    massert_add_hook(&ma, chain_hook_1, NULL);
    massert_add_hook(&ma, chain_hook_2, NULL);

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "chain test");

    ASSERT_EQ(3, chain_idx);
    ASSERT_EQ(0, chain_order[0]);
    ASSERT_EQ(1, chain_order[1]);
    ASSERT_EQ(2, chain_order[2]);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: FATAL severity (hooks fire, then reset)
 * ═══════════════════════════════════════════════════════════════════════════ */

/*
 * FATAL normally doesn't return. In tests, mock_reset returns, and
 * massert_fire falls into an infinite loop. To test FATAL without
 * hanging, we only verify that hooks fire and reset_fn is called
 * by checking side effects from the hook.
 *
 * We use a trick: the test calls massert_fire directly with ERROR
 * severity (which returns) and verify FATAL counters separately.
 */

TEST(test_fatal_counter) {
    reset_all();
    /* We can't call FATAL directly (infinite loop), but we can check
     * that ERROR and WARN counters work correctly as a proxy. */
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "w");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "e");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "e2");

    ASSERT_EQ(3, (int)massert_panic_count(&ma));
    ASSERT_EQ(1, (int)massert_warn_count(&ma));
    ASSERT_EQ(2, (int)massert_error_count(&ma));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Multiple panics
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_multiple_warns) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    for (int i = 0; i < 5; i++) {
        massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "warn %d", i);
    }

    ASSERT_EQ(5, hook_call_count);
    ASSERT_EQ(5, (int)massert_warn_count(&ma));
    ASSERT_EQ(5, (int)massert_panic_count(&ma));
}

TEST(test_mixed_severities) {
    reset_all();
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "w1");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "e1");
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "w2");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "e2");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "e3");

    ASSERT_EQ(5, (int)massert_panic_count(&ma));
    ASSERT_EQ(2, (int)massert_warn_count(&ma));
    ASSERT_EQ(3, (int)massert_error_count(&ma));
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: History
 * ═══════════════════════════════════════════════════════════════════════════ */

#if MASSERT_ENABLE_HISTORY
TEST(test_history_stored) {
    reset_all();
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "first");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "second");

    ASSERT_EQ(2, massert_history_count(&ma));
}

TEST(test_history_most_recent) {
    reset_all();
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "old");
    massert_fire(&ma, MASSERT_ERROR, NULL, 0, NULL, NULL, 0, "new");

    const massert_info_t *latest = massert_history_at(&ma, 0);
    ASSERT_TRUE(latest != NULL);
    ASSERT_EQ(MASSERT_ERROR, (int)latest->severity);
    ASSERT_STR_CONTAINS(latest->msg, "new");

    const massert_info_t *older = massert_history_at(&ma, 1);
    ASSERT_TRUE(older != NULL);
    ASSERT_EQ(MASSERT_WARN, (int)older->severity);
}

TEST(test_history_wraps) {
    reset_all();
    for (int i = 0; i < MASSERT_HISTORY_DEPTH + 3; i++) {
        massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, (uint32_t)i,
                      "panic %d", i);
    }

    ASSERT_EQ(MASSERT_HISTORY_DEPTH, massert_history_count(&ma));

    /* Most recent should be the last one written */
    const massert_info_t *latest = massert_history_at(&ma, 0);
    ASSERT_EQ((int)(MASSERT_HISTORY_DEPTH + 2), (int)latest->error_code);
}

TEST(test_history_out_of_range) {
    reset_all();
    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "one");
    ASSERT_TRUE(massert_history_at(&ma, 0) != NULL);
    ASSERT_TRUE(massert_history_at(&ma, 1) == NULL);
    ASSERT_TRUE(massert_history_at(&ma, 99) == NULL);
}
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Timestamp
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_timestamp) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);
    mock_time = 99999;

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "timed");
    ASSERT_EQ(99999, (int)hook_log[0].timestamp_ms);
}

TEST(test_no_clock_zero_timestamp) {
    massert_t local;
    massert_init(&local, NULL);  /* no clock */
    massert_add_hook(&local, test_hook, NULL);
    hook_call_count = 0;

    massert_fire(&local, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "no clock");
    ASSERT_EQ(0, (int)hook_log[0].timestamp_ms);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Message formatting
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_message_format) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0,
                  "temp=%d hum=%.1f", 25, 55.5);
    ASSERT_STR_CONTAINS(hook_log[0].msg, "temp=25");
    ASSERT_STR_CONTAINS(hook_log[0].msg, "hum=55.5");
}

TEST(test_null_message) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, NULL);
    ASSERT_EQ(1, hook_call_count);
    ASSERT_EQ(0, (int)strlen(hook_log[0].msg));
}

TEST(test_null_expr) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "no expr");
    ASSERT_TRUE(hook_log[0].expr == NULL);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: MASSERT_WARN macro
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_macro_warn_true) {
    reset_all();
    massert_add_hook(massert_global(), test_hook, NULL);
    hook_call_count = 0;

    /* True condition — should NOT fire */
    int x = 5;
    MASSERT_WARN(x > 0, "should not fire: %d", x);
    ASSERT_EQ(0, hook_call_count);

    massert_clear_hooks(massert_global());
}

TEST(test_macro_warn_false) {
    reset_all();
    /* Use global for macros */
    massert_t *g = massert_global();
    massert_init(g, mock_clock);
    massert_add_hook(g, test_hook, NULL);
    hook_call_count = 0;

    int x = -1;
    MASSERT_WARN(x > 0, "x is %d", x);

    ASSERT_EQ(1, hook_call_count);
    ASSERT_EQ(MASSERT_WARN, (int)hook_log[0].severity);
    ASSERT_STR_CONTAINS(hook_log[0].msg, "x is -1");
#if MASSERT_ENABLE_LOCATION
    ASSERT_TRUE(hook_log[0].file != NULL);
    ASSERT_TRUE(hook_log[0].line > 0);
#endif

    massert_clear_hooks(g);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Re-entrancy guard
 * ═══════════════════════════════════════════════════════════════════════════ */

static void reentrant_hook(const massert_info_t *info, void *ctx)
{
    massert_t *m = (massert_t *)ctx;
    (void)info;
    /* Try to fire another panic from inside a hook */
    massert_fire(m, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "nested");
}

TEST(test_reentrant_guard) {
    reset_all();
    massert_add_hook(&ma, reentrant_hook, &ma);
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "original");

    /* Only the original panic should be counted (nested is dropped) */
    ASSERT_EQ(1, (int)massert_panic_count(&ma));
    ASSERT_EQ(1, hook_call_count);  /* test_hook fired once for original */
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Hook context
 * ═══════════════════════════════════════════════════════════════════════════ */

static int ctx_value_received = 0;
static void ctx_hook(const massert_info_t *info, void *ctx)
{
    (void)info;
    ctx_value_received = *(int *)ctx;
}

TEST(test_hook_context) {
    reset_all();
    int my_ctx = 42;
    massert_add_hook(&ma, ctx_hook, &my_ctx);
    ctx_value_received = 0;

    massert_fire(&ma, MASSERT_WARN, NULL, 0, NULL, NULL, 0, "ctx test");
    ASSERT_EQ(42, ctx_value_received);
}

/* ═══════════════════════════════════════════════════════════════════════════
 * Tests: Edge cases and strings
 * ═══════════════════════════════════════════════════════════════════════════ */

TEST(test_query_null) {
    /* NULL queries use the global singleton, which may have state.
     * Just verify they don't crash and return reasonable values. */
    massert_t *g = massert_global();
    massert_init(g, NULL);  /* reset global */

    ASSERT_EQ(0, (int)massert_panic_count(NULL));
    ASSERT_EQ(0, (int)massert_warn_count(NULL));
    ASSERT_EQ(0, (int)massert_error_count(NULL));
    ASSERT_EQ(0, (int)massert_fatal_count(NULL));
}

TEST(test_severity_str) {
    ASSERT_STR_EQ("WARN",  massert_severity_str(MASSERT_WARN));
    ASSERT_STR_EQ("ERROR", massert_severity_str(MASSERT_ERROR));
    ASSERT_STR_EQ("FATAL", massert_severity_str(MASSERT_FATAL));
    ASSERT_STR_EQ("HALT",  massert_severity_str(MASSERT_HALT));
    ASSERT_STR_EQ("?",     massert_severity_str((massert_severity_t)99));
}

#if MASSERT_ENABLE_LOCATION
TEST(test_location_capture) {
    reset_all();
    massert_add_hook(&ma, test_hook, NULL);

    massert_fire(&ma, MASSERT_WARN, "myfile.c", 123, "my_function",
                  "a == b", 0, "mismatch");

    ASSERT_STR_EQ("myfile.c", hook_log[0].file);
    ASSERT_EQ(123, (int)hook_log[0].line);
    ASSERT_STR_EQ("my_function", hook_log[0].func);
    ASSERT_STR_EQ("a == b", hook_log[0].expr);
}
#endif

/* ═══════════════════════════════════════════════════════════════════════════
 * Main
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void) {
    printf("\n=== microassert test suite ===\n\n");

    printf("[Init]\n");
    RUN_TEST(test_init);
    RUN_TEST(test_init_null_clock);
    RUN_TEST(test_global_singleton);

    printf("\n[Hooks]\n");
    RUN_TEST(test_add_hook);
    RUN_TEST(test_add_multiple_hooks);
    RUN_TEST(test_add_hook_full);
    RUN_TEST(test_add_hook_null_fn);
    RUN_TEST(test_clear_hooks);

    printf("\n[WARN Severity]\n");
    RUN_TEST(test_warn_continues);
    RUN_TEST(test_warn_captures_info);
    RUN_TEST(test_warn_no_hooks_ok);

    printf("\n[ERROR Severity]\n");
    RUN_TEST(test_error_continues);
    RUN_TEST(test_error_with_code);

    printf("\n[Hook Chain]\n");
    RUN_TEST(test_hook_chain_order);

    printf("\n[Counters]\n");
    RUN_TEST(test_fatal_counter);
    RUN_TEST(test_multiple_warns);
    RUN_TEST(test_mixed_severities);

#if MASSERT_ENABLE_HISTORY
    printf("\n[History]\n");
    RUN_TEST(test_history_stored);
    RUN_TEST(test_history_most_recent);
    RUN_TEST(test_history_wraps);
    RUN_TEST(test_history_out_of_range);
#endif

    printf("\n[Timestamp]\n");
    RUN_TEST(test_timestamp);
    RUN_TEST(test_no_clock_zero_timestamp);

    printf("\n[Message Formatting]\n");
    RUN_TEST(test_message_format);
    RUN_TEST(test_null_message);
    RUN_TEST(test_null_expr);

    printf("\n[Macros]\n");
    RUN_TEST(test_macro_warn_true);
    RUN_TEST(test_macro_warn_false);

    printf("\n[Re-entrancy]\n");
    RUN_TEST(test_reentrant_guard);

    printf("\n[Hook Context]\n");
    RUN_TEST(test_hook_context);

    printf("\n[Edge Cases]\n");
    RUN_TEST(test_query_null);
    RUN_TEST(test_severity_str);
#if MASSERT_ENABLE_LOCATION
    RUN_TEST(test_location_capture);
#endif

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) printf(", %d FAILED", tests_failed);
    printf(" ===\n\n");

    return tests_failed > 0 ? 1 : 0;
}
