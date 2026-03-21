# Design Rationale

## 1. Hook chain instead of a single callback
Multiple hooks fire in order. This lets you compose: log first, then dump, then flush. Each hook is independent and optional.

## 2. Four severities, not two
WARN = soft assert (log + continue). ERROR = serious but recoverable. FATAL = reset. HALT = infinite loop for debugger. This matches real-world needs better than just "assert or not."

## 3. Re-entrancy guard
If a hook (e.g., nvlog_append) triggers another panic, the nested panic is suppressed. Without this, you get infinite recursion → stack overflow → real crash with no diagnostics.

## 4. Global singleton + instances
Macros use the global for zero-config convenience. Separate instances for testing or library isolation.

## 5. Location as compile-time option
file/line/func strings consume ROM. On tiny MCUs, disable with MASSERT_ENABLE_LOCATION=0.

| Decision | Gains | Costs |
|----------|-------|-------|
| Hook chain | Composable, ordered | Max 4 hooks |
| Four severities | Granular response | One more enum to learn |
| Re-entrancy guard | No infinite recursion | Nested panics silently dropped |
| Location capture | Full diagnostics | ROM for string literals |
