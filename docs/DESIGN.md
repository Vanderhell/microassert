# Design Rationale

## 1. Caller-Owned Storage
The library does not allocate its own hook or history storage. The caller decides
whether the instance is global, static, stack-based, or embedded in another
object.

## 2. Retained History Is Copied, Not Borrowed
Event data and retained history are separate. The live dispatch path can use
temporary formatting buffers, while the history buffer stores bounded copies.

## 3. Four Severities
`WARN` and `ERROR` continue after hooks run. `FATAL` requests reset, then halts
if reset returns. `HALT` requests halt immediately. This is more precise than a
binary assert-or-not model.

## 4. Deterministic Nested Panic Handling
If a hook fires another `WARN` or `ERROR`, the nested event is counted and
suppressed. If a hook fires `FATAL` or `HALT`, the terminal halt path is taken
immediately.

## 5. Explicit Locations
File, line, and function strings improve diagnostics. They can be disabled for
size-constrained builds through `MASSERT_ENABLE_LOCATION`.

| Decision | Gain | Cost |
|----------|------|------|
| Caller-owned storage | No hidden allocation | More setup |
| Copied history | Stable retained records | Extra storage |
| Four severities | Better control flow | One more concept |
| Nested suppression | No recursive panic loop | Nested soft events are dropped |
| Location capture | Better diagnostics | More ROM |
