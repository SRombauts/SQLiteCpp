# SQLiteCpp Deep Code Review

_Generated 2026-06-26. Scope: include/SQLiteCpp/*.h and src/*.cpp (wrapper only; bundled sqlite3 amalgamation excluded)._

> **Fix tracking:** every findings table and the ranked-fix tables below carry a `Done` column.
> `[ ]` is open, `[x]` is fixed (the PR that fixed it is noted in the same cell). Tick an item when its fix merges.

## 1. Scoring Matrix

| File | Lines | Complexity | Importance | Bug Risk | Security Risk | Notes |
|------|------:|:----------:|:----------:|:--------:|:-------------:|-------|
| Database.h + Database.cpp | 625 / 440 | 4 | 5 | 4 | 5 | Connection RAII + handle deleter; raw header byte parsing, encryption key/rekey, extension loading, custom function registration. Header+cpp form the core API unit. |
| Statement.h + Statement.cpp | 726 / 386 | 4 | 5 | 4 | 3 | Largest header (many bind overloads + getColumns templates); shared_ptr stmt lifetime, manual state machine (mbHasRow/mbDone), getExpandedSQL frees C buffer. Central to all queries. |
| Column.h + Column.cpp | 268 / 124 | 3 | 5 | 3 | 3 | Implicit cast operators and getColumns template glue; raw text/blob pointers with lifetime caveats, ostream inserter writes getBytes() bytes. Returned by every SELECT. |
| Transaction.h + Transaction.cpp | 99 / 93 | 1 | 4 | 2 | 1 | Simple RAII begin/commit/rollback over fixed SQL strings; destructor swallows exceptions. No user data in SQL. |
| Savepoint.h + Savepoint.cpp | 99 / 82 | 2 | 3 | 3 | 2 | Name interpolated into SQL but escaped via SELECT quote(?); destructor does rollback+release; double-exec ordering in dtor is subtle. |
| Backup.h + Backup.cpp | 132 / 84 | 2 | 3 | 2 | 2 | sqlite3_backup handle RAII via unique_ptr+Deleter; thin wrappers over backup_step/remaining/pagecount. |
| Exception.h + Exception.cpp | 92 / 46 | 1 | 4 | 2 | 1 | std::runtime_error subclass; constructors read errmsg/errcode from sqlite3*. Used everywhere but trivial logic. |
| VariadicBind.h | 99 | 4 | 3 | 2 | 1 | Header-only variadic + tuple bind via initializer_list/index_sequence; template-heavy but mechanical, no resource handling. |
| ExecuteMany.h | 92 | 4 | 2 | 2 | 1 | Header-only variadic execute_many on top of VariadicBind; perfect-forwarding fold over parameter sets. C++14-gated. |
| Utils.h | 31 | 1 | 2 | 1 | 1 | Only defines the SQLITECPP_PURE_FUNC compiler-attribute macro. |
| Assertion.h | 47 | 1 | 3 | 1 | 1 | SQLITECPP_ASSERT macro with optional user assert handler; used in destructors. Macro-only. |
| SQLiteCppExport.h | 38 | 1 | 2 | 1 | 1 | DLL import/export macros and MSVC warning suppression only. |
| SQLiteCpp.h | 46 | 1 | 3 | 1 | 1 | Umbrella include + SQLITECPP_VERSION macros. No logic. |

_Lines are reported as `header / cpp` for the paired class units, using the true last-line numbers (files without a trailing newline read one line higher than `wc -l`)._

### Score justifications

- **Database.h + Database.cpp** — Complexity 4: filesystem/`std::experimental` feature detection in the header, a `unique_ptr<sqlite3, Deleter>` handle, and `getHeaderInfo()` doing manual big-endian byte assembly from a 100-byte buffer. Importance 5: it is the root of the API — every Statement, Transaction, Savepoint and Backup takes a Database, and it is a `friend` of Statement. Bug Risk 4: raw `ifstream` reads in `isUnencrypted()`/`getHeaderInfo()`, error-path ordering in the constructor (handle reset before throw), and the `backup()` helper open multiple paths. Security Risk 5: the only unit touching `key()`/`rekey()` (encryption), `loadExtension()` (code loading), and raw file/header parsing — the widest sensitive surface in the library.
- **Statement.h + Statement.cpp** — Complexity 4: dozens of overloaded `bind`/`bindNoCopy` signatures, a `getColumns<T,N>` integer-sequence template, a shared_ptr-wrapped `sqlite3_stmt` with a finalizing deleter, and a hand-rolled `mbHasRow`/`mbDone` step state machine. Importance 5: the workhorse for all parameterized queries and result iteration. Bug Risk 4: lifetime of the shared statement vs. Columns, `tryExecuteStep()` MISUSE handling, lazy column-name map build, and `getExpandedSQL()` allocating then `sqlite3_free`-ing. Security Risk 3: queries are parameterized (good), but it still owns the prepared-statement handle and raw error-string access.
- **Column.h + Column.cpp** — Complexity 3: a long list of implicit conversion operators plus the `getColumns` template definitions live here. Importance 5: produced by every `getColumn()`/SELECT result access. Bug Risk 3: `getText()`/`getBlob()` hand back pointers valid only while the statement lives (documented but easy to misuse); `getString()` relies on the blob+bytes ordering; `operator<<` writes `getBytes()` bytes from `getText()`. Security Risk 3: returns raw buffers whose length the caller must respect.
- **Transaction.h + Transaction.cpp** — Complexity 1: a behavior `switch` over three fixed BEGIN strings and plain commit/rollback. Importance 4: widely used and demonstrated, but mechanically simple. Bug Risk 2: destructor rollback wrapped in try/catch; `commit()`/`rollback()` guard on `mbCommited`. Security Risk 1: only fixed SQL literals, no user input interpolated.
- **Savepoint.h + Savepoint.cpp** — Complexity 2: builds SQL by concatenating a savepoint name. Importance 3: a less central feature. Bug Risk 3: the destructor calls `rollback()` then `release()`, and the name is interpolated into `SAVEPOINT`/`RELEASE`/`ROLLBACK TO` strings — correctness hinges on the `SELECT quote(?)` escaping done in the constructor. Security Risk 2: name injection is mitigated by `quote()`, but it is the only unit that interpolates a value into SQL text.
- **Backup.h + Backup.cpp** — Complexity 2: a `unique_ptr<sqlite3_backup, Deleter>` and thin wrappers. Importance 3: optional feature used by `Database::backup()`. Bug Risk 2: null-handle check after `backup_init`, straightforward result-code filtering in `executeStep()`. Security Risk 2: operates on caller-provided database handles/names only.
- **Exception.h + Exception.cpp** — Complexity 1: a `std::runtime_error` subclass storing two error codes. Importance 4: thrown and caught throughout the library and included by most headers. Bug Risk 2: constructors dereference `sqlite3*` to fetch errmsg/errcode (assumes non-null). Security Risk 1: no external attack surface.
- **VariadicBind.h** — Complexity 4: variadic template + `std::initializer_list` comma-expansion and a tuple/`index_sequence` overload set. Importance 3: convenience layer, not required for core use. Bug Risk 2: purely forwards to `Statement::bind`. Security Risk 1: no resources or parsing.
- **ExecuteMany.h** — Complexity 4: perfect-forwarding fold over parameter sets calling reset+bind+exec, C++14-gated. Importance 2: niche convenience helper. Bug Risk 2: relies entirely on Statement/VariadicBind correctness. Security Risk 1: none of its own.
- **Utils.h** — Complexity/Bug/Security 1: just the `SQLITECPP_PURE_FUNC` attribute macro. Importance 2: included by Statement.h.
- **Assertion.h** — Complexity 1, Bug 1, Security 1: a single macro with an optional user handler hook. Importance 3: used by destructors across the library to avoid throwing.
- **SQLiteCppExport.h** — All 1 except importance 2: DLL visibility macros and MSVC warning pragmas only.
- **SQLiteCpp.h** — All 1 except importance 3: umbrella header plus version macros; no behavior.

### Recommended review priority order

1. **Database (Database.h + Database.cpp)** — highest combined importance + security (encryption key/rekey, extension loading, raw header/file parsing) and central to the whole API.
2. **Statement (Statement.h + Statement.cpp)** — highest complexity tier, owns the prepared-statement lifetime and the step state machine, used by every query.
3. **Column (Column.h + Column.cpp)** — implicit conversions and raw text/blob pointer lifetimes; touched by every result read.
4. **Savepoint (Savepoint.h + Savepoint.cpp)** — only unit interpolating a value into SQL text, plus subtle destructor ordering.
5. **Backup (Backup.h + Backup.cpp)** — manual backup-handle RAII and result-code handling.
6. **VariadicBind.h / ExecuteMany.h** — template convenience layers; review together for forwarding/index correctness.
7. **Transaction (Transaction.h + Transaction.cpp)** — simple fixed-SQL RAII; low risk.
8. **Exception (Exception.h + Exception.cpp)** — trivial but pervasive; verify null-handle assumptions.
9. **Assertion.h / Utils.h / SQLiteCpp.h / SQLiteCppExport.h** — macro/umbrella headers; lowest priority.

## 2. Verification Results

Run on Windows 11 (CMake 3.29.2; MinGW GCC + Ninja; Clang/LLVM 22; cppcheck 2.14.0; Python 3.14). Full log in [`code-review/verification.md`](code-review/verification.md).

| Check | Result |
|-------|--------|
| Build (GCC + Ninja, Debug, lib + tests + examples) | **OK** |
| Compiler warnings (`-Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum`) | **0** (wrapper + bundled sqlite3) |
| `ctest` | **100% passed** — `UnitTests` + `Example1Run` |
| cpplint (`--linelength=120`) | 61 findings, **all style-level** (CRLF endings, `#pragma once` vs `#ifndef`, `long` usage, IWYU) |
| cppcheck | **Could not run** — Strawberry build has a broken `FILESDIR`, cannot load `std.cfg` (host-tooling limitation) |
| AddressSanitizer | **Not feasible on this host** — repo `SQLITECPP_USE_ASAN` instruments only the lib target → `lld-link /failifmismatch annotate_string`; uniform ASAN fails compiling vendored `sqlite3.c` under Clang; GCC path hardcodes `-fuse-ld=gold` (absent on Windows). Documented, not run. |

The wrapper compiles warning-clean and all tests pass. ASAN's marginal value here is low: the higher-severity UB findings sit on OOM/size-limit/malformed-input paths the happy-path tests don't exercise. A real CI gap exists — GitHub Actions runs no ASAN/Valgrind/coverage/static-analysis (those live only in legacy Travis).

## 3. Per-Unit Findings

Full per-finding detail (description, impact, proposed fix, file:line) is in `code-review/<Unit>-review.md`. Totals across the wrapper: **4 High, 22 Medium, 30 Low, 21 Info** (0 Critical) over ~77 findings. No memory leaks, double-frees, or SQL-injection vulnerabilities were found; the issues cluster around **null C-pointer → `std::string` UB**, **destructor exception/error handling**, **fixed-width/sign correctness**, and **API modernization**.

### Database — `code-review/Database-review.md` (H1 M4 L4 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | DB-01 | High | High | `isUnencrypted()` `getline()` makes the 16-byte magic compare unreliable + reads uninitialized stack on short files (security gate) |
| [ ] | DB-02 | Med | High | `getHeaderInfo()` big-endian assembly is signed left-shift **UB**; sign-extends on LP64 |
| [ ] | DB-03 | Med | High | `Header` struct uses platform-variable `unsigned long` → cross-platform field-width mismatch |
| [ ] | DB-04 | Med | High | Constructor `mFilename(apFilename)` is UB on a null `const char*` |
| [ ] | DB-05 | Med | Med | `key()`/`rekey()` declared `const` but mutate the database |
| [ ] | DB-06 | Low | High | `loadExtension()` discards SQLite's error message (passes `0` as `pzErrMsg`) |
| [ ] | DB-07 | Low | Med | No guard/warning against unsupported Serialized mode (`OPEN_FULLMUTEX`) |
| [ ] | DB-08 | Low | High | `key()` vs `rekey()` inconsistent `int passLen` narrowing |
| [ ] | DB-09 | Low | Med | `getHeaderInfo()` reads `gcount()` after `close()` (fragile) |
| [ ] | DB-10 | Info | High | `execAndGet`/`tableExists` rely on `executeStep()`+`getColumn` throwing (cross-class coupling) |
| [ ] | DB-11 | Info | Med | `backup()` opens a `Load` source with `READWRITE\|CREATE` (creates empty file instead of failing) |

### Statement — `code-review/Statement-review.md` (H1 M4 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | STMT-01 | High | High | `getExpandedSQL()` builds `std::string` from possibly-NULL `sqlite3_expanded_sql()` (**UB**; reachable via size-limit) |
| [ ] | STMT-02 | Med | Med | Lazy column-name map inserts possibly-NULL `sqlite3_column_name()` (**UB**) |
| [ ] | STMT-03 | Med | High | `getChanges()` is connection-wide, not statement-specific; misleading on shared connections |
| [ ] | STMT-04 | Med | High | Defaulted move-assignment leaves moved-from object partially live (inconsistent with move ctor) |
| [ ] | STMT-05 | Med | Med | blob/text binds cast `size()`→`int` (truncation/UB for >2 GB) |
| [ ] | STMT-06 | Low | Med | Error classification via `ret == sqlite3_errcode()` is fragile on shared connections |
| [ ] | STMT-07 | Low | High | Metadata accessors don't `checkRow()` (correct but undocumented asymmetry) |
| [ ] | STMT-08 | Low | High | Missing `[[nodiscard]]`/`noexcept` opportunities |
| [ ] | STMT-09 | Info | High | `bind(uint32_t)`→`int64` is correct (documented non-issue) |
| [ ] | STMT-10 | Info | High | shared_ptr + finalize ownership model verified correct |

### Column — `code-review/Column-review.md` (H1 M3 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | COL-01 | High | Med | `operator<<` writes `getText()` data with `getBytes()` length — wrong count for BLOBs + unspecified arg eval order |
| [ ] | COL-02 | Med | High | All `noexcept` getters deref raw `sqlite3_stmt*` with no row/index guard (UB by design; doc-only contract) |
| [ ] | COL-03 | Med | High | `getUInt()` truncates a 64-bit value to 32 bits |
| [ ] | COL-04 | Med | High | Implicit conversion operators: cross-compiler ambiguity + silent narrowing |
| [ ] | COL-05 | Low | High | `getString()` redundant/fragile 3-call `bytes,blob,bytes` dance |
| [ ] | COL-06 | Low | High | `getString()` constructs `std::string(nullptr, 0)` (relies on edge guarantee) |
| [ ] | COL-07 | Low | Med | `getText()` default-value pointer lifetime undocumented |
| [ ] | COL-08 | Info | High | Copyable `Column` keeps stmt allocated but not row-valid (footgun, documented) |
| [ ] | COL-09 | Info | High | Missing `[[nodiscard]]` on pure getters |

### Savepoint — `code-review/Savepoint-review.md` (H1 M3 L2 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | SP-01 | High | High | Destructor `rollback()`+`release()` always discards work; differs from `Transaction`, conflicts with `release()` "commit" doc (footgun) |
| [ ] | SP-02 | Med | High | Only `mbReleased` flag; no rolled-back state → fragile double-rollback, can leak a savepoint |
| [ ] | SP-03 | Med | High | Destructor catches only `SQLite::Exception` → `std::bad_alloc` escapes → `std::terminate` |
| [ ] | SP-04 | Med | High | `quote()` truncates name at first embedded NUL (silent name change) |
| [ ] | SP-05 | Low | High | Non-movable reference-holding RAII type |
| [ ] | SP-06 | Low | Med | `rollback()` lacks `[[deprecated]]`; `release()` doc overstates "commit" for nested savepoints |
| [ ] | SP-07 | Info | High | Throwing-constructor RAII verified correct |
| [ ] | SP-08 | Info | High | **SQL injection via name is correctly mitigated** (`quote(?)` + consistent reuse) — verified safe |

### Backup — `code-review/Backup-review.md` (M1 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | BKP-01 | Med | High | `executeStep()` not `[[nodiscard]]`; `Database::backup()` ignores its return → silent incomplete backup on BUSY/LOCKED |
| [ ] | BKP-02 | Low | High | Deleted copy suppresses move → `Backup` non-movable (inconsistent with Statement/Column) |
| [ ] | BKP-03 | Low | High | Page-count getters lack `noexcept`/`[[nodiscard]]` |
| [ ] | BKP-04 | Low | Med | Step-failure exception drops extended code + connection message (poorer than init path) |
| [ ] | BKP-05 | Info | High | No retained `Database` ref → latent use-after-free if a connection is destroyed first (documented design) |
| [ ] | BKP-06 | Info | High | Deleter swallowing `backup_finish` code verified correct |

### Exception — `code-review/Exception-review.md` (M2 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | EXC-01 | Med | High | Extended error code is silently `-1` for the `(const char*, int)` ctor (getters disagree for same error) |
| [ ] | EXC-02 | Med | Med | `Exception(const char*, int)` forwards a possibly-null message to `std::runtime_error` (**UB**) |
| [ ] | EXC-03 | Low | High | Accessors are good `[[nodiscard]]` candidates |
| [ ] | EXC-04 | Low | High | `getErrorStr()` pointer-lifetime undocumented; `SQLITECPP_PURE_FUNC` candidate |
| [ ] | EXC-05 | Low | Med | Error-code members intentionally non-`const` (undocumented rationale) |
| [ ] | EXC-06 | Info | High | Doc/typo nits (`Exception_test.cpp` header mislabeled `Transaction_test.cpp`; "avaiable") |
| [ ] | EXC-07 | Info | High | `-1` sentinel duplicated across ctors |
| [ ] | — | Info | High | **Headline non-issue verified:** `sqlite3_errmsg/errcode(NULL)` are *defined* (return static "out of memory"/`SQLITE_NOMEM`), so `sqlite3*` ctors are null-safe |

### Transaction — `code-review/Transaction-review.md` (M2 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | TXN-01 | Med | High | Destructor swallows ALL rollback failures (incl. real BUSY/IO) with no `SQLITECPP_ASSERT`/diagnostic |
| [ ] | TXN-02 | Med | High | `default:` in behavior switch defeats `-Wswitch-enum` exhaustiveness |
| [ ] | TXN-03 | Low | High | `rollback()` after `commit()` throws misleading "already committed" message |
| [ ] | TXN-04 | Low | High | Manual `rollback()` sets no flag → destructor issues a second failing `ROLLBACK` (couples to TXN-01) |
| [ ] | TXN-05 | Low | High | Move ops not explicitly declared (Rule-of-5 intent implicit) |
| [ ] | TXN-06 | Info | High | `Database&` lifetime contract (documented, inherent) |
| [ ] | TXN-07 | Info | High | `[[nodiscard]]` on the guard type (N/A under C++11) |

### ExecuteMany — `code-review/ExecuteMany-review.md` (M1 L2 I3)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | EM-01 | Med | High | Stale bindings leak between parameter sets (`reset()` without `clearBindings()`) → **silent data corruption** for variable-arity sets |
| [ ] | EM-02 | Low | High | First set not reset; relies on a fresh Statement (public `bind_exec` footgun) |
| [ ] | EM-03 | Low | High | `bind_exec`/`reset_bind_exec` pollute `SQLite` namespace, used-before-definition, `apQuery` mis-prefixed |
| [ ] | EM-04 | Info | High | `execute_many` requires ≥1 set, undocumented |
| [ ] | EM-05 | Info | High | No use-after-move despite forwarding chain (verified) |
| [ ] | EM-06 | Info | High | C++14 gating correct |

### VariadicBind — `code-review/VariadicBind-review.md` (L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | VB-01 | Low | High | `std::forward<decltype(args)>` is a misleading no-op (params are `const&`) |
| [ ] | VB-03 | Low | High | Single-tuple call relies on overload partial-ordering (latent fragility) |
| [ ] | VB-04 | Low | Med | Doc examples use non-SQL `&&`; omit the always-copy (`SQLITE_TRANSIENT`) caveat |
| [ ] | VB-02 / VB-05 | Info | High | `void` return (no `[[nodiscard]]`); C++14 gate duplicated 3× |

### Trivial/support headers — `code-review/TrivialHeaders-review.md` (M2 L4 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | HDR-01 | Med | High | `SQLITECPP_ASSERT` (handler path) is an unbraced `if` → dangling-else / scope-capture hazard |
| [ ] | HDR-02 | Med | High | `assertion_failed` declared `int apLine` but defined `long apLine` everywhere (ODR/link mismatch) |
| [ ] | HDR-03 | Low | High | `#if SQLITECPP_DLL_EXPORT` uses value-truthiness, not `defined()`; comment misnames the macro |
| [ ] | HDR-04 | Low | High | `WIN32` vs `_WIN32` inconsistency in warning-suppression block |
| [ ] | HDR-05 | Low | Med | Unconditional `#define __func__ __FUNCTION__` under MSVC (reserved-id redefinition, leaks downstream) |
| [ ] | HDR-06 | Low | High | Umbrella `SQLiteCpp.h` omits `Backup`/`Savepoint`/`VariadicBind`/`ExecuteMany` despite "all functionality" claim |
| [ ] | HDR-07 | Info | High | `SQLITECPP_VERSION` string is non-canonical `"3.03.03"` (should be `"3.3.3"`) |
| [ ] | HDR-08 | Info | Med | Macro args not defensively parenthesized |

### Cross-cutting themes
- **Null C-pointer → `std::string`/`runtime_error` UB**: STMT-01, STMT-02, EXC-02, DB-04 (and the read in DB-01). A small, uniform "guard the pointer" pattern fixes all. _(STMT-01, STMT-02, EXC-02, DB-04 fixed in #552; DB-01 still open.)_
- **Destructor error handling / terminal-state flags**: TXN-01, TXN-04, SP-02, SP-03 — library-wide habit of swallowing all exceptions and lacking a "finished" flag; route through `SQLITECPP_ASSERT`, broaden to `catch(...)`, track terminal state.
- **`SQLITECPP_NODISCARD` macro**: requested by STMT-08, COL-09, BKP-01/03, EXC-03 — one feature-gated macro (like the existing `SQLITECPP_PURE_FUNC`) serves all; functionally important for `Backup::executeStep`.
- **Fixed-width / sign correctness in header parsing**: DB-02 + DB-03.
- **Move-semantics consistency**: BKP-02, SP-05, TXN-05.

## 4. Ranked Fixes

Ranked by Severity × Likelihood × (inverse) Effort, with confidence. **P0** = correctness/UB/security, small & safe; **P1** = correctness/robustness worth doing; **P2** = API/modernization/docs/build hygiene.

### P0 — fix first (correctness / UB / security; low effort, high confidence)
| Done | # | Fix | Finding(s) | Files | Effort |
|:--:|---|-----|-----------|-------|--------|
| [ ] | 1 | Read 16 raw bytes (`read`+`gcount`+`memcmp`) instead of `getline()` in `isUnencrypted()` (reuse `getHeaderInfo` pattern) | DB-01 | `src/Database.cpp` | S |
| [ ] | 2 | Guard `sqlite3_expanded_sql()` NULL before constructing `std::string` (throw or empty) | STMT-01 | `src/Statement.cpp` | S |
| [ ] | 3 | Add `clearBindings()` to `reset_bind_exec()`; add decreasing-arity regression test | EM-01 | `include/SQLiteCpp/ExecuteMany.h`, `tests/ExecuteMany_test.cpp` | S |
| [ ] | 4 | Skip NULL `sqlite3_column_name()` when building the column-name map | STMT-02 | `src/Statement.cpp` | S |
| [ ] | 5 | Guard null message in `Exception(const char*, int)` (`msg ? msg : ""`) | EXC-02 | `src/Exception.cpp` | S |
| [ ] | 6 | Guard null `apFilename` in the raw-`const char*` `Database` ctor | DB-04 | `src/Database.cpp` | S |

### P1 — should fix (correctness / robustness / UB-by-design)
| Done | # | Fix | Finding(s) | Files | Effort |
|:--:|---|-----|-----------|-------|--------|
| [ ] | 7 | Cast bytes to `uint32_t` before shifting in `getHeaderInfo()`; move `Header` to fixed-width types | DB-02, DB-03 | `src/Database.cpp`, `include/SQLiteCpp/Database.h` | M |
| [ ] | 8 | Fix `operator<<` byte-count/eval-order (use `getString()` or sequence text-then-bytes) + BLOB test | COL-01 | `src/Column.cpp`, `tests/Column_test.cpp` | S |
| [ ] | 9 | Broaden Savepoint destructor to `catch(...)`; add terminal-state flag; minimize dtor commands | SP-02, SP-03 | `src/Savepoint.cpp`, `include/SQLiteCpp/Savepoint.h` | M |
| [ ] | 10 | Route swallowed destructor errors through `SQLITECPP_ASSERT`; add a single "finished" flag (Transaction + Savepoint) | TXN-01, TXN-04 | `src/Transaction.cpp`, `src/Savepoint.cpp` | M |
| [ ] | 11 | Reject/throw on bind sizes > `INT_MAX` in blob/text binds | STMT-05 | `src/Statement.cpp` | S |
| [ ] | 12 | Make `Database::backup()` check `executeStep()`; add `SQLITECPP_NODISCARD` to `Backup::executeStep` | BKP-01 | `src/Database.cpp`, `include/SQLiteCpp/Backup.h` | S |
| [ ] | 13 | Remove `default:` from Transaction behavior switch to restore `-Wswitch-enum` (explicit post-switch throw) | TXN-02 | `src/Transaction.cpp` | S |
| [ ] | 14 | Fix `assertion_failed` `int`/`long` mismatch across header + examples + tests + docs | HDR-02 | `include/SQLiteCpp/Assertion.h`, examples, tests, READMEs | S |
| [ ] | 15 | Wrap `SQLITECPP_ASSERT` (handler path) in `do { } while(0)` | HDR-01 | `include/SQLiteCpp/Assertion.h` | S |
| [ ] | 16 | Reject savepoint names with embedded NUL (or document truncation) | SP-04 | `src/Savepoint.cpp` | S |
| [ ] | 17 | Document `getChanges()` connection-scope (or cache per-statement) | STMT-03 | `include/SQLiteCpp/Statement.h`, `src/Statement.cpp` | S |
| [ ] | 18 | Hand-write `Statement` move-assignment to mirror the move ctor (scrub source) | STMT-04 | `include/SQLiteCpp/Statement.h`, `src/Statement.cpp` | S |

### P2 — nice to have (API / modernization / docs / build hygiene)
| Done | # | Fix | Finding(s) | Effort |
|:--:|---|-----|-----------|--------|
| [ ] | 19 | Add a feature-gated `SQLITECPP_NODISCARD` macro and apply to pure getters/return-bearing methods | STMT-08, COL-09, BKP-03, EXC-03 | M |
| [ ] | 20 | Fix `loadExtension()` to capture & free SQLite's `pzErrMsg` | DB-06 | S |
| [ ] | 21 | Reconcile `key()`/`rekey()` const-correctness + `passLen` consistency | DB-05, DB-08 | S |
| [ ] | 22 | Default move ops for `Backup`/`Savepoint`/`Transaction` (or document the seal) | BKP-02, SP-05, TXN-05 | S |
| [ ] | 23 | Simplify `getString()` to the documented blob-then-bytes 2-call form; guard null | COL-05, COL-06 | S |
| [ ] | 24 | `Column` implicit-conversion narrowing — document / consider `explicit` (major) + `getUInt` low-32 doc | COL-04, COL-03 | M |
| [ ] | 25 | Drop misleading `std::forward` in `VariadicBind`; fix doc `&&`→`AND`; move `ExecuteMany` helpers to `detail` | VB-01, VB-04, EM-02, EM-03 | S |
| [ ] | 26 | `SQLiteCppExport.h`: `#if defined(SQLITECPP_DLL_EXPORT)`, unify `_WIN32`, fix comment | HDR-03, HDR-04 | S |
| [ ] | 27 | Guard/remove MSVC `#define __func__` | HDR-05 | S |
| [ ] | 28 | Add missing headers to umbrella (or document the omission) | HDR-06 | S |
| [ ] | 29 | Version string `"3.03.03"` → `"3.3.3"` (sync with `sqlitecpp-release`) | HDR-07 | S |
| [ ] | 30 | `backup()` `Load` should open source `READONLY`; seed `Exception` extended code from `ret`; doc/typo fixes | DB-11, EXC-01, EXC-06 | S |
| [ ] | 31 | Normalize line endings to LF / address cpplint style set | cpplint | S |

> **Note on EXC-01 / EXC-related test churn:** seeding the extended code from `ret` changes existing `tests/Exception_test.cpp` expectations (`-1`), so it's a deliberate API decision to confirm before applying.

---
_Per-unit detail: see the `code-review/` folder. Verification log: `code-review/verification.md`._
