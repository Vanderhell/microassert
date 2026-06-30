/*
 * microassert test suite.
 */

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "massert.h"
#include "massert.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static const char *g_exe_path = NULL;
static const char *g_child_log_path = NULL;
static massert_t *g_child_ma = NULL;
static int g_side_effect_expr_calls = 0;
static int g_side_effect_fmt_calls = 0;
static int g_side_effect_code_calls = 0;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#ifdef _WIN32
#define TEST_TMP_DIR "."
#else
#define TEST_TMP_DIR "."
#endif

#define TEST(name) static void name(void)

static void test_fail(const char *file, int line, const char *message)
{
    printf("FAIL\n    %s:%d: %s\n", file, line, message);
    tests_failed++;
}

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        char buffer[128]; \
        snprintf(buffer, sizeof(buffer), "expected true: %s", #expr); \
        test_fail(__FILE__, __LINE__, buffer); \
        return; \
    } \
} while (0)

#define ASSERT_FALSE(expr) do { \
    if (expr) { \
        char buffer[128]; \
        snprintf(buffer, sizeof(buffer), "expected false: %s", #expr); \
        test_fail(__FILE__, __LINE__, buffer); \
        return; \
    } \
} while (0)

#define ASSERT_EQ(expected, actual) do { \
    long long _expected = (long long)(expected); \
    long long _actual = (long long)(actual); \
    if (_expected != _actual) { \
        char buffer[160]; \
        snprintf(buffer, sizeof(buffer), "expected %lld, got %lld", _expected, _actual); \
        test_fail(__FILE__, __LINE__, buffer); \
        return; \
    } \
} while (0)

#define ASSERT_STR_EQ(expected, actual) do { \
    const char *_expected = (expected); \
    const char *_actual = (actual); \
    if (_expected == NULL || _actual == NULL || strcmp(_expected, _actual) != 0) { \
        char buffer[256]; \
        snprintf(buffer, sizeof(buffer), "expected \"%s\", got \"%s\"", \
                 _expected != NULL ? _expected : "(null)", \
                 _actual != NULL ? _actual : "(null)"); \
        test_fail(__FILE__, __LINE__, buffer); \
        return; \
    } \
} while (0)

#define ASSERT_CONTAINS(haystack, needle) do { \
    const char *_haystack = (haystack); \
    const char *_needle = (needle); \
    if (_haystack == NULL || _needle == NULL || strstr(_haystack, _needle) == NULL) { \
        char buffer[256]; \
        snprintf(buffer, sizeof(buffer), "\"%s\" not in \"%s\"", \
                 _needle != NULL ? _needle : "(null)", \
                 _haystack != NULL ? _haystack : "(null)"); \
        test_fail(__FILE__, __LINE__, buffer); \
        return; \
    } \
} while (0)

static int side_effect_expr_true(void)
{
    g_side_effect_expr_calls++;
    return 1;
}

static int side_effect_expr_false(void)
{
    g_side_effect_expr_calls++;
    return 0;
}

static int side_effect_fmt_arg(void)
{
    g_side_effect_fmt_calls++;
    return 7;
}

static FILE *massert_test_fopen(const char *path, const char *mode);
static void append_log_line(const char *path, const char *fmt, ...) MICROASSERT_DETAIL_PRINTF_LIKE(2, 3);

static uint32_t side_effect_code(void)
{
    g_side_effect_code_calls++;
    if (g_child_log_path != NULL) {
        append_log_line(g_child_log_path, "code=%d", g_side_effect_code_calls);
    }
    return 77u;
}

static uint32_t fixed_clock(void)
{
    return 123456u;
}

static FILE *massert_test_fopen(const char *path, const char *mode)
{
    FILE *file = NULL;

#ifdef _WIN32
    if (fopen_s(&file, path, mode) != 0) {
        return NULL;
    }
    return file;
#else
    return fopen(path, mode);
#endif
}

static void append_log_line(const char *path, const char *fmt, ...)
{
    FILE *file;
    va_list args;

    if (path == NULL) {
        return;
    }

    file = massert_test_fopen(path, "ab");
    if (file == NULL) {
        return;
    }

    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);
    fputc('\n', file);
    fclose(file);
}

typedef struct {
    massert_t ma;
    massert_hook_slot_t hooks[8];
    massert_history_entry_t history[16];
} test_fixture_t;

static massert_status_t fixture_init(test_fixture_t *fixture,
                                     size_t hook_capacity,
                                     size_t history_capacity,
                                     massert_clock_fn clock_fn,
                                     massert_reset_fn reset_fn,
                                     void *reset_ctx,
                                     massert_halt_fn halt_fn,
                                     void *halt_ctx)
{
    massert_config_t config;
    config.hook_slots = fixture->hooks;
    config.hook_capacity = hook_capacity;
    config.history_entries = fixture->history;
    config.history_capacity = history_capacity;
    config.clock_fn = clock_fn;
    config.reset_fn = reset_fn;
    config.reset_ctx = reset_ctx;
    config.halt_fn = halt_fn;
    config.halt_ctx = halt_ctx;
    return massert_init(&fixture->ma, &config);
}

static massert_info_t g_last_info;
static int g_hook_calls = 0;
static int g_hook2_calls = 0;
static int g_hook3_calls = 0;
static int g_mutation_status = MASSERT_STATUS_OK;
static size_t g_mutation_slot = 0u;
static int g_ctx_value = 0;

static void tracking_hook(const massert_info_t *info, void *ctx)
{
    g_last_info = *info;
    g_hook_calls++;
    if (ctx != NULL) {
        g_ctx_value = *(const int *)ctx;
    }
}

static void hook_two(const massert_info_t *info, void *ctx)
{
    (void)info;
    (void)ctx;
    g_hook2_calls++;
}

static void hook_three(const massert_info_t *info, void *ctx)
{
    (void)info;
    (void)ctx;
    g_hook3_calls++;
}

static void hook_add_during_dispatch(const massert_info_t *info, void *ctx)
{
    test_fixture_t *fixture = (test_fixture_t *)ctx;
    (void)info;
    g_mutation_status = massert_add_hook(&fixture->ma, hook_three, NULL, &g_mutation_slot);
}

static void hook_clear_during_dispatch(const massert_info_t *info, void *ctx)
{
    test_fixture_t *fixture = (test_fixture_t *)ctx;
    (void)info;
    g_mutation_status = massert_clear_hooks(&fixture->ma);
}

static void hook_reinit_during_dispatch(const massert_info_t *info, void *ctx)
{
    test_fixture_t *fixture = (test_fixture_t *)ctx;
    massert_config_t config;
    (void)info;
    config.hook_slots = fixture->hooks;
    config.hook_capacity = 8u;
    config.history_entries = fixture->history;
    config.history_capacity = 16u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    g_mutation_status = massert_init(&fixture->ma, &config);
}

static void hook_nested_warn(const massert_info_t *info, void *ctx)
{
    test_fixture_t *fixture = (test_fixture_t *)ctx;
    (void)info;
    massert_fire(&fixture->ma, MASSERT_SEVERITY_WARN, "nested.c", 9u, "nested", NULL, 0u, "nested warn");
}

static void hook_nested_error(const massert_info_t *info, void *ctx)
{
    test_fixture_t *fixture = (test_fixture_t *)ctx;
    (void)info;
    massert_fire(&fixture->ma, MASSERT_SEVERITY_ERROR, "nested.c", 10u, "nested", NULL, 0u, "nested error");
}

static void hook_nested_fatal(const massert_info_t *info, void *ctx)
{
    test_fixture_t *fixture = (test_fixture_t *)ctx;
    (void)info;
    massert_panic_fatal(&fixture->ma, "nested.c", 11u, "nested", NULL, 0u, "nested fatal");
}

static uint32_t panic_clock(void)
{
    if (g_child_log_path != NULL) {
        append_log_line(g_child_log_path, "clock");
    }
    if (g_child_ma != NULL) {
        massert_fire(g_child_ma, MASSERT_SEVERITY_FATAL, "clock.c", 1u, "clock", NULL, 0u, "clock panic");
    }
    return 99u;
}

static void child_reset_log_then_return(void *ctx)
{
    const char *path = (const char *)ctx;
    append_log_line(path, "reset");
}

static void child_halt_log_then_return(void *ctx)
{
    const char *path = (const char *)ctx;
    append_log_line(path, "halt");
}

static void child_halt_log_then_exit(void *ctx)
{
    const char *path = (const char *)ctx;
    append_log_line(path, "halt");
    exit(0);
}

static void child_reset_panic(void *ctx)
{
    const char *path = (const char *)ctx;
    append_log_line(path, "reset");
    if (g_child_ma != NULL) {
        massert_panic_fatal(g_child_ma, "reset.c", 2u, "reset", NULL, 0u, "reset panic");
    }
}

static int compare_text_file(const char *path, const char *needle)
{
    char buffer[256];
    FILE *file;
    size_t total;

    file = massert_test_fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }
    total = fread(buffer, 1u, sizeof(buffer) - 1u, file);
    buffer[total] = '\0';
    fclose(file);

    return strstr(buffer, needle) != NULL;
}

typedef struct {
    int exit_code;
    int timed_out;
} child_result_t;

#ifdef _WIN32
static child_result_t spawn_child(const char *case_name, const char *log_path, unsigned timeout_ms)
{
    char exe_path[MAX_PATH];
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char command[1024];
    child_result_t result;
    DWORD wait_result;
    DWORD exit_code;

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    if (GetModuleFileNameA(NULL, exe_path, sizeof(exe_path)) == 0u) {
        result.exit_code = -1;
        result.timed_out = 0;
        return result;
    }
    snprintf(command, sizeof(command), "\"%s\" --child %s \"%s\"",
             exe_path, case_name, log_path);

    if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        result.exit_code = -1;
        result.timed_out = 0;
        return result;
    }

    wait_result = WaitForSingleObject(pi.hProcess, timeout_ms);
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1u);
        WaitForSingleObject(pi.hProcess, INFINITE);
        result.timed_out = 1;
    } else {
        result.timed_out = 0;
    }

    if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
        exit_code = 0xFFFFFFFFu;
    }
    result.exit_code = (int)exit_code;

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return result;
}
#else
static child_result_t spawn_child(const char *case_name, const char *log_path, unsigned timeout_ms)
{
    pid_t pid;
    child_result_t result;
    unsigned elapsed = 0u;
    int status = 0;

    pid = fork();
    if (pid == 0) {
        execl(g_exe_path, g_exe_path, "--child", case_name, log_path, (char *)NULL);
        _exit(127);
    }

    if (pid < 0) {
        result.exit_code = -1;
        result.timed_out = 0;
        return result;
    }

    for (;;) {
        pid_t waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid) {
            if (WIFEXITED(status)) {
                result.exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                result.exit_code = 128 + WTERMSIG(status);
            } else {
                result.exit_code = -1;
            }
            result.timed_out = 0;
            return result;
        }
        if (waited < 0) {
            result.exit_code = -1;
            result.timed_out = 0;
            return result;
        }
        if (elapsed >= timeout_ms) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            result.exit_code = -1;
            result.timed_out = 1;
            return result;
        }
        {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 10L * 1000L * 1000L;
            nanosleep(&ts, NULL);
        }
        elapsed += 10u;
    }
}
#endif

static void require_child_line(const char *path, const char *needle)
{
    ASSERT_TRUE(compare_text_file(path, needle));
}

static void child_main(const char *case_name, char *log_path)
{
    test_fixture_t fixture;
    massert_config_t config;
    massert_status_t status;

    g_child_log_path = log_path;
    memset(&fixture, 0, sizeof(fixture));

    config.hook_slots = fixture.hooks;
    config.hook_capacity = 8u;
    config.history_entries = fixture.history;
    config.history_capacity = 16u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    status = massert_init(&fixture.ma, &config);
    if (status != MASSERT_STATUS_OK) {
        exit(90);
    }

    g_child_ma = &fixture.ma;

    if (strcmp(case_name, "fatal_path") == 0) {
        massert_set_reset(&fixture.ma, child_reset_log_then_return, log_path);
        massert_set_halt(&fixture.ma, child_halt_log_then_exit, log_path);
        massert_panic_fatal(&fixture.ma, "fatal.c", 1u, "fatal", NULL, 0u, "fatal");
    } else if (strcmp(case_name, "fatal_fallback") == 0) {
        massert_set_reset(&fixture.ma, child_reset_log_then_return, log_path);
        massert_set_halt(&fixture.ma, child_halt_log_then_return, log_path);
        massert_panic_fatal(&fixture.ma, "fatal.c", 1u, "fatal", NULL, 0u, "fatal");
    } else if (strcmp(case_name, "halt_path") == 0) {
        massert_set_halt(&fixture.ma, child_halt_log_then_exit, log_path);
        massert_panic_halt(&fixture.ma, "halt.c", 1u, "halt", NULL, 0u, "halt");
    } else if (strcmp(case_name, "halt_fallback") == 0) {
        massert_set_halt(&fixture.ma, child_halt_log_then_return, log_path);
        massert_panic_halt(&fixture.ma, "halt.c", 1u, "halt", NULL, 0u, "halt");
    } else if (strcmp(case_name, "invalid_severity") == 0) {
        massert_set_halt(&fixture.ma, child_halt_log_then_exit, log_path);
        massert_fire(&fixture.ma, (massert_severity_t)99, "bad.c", 1u, "bad", NULL, 0u, "bad");
    } else if (strcmp(case_name, "nested_warn_warn") == 0) {
        massert_add_hook(&fixture.ma, hook_nested_warn, &fixture, NULL);
        massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "outer.c", 1u, "outer", NULL, 0u, "outer");
        append_log_line(log_path, "nested=%u", (unsigned)massert_nested_suppressed_count(&fixture.ma));
    } else if (strcmp(case_name, "nested_error_warn") == 0) {
        massert_add_hook(&fixture.ma, hook_nested_error, &fixture, NULL);
        massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "outer.c", 1u, "outer", NULL, 0u, "outer");
        append_log_line(log_path, "nested=%u", (unsigned)massert_nested_suppressed_count(&fixture.ma));
    } else if (strcmp(case_name, "nested_fatal_warn") == 0) {
        massert_set_halt(&fixture.ma, child_halt_log_then_exit, log_path);
        massert_add_hook(&fixture.ma, hook_nested_fatal, &fixture, NULL);
        massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "outer.c", 1u, "outer", NULL, 0u, "outer");
    } else if (strcmp(case_name, "nested_fatal_fatal") == 0) {
        massert_set_halt(&fixture.ma, child_halt_log_then_exit, log_path);
        massert_add_hook(&fixture.ma, hook_nested_fatal, &fixture, NULL);
        massert_panic_fatal(&fixture.ma, "outer.c", 1u, "outer", NULL, 0u, "outer");
    } else if (strcmp(case_name, "reset_panic") == 0) {
        massert_set_reset(&fixture.ma, child_reset_panic, log_path);
        massert_set_halt(&fixture.ma, child_halt_log_then_exit, log_path);
        massert_panic_fatal(&fixture.ma, "outer.c", 1u, "outer", NULL, 0u, "outer");
    } else if (strcmp(case_name, "clock_panic") == 0) {
        config.clock_fn = panic_clock;
        config.halt_fn = child_halt_log_then_exit;
        config.halt_ctx = log_path;
        ASSERT_EQ(MASSERT_STATUS_OK, massert_init(&fixture.ma, &config));
        g_child_ma = &fixture.ma;
        massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "outer.c", 1u, "outer", NULL, 0u, "outer");
    } else if (strcmp(case_name, "macro_code_eval") == 0) {
        ASSERT_EQ(MASSERT_STATUS_OK, massert_init(NULL, &config));
        ASSERT_EQ(MASSERT_STATUS_OK, massert_set_halt(NULL, child_halt_log_then_exit, log_path));
        MPANIC_CODE(side_effect_code(), "macro fatal");
    } else {
        exit(91);
    }
}

TEST(test_include_guard) {
    ASSERT_TRUE(strcmp(massert_severity_str(MASSERT_SEVERITY_WARN), "WARN") == 0);
}

TEST(test_status_and_init) {
    test_fixture_t fixture;
    massert_config_t config;

    memset(&fixture, 0, sizeof(fixture));
    config.hook_slots = fixture.hooks;
    config.hook_capacity = 0u;
    config.history_entries = fixture.history;
    config.history_capacity = 0u;
    config.clock_fn = NULL;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(&fixture.ma, &config));
    ASSERT_EQ(0u, massert_total_count(&fixture.ma));
    ASSERT_EQ(0u, massert_history_count(&fixture.ma));
}

TEST(test_layout_and_offsets) {
    ASSERT_TRUE(sizeof(massert_t) > 0u);
    ASSERT_TRUE(offsetof(massert_t, magic) < offsetof(massert_t, hook_slots));
    ASSERT_TRUE(offsetof(massert_t, hook_slots) < offsetof(massert_t, history_entries));
    ASSERT_TRUE(offsetof(massert_t, history_entries) < offsetof(massert_t, total_count));
    ASSERT_TRUE(sizeof(massert_history_entry_t) > sizeof(massert_info_t));
}

TEST(test_invalid_storage_pairs) {
    massert_t ma;
    massert_config_t config;
    memset(&ma, 0, sizeof(ma));

    config.hook_slots = NULL;
    config.hook_capacity = 1u;
    config.history_entries = NULL;
    config.history_capacity = 1u;
    config.clock_fn = NULL;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_INVALID_ARGUMENT, massert_init(&ma, &config));
}

TEST(test_zero_capacity_disables_storage) {
    test_fixture_t fixture;
    massert_config_t config;
    memset(&fixture, 0, sizeof(fixture));
    config.hook_slots = NULL;
    config.hook_capacity = 0u;
    config.history_entries = NULL;
    config.history_capacity = 0u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(&fixture.ma, &config));
    ASSERT_EQ(MASSERT_STATUS_INVALID_STATE, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    ASSERT_EQ(0u, massert_history_count(&fixture.ma));
}

TEST(test_one_entry_capacity) {
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 1u, 1u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_FULL, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
}

TEST(test_full_capacity) {
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 1u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_three, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_three, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_FULL, massert_add_hook(&fixture.ma, hook_three, NULL, NULL));
}

TEST(test_large_capacity) {
    massert_t ma;
    massert_hook_slot_t hooks[32];
    massert_history_entry_t history[32];
    massert_status_t status;
    size_t i;
    massert_config_t config;

    memset(&ma, 0, sizeof(ma));
    memset(hooks, 0, sizeof(hooks));
    memset(history, 0, sizeof(history));
    config.hook_slots = hooks;
    config.hook_capacity = 32u;
    config.history_entries = history;
    config.history_capacity = 32u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    status = massert_init(&ma, &config);
    ASSERT_EQ(MASSERT_STATUS_OK, status);
    for (i = 0u; i < 32u; i++) {
        ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&ma, tracking_hook, NULL, NULL));
    }
    ASSERT_EQ(MASSERT_STATUS_FULL, massert_add_hook(&ma, hook_two, NULL, NULL));
}

TEST(test_large_capacity_above_255) {
    massert_t ma;
    massert_hook_slot_t hooks[256];
    massert_history_entry_t history[256];
    massert_config_t config;
    size_t i;

    memset(&ma, 0, sizeof(ma));
    memset(hooks, 0, sizeof(hooks));
    memset(history, 0, sizeof(history));
    config.hook_slots = hooks;
    config.hook_capacity = 256u;
    config.history_entries = history;
    config.history_capacity = 256u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(&ma, &config));
    for (i = 0u; i < 256u; i++) {
        ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&ma, tracking_hook, NULL, NULL));
    }
    ASSERT_EQ(MASSERT_STATUS_FULL, massert_add_hook(&ma, hook_two, NULL, NULL));
}

TEST(test_config_copy_by_value_and_independent_instances) {
    test_fixture_t fixture_a;
    test_fixture_t fixture_b;
    massert_config_t config;
    int hook_value_a = 7;
    int hook_value_b = 11;
    void *hook_ctx_a = &hook_value_a;
    void *hook_ctx_b = &hook_value_b;
    int mutated_ctx = 99;

    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture_a, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture_b, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));

    g_hook_calls = 0;
    g_hook2_calls = 0;
    g_ctx_value = 0;

    config.hook_slots = fixture_a.hooks;
    config.hook_capacity = 8u;
    config.history_entries = fixture_a.history;
    config.history_capacity = 8u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(&fixture_a.ma, &config));
    config.hook_slots = fixture_b.hooks;
    config.history_entries = fixture_b.history;
    config.hook_capacity = 8u;
    config.history_capacity = 8u;
    config.clock_fn = fixed_clock;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(&fixture_b.ma, &config));
    config.clock_fn = NULL;
    config.hook_slots = NULL;
    config.history_entries = NULL;

    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture_a.ma, tracking_hook, hook_ctx_a, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture_b.ma, tracking_hook, hook_ctx_b, NULL));
    hook_ctx_a = &mutated_ctx;
    hook_ctx_b = &mutated_ctx;

    massert_fire(&fixture_a.ma, MASSERT_SEVERITY_WARN, "a.c", 1u, "a", NULL, 0u, "a");
    ASSERT_EQ(123456u, g_last_info.timestamp_ms);
    ASSERT_EQ(7, g_ctx_value);
    ASSERT_EQ(1, g_hook_calls);
    ASSERT_EQ(1u, massert_history_count(&fixture_a.ma));
    ASSERT_EQ(0u, massert_history_count(&fixture_b.ma));

    g_hook_calls = 0;
    g_ctx_value = 0;
    massert_fire(&fixture_b.ma, MASSERT_SEVERITY_WARN, "b.c", 2u, "b", NULL, 0u, "b");
    ASSERT_EQ(123456u, g_last_info.timestamp_ms);
    ASSERT_EQ(11, g_ctx_value);
    ASSERT_EQ(1, g_hook_calls);
    ASSERT_EQ(1u, massert_history_count(&fixture_b.ma));
    ASSERT_EQ(1u, massert_history_count(&fixture_a.ma));
}

TEST(test_invalid_init_variants) {
    massert_t ma;
    massert_config_t config;
    memset(&ma, 0, sizeof(ma));

    ASSERT_EQ(MASSERT_STATUS_INVALID_ARGUMENT, massert_init(NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_INVALID_ARGUMENT, massert_init(&ma, NULL));

    config.hook_slots = NULL;
    config.hook_capacity = 1u;
    config.history_entries = NULL;
    config.history_capacity = 0u;
    config.clock_fn = NULL;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_INVALID_ARGUMENT, massert_init(&ma, &config));

    config.hook_slots = NULL;
    config.hook_capacity = 0u;
    config.history_entries = NULL;
    config.history_capacity = 1u;
    ASSERT_EQ(MASSERT_STATUS_INVALID_ARGUMENT, massert_init(&ma, &config));
}

TEST(test_hook_order_and_context) {
    test_fixture_t fixture;
    int ctx_value = 42;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    g_hook_calls = 0;
    g_hook2_calls = 0;
    g_hook3_calls = 0;
    g_ctx_value = 0;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, &ctx_value, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "file.c", 2u, "func", "expr", 77u, "hello");
    ASSERT_EQ(1, g_hook_calls);
    ASSERT_EQ(42, g_ctx_value);
    ASSERT_EQ(MASSERT_SEVERITY_WARN, g_last_info.severity);
    ASSERT_EQ(77u, g_last_info.error_code);
    ASSERT_STR_EQ("file.c", g_last_info.file);
    ASSERT_STR_EQ("func", g_last_info.func);
    ASSERT_STR_EQ("expr", g_last_info.expr);
    ASSERT_EQ(1, g_hook2_calls);
}

TEST(test_hook_mutation_during_dispatch) {
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    g_mutation_status = MASSERT_STATUS_OK;
    g_hook2_calls = 0;
    g_hook3_calls = 0;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_add_during_dispatch, &fixture, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "file.c", 3u, "func", NULL, 0u, "hello");
    ASSERT_EQ(MASSERT_STATUS_BUSY, g_mutation_status);
    ASSERT_EQ(1, g_hook2_calls);
    ASSERT_EQ(0, g_hook3_calls);
    ASSERT_EQ(2u, fixture.ma.hook_count);

    g_mutation_status = MASSERT_STATUS_OK;
    g_hook2_calls = 0;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_clear_hooks(&fixture.ma));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_clear_during_dispatch, &fixture, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "file.c", 4u, "func", NULL, 0u, "hello");
    ASSERT_EQ(MASSERT_STATUS_BUSY, g_mutation_status);
    ASSERT_EQ(1, g_hook2_calls);
    ASSERT_EQ(2u, fixture.ma.hook_count);
}

TEST(test_reinit_during_dispatch)
{
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    g_mutation_status = MASSERT_STATUS_OK;
    g_hook2_calls = 0;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_reinit_during_dispatch, &fixture, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_two, NULL, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "file.c", 5u, "func", NULL, 0u, "hello");
    ASSERT_EQ(MASSERT_STATUS_BUSY, g_mutation_status);
    ASSERT_EQ(1, g_hook2_calls);
}

TEST(test_nested_warn_and_error_suppressed) {
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_nested_warn, &fixture, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "outer.c", 1u, "outer", NULL, 0u, "outer");
    ASSERT_EQ(1u, massert_nested_suppressed_count(&fixture.ma));
    ASSERT_EQ(1u, massert_total_count(&fixture.ma));
    ASSERT_EQ(0u, massert_error_count(&fixture.ma));

    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, hook_nested_error, &fixture, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "outer.c", 2u, "outer", NULL, 0u, "outer");
    ASSERT_EQ(1u, massert_nested_suppressed_count(&fixture.ma));
    ASSERT_EQ(1u, massert_total_count(&fixture.ma));
    ASSERT_EQ(0u, massert_error_count(&fixture.ma));
}

TEST(test_history_storage_and_copy) {
    test_fixture_t fixture;
    char file[32];
    char func[32];
    char expr[32];
    int i;

    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 4u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    snprintf(file, sizeof(file), "%s", "stack-file.c");
    snprintf(func, sizeof(func), "%s", "stack_func");
    snprintf(expr, sizeof(expr), "%s", "x > 1");
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, file, 123u, func, expr, 44u, "msg %d", 7);
    snprintf(file, sizeof(file), "%s", "mutated");
    snprintf(func, sizeof(func), "%s", "mutated");
    snprintf(expr, sizeof(expr), "%s", "mutated");
    ASSERT_EQ(1u, massert_history_count(&fixture.ma));
    ASSERT_STR_EQ("stack-file.c", massert_history_at(&fixture.ma, 0u)->file);
    ASSERT_STR_EQ("stack_func", massert_history_at(&fixture.ma, 0u)->func);
    ASSERT_STR_EQ("x > 1", massert_history_at(&fixture.ma, 0u)->expr);
    ASSERT_STR_EQ("msg 7", massert_history_at(&fixture.ma, 0u)->msg);

    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 2u, fixed_clock, NULL, NULL, NULL, NULL));
    for (i = 0; i < 3; i++) {
        massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "w.c", (uint32_t)(10 + i), "f", "e", (uint32_t)i, "m%d", i);
    }
    ASSERT_EQ(2u, massert_history_count(&fixture.ma));
    ASSERT_EQ(2u, massert_history_at(&fixture.ma, 0u)->error_code);
    ASSERT_EQ(1u, massert_history_at(&fixture.ma, 1u)->error_code);
}

TEST(test_history_disabled_and_query_global) {
    massert_t *global = massert_global();
    massert_config_t config;
    memset(global, 0, sizeof(*global));
    ASSERT_TRUE(global == massert_global());
    ASSERT_EQ(0u, massert_total_count(NULL));
    config.hook_slots = NULL;
    config.hook_capacity = 0u;
    config.history_entries = NULL;
    config.history_capacity = 0u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(NULL, &config));
    ASSERT_EQ(0u, massert_history_count(NULL));
    massert_fire(NULL, MASSERT_SEVERITY_WARN, "g.c", 1u, "g", NULL, 0u, "global");
    ASSERT_EQ(1u, massert_total_count(NULL));
    ASSERT_EQ(1u, massert_warn_count(NULL));
    ASSERT_EQ(0u, massert_history_count(NULL));
    ASSERT_TRUE(global == massert_global());
}

TEST(test_message_formatting)
{
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "msg.c", 1u, "msg", NULL, 0u, "temp=%d hum=%.1f", 25, 55.5);
    ASSERT_CONTAINS(g_last_info.msg, "temp=25");
    ASSERT_CONTAINS(g_last_info.msg, "hum=55.5");
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "msg.c", 1u, "msg", NULL, 0u, NULL);
    ASSERT_STR_EQ("", g_last_info.msg);
}

TEST(test_message_truncation_and_exact_fit) {
    test_fixture_t fixture;
    char exact[MASSERT_MESSAGE_CAPACITY];
    char long_msg[MASSERT_MESSAGE_CAPACITY + 1u];
    size_t i;

    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(&fixture.ma, tracking_hook, NULL, NULL));

    memset(exact, 'A', sizeof(exact));
    exact[MASSERT_MESSAGE_CAPACITY - 1u] = '\0';
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "msg.c", 2u, "msg", NULL, 0u, "%s", exact);
    ASSERT_FALSE(g_last_info.message_truncated);
    ASSERT_FALSE(g_last_info.format_failed);
    ASSERT_EQ(MASSERT_MESSAGE_CAPACITY - 1u, strlen(g_last_info.msg));

    for (i = 0u; i < sizeof(long_msg) - 1u; i++) {
        long_msg[i] = 'B';
    }
    long_msg[sizeof(long_msg) - 1u] = '\0';
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "msg.c", 3u, "msg", NULL, 0u, "%s", long_msg);
    ASSERT_TRUE(g_last_info.message_truncated);
    ASSERT_EQ(MASSERT_MESSAGE_CAPACITY - 1u, strlen(g_last_info.msg));
}

TEST(test_macro_evaluation)
{
    test_fixture_t fixture;
    massert_t *global = massert_global();
    massert_config_t config;
    g_side_effect_expr_calls = 0;
    g_side_effect_fmt_calls = 0;
    g_hook_calls = 0;
    config.hook_slots = fixture.hooks;
    config.hook_capacity = 8u;
    config.history_entries = fixture.history;
    config.history_capacity = 8u;
    config.clock_fn = fixed_clock;
    config.reset_fn = NULL;
    config.reset_ctx = NULL;
    config.halt_fn = NULL;
    config.halt_ctx = NULL;
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(NULL, &config));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(NULL, tracking_hook, NULL, NULL));

    MASSERT_WARN(side_effect_expr_true(), "unused %d", side_effect_fmt_arg());
    ASSERT_EQ(1, g_side_effect_expr_calls);
    ASSERT_EQ(0, g_side_effect_fmt_calls);
    ASSERT_EQ(0, g_hook_calls);

    MASSERT_WARN(side_effect_expr_false(), "used %d", side_effect_fmt_arg());
    ASSERT_EQ(2, g_side_effect_expr_calls);
    ASSERT_EQ(1, g_side_effect_fmt_calls);
    ASSERT_EQ(1, g_hook_calls);

    g_side_effect_expr_calls = 0;
    g_side_effect_fmt_calls = 0;
    MASSERT_MSG(side_effect_expr_true(), "unused %d", side_effect_fmt_arg());
    ASSERT_EQ(1, g_side_effect_expr_calls);
    ASSERT_EQ(0, g_side_effect_fmt_calls);
    (void)global;
}

TEST(test_unbraced_if_else_and_same_line_macros)
{
    test_fixture_t fixture;
    int count = 0;
    g_hook_calls = 0;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_init(NULL, &(massert_config_t){
        .hook_slots = fixture.hooks,
        .hook_capacity = 8u,
        .history_entries = fixture.history,
        .history_capacity = 8u,
        .clock_fn = fixed_clock,
        .reset_fn = NULL,
        .reset_ctx = NULL,
        .halt_fn = NULL,
        .halt_ctx = NULL
    }));
    ASSERT_EQ(MASSERT_STATUS_OK, massert_add_hook(NULL, tracking_hook, NULL, NULL));
    if (count == 0)
        MASSERT_WARN(0, "first");
    else
        count++;
    MASSERT_WARN(0, "line one"); MASSERT_WARN(0, "line two");
    ASSERT_EQ(3, g_hook_calls);
}

TEST(test_saturating_counters)
{
    test_fixture_t fixture;
    ASSERT_EQ(MASSERT_STATUS_OK, fixture_init(&fixture, 8u, 8u, fixed_clock, NULL, NULL, NULL, NULL));
    fixture.ma.total_count = UINT32_MAX - 1u;
    fixture.ma.warn_count = UINT32_MAX - 1u;
    fixture.ma.error_count = UINT32_MAX - 1u;
    fixture.ma.fatal_count = UINT32_MAX - 1u;
    fixture.ma.halt_count = UINT32_MAX - 1u;
    fixture.ma.nested_suppressed_count = UINT32_MAX - 1u;
    massert_fire(&fixture.ma, MASSERT_SEVERITY_WARN, "c.c", 1u, "c", NULL, 0u, "one");
    massert_fire(&fixture.ma, MASSERT_SEVERITY_ERROR, "c.c", 2u, "c", NULL, 0u, "two");
    ASSERT_EQ(UINT32_MAX, massert_total_count(&fixture.ma));
    ASSERT_EQ(UINT32_MAX, massert_warn_count(&fixture.ma));
    ASSERT_EQ(UINT32_MAX, massert_error_count(&fixture.ma));
}

TEST(test_direct_terminal_paths)
{
    char log_path[256];
    child_result_t result;
    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_fatal_path.log");
    remove(log_path);
    result = spawn_child("fatal_path", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    ASSERT_TRUE(result.exit_code == 0);
    require_child_line(log_path, "reset");
    require_child_line(log_path, "halt");

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_halt_path.log");
    remove(log_path);
    result = spawn_child("halt_path", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    ASSERT_TRUE(result.exit_code == 0);
    require_child_line(log_path, "halt");
}

TEST(test_terminal_fallback_paths)
{
    char log_path[256];
    child_result_t result;
    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_fatal_fallback.log");
    remove(log_path);
    result = spawn_child("fatal_fallback", log_path, 200u);
    ASSERT_TRUE(result.timed_out);
    require_child_line(log_path, "reset");
    require_child_line(log_path, "halt");

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_halt_fallback.log");
    remove(log_path);
    result = spawn_child("halt_fallback", log_path, 200u);
    ASSERT_TRUE(result.timed_out);
    require_child_line(log_path, "halt");
}

TEST(test_invalid_severity_and_nested_terminal_paths)
{
    char log_path[256];
    child_result_t result;
    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_invalid.log");
    remove(log_path);
    result = spawn_child("invalid_severity", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    ASSERT_TRUE(result.exit_code == 0);
    require_child_line(log_path, "halt");

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_nested_warn.log");
    remove(log_path);
    result = spawn_child("nested_warn_warn", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    ASSERT_TRUE(compare_text_file(log_path, "nested=1"));

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_nested_error.log");
    remove(log_path);
    result = spawn_child("nested_error_warn", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    ASSERT_TRUE(compare_text_file(log_path, "nested=1"));

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_nested_fatal_warn.log");
    remove(log_path);
    result = spawn_child("nested_fatal_warn", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    require_child_line(log_path, "halt");

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_nested_fatal_fatal.log");
    remove(log_path);
    result = spawn_child("nested_fatal_fatal", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    require_child_line(log_path, "halt");
}

TEST(test_reset_and_clock_panic_paths)
{
    char log_path[256];
    child_result_t result;
    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_reset_panic.log");
    remove(log_path);
    result = spawn_child("reset_panic", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    require_child_line(log_path, "reset");
    require_child_line(log_path, "halt");

    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_clock_panic.log");
    remove(log_path);
    result = spawn_child("clock_panic", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    require_child_line(log_path, "clock");
    require_child_line(log_path, "halt");
}

TEST(test_macro_code_evaluation)
{
    char log_path[256];
    child_result_t result;
    snprintf(log_path, sizeof(log_path), "%s/%s", TEST_TMP_DIR, "microassert_code_eval.log");
    remove(log_path);
    result = spawn_child("macro_code_eval", log_path, 2000u);
    ASSERT_FALSE(result.timed_out);
    ASSERT_TRUE(result.exit_code == 0);
    require_child_line(log_path, "code=1");
    require_child_line(log_path, "halt");
}

typedef void (*test_fn_t)(void);

typedef struct {
    const char *name;
    test_fn_t fn;
} test_case_t;

static void run_case(const char *name, test_fn_t fn)
{
    int before_failures = tests_failed;

    tests_run++;
    printf("  %-55s ", name);
    fn();
    if (tests_failed == before_failures) {
        printf("PASS\n");
        tests_passed++;
    }
}

static void run_suite(void)
{
    test_case_t cases[] = {
        {"test_include_guard", test_include_guard},
        {"test_status_and_init", test_status_and_init},
        {"test_layout_and_offsets", test_layout_and_offsets},
        {"test_invalid_storage_pairs", test_invalid_storage_pairs},
        {"test_zero_capacity_disables_storage", test_zero_capacity_disables_storage},
        {"test_one_entry_capacity", test_one_entry_capacity},
        {"test_full_capacity", test_full_capacity},
        {"test_large_capacity", test_large_capacity},
        {"test_large_capacity_above_255", test_large_capacity_above_255},
        {"test_config_copy_by_value_and_independent_instances", test_config_copy_by_value_and_independent_instances},
        {"test_invalid_init_variants", test_invalid_init_variants},
        {"test_hook_order_and_context", test_hook_order_and_context},
        {"test_hook_mutation_during_dispatch", test_hook_mutation_during_dispatch},
        {"test_reinit_during_dispatch", test_reinit_during_dispatch},
        {"test_nested_warn_and_error_suppressed", test_nested_warn_and_error_suppressed},
        {"test_history_storage_and_copy", test_history_storage_and_copy},
        {"test_history_disabled_and_query_global", test_history_disabled_and_query_global},
        {"test_message_formatting", test_message_formatting},
        {"test_message_truncation_and_exact_fit", test_message_truncation_and_exact_fit},
        {"test_macro_evaluation", test_macro_evaluation},
        {"test_unbraced_if_else_and_same_line_macros", test_unbraced_if_else_and_same_line_macros},
        {"test_saturating_counters", test_saturating_counters},
        {"test_direct_terminal_paths", test_direct_terminal_paths},
        {"test_terminal_fallback_paths", test_terminal_fallback_paths},
        {"test_invalid_severity_and_nested_terminal_paths", test_invalid_severity_and_nested_terminal_paths},
        {"test_reset_and_clock_panic_paths", test_reset_and_clock_panic_paths},
        {"test_macro_code_evaluation", test_macro_code_evaluation}
    };
    size_t i;

    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); i++) {
        run_case(cases[i].name, cases[i].fn);
    }
}

int main(int argc, char **argv)
{
    if (argc >= 4 && strcmp(argv[1], "--child") == 0) {
        child_main(argv[2], argv[3]);
        return 0;
    }

    g_exe_path = argv[0];
    run_suite();

    printf("\n=== Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(", %d FAILED", tests_failed);
    }
    printf(" ===\n\n");

    return tests_failed == 0 ? 0 : 1;
}
