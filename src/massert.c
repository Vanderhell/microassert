/*
 * microassert - implementation.
 *
 * SPDX-License-Identifier: MIT
 */

#include "massert.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

enum {
    MICROASSERT_DETAIL_STATE_MAGIC = 0x4D415354u,
    MICROASSERT_DETAIL_GLOBAL_HOOK_CAPACITY = 8u,
    MICROASSERT_DETAIL_GLOBAL_HISTORY_CAPACITY = 16u
};

static massert_hook_slot_t g_global_hook_slots[MICROASSERT_DETAIL_GLOBAL_HOOK_CAPACITY];
static massert_history_entry_t g_global_history_entries[MICROASSERT_DETAIL_GLOBAL_HISTORY_CAPACITY];
static massert_t g_global = {
    MICROASSERT_DETAIL_STATE_MAGIC,
    g_global_hook_slots,
    MICROASSERT_DETAIL_GLOBAL_HOOK_CAPACITY,
    0u,
    g_global_history_entries,
    MICROASSERT_DETAIL_GLOBAL_HISTORY_CAPACITY,
    0u,
    0u,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    0u,
    0u,
    0u,
    0u,
    0u,
    0u,
    false
};

static massert_t *massert_resolve(massert_t *ma)
{
    return (ma != NULL) ? ma : &g_global;
}

static const massert_t *massert_resolve_const(const massert_t *ma)
{
    return (ma != NULL) ? ma : &g_global;
}

static bool massert_is_valid_severity(massert_severity_t severity)
{
    return severity == MASSERT_SEVERITY_WARN ||
           severity == MASSERT_SEVERITY_ERROR ||
           severity == MASSERT_SEVERITY_FATAL ||
           severity == MASSERT_SEVERITY_HALT;
}

static bool massert_is_terminal_severity(massert_severity_t severity)
{
    return severity == MASSERT_SEVERITY_FATAL ||
           severity == MASSERT_SEVERITY_HALT;
}

MICROASSERT_DETAIL_NORETURN static void massert_spin_forever(void)
{
    for (;;) {
    }
}

static void massert_saturating_increment(uint32_t *value)
{
    if (*value != UINT32_MAX) {
        (*value)++;
    }
}

static bool massert_copy_text(char *dst, size_t cap, const char *src)
{
    size_t i = 0u;
    bool truncated = false;

    if (cap == 0u) {
        return false;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return false;
    }

    while ((i + 1u) < cap && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    if (src[i] != '\0') {
        truncated = true;
        while (src[i] != '\0') {
            i++;
        }
    }

    dst[i < cap ? i : (cap - 1u)] = '\0';
    return truncated;
}

static MICROASSERT_DETAIL_PRINTF_LIKE(3, 0) void massert_store_message(char *dst, size_t cap, const char *fmt,
                                  va_list args, bool *format_failed,
                                  bool *truncated)
{
    int written;

    *format_failed = false;
    *truncated = false;

    if (cap == 0u) {
        return;
    }

    if (fmt == NULL) {
        dst[0] = '\0';
        return;
    }

    written = vsnprintf(dst, cap, fmt, args);
    if (written < 0) {
        *format_failed = true;
        dst[0] = '\0';
        return;
    }

    if ((size_t)written >= cap) {
        *truncated = true;
    }
}

static void massert_copy_event_to_history(const massert_info_t *info,
                                          massert_history_entry_t *entry)
{
    entry->severity = info->severity;
    entry->timestamp_ms = info->timestamp_ms;
    entry->line = info->line;
    entry->error_code = info->error_code;
    entry->file_truncated = massert_copy_text(entry->file, sizeof(entry->file), info->file);
    entry->func_truncated = massert_copy_text(entry->func, sizeof(entry->func), info->func);
    entry->expr_truncated = massert_copy_text(entry->expr, sizeof(entry->expr), info->expr);
    entry->message_truncated = massert_copy_text(entry->msg, sizeof(entry->msg), info->msg);
    entry->format_failed = info->format_failed;
}

static void massert_record_history(massert_t *ma, const massert_info_t *info)
{
    if (ma->history_capacity == 0u) {
        return;
    }

    massert_copy_event_to_history(info, &ma->history_entries[ma->history_head]);
    ma->history_head = (ma->history_head + 1u) % ma->history_capacity;
    if (ma->history_count < ma->history_capacity) {
        ma->history_count++;
    }
}

static void massert_clear_config_storage(const massert_config_t *config)
{
    if (config->hook_slots != NULL && config->hook_capacity > 0u) {
        memset(config->hook_slots, 0, config->hook_capacity * sizeof(*config->hook_slots));
    }
    if (config->history_entries != NULL && config->history_capacity > 0u) {
        memset(config->history_entries, 0,
               config->history_capacity * sizeof(*config->history_entries));
    }
}

static bool massert_config_is_valid(const massert_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (config->hook_capacity > 0u && config->hook_slots == NULL) {
        return false;
    }
    if (config->history_capacity > 0u && config->history_entries == NULL) {
        return false;
    }

    return true;
}

static void massert_assign_config(massert_t *ma, const massert_config_t *config)
{
    ma->magic = MICROASSERT_DETAIL_STATE_MAGIC;
    ma->hook_slots = config->hook_slots;
    ma->hook_capacity = config->hook_capacity;
    ma->hook_count = 0u;

    ma->history_entries = config->history_entries;
    ma->history_capacity = config->history_capacity;
    ma->history_head = 0u;
    ma->history_count = 0u;

    ma->clock_fn = config->clock_fn;
    ma->reset_fn = config->reset_fn;
    ma->reset_ctx = config->reset_ctx;
    ma->halt_fn = config->halt_fn;
    ma->halt_ctx = config->halt_ctx;

    ma->total_count = 0u;
    ma->warn_count = 0u;
    ma->error_count = 0u;
    ma->fatal_count = 0u;
    ma->halt_count = 0u;
    ma->nested_suppressed_count = 0u;
    ma->in_panic = false;
}

static void massert_emergency_halt(massert_t *ma)
{
    if (ma->halt_fn != NULL) {
        ma->halt_fn(ma->halt_ctx);
    }
    massert_spin_forever();
}

static void massert_terminal_fatal(massert_t *ma)
{
    if (ma->reset_fn != NULL) {
        ma->reset_fn(ma->reset_ctx);
    }
    if (ma->halt_fn != NULL) {
        ma->halt_fn(ma->halt_ctx);
    }
    massert_spin_forever();
}

static MICROASSERT_DETAIL_PRINTF_LIKE(8, 0) void massert_dispatch(massert_t *ma, massert_severity_t severity,
                             const char *file, uint32_t line, const char *func,
                             const char *expr, uint32_t code,
                             const char *fmt, va_list args)
{
    massert_info_t info;
    size_t hook_count;
    size_t i;

    if (!massert_is_valid_severity(severity)) {
        ma->in_panic = true;
        massert_emergency_halt(ma);
    }

    if (ma->in_panic) {
        massert_saturating_increment(&ma->nested_suppressed_count);
        if (massert_is_terminal_severity(severity)) {
            massert_emergency_halt(ma);
        }
        return;
    }

    ma->in_panic = true;

    memset(&info, 0, sizeof(info));
    info.severity = severity;
    info.timestamp_ms = (ma->clock_fn != NULL) ? ma->clock_fn() : 0u;
    info.file = file;
    info.line = line;
    info.func = func;
    info.expr = expr;
    info.error_code = code;

    if (fmt != NULL) {
        va_list copy;
        va_copy(copy, args);
        massert_store_message(info.msg, sizeof(info.msg), fmt, copy,
                              &info.format_failed, &info.message_truncated);
        va_end(copy);
    } else {
        info.msg[0] = '\0';
    }

    massert_saturating_increment(&ma->total_count);
    switch (severity) {
    case MASSERT_SEVERITY_WARN:
        massert_saturating_increment(&ma->warn_count);
        break;
    case MASSERT_SEVERITY_ERROR:
        massert_saturating_increment(&ma->error_count);
        break;
    case MASSERT_SEVERITY_FATAL:
        massert_saturating_increment(&ma->fatal_count);
        break;
    case MASSERT_SEVERITY_HALT:
        massert_saturating_increment(&ma->halt_count);
        break;
    }

    massert_record_history(ma, &info);

    hook_count = ma->hook_count;
    for (i = 0u; i < hook_count; i++) {
        massert_hook_fn hook = ma->hook_slots[i].hook;
        if (hook != NULL) {
            hook(&info, ma->hook_slots[i].ctx);
        }
    }

    if (severity == MASSERT_SEVERITY_WARN || severity == MASSERT_SEVERITY_ERROR) {
        ma->in_panic = false;
        return;
    }

    if (severity == MASSERT_SEVERITY_FATAL) {
        massert_terminal_fatal(ma);
    } else {
        massert_emergency_halt(ma);
    }
}

const char *massert_severity_str(massert_severity_t severity)
{
    switch (severity) {
    case MASSERT_SEVERITY_WARN:
        return "WARN";
    case MASSERT_SEVERITY_ERROR:
        return "ERROR";
    case MASSERT_SEVERITY_FATAL:
        return "FATAL";
    case MASSERT_SEVERITY_HALT:
        return "HALT";
    default:
        return "INVALID";
    }
}

massert_t *massert_global(void)
{
    return &g_global;
}

massert_status_t massert_init(massert_t *ma, const massert_config_t *config)
{
    massert_t *target = massert_resolve(ma);
    bool was_initialized = target->magic == MICROASSERT_DETAIL_STATE_MAGIC;

    if (!massert_config_is_valid(config)) {
        return MASSERT_STATUS_INVALID_ARGUMENT;
    }
    if (was_initialized && target->in_panic) {
        return MASSERT_STATUS_BUSY;
    }

    massert_clear_config_storage(config);
    massert_assign_config(target, config);

    return MASSERT_STATUS_OK;
}

massert_status_t massert_set_reset(massert_t *ma, massert_reset_fn fn, void *ctx)
{
    massert_t *target = massert_resolve(ma);

    if (target->in_panic) {
        return MASSERT_STATUS_BUSY;
    }
    if (target->magic != MICROASSERT_DETAIL_STATE_MAGIC) {
        return MASSERT_STATUS_INVALID_STATE;
    }

    target->reset_fn = fn;
    target->reset_ctx = ctx;
    return MASSERT_STATUS_OK;
}

massert_status_t massert_set_halt(massert_t *ma, massert_halt_fn fn, void *ctx)
{
    massert_t *target = massert_resolve(ma);

    if (target->in_panic) {
        return MASSERT_STATUS_BUSY;
    }
    if (target->magic != MICROASSERT_DETAIL_STATE_MAGIC) {
        return MASSERT_STATUS_INVALID_STATE;
    }

    target->halt_fn = fn;
    target->halt_ctx = ctx;
    return MASSERT_STATUS_OK;
}

massert_status_t massert_add_hook(massert_t *ma, massert_hook_fn fn, void *ctx, size_t *slot_index)
{
    massert_t *target = massert_resolve(ma);
    size_t index;

    if (fn == NULL) {
        return MASSERT_STATUS_INVALID_ARGUMENT;
    }
    if (target->in_panic) {
        return MASSERT_STATUS_BUSY;
    }
    if (target->magic != MICROASSERT_DETAIL_STATE_MAGIC) {
        return MASSERT_STATUS_INVALID_STATE;
    }
    if (target->hook_capacity == 0u || target->hook_slots == NULL) {
        return MASSERT_STATUS_INVALID_STATE;
    }
    if (target->hook_count >= target->hook_capacity) {
        return MASSERT_STATUS_FULL;
    }

    index = target->hook_count;
    target->hook_slots[index].hook = fn;
    target->hook_slots[index].ctx = ctx;
    target->hook_count++;

    if (slot_index != NULL) {
        *slot_index = index;
    }

    return MASSERT_STATUS_OK;
}

massert_status_t massert_clear_hooks(massert_t *ma)
{
    massert_t *target = massert_resolve(ma);

    if (target->in_panic) {
        return MASSERT_STATUS_BUSY;
    }
    if (target->magic != MICROASSERT_DETAIL_STATE_MAGIC) {
        return MASSERT_STATUS_INVALID_STATE;
    }
    if (target->hook_slots != NULL && target->hook_capacity > 0u) {
        memset(target->hook_slots, 0, target->hook_capacity * sizeof(*target->hook_slots));
    }
    target->hook_count = 0u;
    return MASSERT_STATUS_OK;
}

void massert_fire(massert_t *ma, massert_severity_t severity,
                  const char *file, uint32_t line, const char *func,
                  const char *expr, uint32_t code,
                  const char *fmt, ...)
{
    massert_t *target = massert_resolve(ma);
    va_list args;

    va_start(args, fmt);
    massert_dispatch(target, severity, file, line, func, expr, code, fmt, args);
    va_end(args);
}

MICROASSERT_DETAIL_NORETURN void massert_panic_fatal(
    massert_t *ma, const char *file, uint32_t line, const char *func,
    const char *expr, uint32_t code,
    const char *fmt, ...)
{
    massert_t *target = massert_resolve(ma);
    va_list args;

    va_start(args, fmt);
    massert_dispatch(target, MASSERT_SEVERITY_FATAL, file, line, func, expr, code, fmt, args);
    va_end(args);
    massert_spin_forever();
}

MICROASSERT_DETAIL_NORETURN void massert_panic_halt(
    massert_t *ma, const char *file, uint32_t line, const char *func,
    const char *expr, uint32_t code,
    const char *fmt, ...)
{
    massert_t *target = massert_resolve(ma);
    va_list args;

    va_start(args, fmt);
    massert_dispatch(target, MASSERT_SEVERITY_HALT, file, line, func, expr, code, fmt, args);
    va_end(args);
    massert_spin_forever();
}

uint32_t massert_total_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->total_count;
}

uint32_t massert_warn_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->warn_count;
}

uint32_t massert_error_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->error_count;
}

uint32_t massert_fatal_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->fatal_count;
}

uint32_t massert_halt_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->halt_count;
}

uint32_t massert_nested_suppressed_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->nested_suppressed_count;
}

size_t massert_history_count(const massert_t *ma)
{
    return massert_resolve_const(ma)->history_count;
}

const massert_history_entry_t *massert_history_at(const massert_t *ma, size_t offset)
{
    const massert_t *target = massert_resolve_const(ma);
    size_t index;

    if (offset >= target->history_count || target->history_capacity == 0u) {
        return NULL;
    }

    index = target->history_head;
    if (index == 0u) {
        index = target->history_capacity;
    }
    index = (index + target->history_capacity - 1u - offset) % target->history_capacity;

    return &target->history_entries[index];
}
