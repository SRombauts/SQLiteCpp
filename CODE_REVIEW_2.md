# SQLiteCpp — Deep Code Review

**Date:** 2026-06-30/07-01 · **Branch:** `fix-savepoint-destructor-exception-safety` · **Reviewed version:** 3.3.3
**Scope:** the SQLiteCpp C++ wrapper (`include/SQLiteCpp/`, `src/`), its tests, examples, and build/CI tooling.
**Out of scope (skipped):** `sqlite3/sqlite3.c` + `sqlite3/sqlite3.h` (vendored SQLite 3.53.2 amalgamation), `googletest/` (submodule), `cpplint.py` (vendored tool), and binary assets (`example.db3`, `logo.png`).

This file is the scannable summary. Full per-finding detail lives in [`code-review/`](code-review/):
[triage](code-review/triage.md) · [verification](code-review/verification.md) · [database](code-review/findings-database.md) · [statement](code-review/findings-statement.md) · [column+backup](code-review/findings-column-backup.md) · [transaction+savepoint](code-review/findings-transaction-savepoint.md) · [exception+headers](code-review/findings-exception-headers.md) · [build+examples](code-review/findings-build-examples.md)

Every finding and fix below is **tickable**: flip `[ ]` → `[x]` (optionally annotated, e.g. `[x] #560`) as fixes land.

---

## 0. Inventory & review units

In-scope source: 13 public headers (2,385 LoC), 7 implementation files (1,198 LoC), 9 test files (2,865 LoC), 2 examples (591 LoC), plus build/CI tooling. Logical review units:

| Unit | Files | Test |
|------|-------|------|
| U1 Database | `Database.h` + `src/Database.cpp` | `Database_test.cpp` |
| U2 Statement | `Statement.h` + `src/Statement.cpp` | `Statement_test.cpp` |
| U3 Column | `Column.h` + `src/Column.cpp` | `Column_test.cpp` |
| U4 Backup | `Backup.h` + `src/Backup.cpp` | `Backup_test.cpp` |
| U5 Transaction | `Transaction.h` + `src/Transaction.cpp` | `Transaction_test.cpp` |
| U6 Savepoint | `Savepoint.h` + `src/Savepoint.cpp` | `Savepoint_test.cpp` |
| U7 Exception | `Exception.h` + `src/Exception.cpp` | `Exception_test.cpp` |
| U8 Header-only helpers | `ExecuteMany.h`, `VariadicBind.h`, `Utils.h`, `Assertion.h`, `SQLiteCpp.h`, `SQLiteCppExport.h` | `ExecuteMany_test.cpp`, `VariadicBind_test.cpp` |
| U9 Build/CI tooling | `CMakeLists.txt`, `meson.build`, `meson_options.txt`, `sqlite3/CMakeLists.txt`, `cmake/*`, `.github/workflows/*`, `appveyor.yml`, `.travis.yml`, `build.*`, `package.xml`, `subprojects/*` | — |
| U10 Examples | `examples/example1/main.cpp`, `examples/example2/src/main.cpp` | — |

---

## 1. Triage matrix (full table in [`code-review/triage.md`](code-review/triage.md))

Scores 1–5 (5 = highest/worst). Compact view:

| Unit | Complexity | Blast radius | Bug risk | Security risk | Test gap |
|------|:--:|:--:|:--:|:--:|:--:|
| U2 Statement | 5 | 5 | 4 | 2 | 2 |
| U1 Database | 4 | 5 | 4 | 4 | 2 |
| U6 Savepoint | 3 | 3 | 4 | 2 | 4 |
| U3 Column | 3 | 4 | 3 | 2 | 3 |
| U8 Header helpers | 3 | 4 | 3 | 2 | 3 |
| U4 Backup | 2 | 3 | 2 | 1 | 4 |
| U5 Transaction | 2 | 4 | 2 | 1 | 3 |
| U7 Exception | 1 | 4 | 2 | 1 | 2 |
| U9 Build/CI | 4 | 3 | 2 | 3 | 4 |
| U10 Examples | 2 | 1 | 2 | 1 | 5 |

**Recommended review order:** U2 Statement → U1 Database → U6 Savepoint → U3 Column → U8 helpers → U4 Backup → U5 Transaction → U7 Exception → U9 Build/CI → U10 Examples. *(This repo is small enough that every unit was reviewed in full, not sampled.)*

---

## 2. Verification results (full detail in [`code-review/verification.md`](code-review/verification.md))

| Check | Result |
|-------|--------|
| g++ 13.2 strict-warning build (`-Wall -Wextra -Wpedantic -Wswitch-enum -Wshadow`) | **PASS — 0 warnings** |
| Full test suite + example (`ctest`) | **PASS — 2/2 (≈60 GoogleTest cases), 0 failures** |
| g++ extra warnings (`-Wconversion -Wsign-conversion -Wold-style-cast`, library only) | **1 warning** — `Column.cpp:101` (corroborates COL-001) |
| clang 22 compile (C++11) | compiles; surfaced TST-001 and example `fopen` deprecation |
| clang ASan + UBSan | **COULD NOT RUN TO COMPLETION** — instrumented binary aborts at C-runtime static-init (clang-ASan vs MSVC debug-CRT incompatibility), **no SQLiteCpp frames**. No usable sanitizer signal on Windows. |
| cppcheck / cpplint / coverage | **NOT RUN** this pass (follow-up; coverage is a Linux-only CI job) |

> **Honest caveat:** the primary build + test checks are green and the library is clean under extra conversion/cast warnings. Sanitizers were *attempted but did not execute* on this Windows host — this is a tooling limitation, not a pass. **Run `-fsanitize=address,undefined` on Linux** (the project already builds there) before closing the lifetime findings DB-001/ST-001.

---

## 3. Findings index

42 findings: **1 high, 10 medium, 28 low, 3 info** (plus one reviewer claim, HDR-001, reconciled to a **verified non-issue**). No critical findings.

| Unit | Findings |
|------|----------|
| Database | DB-001 (med), DB-005 (med), DB-004, DB-002, DB-003 (low), DB-006 (info) |
| Statement | ST-001 (med), ST-004, ST-002, ST-003, ST-005 (low), ST-006 (info) |
| Column/Backup | BK-001, COL-001, BK-002 (low), COL-002 (info) |
| Transaction/Savepoint | TX-001 (med), SP-001, TX-002, SP-002, SP-003, SP-004, TX-003 (low) |
| Exception/headers | UTL-001 (med), VB-001 (med), EM-001, HDR-003, VB-002, UTL-002, HDR-002 (low) |
| Build/Examples | BLD-001 (high), EXM-001 (med), EXM-002 (med), BLD-002 (med), BLD-003 (med), BLD-004, BLD-005, BLD-006, EXM-003, BLD-007, BLD-008, TST-001 (low) |

---

## 4. Cross-cutting themes (one fix resolves several)

- **[x] #559 T1 — Throw-from-destructor safety is incomplete.** The branch's own headline fix (Savepoint `catch (...)`, afa51d3) was **not propagated to `Transaction`** (TX-001), which still catches only `SQLite::Exception&` and can `std::terminate` on `bad_alloc`. Audit all RAII destructors for the same pattern. *Resolves TX-001; confirms SP design.*
- **[ ] T2 — `size_t`→`int` narrowing without overflow guards.** ST-004 (bind/bindNoCopy string + prepare), DB-002 (rekey), and the lone `-Wsign-conversion` hit COL-001 (`Column.cpp:101`) share one root. A small checked-narrowing helper covers all. *Resolves ST-004, DB-002, COL-001.*
- **[ ] T3 — Defaulted moves leave inconsistent/dangerous moved-from state.** ST-001 (Statement move-assign copies raw ptr + stale flags) and DB-001 (Database move with live Statements) are the same root cause; both need a user-defined move-assignment / documented-and-enforced precondition. *Resolves ST-001, DB-001.*
- **[ ] T4 — Inconsistent `Exception` construction loses error context.** BK-001 uses `sqlite3_errstr(res)` (no message, no extended code) and ST-002 reports a misleading message; standardize on `Exception(sqlite3*, ret)`. *Resolves BK-001, ST-002.*
- **[ ] T5 — Supply-chain / CI hardening.** Pin Actions to SHAs + add `permissions:` (BLD-003, BLD-005), bump the stale googletest submodule (BLD-002), remove dead Travis/AppVeyor + rotate the embedded token (BLD-004), add a Linux sanitizer job (BLD-006, also closes the verification gap above).
- **[ ] T6 — Examples teach footguns.** EXM-001 (OOB write), EXM-002 (`FILE*` leak), EXM-003 (double-quoted SQL literals) are copy-paste hazards in teaching code.

---

## 5. Ranked fix list

**Effort:** S ≈ <30 min · M ≈ a few hours · L ≈ larger. **Breaking** = source/behavior/test change to flag for the maintainer.

### P0 — Critical
*None.* No critical-severity defects were found.

### P1 — Fix first (high impact, mostly low effort)

| Done | ID | Sev / Conf | Effort | Breaking? | Files | Fix |
|:--:|----|-----------|:--:|----|-------|-----|
| [x] #559 | **TX-001** | med / high | S | Behavior (catches more); add test | `src/Transaction.cpp:58` | Broaden `catch (SQLite::Exception&)` → `catch (...)`, matching the just-landed Savepoint fix. |
| [ ] | **BLD-001** | high / high | S | No | `meson.build:132` | `sqlitecpp_cxx_flags` → `sqlitecpp_args`; Meson build is currently broken for `SQLITECPP_DISABLE_STD_FILESYSTEM`. |
| [ ] | **DB-005** | med / med | M | **Behavior** (Load no longer creates) | `src/Database.cpp:365-379` | Open the `Load` source `OPEN_READONLY`; today a bad source path silently creates an empty DB and **wipes the destination**. Add a test. |
| [ ] | **EXM-001** | med / high | S | No | `examples/example1/main.cpp:415-419` | Cap `fread` at `sizeof(buffer)-1` (or drop the NUL write) — current code can write 1 byte past a stack buffer. |
| [ ] | **EXM-002** | med / high | S | No | `examples/example1/main.cpp:441-462` | RAII the `FILE*` (or unconditional `fclose`) — leaks on the no-row/exception path. |
| [ ] | **ST-001** | med / high | M | **Behavior** (adds move-assign) | `Statement.h:83` | User-defined move-assignment that resets the moved-from `mpSQLite`/`mColumnCount`/`mbHasRow`/`mbDone`. |
| [ ] | **DB-001** | med / high | M | Doc/enforcement | `Database.h:254-255` | Document + enforce "no outstanding statements" on move/close (assert count==0). Add regression test. |

### P2 — Should fix

| Done | ID | Sev / Conf | Effort | Breaking? | Files | Fix |
|:--:|----|-----------|:--:|----|-------|-----|
| [ ] | **VB-001** | med / high | M | **Behavior** (overload selection) | `VariadicBind.h:48,52` | Real forwarding references so rvalues can pick move/`bindNoCopy`; current `std::forward` over `const&` is dead. |
| [ ] | **UTL-001** | med / high | S | No | `Assertion.h:36-37` | Wrap the handler branch in `do{…}while(0)` — fixes dangling-else / statement-shape. |
| [ ] | **BK-001** | low / high | S | No | `src/Backup.cpp:57` | Throw `Exception(destHandle, res)` to keep the message + extended code (T4). |
| [ ] | **ST-002** | low / high | S | Behavior (better message) | `src/Statement.cpp:78-81` | Detect `index==0` from `sqlite3_bind_parameter_index` and throw "Unknown bind parameter name." (T4). |
| [ ] | **ST-004** | low / high | S | No | `src/Statement.cpp:114-115,136-137,366` | Guard `size() > INT_MAX` before narrowing (T2). |
| [ ] | **COL-001** | low / med | S | No | `src/Column.cpp:96-101` | `blob()` then `bytes()`; drop the redundant leading `bytes()`; fixes the `-Wsign-conversion` hit (T2). |
| [ ] | **ST-003** | low / med | S | No | `src/Statement.cpp:281-303` | Track "built" with a flag, not `empty()`; handle NULL column name. |
| [ ] | **BLD-002** | med / high | M | No | `.gitmodules` | Bump googletest submodule to a current tag (≈ wrap's 1.15.0). |
| [ ] | **BLD-003** | med / high | M | No | `.github/workflows/*` | Pin actions to SHAs; add `permissions: { contents: read }` (T5). |
| [ ] | **BLD-005** | low / med | S | No | `coverage.yml`, `coverity.yml` | Add least-privilege `permissions:` (T5). |
| [ ] | **BLD-006** | low / med | S | No | `CMakeLists.txt:263-273`, `build.sh:14` | Drop forced `-fuse-ld=gold`; add a Linux ASan/UBSan CI job (T5; also closes the verification gap). |
| [ ] | **BLD-004** | low / med | S | Removes CI files | `.travis.yml`, `appveyor.yml`, `CMakeLists.txt:198` | Remove dead CI; rotate the embedded Coverity token. |
| [ ] | **DB-004** | low / med | S | **API** (field type) | `Database.h:138`, `src/Database.cpp:353` | Make `defaultPageCacheSizeBytes` signed (`int32_t`). |
| [ ] | **DB-002** | low / high | S | No | `src/Database.cpp:248` | `static_cast<int>(aNewKey.length())` (T2). |
| [ ] | **DB-003** | low / med | S | **API** (const→non-const) | `Database.h:553` | Make `rekey()` non-`const`. |
| [ ] | **TX-002** | low / high | S | No (private symbol) | `Transaction.h:95` +uses | Rename `mbCommited` → `mbCommitted`. |
| [ ] | **SP-002** | low / high | S | Possible deprecation warning | `Savepoint.h:89-90` | Mark `rollback()` with the deprecation macro (migrate test callers) or drop the note. |
| [ ] | **HDR-003** | low / med | S | No | `SQLiteCppExport.h:35` | Use `_WIN32` (not `WIN32`) for the C4251/4275 suppression. |
| [ ] | **HDR-002** | low / high | S | No | `SQLiteCppExport.h:17` | Fix the doc: `SQLITECPP_DLL_EXPORT`. |
| [ ] | **UTL-002** | low / med | S | No | `Assertion.h:32-34` | Don't redefine `__func__`; use `__FUNCTION__` directly on MSVC. |
| [ ] | **EM-001** | low / high | S | No | `ExecuteMany.h:50-53` | Route the first parameter set through `reset_bind_exec` too. |
| [ ] | **VB-002** | low / med | S | No | `VariadicBind.h:74-94` | Add a tuple-of-tuple test pinning dispatch behavior. |
| [ ] | **BLD-007** | low / med | S | No | `examples/example2/CMakeLists.txt:17-20` | Drop `FORCE` on the cache options. |
| [ ] | **BLD-008** | low / med | S | No | `sqlite3/CMakeLists.txt:14-18` | Use `target_compile_definitions(sqlite3 PRIVATE …)`. |
| [ ] | **EXM-003** | low / high | S | No | `examples/example2/src/main.cpp:53,57,61` | Single-quote SQL string literals. |
| [ ] | **TST-001** | low / high | S | No | `tests/*_test.cpp` | Raise tests to C++17 or drop C++17 `[[maybe_unused]]` at C++11. |
| [ ] | **ST-005** | low / med | S | No | `src/Statement.cpp:206-211` | Document/test the post-done `SQLITE_MISUSE` return. |
| [ ] | **SP-001** | low / med | S | No | `src/Savepoint.cpp:43-48` | Optionally attempt `release()` independently of `rollbackTo()`. |
| [ ] | **SP-003** | low / high | S | No (adds test) | `tests/Savepoint_test.cpp` | Add nested-savepoint test. |
| [ ] | **SP-004** | low / high | S | No (adds test) | `tests/Savepoint_test.cpp` | Add hostile-savepoint-name (injection) test. |
| [ ] | **TX-003** | low / med | S | No (adds test) | `tests/Transaction_test.cpp` | Add a failing-`commit()` regression test. |
| [ ] | **DB-006** | info / med | S | No (adds test) | `tests/Database_test.cpp` | Add `tableExists` case-sensitivity test. |
| [ ] | **COL-002** | info / med | — | No | `src/Column.cpp:77-81` | Awareness only; `getString()` is the recommended path. |
| [ ] | **ST-006** | info / high | — | No | `src/Statement.cpp` | Non-issue; metadata accessors correctly skip `checkRow`. |

---

## 6. Notable verified non-issue (cross-reviewer reconciliation)

- **HDR-001 — `#if SQLITECPP_DLL_EXPORT` is NOT ill-formed.** One reviewer flagged it as an empty-macro test that breaks/weakens DLL export; another verified it is fine. **Resolution:** a value-less `-D`/`/D` macro is defined to `1` by GCC, Clang, and MSVC, so the directive is `#if 1`; the block is MSVC-DLL-only and the MSVC `BUILD_SHARED_LIBS=ON` CI job passes. The only residual is a stylistic preference for `#ifdef`. Recorded so nobody re-flags it.

---

*No code was changed. Awaiting approval before applying any fixes.*
