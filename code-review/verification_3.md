# Verification 3 — Linux build, tests, sanitizers (3rd-pass review, Phase 1b)

Date: 2026-07-02 | SQLiteCpp 3.3.3 (bundled SQLite 3.53.2) | Linux (Ubuntu 22.04 container), gcc/g++ 11.4.0, x86_64, 2 cores.
Previous passes ran on Windows where ASan could not run; this pass adds sanitizer coverage on Linux.

## Environment caveats (read first)

- **CMake was NOT available** in the Linux workspace, and the network proxy returned `403 Forbidden`
  for pip, apt, and github.com, so it could not be installed. The documented CMake configure and
  `ctest` therefore **did not run**.
- Substitute: a hand-written Makefile replicating the CMake build — bundled `sqlite3/sqlite3.c` +
  `src/*.cpp` (`-std=c++11`, matching the CMake default), vendored googletest 1.16 submodule
  (`-std=c++14`, its minimum), all 9 `tests/*.cpp`, and `examples/example1`, with
  `-DSQLITE_ENABLE_COLUMN_METADATA` (the CMake default ON). Built out-of-tree in /tmp; repo untouched.
- Instead of ctest, the test binary (the exact thing ctest invokes) was run directly.
- **cppcheck and clang-tidy are not installed** and could not be installed (same proxy block).
  Static analysis beyond compiler warnings **did not run**.

## Results summary

| Check | Result |
|---|---|
| Debug build, gcc 11 (`-Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum`) | PASS, builds clean |
| Wrapper warnings (src/ + include/, 7 TUs) | **0** |
| Example1 warnings | **0** |
| Test-code warnings (9 TUs) | **1** (see below) |
| Bundled sqlite3.c warnings (`-Wall -Wextra`) | **0** |
| Unit tests (normal build) | **60/60 passed** (10 suites, 190 ms) |
| Unit tests under ASan+UBSan+LSan | **60/60 passed, zero sanitizer reports, zero leaks** |
| examples/example1 under ASan+UBSan+LSan | PASS, clean, exit 0 |
| gcov line coverage of src/*.cpp from unit tests | 99.3–100% per file (below) |
| ctest | did not run (no CMake) |
| cppcheck | did not run (not installed, proxy blocked) |

## Warning detail (the single finding)

`tests/Database_test.cpp:551` — `-Wsign-compare` through gtest's `CmpHelperEQ`:
```
EXPECT_EQ(h.userVersion, 12345);   // h.userVersion is unsigned int, literal is int
```
Test-code-only, harmless (positive literal). Fix: `12345u`. The neighbouring
`EXPECT_EQ(h.applicationId, 2468)` at :552 compiles without warning only because
gcc folds it; using unsigned literals on both lines is the tidy fix.

## Sanitizer runs (the key new evidence)

Build: fresh dir, `-fsanitize=address,undefined -fno-sanitize-recover=all -g -O0` applied uniformly
to sqlite3.c (gcc), all C++ TUs, and the link line (per instructions, the repo's broken
`SQLITECPP_USE_ASAN` option was not used). Run with `ASAN_OPTIONS=detect_leaks=1
UBSAN_OPTIONS=print_stacktrace=1`.

- `SQLiteCpp_tests`: all 60 tests pass in 288 ms, **no ASan, UBSan, or LeakSanitizer output of any
  kind** (verified by grepping the captured log for `runtime error` / `AddressSanitizer` /
  `SUMMARY`: 0 hits), process exit code 0.
- `SQLiteCpp_example1` (run from its source dir so `logo.png` is found): completes
  ("everything ok, quitting"), no sanitizer output, exit code 0.

Since `-fno-sanitize-recover=all` aborts on the first report, the clean exits are strong evidence:
no heap errors, no UB (signed overflow, misaligned/invalid loads, null derefs, invalid enum/bool
loads, etc.), and no leaks on any path the test suite exercises — including the wrapper's
error/exception paths, which the suite covers heavily (see coverage).

## Test coverage of the wrapper (gcov, `--coverage` build, unit tests only)

| File | Lines executed |
|---|---|
| src/Backup.cpp | 100.00% of 26 |
| src/Column.cpp | 100.00% of 36 |
| src/Database.cpp | 99.28% of 138 |
| src/Exception.cpp | 100.00% of 14 |
| src/Savepoint.cpp | 100.00% of 28 |
| src/Statement.cpp | 99.41% of 170 |
| src/Transaction.cpp | 100.00% of 37 |

Near-total line coverage of src/; the sanitizer-clean result above therefore applies to essentially
the whole compiled wrapper, not just a happy path. (Header-only code — VariadicBind.h,
ExecuteMany.h, Utils.h — is exercised by its dedicated test suites but not itemised by gcov here.)

## Honest gaps

1. CMake/ctest and the repo's own build scripts were not exercised on Linux (tool unavailable).
2. No cppcheck/clang-tidy (unavailable). Compiler warnings at high levels are the only static
   analysis in this pass; earlier Windows passes covered MSVC /W4 analysis.
3. C++17 configuration (std::filesystem constructor paths) not built here — the c++11 default was
   used; the filesystem overloads compile out and were not tested on Linux.
4. Coverage run used gcov line summaries only; no branch coverage or lcov HTML (kept cheap).

## Verdict

Wrapper code builds warning-free at strict levels on gcc 11, all 60 unit tests pass, and — the
main new result of this pass — the full test suite and example1 run **completely clean under
ASan+UBSan+LeakSanitizer** with ~99.5% line coverage of src/. No new defects found by dynamic
analysis; the only compiler finding is one benign sign-compare warning in test code.
