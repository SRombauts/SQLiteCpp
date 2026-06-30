# Phase 1b — Verification (build / test / sanitizers)

Run on the review host: Windows 11, CMake 3.29.2, Ninja 1.12, g++ (MinGW UCRT) 13.2.0, clang 22.1.0.
Bundled SQLite amalgamation (`SQLITECPP_INTERNAL_SQLITE=ON`) and the in-tree googletest submodule were used.

| Check | Tool / flags | Result | Notes |
|-------|--------------|--------|-------|
| Strict-warning build | g++ 13.2, project flags `-Wall -Wextra -Wpedantic -Wswitch-enum -Wshadow -Wno-long-long`, Debug | **PASS — 0 warnings** | Library, tests and example all compiled clean. |
| Unit tests + example | `ctest` (UnitTests = ~60 GoogleTest cases, Example1Run) | **PASS — 2/2, 0 failures** | `100% tests passed`. |
| Extra conversion warnings | g++ 13.2, library only, added `-Wconversion -Wsign-conversion -Wold-style-cast -Wshadow` | **1 warning** | `src/Column.cpp:101` `-Wsign-conversion` (int→`size_type`). Corroborates **COL-001**. No `-Wold-style-cast` hits in the library. |
| Clang compile (C++11) | clang 22, Debug | **compiles** | Surfaced two portability signals: tests use C++17 `[[maybe_unused]]` under a C++11 target (`-Wc++17-attribute-extensions`, see **TST-001**); `examples/example1` uses deprecated `fopen` under MSVC headers (relates to **EXM-001/002**). |
| ASan + UBSan | clang 22, `-fsanitize=address,undefined`, Debug | **COULD NOT RUN TO COMPLETION** | Build succeeded. The instrumented binary aborts at **static-init time, before any test runs**, with an AddressSanitizer `bad-free`/wild-pointer whose entire stack is in `ucrtbased.dll` + `MSVCP140D.dll` + `ntdll.dll` — **zero SQLiteCpp frames**. This is the documented incompatibility between clang's ASan allocator interceptors and the MSVC **debug** CRT (`/MDd`), not a library defect. **No usable ASan signal obtained on Windows.** |
| UBSan (runtime errors) | as above | **No `runtime error:` reports** | UBSan emitted nothing before the CRT-level abort; this is weak evidence (tests did not execute), not a clean pass. |
| Static analysis (cppcheck / cpplint) | available in CMake (`SQLITECPP_RUN_CPPCHECK`, `SQLITECPP_RUN_CPPLINT`) | **NOT RUN** | Not executed this pass; CI also disables them in the main matrix. Candidate for a follow-up run. |
| Coverage (gcov/lcov) | `SQLITECPP_USE_GCOV` exists; lcov is a Linux-only CI job | **NOT RUN** here | Coverage is collected only in the Ubuntu `coverage.yml` job. |

## Honest summary

- The **primary checks are green**: a strict-warning g++ build and the full GoogleTest suite + example pass with zero warnings and zero failures.
- The library is notably clean under extra conversion/cast warnings: a **single** `-Wsign-conversion` hit (`Column.cpp:101`).
- **Sanitizers could not be run to completion in this Windows environment** because clang's ASan runtime is incompatible with the MSVC debug CRT and aborts during C-runtime static initialization. This is a tooling limitation, **not** evidence of a clean or a dirty result. **Recommendation:** run `-fsanitize=address,undefined` on Linux (GCC or clang) — the project already builds there in CI — to obtain a real sanitizer verdict before closing memory-safety findings (notably DB-001/ST-001 lifetime issues).
- cppcheck/cpplint and coverage were not run this pass and are flagged as follow-ups.
