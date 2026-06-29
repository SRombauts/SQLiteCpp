# Verification Pass

_Run 2026-06-26 on Windows 11, toolchains: CMake 3.29.2, MinGW GCC + Ninja (Strawberry), Clang/LLVM 22, cppcheck 2.14.0 (Strawberry), Python 3.14._

## Build + tests (baseline)

| Step | Toolchain | Result |
|------|-----------|--------|
| Configure + build lib + tests + examples | GCC + Ninja, Debug | **OK (exit 0)** |
| Compiler warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum`) | GCC | **0 warnings** (wrapper and bundled sqlite3) |
| `ctest` | GCC build | **100% passed** — `UnitTests` (the GoogleTest suite) + `Example1Run` |

The wrapper compiles warning-clean under the project's full warning set and all tests pass.

## Static analysis

| Tool | Result |
|------|--------|
| **cpplint** (`--linelength=120`) | **61 findings, all style-level**: 18 CRLF line-endings (`whitespace/newline`), 14 `#pragma once`-vs-`#ifndef` guard mismatches (project deliberately uses `#pragma once`), 14 `runtime/int` (`long` usage in `Database.h` Header struct — see DB-03), 7 IWYU (`build/include_what_you_use`), 2 include-order, plus minor whitespace. No correctness findings. |
| **cppcheck** | **Could not run.** The Strawberry-bundled cppcheck 2.14.0 has a broken `FILESDIR` (compiled to point at a non-existent `R:/winlibs64ucrt_stage/...` path), so it cannot load `std.cfg` regardless of cwd or flags. Documented as a host-tooling limitation, not a project defect. |

## AddressSanitizer — attempted, not feasible on this host

Two approaches were attempted; both failed for toolchain reasons (not project bugs):

1. **Repo `SQLITECPP_USE_ASAN=ON` (Clang):** the option applies `-fsanitize=address` only to the `SQLiteCpp` library target (not tests/sqlite3). Linking the partially-instrumented objects against the MSVC STL fails:
   `lld-link: error: /failifmismatch: mismatch detected for 'annotate_string'` (instrumented lib obj vs non-instrumented test obj). This is itself a finding about the build config (the ASAN option produces an unlinkable mix under modern Clang/MSVC-STL).
2. **Uniform ASAN on all TUs** (`-fsanitize=address` via global `CMAKE_CXX/C_FLAGS`): fails compiling the vendored `sqlite3/sqlite3.c` under Clang+ASAN.

The repo's ASAN linker flag also hardcodes `-fuse-ld=gold` for GCC, which does not exist on Windows MinGW. **Conclusion:** ASAN was not run; dynamic signal comes from the clean warning-free build and the passing test suite. Note the higher-severity UB findings (null-`char*`→`std::string`, signed-shift) live on OOM / size-limit / malformed-input paths that the existing happy-path tests do not exercise, so ASAN over the current tests would likely not have surfaced them regardless.

## Reviewer leverage notes

- A CI gap worth flagging: GitHub Actions runs neither ASAN, Valgrind, coverage, nor static analysis (cpplint/cppcheck are disabled in the CMake workflows). The heavier gates exist only in the legacy Travis config.
- The repo's `SQLITECPP_USE_ASAN` option needs rework to be usable on Clang/Windows (instrument all targets, drop the unconditional gold linker).
