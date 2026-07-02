# Build / CI / Examples / Tests — Findings, 3rd independent review pass

_Date: 2026-07-02. Scope: CMakeLists.txt, sqlite3/CMakeLists.txt, cmake/, meson.build, .github/workflows/, appveyor.yml, package.xml, examples/, tests/ (hygiene/portability). Reviewed directly against the current tree._

## 1. Fix verification

| ID | Claimed fix | Verified in current code |
|----|-------------|--------------------------|
| BLD-001 | #560 / a6537fe | **Fixed.** `meson.build:132` now appends `-DSQLITECPP_DISABLE_STD_FILESYSTEM` to `sqlitecpp_args` (the undeclared `sqlitecpp_cxx_flags` is gone; the whole file consistently uses `sqlitecpp_args`). |

## 2. Status of still-open items

- **BLD-002** — Still valid: googletest submodule unchanged (stale 2018 pin vs Meson `gtest.wrap`).
- **BLD-003** — Still valid: all workflows use mutable tags (`actions/checkout@v4` in cmake.yml:56 etc.); no `permissions:` block in any of the 6 workflows.
- **BLD-004** — Still valid: `.travis.yml` gone from root? No — `appveyor.yml` still present with retired images; Travis file removed earlier but token rotation not evidenced. Treat as open until confirmed.
- **BLD-005** — Still valid, and broader than recorded: **none** of the 6 workflows (cmake.yml, cmake_builtin_lib.yml, cmake_subdir_example.yml, coverage.yml, coverity.yml, meson.yml) has a `permissions:` block.
- **BLD-006** — Still valid: `SQLITECPP_USE_ASAN` unchanged; no sanitizer CI job. (Note: 3rd-pass verification ran ASan/UBSan manually on Linux — clean; see `verification_3.md`.)
- **BLD-007 / BLD-008** — Still valid: `examples/example2/CMakeLists.txt` `CACHE BOOL "" FORCE` unchanged; `sqlite3/CMakeLists.txt:16` still uses directory-global `add_definitions("-DSQLITE_API=__declspec(dllexport)")` (export-only).
- **EXM-001 / EXM-002 / EXM-003** — Still valid: no commits touched `examples/` since the 2nd pass (`git diff 5fa55c6..HEAD -- examples` is empty).
- **TST-001** — Still valid: tests still use `[[maybe_unused]]` while the CMake default is C++11 (`CMakeLists.txt:12-13`).

## 3. NEW findings

| Done | ID | Severity | Confidence | Category | Location | Finding / Impact | Concrete fix |
|:--:|----|----------|------------|----------|----------|------------------|--------------|
| [ ] | BLD-009 | Low | High | build / packaging | `sqlite3/CMakeLists.txt:72-79` | `install(TARGETS sqlite3 ...)` + `install(FILES sqlite3.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})` run **unconditionally** whenever the internal SQLite is built — they ignore `SQLITECPP_INSTALL=OFF`, and installing a bare `sqlite3.h` into the include root can shadow or clash with a system SQLite of a different version for every other package on the prefix. | Gate the sqlite3 install rules on `SQLITECPP_INSTALL`, and consider installing the header under a subdirectory (or not at all, since the wrapper's public headers only need the sqlite3 target at link time when `SQLITE_HAS_CODEC`/legacy-struct options are off). |
| [ ] | BLD-010 | Low | High | build / packaging | `CMakeLists.txt:388`, `cmake/SQLiteCppConfig.cmake.in:13` | `install(EXPORT SQLiteCppTargets ...)` has **no `NAMESPACE`**, so consumers of `find_package(SQLiteCpp)` get bare global target names `SQLiteCpp` and `sqlite3` instead of the conventional `SQLiteCpp::SQLiteCpp`. A consumer that also defines/imports a target named `sqlite3` (very common) collides. | Add `NAMESPACE SQLiteCpp::` to the `install(EXPORT)` and (for compatibility) an `add_library(SQLiteCpp ALIAS ...)` shim note in the Config file; document the migration. Breaking for existing consumers — flag in CHANGELOG. |
| [ ] | BLD-011 | Low | Medium | build / ABI | `CMakeLists.txt:538` | `SOVERSION` is hardcoded `0` with no ABI policy while the 3.x series changes public headers (e.g. `Header` struct fields in #558 changed layout/types). Shared-library consumers on Linux get silent ABI drift under an unchanged soname `libSQLiteCpp.so.0`. | Either bump SOVERSION on ABI-breaking releases (document the policy) or set it from `${PROJECT_VERSION_MAJOR}`. |
| [ ] | BLD-012 | Info | High | build / consistency | `meson.build:4-7` vs `CMakeLists.txt:12-16` | The two build systems compile different language modes by default: Meson forces `cpp_std=c++17, warning_level=3`; CMake defaults to C++11 with a hand-rolled warning list. CI therefore never builds the C++11 mode with Meson nor C++17 with the default CMake flags — masking standard-dependent issues (TST-001 is invisible to the Meson CI). | Align the defaults, or add one CI job per standard per build system (a small matrix axis). |
| [ ] | TST-002 | Info | Medium | tests / hygiene | `tests/*.cpp` (e.g. `Database_test.cpp:50,79,91`; 44 uses of `"test.db3"`, plus `backup_test.db3`, `short.db3`, ...) | All suites hardcode relative db filenames in the CWD and delete them with `remove()` on the success path only. A mid-test failure/abort leaves stale db files that can poison subsequent runs (tests that assume absence/CREATE), and two test processes cannot run from the same directory. | Use per-test unique names (gtest `UnitTest::GetInstance()->current_test_info()`) or a RAII temp-file fixture that removes in `TearDown()`; at minimum `remove()` in SetUp too. |
| [ ] | TST-003 | Info | High | tests / warnings | `tests/Database_test.cpp:551` | `EXPECT_EQ(h.userVersion, 12345)` compares unsigned field vs signed literal → `-Wsign-compare`, the **only** warning in an otherwise clean `-Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum` build of lib+tests+examples (see `verification_3.md`). Breaks `-Werror` test builds. | `12345u` (and matching literal for the field's fixed-width type). |

## 4. Verified non-issues

- **Internal-sqlite export is complete** (looked wrong, is correct): `SQLiteCpp` links `PUBLIC SQLite::SQLite3` (alias of internal `sqlite3`), and `sqlite3/CMakeLists.txt:73` adds `sqlite3` to the *same* `SQLiteCppTargets` export set, so `install(EXPORT)` generates without error and consumers resolve the dependency; `SQLiteCppConfig.cmake.in` correctly `find_dependency(SQLite3)` only when `NOT SQLITECPP_INTERNAL_SQLITE`, plus `Threads` on UNIX.
- **Version consistency**: CMake `project(SQLiteCpp VERSION 3.3.3)`, `meson.build version: '3.3.3'`, `package.xml <version>3.3.3</version>` all agree (only the `SQLITECPP_VERSION` *string* `"3.03.03"` is non-canonical — already HDR-07).
- **CMake C++ standard guard** is correct: respects a user-provided `CMAKE_CXX_STANDARD`, defaults 11, warns below 11, `CMAKE_CXX_STANDARD_REQUIRED ON`.
- **`SQLITE_HAS_CODEC` + internal SQLite** is a clean `FATAL_ERROR` (CMakeLists.txt:285-287) — the unsupported combination cannot be misconfigured silently.
