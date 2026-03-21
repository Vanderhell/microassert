/*
 * microassert — Implementation.
 *
 * SPDX-License-Identifier: MIT
 * https://github.com/Vanderhell/microassert
 */

#include "massert.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ── Strings ───────────────────────────────────────────────────────────── */

const char *massert_severity_str(massert_severity_t sev)
{
    switch (sev) {
    case MASSERT_WARN:  return "WARN";
    case MASSERT_ERROR: return "ERROR";
    case MASSERT_FATAL: return "FATAL";
    case MASSERT_HALT:  return "HALT";
    default:            return "?";
    }
}

/* ── Global singleton ──────────────────────────────────────────────────── */

static massert_t g_massert;
static bool g_initialized = false;

massert_t *massert_global(void)
{
    if (!g_initialized) {
        massert_init(&g_massert, NULL);
        g_initialized = true;
    }
    return &g_massert;
}

/* ── Init ──────────────────────────────────────────────────────────────── */

void massert_init(massert_t *ma, massert_clock_fn clock)
{
    if (ma == NULL) ma = massert_global();
    memset(ma, 0, sizeof(*ma));
    ma->clock = clock;
    if (ma == &g_massert) g_initialized = true;
}

void massert_set_reset(massert_t *ma, massert_reset_fn fn, void *ctx)
{
    if (ma == NULL) ma = massert_global();
    ma->reset_fn  = fn;
    ma->reset_ctx = ctx;
}

int massert_add_hook(massert_t *ma, massert_hook_fn fn, void *ctx)
{
    if (ma == NULL) ma = massert_global();
    if (fn == NULL) return -1;
    if (ma->num_hooks >= MASSERT_MAX_HOOKS) return -1;

    uint8_t idx = ma->num_hooks;
    ma->hooks[idx]    = fn;
    ma->hook_ctx[idx] = ctx;
    ma->num_hooks++;

    return (int)idx;
}

void massert_clear_hooks(massert_t *ma)
{
    if (ma == NULL) ma = massert_global();
    ma->num_hooks = 0;
    memset(ma->hooks, 0, sizeof(ma->hooks));
    memset(ma->hook_ctx, 0, sizeof(ma->hook_ctx));
}

/* ── Core panic ────────────────────────────────────────────────────────── */

void massert_fire(massert_t *ma, massert_severity_t severity,
                   const char *file, uint32_t line, const char *func,
                   const char *expr, uint32_t code,
                   const char *fmt, ...)
{
    if (ma == NULL) ma = massert_global();

    /* Re-entrancy guard: if a hook panics, don't recurse */
    if (ma->in_panic) {
        /* Nested panic — just halt */
        if (severity >= MASSERT_HALT) {
            for (;;) { /* infinite loop */ }
        }
        return;
    }
    ma->in_panic = true;

    /* Build panic info */
    massert_info_t info;
    memset(&info, 0, sizeof(info));

    info.severity     = severity;
    info.timestamp_ms = (ma->clock != NULL) ? ma->clock() : 0;
    info.error_code   = code;
    info.expr         = expr;

#if MASSERT_ENABLE_LOCATION
    info.file = file;
    info.line = line;
    info.func = func;
#else
    (void)file; (void)line; (void)func;
#endif

    /* Format message */
    if (fmt != NULL) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(info.msg, sizeof(info.msg), fmt, args);
        va_end(args);
    }

    /* Update counters */
    ma->panic_count++;
    switch (severity) {
    case MASSERT_WARN:  ma->warn_count++;  break;
    case MASSERT_ERROR: ma->error_count++; break;
    case MASSERT_FATAL: ma->fatal_count++; break;
    case MASSERT_HALT:  ma->fatal_count++; break;
    }

    /* Store in history */
#if MASSERT_ENABLE_HISTORY
    memcpy(&ma->history[ma->hist_head], &info, sizeof(info));
    ma->hist_head = (ma->hist_head + 1) % MASSERT_HISTORY_DEPTH;
    if (ma->hist_count < MASSERT_HISTORY_DEPTH) {
        ma->hist_count++;
    }
#endif

    /* Fire hooks */
    for (uint8_t i = 0; i < ma->num_hooks; i++) {
        if (ma->hooks[i] != NULL) {
            ma->hooks[i](&info, ma->hook_ctx[i]);
        }
    }

    ma->in_panic = false;

    /* Post-hook action based on severity */
    switch (severity) {
    case MASSERT_WARN:
    case MASSERT_ERROR:
        /* Continue execution */
        break;

    case MASSERT_FATAL:
        if (ma->reset_fn != NULL) {
            ma->reset_fn(ma->reset_ctx);
            /* Should not return, but if it does... */
        }
        for (;;) { /* fallback halt */ }
        break; /* unreachable */

    case MASSERT_HALT:
        for (;;) { /* deliberate halt for debugger */ }
        break; /* unreachable */
    }
}

/* ── Query ─────────────────────────────────────────────────────────────── */

uint32_t massert_panic_count(const massert_t *ma)
{
    if (ma == NULL) ma = &g_massert;
    return ma->panic_count;
}

uint32_t massert_warn_count(const massert_t *ma)
{
    if (ma == NULL) ma = &g_massert;
    return ma->warn_count;
}

uint32_t massert_error_count(const massert_t *ma)
{
    if (ma == NULL) ma = &g_massert;
    return ma->error_count;
}

uint32_t massert_fatal_count(const massert_t *ma)
{
    if (ma == NULL) ma = &g_massert;
    return ma->fatal_count;
}

#if MASSERT_ENABLE_HISTORY
uint8_t massert_history_count(const massert_t *ma)
{
    if (ma == NULL) ma = &g_massert;
    return ma->hist_count;
}

const massert_info_t *massert_history_at(const massert_t *ma, uint8_t offset)
{
    if (ma == NULL) ma = &g_massert;
    if (offset >= ma->hist_count) return NULL;

    int idx = (int)ma->hist_head - 1 - (int)offset;
    if (idx < 0) idx += MASSERT_HISTORY_DEPTH;

    return &ma->history[idx];
}
#endif
