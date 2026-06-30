# Verification

## Status

This document records verification evidence already available in the repository history and user-provided CI output. It is not a release-readiness claim.

## Verified Baseline

- Audited commit: `0b2feb11a33e3af9335560a9266c7024ee19ff7f`

Locally verified before subsequent fixes:

- direct GCC C99 strict runtime
- direct GCC C11 strict runtime
- GCC ASan
- GCC UBSan
- GCC ASan+UBSan
- direct Clang C99 strict runtime
- direct Clang C11 strict runtime
- Clang ASan
- Clang UBSan
- Clang ASan+UBSan
- CMake GCC configure/build/CTest
- CMake Clang configure/build/CTest
- install and external `find_package()` consumer
- Makefile flag injection dry-run and runtime
- `cppcheck` strict
- ARM core compile-only

At that point, the remaining pending or failing items were:

- `clang-tidy` analyzer subset
- `scan-build` CMake build
- generated ARM header-consumer artifact failure

## Subsequent Fixes After That Audit

Subsequent tracked fixes were made for:

- analyzer helper logging path in `tests/test_all.c`
- Windows MSVC generator and compile-fail harness portability
- ARM CMake cross-compile probe setup
- install-consumer toolchain and sanitizer flag propagation
- README example `cppcheck` portability

These fixes were based on local verification and user-provided CI output after that baseline. They are not recorded here as fully closed until the relevant jobs are rerun.

## Current Pending User Re-Verification

The following items still require user rerun or fresh audit evidence after the latest fixes:

- `clang-tidy` analyzer subset
- `scan-build` CMake build
- Windows MSVC CTest matrix after compile-fail harness fixes
- install-consumer path under sanitizer-enabled CI jobs
- ARM CI job after the toolchain-file change

## Publication Note

The local verification artifacts are not published repository documentation and should not be treated as the public source of record.

For published verification status, rely on:

- documented verified commit references in this file
- committed test and CI configuration in the repository
- GitHub Actions results for rerun jobs on current commits

## Closeout Rule

Do not claim full verification closure unless a fresh user audit or CI run demonstrates that the remaining pending items pass on the repaired commit.
