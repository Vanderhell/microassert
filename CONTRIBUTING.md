# Contributing

## Project Rules

- C99 is the minimum language level.
- The implementation must not require heap allocation.
- The implementation must not require an OS or RTOS.
- Do not introduce hidden global mutable state beyond the defined global singleton contract.
- Do not weaken tests to make failures disappear.
- Do not remove compile-fail tests.
- Do not create a tag or release unless explicitly requested.

Out of scope:

- stack unwinding guarantees
- signal handling guarantees
- dynamic allocation requirements
- thread-safety claims
- hardware fault replacement claims

## Workflow

1. Open an issue or document the problem clearly in the PR.
2. Make the smallest defensible change.
3. Keep docs, tests, and compile-fail coverage aligned with behavior changes.
4. Preserve caller-controlled flags and caller-owned storage patterns.
5. Submit the PR with exact verification evidence for the changed paths.

## Documentation

If behavior, toolchain support, or verification expectations change, update:

- `README.md`
- `docs/COOKBOOK.md`
- `docs/TROUBLESHOOTING.md`
- `docs/VERIFICATION.md`
- `CHANGELOG.md`

By contributing, you agree to the MIT License.
