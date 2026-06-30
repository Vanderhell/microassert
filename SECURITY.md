# Security Policy

## Reporting

Use GitHub security reporting if available for the repository. If private reporting is unavailable, open an issue only for non-sensitive hardening topics.

Do not publish exploit details for unrepaired defects that could affect downstream users.

## Scope Notes

`microassert` is a small assertion and panic utility. Its contract does not include:

- sandboxing
- memory isolation
- signal safety
- thread safety
- persistence guarantees
- crash recovery guarantees

## Hardening Expectations

- keep the library heap-free
- keep the library OS-independent
- keep public APIs explicit and status-returning where mutation can fail
- preserve deterministic terminal-path behavior
- preserve bounded string handling and truncation reporting
