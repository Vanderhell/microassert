/*
 * microassert — Unified panic & assert system for embedded systems.
 *
 * Replaces scattered assert(), while(1), and HardFault handlers with a
 * single, structured panic pipeline: capture context → log → dump → reset.
 * Bridges panicdump, nvlog, defer, and microlog.
 *
 * C99 · Zero dependencies · Zero allocations · Callback-driven · Portable
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microassert
 */

#ifndef MASSERT_H
#define MASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Configuration ─────────────────────────────────────────────────────── */

/** Maximum number of panic hooks in the chain. */
#ifndef MASSERT_MAX_HOOKS
#define MASSERT_MAX_HOOKS 4
#endif

/** Maximum length of the panic message buffer. */
#ifndef MASSERT_MSG_SIZE
#define MASSERT_MSG_SIZE 96
#endif

/** Enable file/line capture in panic info. Set 0 to save ROM. */
#ifndef MASSERT_ENABLE_LOCATION
#define MASSERT_ENABLE_LOCATION 1
#endif

/** Enable panic history (ring of last N panics). */
#ifndef MASSERT_ENABLE_HISTORY
#define MASSERT_ENABLE_HISTORY 1
#endif

/** Number of panic history entries. */
#ifndef MASSERT_HISTORY_DEPTH
#define MASSERT_HISTORY_DEPTH 4
#endif

/* ── Panic severity ────────────────────────────────────────────────────── */

typedef enum {
    MASSERT_WARN    = 0,   /**< Log + continue (soft assert).            */
    MASSERT_ERROR   = 1,   /**< Log + hooks fire, but continue.          */
    MASSERT_FATAL   = 2,   /**< Log + hooks + reset (no return).         */
    MASSERT_HALT    = 3,   /**< Log + hooks + infinite loop (debug).     */
} massert_severity_t;

const char *massert_severity_str(massert_severity_t sev);

/* ── Panic info ────────────────────────────────────────────────────────── */

/** Captured context at the point of panic. */
typedef struct {
    massert_severity_t  severity;
    uint32_t            timestamp_ms;
#if MASSERT_ENABLE_LOCATION
    const char         *file;            /**< Source file (or NULL).       */
    uint32_t            line;            /**< Source line (or 0).          */
    const char         *func;            /**< Function name (or NULL).     */
#endif
    const char         *expr;            /**< Failed expression (or NULL). */
    char                msg[MASSERT_MSG_SIZE]; /**< Formatted message.     */
    uint32_t            error_code;      /**< Optional app error code.     */
} massert_info_t;

/* ── Callbacks ─────────────────────────────────────────────────────────── */

/** Clock function — milliseconds uptime. */
typedef uint32_t (*massert_clock_fn)(void);

/**
 * Panic hook — called in sequence when a panic fires.
 *
 * Typical hooks:
 *   1. Log to microlog:  MLOG_ERROR("PANIC", "%s", info->msg);
 *   2. Write to nvlog:   nvlog_append(&log, info, sizeof(*info));
 *   3. Capture dump:     panicdump_save();
 *   4. Flush buffers:    uart_flush();
 *
 * Hooks must be short. On FATAL, the system resets after all hooks run.
 * On HALT, it enters an infinite loop after hooks.
 */
typedef void (*massert_hook_fn)(const massert_info_t *info, void *ctx);

/**
 * Reset function — called on FATAL severity after all hooks.
 * Should not return. Example: NVIC_SystemReset(), esp_restart().
 */
typedef void (*massert_reset_fn)(void *ctx);

/* ── Assert engine instance ────────────────────────────────────────────── */

typedef struct {
    massert_hook_fn   hooks[MASSERT_MAX_HOOKS];
    void             *hook_ctx[MASSERT_MAX_HOOKS];
    uint8_t           num_hooks;

    massert_clock_fn  clock;
    massert_reset_fn  reset_fn;
    void             *reset_ctx;

    uint32_t          panic_count;       /**< Total panics since init.     */
    uint32_t          warn_count;
    uint32_t          error_count;
    uint32_t          fatal_count;

    bool              in_panic;          /**< Re-entrancy guard.           */

#if MASSERT_ENABLE_HISTORY
    massert_info_t    history[MASSERT_HISTORY_DEPTH];
    uint8_t           hist_head;
    uint8_t           hist_count;
#endif
} massert_t;

/* ── Global instance ───────────────────────────────────────────────────── */

/** Get the global assert engine singleton. */
massert_t *massert_global(void);

/* ── Init ──────────────────────────────────────────────────────────────── */

/**
 * Initialise assert engine.
 * @param ma     Instance (caller-allocated, or NULL for global).
 * @param clock  Clock function (may be NULL — timestamps will be 0).
 */
void massert_init(massert_t *ma, massert_clock_fn clock);

/** Set reset function for FATAL panics. */
void massert_set_reset(massert_t *ma, massert_reset_fn fn, void *ctx);

/**
 * Add a panic hook to the chain. Hooks fire in registration order.
 * @return Hook index (0-based) or -1 if full.
 */
int massert_add_hook(massert_t *ma, massert_hook_fn fn, void *ctx);

/** Remove all hooks. */
void massert_clear_hooks(massert_t *ma);

/* ── Core panic function ───────────────────────────────────────────────── */

/**
 * Fire a panic. This is the core function — all macros call this.
 *
 * @param ma        Instance (NULL = global).
 * @param severity  WARN, ERROR, FATAL, or HALT.
 * @param file      Source file (or NULL).
 * @param line      Source line (or 0).
 * @param func      Function name (or NULL).
 * @param expr      Failed expression string (or NULL).
 * @param code      Application error code (or 0).
 * @param fmt       Printf-style message (or NULL).
 */
void massert_fire(massert_t *ma, massert_severity_t severity,
                   const char *file, uint32_t line, const char *func,
                   const char *expr, uint32_t code,
                   const char *fmt, ...);

/* ── Query ─────────────────────────────────────────────────────────────── */

/** Total panic count. */
uint32_t massert_panic_count(const massert_t *ma);

/** Count by severity. */
uint32_t massert_warn_count(const massert_t *ma);
uint32_t massert_error_count(const massert_t *ma);
uint32_t massert_fatal_count(const massert_t *ma);

#if MASSERT_ENABLE_HISTORY
/** Number of stored panic history entries. */
uint8_t massert_history_count(const massert_t *ma);

/**
 * Get panic history entry (0 = most recent).
 * @return Pointer to entry, or NULL if offset >= count.
 */
const massert_info_t *massert_history_at(const massert_t *ma, uint8_t offset);
#endif

/* ── Convenience macros ────────────────────────────────────────────────── */

#if MASSERT_ENABLE_LOCATION
#define MASSERT_LOC_ARGS  __FILE__, (uint32_t)__LINE__, __func__
#else
#define MASSERT_LOC_ARGS  NULL, 0, NULL
#endif

/**
 * MASSERT(expr) — assert that expr is true. FATAL if false.
 *
 * Usage: MASSERT(ptr != NULL);
 */
#define MASSERT(expr)                                                     \
    do {                                                                   \
        if (!(expr)) {                                                     \
            massert_fire(NULL, MASSERT_FATAL, MASSERT_LOC_ARGS,            \
                         #expr, 0, "assertion failed: %s", #expr);         \
        }                                                                  \
    } while (0)

/**
 * MASSERT_MSG(expr, fmt, ...) — assert with a custom message.
 *
 * Usage: MASSERT_MSG(len > 0, "invalid length: %d", len);
 */
#define MASSERT_MSG(expr, fmt, ...)                                       \
    do {                                                                   \
        if (!(expr)) {                                                     \
            massert_fire(NULL, MASSERT_FATAL, MASSERT_LOC_ARGS,            \
                         #expr, 0, fmt, ##__VA_ARGS__);                    \
        }                                                                  \
    } while (0)

/**
 * MASSERT_WARN(expr, fmt, ...) — soft assert: log + continue.
 */
#define MASSERT_WARN(expr, fmt, ...)                                      \
    do {                                                                   \
        if (!(expr)) {                                                     \
            massert_fire(NULL, MASSERT_WARN, MASSERT_LOC_ARGS,             \
                         #expr, 0, fmt, ##__VA_ARGS__);                    \
        }                                                                  \
    } while (0)

/**
 * MPANIC(fmt, ...) — unconditional fatal panic.
 *
 * Usage: MPANIC("out of memory: needed %d bytes", size);
 */
#define MPANIC(fmt, ...)                                                  \
    massert_fire(NULL, MASSERT_FATAL, MASSERT_LOC_ARGS,                    \
                 NULL, 0, fmt, ##__VA_ARGS__)

/**
 * MPANIC_CODE(code, fmt, ...) — panic with application error code.
 */
#define MPANIC_CODE(code, fmt, ...)                                       \
    massert_fire(NULL, MASSERT_FATAL, MASSERT_LOC_ARGS,                    \
                 NULL, (code), fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* MASSERT_H */
