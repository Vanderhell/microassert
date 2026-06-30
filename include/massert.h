/*
 * microassert - deterministic panic/assert support for embedded C.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MICROASSERT_MASSERT_H
#define MICROASSERT_MASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__clang__) || defined(__GNUC__)
#define MICROASSERT_DETAIL_PRINTF_LIKE(format_index, first_arg_index) \
    __attribute__((format(printf, format_index, first_arg_index)))
#else
#define MICROASSERT_DETAIL_PRINTF_LIKE(format_index, first_arg_index)
#endif

#if defined(_MSC_VER)
#define MICROASSERT_DETAIL_NORETURN __declspec(noreturn)
#elif defined(__cplusplus) && __cplusplus >= 201103L
#define MICROASSERT_DETAIL_NORETURN [[noreturn]]
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define MICROASSERT_DETAIL_NORETURN _Noreturn
#elif defined(__clang__) || defined(__GNUC__)
#define MICROASSERT_DETAIL_NORETURN __attribute__((noreturn))
#else
#define MICROASSERT_DETAIL_NORETURN
#endif

#ifndef MICROASSERT_ENABLE_LOCATION
#define MICROASSERT_ENABLE_LOCATION 1
#endif

enum {
    MASSERT_MESSAGE_CAPACITY = 128u,
    MASSERT_TEXT_COPY_CAPACITY = 64u
};

typedef enum {
    MASSERT_SEVERITY_WARN = 0,
    MASSERT_SEVERITY_ERROR = 1,
    MASSERT_SEVERITY_FATAL = 2,
    MASSERT_SEVERITY_HALT = 3
} massert_severity_t;

typedef enum {
    MASSERT_STATUS_OK = 0,
    MASSERT_STATUS_INVALID_ARGUMENT = 1,
    MASSERT_STATUS_INVALID_STATE = 2,
    MASSERT_STATUS_BUSY = 3,
    MASSERT_STATUS_FULL = 4
} massert_status_t;

typedef struct {
    massert_severity_t severity;
    uint32_t timestamp_ms;
    const char *file;
    uint32_t line;
    const char *func;
    const char *expr;
    char msg[MASSERT_MESSAGE_CAPACITY];
    uint32_t error_code;
    bool message_truncated;
    bool format_failed;
} massert_info_t;

typedef struct {
    massert_severity_t severity;
    uint32_t timestamp_ms;
    uint32_t line;
    uint32_t error_code;
    char file[MASSERT_TEXT_COPY_CAPACITY];
    char func[MASSERT_TEXT_COPY_CAPACITY];
    char expr[MASSERT_TEXT_COPY_CAPACITY];
    char msg[MASSERT_MESSAGE_CAPACITY];
    bool file_truncated;
    bool func_truncated;
    bool expr_truncated;
    bool message_truncated;
    bool format_failed;
} massert_history_entry_t;

typedef void (*massert_hook_fn)(const massert_info_t *info, void *ctx);
typedef uint32_t (*massert_clock_fn)(void);
typedef void (*massert_reset_fn)(void *ctx);
typedef void (*massert_halt_fn)(void *ctx);

typedef struct {
    massert_hook_fn hook;
    void *ctx;
} massert_hook_slot_t;

typedef struct {
    massert_hook_slot_t *hook_slots;
    size_t hook_capacity;
    massert_history_entry_t *history_entries;
    size_t history_capacity;
    massert_clock_fn clock_fn;
    massert_reset_fn reset_fn;
    void *reset_ctx;
    massert_halt_fn halt_fn;
    void *halt_ctx;
} massert_config_t;

typedef struct {
    uint32_t magic;

    massert_hook_slot_t *hook_slots;
    size_t hook_capacity;
    size_t hook_count;

    massert_history_entry_t *history_entries;
    size_t history_capacity;
    size_t history_head;
    size_t history_count;

    massert_clock_fn clock_fn;
    massert_reset_fn reset_fn;
    void *reset_ctx;
    massert_halt_fn halt_fn;
    void *halt_ctx;

    uint32_t total_count;
    uint32_t warn_count;
    uint32_t error_count;
    uint32_t fatal_count;
    uint32_t halt_count;
    uint32_t nested_suppressed_count;

    bool in_panic;
} massert_t;

const char *massert_severity_str(massert_severity_t severity);

massert_t *massert_global(void);

massert_status_t massert_init(massert_t *ma, const massert_config_t *config);
massert_status_t massert_set_reset(massert_t *ma, massert_reset_fn fn, void *ctx);
massert_status_t massert_set_halt(massert_t *ma, massert_halt_fn fn, void *ctx);
massert_status_t massert_add_hook(massert_t *ma, massert_hook_fn fn, void *ctx, size_t *slot_index);
massert_status_t massert_clear_hooks(massert_t *ma);

void massert_fire(massert_t *ma, massert_severity_t severity,
                  const char *file, uint32_t line, const char *func,
                  const char *expr, uint32_t code,
                  const char *fmt, ...) MICROASSERT_DETAIL_PRINTF_LIKE(8, 9);

MICROASSERT_DETAIL_NORETURN void massert_panic_fatal(
    massert_t *ma, const char *file, uint32_t line, const char *func,
    const char *expr, uint32_t code,
    const char *fmt, ...) MICROASSERT_DETAIL_PRINTF_LIKE(7, 8);

MICROASSERT_DETAIL_NORETURN void massert_panic_halt(
    massert_t *ma, const char *file, uint32_t line, const char *func,
    const char *expr, uint32_t code,
    const char *fmt, ...) MICROASSERT_DETAIL_PRINTF_LIKE(7, 8);

uint32_t massert_total_count(const massert_t *ma);
uint32_t massert_warn_count(const massert_t *ma);
uint32_t massert_error_count(const massert_t *ma);
uint32_t massert_fatal_count(const massert_t *ma);
uint32_t massert_halt_count(const massert_t *ma);
uint32_t massert_nested_suppressed_count(const massert_t *ma);

size_t massert_history_count(const massert_t *ma);
const massert_history_entry_t *massert_history_at(const massert_t *ma, size_t offset);

#define MICROASSERT_DETAIL_LOC_ARGS \
    __FILE__, (uint32_t)__LINE__, __func__

#if !MICROASSERT_ENABLE_LOCATION
#undef MICROASSERT_DETAIL_LOC_ARGS
#define MICROASSERT_DETAIL_LOC_ARGS NULL, 0u, NULL
#endif

#define MASSERT(expr) \
    do { \
        if (!(expr)) { \
            massert_panic_fatal(NULL, MICROASSERT_DETAIL_LOC_ARGS, \
                                #expr, 0u, "assertion failed: %s", #expr); \
        } \
    } while (0)

#define MASSERT_MSG(expr, ...) \
    do { \
        if (!(expr)) { \
            massert_panic_fatal(NULL, MICROASSERT_DETAIL_LOC_ARGS, \
                                #expr, 0u, __VA_ARGS__); \
        } \
    } while (0)

#define MASSERT_WARN(expr, ...) \
    do { \
        if (!(expr)) { \
            massert_fire(NULL, MASSERT_SEVERITY_WARN, MICROASSERT_DETAIL_LOC_ARGS, \
                         #expr, 0u, __VA_ARGS__); \
        } \
    } while (0)

#define MPANIC(...) \
    massert_panic_fatal(NULL, MICROASSERT_DETAIL_LOC_ARGS, NULL, 0u, __VA_ARGS__)

#define MPANIC_CODE(code, ...) \
    massert_panic_fatal(NULL, MICROASSERT_DETAIL_LOC_ARGS, NULL, (uint32_t)(code), __VA_ARGS__)

#define MPANIC_HALT(...) \
    massert_panic_halt(NULL, MICROASSERT_DETAIL_LOC_ARGS, NULL, 0u, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* MICROASSERT_MASSERT_H */
