# SQLiteCpp Deep Code Review

_Generated 2026-06-26. Scope: include/SQLiteCpp/*.h and src/*.cpp (wrapper only; bundled sqlite3 amalgamation excluded)._

> **Fix tracking:** every findings table and the ranked-fix tables below carry a `Done` column.
> `[ ]` is open, `[x]` is fixed (the PR that fixed it is noted in the same cell). Tick an item when its fix merges.

> **Merged 2026-07-02 from a third, independent review pass** (full report `CODE_REVIEW_3.md`; detail in `code-review/findings3-*.md`, `code-review/triage_3.md`, `code-review/verification_3.md`). The third pass verified in the tree that **all 13 previously ticked fixes are genuinely fixed**, re-confirmed every still-open item, ran the **first-ever ASan/UBSan/LeakSanitizer verification (Linux: clean, 60/60 tests, ~100 % line coverage of src/)**, and added 39 new findings tagged **_(3rd pass)_** (1 High, 8 Medium, 20 Low, 10 Info) — the High being **SP-09, a behavioral regression introduced by the afa51d3 Savepoint fix**. It also invalidated two previously proposed fixes: ranked fix #23 as written would break UTF-16 databases (COL-12), and BKP-04's proposed fix reads the wrong handle (see `findings3-backup-exception.md`). New ranked fixes are #47–#72.

> **Merged 2026-06-30/07-01 from a second, independent review pass** (full report `CODE_REVIEW_2.md`; detail in `code-review/findings-*.md`, `code-review/triage.md`, `code-review/verification_2.md`). The two passes agreed on the wrapper internals (the second pass's findings mostly map onto IDs already here). New items the second pass added are tagged **_(2nd pass)_** and extend coverage to three areas this report did not originally include: **build/CI/supply-chain (BLD-*)**, **examples (EXM-*)**, and **test portability (TST-*)** — plus **TXN-08**, the Transaction sibling of the now-fixed Savepoint SP-03. The second pass independently confirmed the SP-02/SP-03 fixes landed by `afa51d3` on this branch.

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

**3rd-pass verification (2026-07-02, Linux, gcc 11.4 — full log [`code-review/verification_3.md`](code-review/verification_3.md)):** the previously infeasible sanitizer run was completed on Linux with uniform `-fsanitize=address,undefined -fno-sanitize-recover=all` (avoiding the broken `SQLITECPP_USE_ASAN`): **60/60 tests pass with zero ASan/UBSan/LeakSanitizer reports and zero leaks**, at gcov line coverage of **99.3–100 % per src/*.cpp file**; example1 clean too. Wrapper + example warnings: **0** under `-Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum`; the sole warning anywhere is a benign tests sign-compare (TST-003). Caveats recorded honestly: CMake/ctest unavailable in that sandbox (build replicated via equivalent hand-written Makefile), cppcheck/clang-tidy not runnable (blocked installs), no C++17/std::filesystem-mode or Meson build covered.

## 3. Per-Unit Findings

Full per-finding detail (description, impact, proposed fix, file:line) is in `code-review/<Unit>-review.md` (passes 1–2) and `code-review/findings3-*.md` (3rd pass). Totals across the wrapper after three passes: **5 High, 30 Medium, 50 Low, 31 Info** (0 Critical) over ~116 findings. No memory leaks, double-frees, or SQL-injection vulnerabilities were found; the issues cluster around **null C-pointer → `std::string` UB**, **destructor exception/error handling**, **fixed-width/sign correctness**, and **API modernization**.

### Database — `code-review/Database-review.md` (H1 M4 L4 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [x] #553 | DB-01 | High | High | `isUnencrypted()` `getline()` makes the 16-byte magic compare unreliable + reads uninitialized stack on short files (security gate) |
| [x] #558 | DB-02 | Med | High | `getHeaderInfo()` big-endian assembly is signed left-shift **UB**; sign-extends on LP64 |
| [x] #558 | DB-03 | Med | High | `Header` struct uses platform-variable `unsigned long` → cross-platform field-width mismatch |
| [x] #552 | DB-04 | Med | High | Constructor `mFilename(apFilename)` is UB on a null `const char*` |
| [ ] | DB-05 | Med | Med | `key()`/`rekey()` declared `const` but mutate the database |
| [ ] | DB-06 | Low | High | `loadExtension()` discards SQLite's error message (passes `0` as `pzErrMsg`) |
| [ ] | DB-07 | Low | Med | No guard/warning against unsupported Serialized mode (`OPEN_FULLMUTEX`) |
| [ ] | DB-08 | Low | High | `key()` vs `rekey()` inconsistent `int passLen` narrowing |
| [ ] | DB-09 | Low | Med | `getHeaderInfo()` reads `gcount()` after `close()` (fragile) |
| [ ] | DB-10 | Info | High | `execAndGet`/`tableExists` rely on `executeStep()`+`getColumn` throwing (cross-class coupling) |
| [ ] | DB-11 | Info | Med | `backup()` opens a `Load` source with `READWRITE\|CREATE` (creates empty file instead of failing) |
| [ ] | DB-12 | Med | High | _(2nd pass)_ Defaulted `Database` move can orphan live `Statement`s / close a busy handle → use-after-free + `SQLITE_BUSY` leak; precondition undocumented & unenforced |
| [ ] | DB-13 | Low | Med | _(2nd pass)_ `getHeaderInfo().defaultPageCacheSizeBytes` is unsigned but the field is **signed** in the file format (negative = KiB), and the name misleads (refines DB-02/DB-03) |
| [ ] | DB-14 | Med | High | _(3rd pass)_ Deleter uses `sqlite3_close` (not `close_v2`): a `Statement` outliving the `Database` → `SQLITE_BUSY` swallowed → connection + file locks **leak permanently** |
| [ ] | DB-15 | Low | High | _(3rd pass)_ `getHeaderInfo()` magic check compares only 15 bytes and forces `headerStr[15]='\0'` — accepts invalid headers, inconsistent with fixed `isUnencrypted()` |
| [ ] | DB-16 | Med | High | _(3rd pass)_ Windows: `isUnencrypted()`/`getHeaderInfo()` open the UTF-8 filename via ANSI-codepage `ifstream` — probes a **different file** than the connection for non-ASCII paths |
| [ ] | DB-17 | Low | High | _(3rd pass)_ `isUnencrypted()`/`getHeaderInfo()` docs claim "path/uri" but `ifstream` fails on URI/`:memory:`/temp names with misleading errors |
| [ ] | DB-18 | Low | High | _(3rd pass)_ `OPEN_NOFOLLOW` silently defined as 0 on pre-3.31 SQLite — symlink protection silently absent |
| [ ] | DB-19 | Low | High | _(3rd pass)_ `SQLITECPP_DISABLE_STD_FILESYSTEM` ignored when `SQLITECPP_HAVE_STD_EXPERIMENTAL_FILESYSTEM` is defined (wrong `#ifndef` branch; unreachable `#undef` at Database.h:42-44) |
| [ ] | DB-20 | Info | High | _(3rd pass)_ `namespace std { namespace filesystem = experimental::filesystem; }` injection into `std` is formally UB, leaks into every consumer TU |
| [ ] | DB-21 | Info | High | _(3rd pass)_ Public `Header` field `incrementalVaccumMode` typo ("Vaccum") locked into the API |
| [ ] | DB-22 | Low | High | _(3rd pass)_ `pageSizeBytes` doesn't decode file-format special value 1 = 65536 — 64 KiB-page DBs report page size 1 (refines DB-13) |

### Statement — `code-review/Statement-review.md` (H1 M4 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [x] #552 | STMT-01 | High | High | `getExpandedSQL()` builds `std::string` from possibly-NULL `sqlite3_expanded_sql()` (**UB**; reachable via size-limit) |
| [x] #552 | STMT-02 | Med | Med | Lazy column-name map inserts possibly-NULL `sqlite3_column_name()` (**UB**) |
| [ ] | STMT-03 | Med | High | `getChanges()` is connection-wide, not statement-specific; misleading on shared connections |
| [ ] | STMT-04 | Med | High | Defaulted move-assignment leaves moved-from object partially live (inconsistent with move ctor) |
| [ ] | STMT-05 | Med | Med | blob/text binds cast `size()`→`int` (truncation/UB for >2 GB) |
| [ ] | STMT-06 | Low | Med | Error classification via `ret == sqlite3_errcode()` is fragile on shared connections |
| [ ] | STMT-07 | Low | High | Metadata accessors don't `checkRow()` (correct but undocumented asymmetry) |
| [ ] | STMT-08 | Low | High | Missing `[[nodiscard]]`/`noexcept` opportunities |
| [ ] | STMT-09 | Info | High | `bind(uint32_t)`→`int64` is correct (documented non-issue) |
| [ ] | STMT-10 | Info | High | shared_ptr + finalize ownership model verified correct |
| [ ] | STMT-11 | Low | High | _(2nd pass)_ Binding by an unknown parameter name reports "column index out of range" instead of "unknown bind parameter" (`sqlite3_bind_parameter_index` returns 0 → `bind(0,…)`); read side already errors clearly |
| [ ] | STMT-12 | Med | High | _(3rd pass)_ NULL `pzTail` in `sqlite3_prepare_v2`: multi-statement SQL **silently truncated to the first statement** (no error, undocumented; `Database::exec()` runs all — silent data loss for migrating users) |
| [ ] | STMT-13 | Med | High | _(3rd pass)_ `getChanges()` on a moved-from `Statement` calls `sqlite3_changes(nullptr)` → null deref in a `noexcept` method (sole moved-from-unsafe accessor in the class) |
| [ ] | STMT-14 | Med | High | _(3rd pass)_ `Statement(db, (const char*)nullptr)` constructs `std::string` from NULL → UB — the #552 defect class (DB-04/EXC-02), missed in the sweep |
| [ ] | STMT-15 | Low | High | _(3rd pass)_ `getColumnIndex(nullptr)`/`getColumn(nullptr)`/`isColumnNull(nullptr)` build a `std::string` map key from NULL → UB |
| [ ] | STMT-16 | Low | High | _(3rd pass)_ `mColumnCount`/`mColumnNames` frozen at prepare time — schema-change auto-reprepare can change SELECT * columns; stale guard throws for present columns |
| [ ] | STMT-17 | Low | High | _(3rd pass)_ `exec()` on the last pending row of a SELECT succeeds and returns an unrelated connection-wide change count (misuse only detected when another row follows; compounds STMT-03) |
| [ ] | STMT-18 | Low | Med | _(3rd pass)_ `SQLITECPP_PURE_FUNC` on throwing `getIndex()` violates GCC `pure` contract — calls may be CSE'd/deleted, eliding the expected exception |
| [ ] | STMT-19 | Info | High | _(3rd pass)_ Exception-message typo "needs to be reseted" (×2, src/Statement.cpp:175,198) |

### Column — `code-review/Column-review.md` (H1 M3 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [x] #556 | COL-01 | High | Med | `operator<<` writes `getText()` data with `getBytes()` length — wrong count for BLOBs + unspecified arg eval order |
| [ ] | COL-02 | Med | High | All `noexcept` getters deref raw `sqlite3_stmt*` with no row/index guard (UB by design; doc-only contract) |
| [ ] | COL-03 | Med | High | `getUInt()` truncates a 64-bit value to 32 bits |
| [ ] | COL-04 | Med | High | Implicit conversion operators: cross-compiler ambiguity + silent narrowing |
| [ ] | COL-05 | Low | High | `getString()` redundant/fragile 3-call `bytes,blob,bytes` dance |
| [ ] | COL-06 | Low | High | `getString()` constructs `std::string(nullptr, 0)` (relies on edge guarantee) |
| [ ] | COL-07 | Low | Med | `getText()` default-value pointer lifetime undocumented |
| [ ] | COL-08 | Info | High | Copyable `Column` keeps stmt allocated but not row-valid (footgun, documented) |
| [ ] | COL-09 | Info | High | Missing `[[nodiscard]]` on pure getters |
| [ ] | COL-10 | Low | High | _(3rd pass)_ `operator<<` doc comment still says "using getText()" — stale after the #556 fix switched to `getString()` |
| [ ] | COL-11 | Med | High | _(3rd pass)_ `getText()`/`getBlob()` `@warning` understates pointer lifetime: omits invalidation by subsequent **type conversions** (getBlob-then-getText can realloc) and by `executeStep()`/`reset()` — silent UAF enabler |
| [ ] | COL-12 | Med | High | _(3rd pass)_ **Ranked fix #23 as proposed is a latent regression**: `getString()`'s leading `sqlite3_column_bytes()` is load-bearing (forces UTF-8 conversion on UTF-16 DBs; `Column.basis16` would fail) — amend #23 before implementing |
| [ ] | COL-13 | Low | Med | _(3rd pass)_ `getName()`/`getOriginName()` can return nullptr (OOM) and are invalidated by reprepare/next call — undocumented; `std::string(getName())` idiom is UB on that path |

### Savepoint — `code-review/Savepoint-review.md` (H1 M3 L2 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | SP-01 | High | High | Destructor `rollback()`+`release()` always discards work; differs from `Transaction`, conflicts with `release()` "commit" doc (footgun) |
| [x] afa51d3 | SP-02 | Med | High | Only `mbReleased` flag; no rolled-back state → fragile double-rollback, can leak a savepoint _(fixed on this branch: added `mbRolledBack`)_ |
| [x] afa51d3 | SP-03 | Med | High | Destructor catches only `SQLite::Exception` → `std::bad_alloc` escapes → `std::terminate` _(fixed on this branch: broadened to `catch(...)`)_ |
| [ ] | SP-04 | Med | High | `quote()` truncates name at first embedded NUL (silent name change) |
| [ ] | SP-05 | Low | High | Non-movable reference-holding RAII type |
| [ ] | SP-06 | Low | Med | `rollback()` lacks `[[deprecated]]`; `release()` doc overstates "commit" for nested savepoints |
| [ ] | SP-07 | Info | High | Throwing-constructor RAII verified correct |
| [ ] | SP-08 | Info | High | **SQL injection via name is correctly mitigated** (`quote(?)` + consistent reuse) — verified safe |
| [ ] | SP-09 | **High** | High | _(3rd pass)_ **Regression introduced by afa51d3 (the SP-02 fix):** after a manual `rollbackTo()`, work written afterwards is silently **committed** on scope exit — the destructor skips `ROLLBACK TO` when `mbRolledBack` is set (src/Savepoint.cpp:43-47) and goes straight to `RELEASE`; pre-fix behavior (and the documented auto-rollback contract) discarded it |

### Backup — `code-review/Backup-review.md` (M1 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | BKP-01 | Med | High | `executeStep()` not `[[nodiscard]]`; `Database::backup()` ignores its return → silent incomplete backup on BUSY/LOCKED |
| [ ] | BKP-02 | Low | High | Deleted copy suppresses move → `Backup` non-movable (inconsistent with Statement/Column) |
| [ ] | BKP-03 | Low | High | Page-count getters lack `noexcept`/`[[nodiscard]]` |
| [ ] | BKP-04 | Low | Med | Step-failure exception drops extended code + connection message (poorer than init path) |
| [ ] | BKP-05 | Info | High | No retained `Database` ref → latent use-after-free if a connection is destroyed first (documented design) |
| [ ] | BKP-06 | Info | High | Deleter swallowing `backup_finish` code verified correct |
| [ ] | BKP-07 | Low | Med | _(2nd pass)_ Page-count getters return 0 (meaningless) before the first `executeStep()`; undocumented → progress-bar divide-by-zero |
| [ ] | BKP-08 | Med | High | _(3rd pass)_ `executeStep()` throw puts SQLite **extended** codes (e.g. `SQLITE_IOERR_WRITE`=778) in `getErrorCode()` while `getExtendedErrorCode()` stays -1 — primary/extended slots inverted vs the rest of the library. Note: BKP-04's previously proposed fix is **invalid** (`sqlite3_backup_step` never posts errors to the destination connection) |
| [ ] | BKP-09 | Low | High | _(3rd pass)_ `executeStep(0)` is an undocumented no-op returning `SQLITE_OK` forever — never `SQLITE_DONE`; infinite retry-loop hazard |
| [ ] | BKP-10 | Info | High | _(3rd pass)_ Redundantly qualified `SQLite::Backup::Deleter::operator()` definition inside `namespace SQLite` (src/Backup.cpp:75) |

### Exception — `code-review/Exception-review.md` (M2 L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | EXC-01 | Med | High | Extended error code is silently `-1` for the `(const char*, int)` ctor (getters disagree for same error) |
| [x] #552 | EXC-02 | Med | Med | `Exception(const char*, int)` forwards a possibly-null message to `std::runtime_error` (**UB**) |
| [ ] | EXC-03 | Low | High | Accessors are good `[[nodiscard]]` candidates |
| [ ] | EXC-04 | Low | High | `getErrorStr()` pointer-lifetime undocumented; `SQLITECPP_PURE_FUNC` candidate |
| [ ] | EXC-05 | Low | Med | Error-code members intentionally non-`const` (undocumented rationale) |
| [ ] | EXC-06 | Info | High | Doc/typo nits (`Exception_test.cpp` header mislabeled `Transaction_test.cpp`; "avaiable") |
| [ ] | EXC-07 | Info | High | `-1` sentinel duplicated across ctors |
| [ ] | EXC-08 | Info | High | _(3rd pass)_ `std::string` ctors truncate messages at embedded NUL (delegate via `.c_str()` instead of `std::runtime_error(const std::string&)`) |
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
| [x] #559 | TXN-08 | Med | High | _(2nd pass)_ Destructor (`src/Transaction.cpp:58`) catches only `SQLite::Exception` → `std::bad_alloc` (from building the exception during `ROLLBACK`) escapes → `std::terminate`. **Same defect just fixed for Savepoint in afa51d3 (SP-03), not propagated to Transaction.** Fixed in `5e4908a`: broadened to `catch(...)`. |
| [ ] | TXN-09 | Low | High | _(3rd pass)_ Nested `Transaction` (BEGIN inside BEGIN) throws at construction with a raw SQLite error; non-nestability and the `Savepoint` alternative are undocumented in Transaction.h |

### ExecuteMany — `code-review/ExecuteMany-review.md` (M1 L2 I3)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [x] #554 | EM-01 | Med | High | Stale bindings leak between parameter sets (`reset()` without `clearBindings()`) → **silent data corruption** for variable-arity sets |
| [ ] | EM-02 | Low | High | First set not reset; relies on a fresh Statement (public `bind_exec` footgun) |
| [ ] | EM-03 | Low | High | `bind_exec`/`reset_bind_exec` pollute `SQLite` namespace, used-before-definition, `apQuery` mis-prefixed |
| [ ] | EM-04 | Info | High | `execute_many` requires ≥1 set, undocumented |
| [ ] | EM-05 | Info | High | No use-after-move despite forwarding chain (verified) |
| [ ] | EM-06 | Info | High | C++14 gating correct |
| [ ] | EM-07 | Info | High | _(3rd pass)_ ExecuteMany_test.cpp:2 Doxygen header mislabeled `@file VariadicBind_test.cpp` |

### VariadicBind — `code-review/VariadicBind-review.md` (L3 I2)
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | VB-01 | Low | High | `std::forward<decltype(args)>` is a misleading no-op (params are `const&`) |
| [ ] | VB-03 | Low | High | Single-tuple call relies on overload partial-ordering (latent fragility) |
| [ ] | VB-04 | Low | Med | Doc examples use non-SQL `&&`; omit the always-copy (`SQLITE_TRANSIENT`) caveat |
| [ ] | VB-02 / VB-05 | Info | High | `void` return (no `[[nodiscard]]`); C++14 gate duplicated 3× |
| [ ] | VB-06 | Low | High | _(3rd pass)_ Tuple+`index_sequence` helper is a public `SQLite::bind` overload — user-supplied wrong sequence silently binds wrong positions (not covered by EM-03/fix #25) |

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
| [ ] | HDR-09 | Low | High | _(3rd pass)_ SQLiteCppExport.h `#pragma warning(disable:4251/4275)` lacks push/pop — suppression leaks into all consumer TUs (activates once HDR-04 is fixed) |
| [ ] | HDR-10 | Low | Med | _(3rd pass)_ MinGW shared builds excluded from the `__declspec` path (`!defined(__GNUC__)`); works only via `--export-all-symbols` default |
| [ ] | HDR-11 | Low | High | _(3rd pass)_ `SQLITECPP_ASSERT` evaluation divergence: handler path always evaluates the expression (even NDEBUG), assert path evaluates nothing under NDEBUG — side-effect/config hazard (HDR-01's dangling-else also now compile-confirmed via `-Wdangling-else`) |
| [ ] | HDR-12 | Info | High | _(3rd pass)_ Assertion.h macros defined lexically inside `namespace SQLite {}` — false scoping impression (macros ignore namespaces) |

### Build / CI / supply chain — `code-review/findings-build-examples.md` (H1 M2 L5) — _new unit, 2nd review pass_
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | BLD-001 | High | High | `meson.build:132` appends to undeclared `sqlitecpp_cxx_flags` (rest of file uses `sqlitecpp_args`) → `meson setup -DSQLITECPP_DISABLE_STD_FILESYSTEM=true` aborts. Option unusable; no Meson CI job sets it |
| [ ] | BLD-002 | Med | High | googletest submodule pinned to a stale 2018 commit (`release-1.8.0-3518-g6910c9d9`), divergent from the Meson `gtest.wrap` (1.15.0); Dependabot tracks only `github-actions` |
| [ ] | BLD-003 | Med | High | All GitHub Actions pinned to mutable tags (`@v4`, `@v1`/`@v2`); `coverity.yml` hands `COVERITY_SCAN_TOKEN` to an unpinned third-party action; no `permissions:` blocks |
| [ ] | BLD-004 | Low | Med | Dead `.travis.yml` still ships an encrypted Coverity token; `appveyor.yml` lists retired VS2015 images |
| [ ] | BLD-005 | Low | Med | `coverage.yml` / `coverity.yml` lack least-privilege `permissions:` blocks |
| [ ] | BLD-006 | Low | Med | `SQLITECPP_USE_ASAN` forces GCC `-fuse-ld=gold` (absent on many toolchains incl. Windows MinGW); no Actions job runs sanitizers (corroborates the verification ASAN gap) |
| [ ] | BLD-007 | Low | Med | `examples/example2/CMakeLists.txt` uses `CACHE BOOL "" FORCE` → clobbers consuming-project options; bad embed template |
| [ ] | BLD-008 | Low | Med | `sqlite3/CMakeLists.txt` shared-build export macro is directory-global + export-only (`add_definitions(-DSQLITE_API=__declspec(dllexport))`) |
| [ ] | BLD-009 | Low | High | _(3rd pass)_ `sqlite3/CMakeLists.txt:72-79` installs the internal sqlite3 lib + bare `sqlite3.h` into the include root **unconditionally** — ignores `SQLITECPP_INSTALL=OFF`, can shadow/clash with a system SQLite |
| [ ] | BLD-010 | Low | High | _(3rd pass)_ `install(EXPORT)` has no `NAMESPACE` — consumers get bare global targets `SQLiteCpp`/`sqlite3` instead of `SQLiteCpp::SQLiteCpp` (collision-prone; breaking to fix) |
| [ ] | BLD-011 | Low | Med | _(3rd pass)_ `SOVERSION` hardcoded 0 with no ABI policy while 3.x changes public headers (#558 changed `Header` layout) — silent ABI drift under one soname |
| [ ] | BLD-012 | Info | High | _(3rd pass)_ Build-system default divergence: Meson forces C++17/warning_level=3, CMake defaults C++11/hand-rolled flags — CI never cross-covers the standards (masks TST-001) |

### Examples — `code-review/findings-build-examples.md` (M2 L1) — _new unit, 2nd review pass_
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | EXM-001 | Med | High | `examples/example1/main.cpp:415-419` writes `buffer[size]='\0'` where `fread` can fill the buffer → 1-byte out-of-bounds **stack write**; `static_assert` only checks the logo size, not the `fread` cap |
| [ ] | EXM-002 | Med | High | `examples/example1/main.cpp:441-462` leaks the `FILE*` on the no-row / exception paths (`fclose` only inside `if (executeStep())`) — leaky idiom users copy |
| [ ] | EXM-003 | Low | High | `examples/example2/src/main.cpp:53,57,61` use double-quoted SQL string literals → break under `SQLITE_DQS=0` (example1 correctly uses single quotes) |

### Tests — _new, 2nd review pass_
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | TST-001 | Low | High | All `tests/*_test.cpp` use the C++17 `[[maybe_unused]]` attribute while the project default is C++11 → clang emits `-Wc++17-attribute-extensions` per `TEST(...)`; a `-Werror` C++11 build would fail. g++ accepts it silently |
| [ ] | TST-002 | Info | Med | _(3rd pass)_ Suites hardcode relative db filenames (44× `"test.db3"`) with success-path-only `remove()` — stale files poison later runs; blocks parallel execution from one directory |
| [ ] | TST-003 | Info | High | _(3rd pass)_ `Database_test.cpp:551` signed/unsigned `EXPECT_EQ(h.userVersion, 12345)` — the only warning in an otherwise clean strict build; breaks `-Werror` |

### Cross-cutting themes
- **Null C-pointer → `std::string`/`runtime_error` UB**: STMT-01, STMT-02, EXC-02, DB-04 (and the read in DB-01). A small, uniform "guard the pointer" pattern fixes all. _(STMT-01, STMT-02, EXC-02, DB-04 fixed in #552; DB-01 fixed in #553.)_
- **Destructor error handling / terminal-state flags**: TXN-01, TXN-04, SP-02, SP-03 — library-wide habit of swallowing all exceptions and lacking a "finished" flag; route through `SQLITECPP_ASSERT`, broaden to `catch(...)`, track terminal state.
- **`SQLITECPP_NODISCARD` macro**: requested by STMT-08, COL-09, BKP-01/03, EXC-03 — one feature-gated macro (like the existing `SQLITECPP_PURE_FUNC`) serves all; functionally important for `Backup::executeStep`.
- **Fixed-width / sign correctness in header parsing**: DB-02 + DB-03. _(Both fixed in #558: `getHeaderInfo()` now assembles via a `uint32_t`-casting `readBE32` helper, and `Header` uses fixed-width `<cstdint>` types.)_
- **Move-semantics consistency**: BKP-02, SP-05, TXN-05.
- **Throw-from-destructor not propagated** _(2nd pass)_: SP-03 was fixed on this branch (afa51d3) but the identical defect remains in `Transaction` (**TXN-08**). One `catch(...)` finishes the job the branch started.
- **Supply-chain / CI hardening** _(2nd pass, new scope)_: BLD-002 (stale gtest), BLD-003/BLD-005 (pin actions to SHAs + add `permissions:`), BLD-004 (dead Travis token), BLD-006 (gold linker + add a Linux ASan/UBSan CI job — also closes the verification ASAN gap).
- **Examples teach footguns** _(2nd pass, new scope)_: EXM-001 (OOB write), EXM-002 (`FILE*` leak), EXM-003 (double-quoted SQL) are copy-paste hazards in canonical teaching code.
- **The #552 null-guard sweep was incomplete** _(3rd pass)_: STMT-14 (ctor `mQuery`), STMT-15 (`getColumnIndex` map key) and the moved-from hole STMT-13 (`getChanges(nullptr)`) are the same one-line pattern — one more pass closes the class for good.
- **Fix-induced regressions are now the top risk** _(3rd pass)_: SP-09 (from afa51d3) and COL-12 (latent, in open ranked fix #23) both came from the fix process, not the original code — destructor-semantics and UTF-16 regression tests should gate future fixes.
- **Raw file probes vs SQLite's VFS** _(3rd pass)_: DB-15/16/17 (+ prior DB-01/09) — `isUnencrypted`/`getHeaderInfo` re-implement file access with `ifstream` and diverge from the connection (encoding, URI, special names); one shared raw-header reader (or PRAGMA-based derivation) fixes the family.
- **Error-code slot consistency** _(3rd pass)_: BKP-08 + EXC-01 — primary vs extended code conventions differ per throw site; one shared Exception-filling helper fixes both.
- **Packaging polish** _(3rd pass, new scope)_: BLD-009/010/011 — unconditional sqlite3 install, unnamespaced export, frozen SOVERSION; cheap fixes but two are breaking for downstream consumers.

## 4. Ranked Fixes

Ranked by Severity × Likelihood × (inverse) Effort, with confidence. **P0** = correctness/UB/security, small & safe; **P1** = correctness/robustness worth doing; **P2** = API/modernization/docs/build hygiene.

### P0 — fix first (correctness / UB / security; low effort, high confidence)
| Done | # | Fix | Finding(s) | Files | Effort |
|:--:|---|-----|-----------|-------|--------|
| [x] #553 | 1 | Read 16 raw bytes (`read`+`gcount`+`memcmp`) instead of `getline()` in `isUnencrypted()` (reuse `getHeaderInfo` pattern) | DB-01 | `src/Database.cpp` | S |
| [x] #552 | 2 | Guard `sqlite3_expanded_sql()` NULL before constructing `std::string` (throw or empty) | STMT-01 | `src/Statement.cpp` | S |
| [x] #554 | 3 | Add `clearBindings()` to `reset_bind_exec()`; add decreasing-arity regression test | EM-01 | `include/SQLiteCpp/ExecuteMany.h`, `tests/ExecuteMany_test.cpp` | S |
| [x] #552 | 4 | Skip NULL `sqlite3_column_name()` when building the column-name map | STMT-02 | `src/Statement.cpp` | S |
| [x] #552 | 5 | Guard null message in `Exception(const char*, int)` (`msg ? msg : ""`) | EXC-02 | `src/Exception.cpp` | S |
| [x] #552 | 6 | Guard null `apFilename` in the raw-`const char*` `Database` ctor | DB-04 | `src/Database.cpp` | S |
| [ ] | 47 | _(3rd pass)_ `~Savepoint`: always `ROLLBACK TO` before `RELEASE` (drop/reset the `mbRolledBack` skip) + write-after-rollbackTo regression test — **fixes the afa51d3 regression** | SP-09 | `src/Savepoint.cpp`, `tests/Savepoint_test.cpp` | S |
| [ ] | 48 | _(3rd pass)_ Null-guard `Statement` ctor query + `getColumnIndex(nullptr)` — completes the #552 class | STMT-14, STMT-15 | `src/Statement.cpp` | S |
| [ ] | 49 | _(3rd pass)_ `getChanges()`: `return mpSQLite ? sqlite3_changes(mpSQLite) : 0;` | STMT-13 | `src/Statement.cpp` | S |

### P1 — should fix (correctness / robustness / UB-by-design)
| Done | # | Fix | Finding(s) | Files | Effort |
|:--:|---|-----|-----------|-------|--------|
| [x] #558 | 7 | Cast bytes to `uint32_t` before shifting in `getHeaderInfo()`; move `Header` to fixed-width types | DB-02, DB-03 | `src/Database.cpp`, `include/SQLiteCpp/Database.h` | M |
| [x] #556 | 8 | Fix `operator<<` byte-count/eval-order (use `getString()` or sequence text-then-bytes) + BLOB test | COL-01 | `src/Column.cpp`, `tests/Column_test.cpp` | S |
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
| [x] #559 | 32 | _(2nd pass)_ Broaden `~Transaction()` to `catch(...)` — finishes the SP-03 fix that wasn't propagated | TXN-08 | `src/Transaction.cpp` | S |
| [ ] | 33 | _(2nd pass)_ `meson.build`: `sqlitecpp_cxx_flags` → `sqlitecpp_args` (Meson build broken for `SQLITECPP_DISABLE_STD_FILESYSTEM`) | BLD-001 | `meson.build` | S |
| [ ] | 34 | _(2nd pass)_ example1: cap `fread` at `sizeof(buffer)-1` (or drop NUL write) — fixes 1-byte stack overflow | EXM-001 | `examples/example1/main.cpp` | S |
| [ ] | 35 | _(2nd pass)_ example1: RAII the `FILE*` (or unconditional `fclose`) — fixes leak on no-row/exception path | EXM-002 | `examples/example1/main.cpp` | S |
| [ ] | 36 | _(2nd pass)_ Document + enforce "no outstanding statements" on `Database` move/close (assert count==0) | DB-12 | `include/SQLiteCpp/Database.h` | M |
| [ ] | 50 | _(3rd pass)_ Switch the `Database` Deleter to `sqlite3_close_v2` (zombie-mode close, no lock leak) — pairs with #36 — **behavior change, flag** | DB-14 | `src/Database.cpp` | S |
| [ ] | 51 | _(3rd pass)_ Detect trailing SQL via a real `pzTail` in `prepareStatement()`; throw or document single-statement truncation + regression test — **breaking if throwing** | STMT-12 | `src/Statement.cpp`, `include/SQLiteCpp/Statement.h` | S |
| [ ] | 52 | _(3rd pass)_ Open raw-header probes with a wide/`filesystem::path` stream on Windows so they probe the same file as the connection | DB-16 | `src/Database.cpp` | M |
| [ ] | 53 | _(3rd pass)_ `Backup::executeStep()`: primary code → `getErrorCode()`, extended → `getExtendedErrorCode()` (shared helper with EXC-01) — **test churn, flag** | BKP-08 | `src/Backup.cpp`, `src/Exception.cpp` | S |
| [ ] | 54 | _(3rd pass)_ `getHeaderInfo()`: compare all 16 magic bytes (memcmp, no NUL overwrite) | DB-15 | `src/Database.cpp` | S |
| [ ] | 55 | _(3rd pass)_ Decode page-size special value 1 → 65536 (fold into #44) | DB-22, DB-13 | `src/Database.cpp`, `include/SQLiteCpp/Database.h` | S |
| [ ] | 56 | _(3rd pass)_ Rewrite `getText`/`getBlob` lifetime `@warning`s (type-conversion, step/reset invalidation) | COL-11 | `include/SQLiteCpp/Column.h` | S |
| [ ] | 57 | _(3rd pass)_ Amend fix #23 per COL-12: keep the leading `sqlite3_column_bytes()` in `getString()` (UTF-16 conversion is load-bearing) — null-guard only | COL-12 | CODE_REVIEW.md #23, `src/Column.cpp` | S |
| [ ] | 58 | _(3rd pass)_ Fix `SQLITECPP_DISABLE_STD_FILESYSTEM` to also disable the experimental branch; remove unreachable `#undef` | DB-19 | `include/SQLiteCpp/Database.h` | S |

### P2 — nice to have (API / modernization / docs / build hygiene)
| Done | # | Fix | Finding(s) | Effort |
|:--:|---|-----|-----------|--------|
| [ ] | 19 | Add a feature-gated `SQLITECPP_NODISCARD` macro and apply to pure getters/return-bearing methods | STMT-08, COL-09, BKP-03, EXC-03 | M |
| [ ] | 20 | Fix `loadExtension()` to capture & free SQLite's `pzErrMsg` | DB-06 | S |
| [ ] | 21 | Reconcile `key()`/`rekey()` const-correctness + `passLen` consistency | DB-05, DB-08 | S |
| [ ] | 22 | Default move ops for `Backup`/`Savepoint`/`Transaction` (or document the seal) | BKP-02, SP-05, TXN-05 | S |
| [ ] | 23 | Simplify `getString()` to the documented blob-then-bytes 2-call form; guard null — **⚠ amended by COL-12/#57 _(3rd pass)_: do NOT drop the leading `sqlite3_column_bytes()` (it forces UTF-8 conversion on UTF-16 DBs); null-guard only** | COL-05, COL-06, COL-12 | S |
| [ ] | 24 | `Column` implicit-conversion narrowing — document / consider `explicit` (major) + `getUInt` low-32 doc | COL-04, COL-03 | M |
| [ ] | 25 | Drop misleading `std::forward` in `VariadicBind`; fix doc `&&`→`AND`; move `ExecuteMany` helpers to `detail` | VB-01, VB-04, EM-02, EM-03 | S |
| [ ] | 26 | `SQLiteCppExport.h`: `#if defined(SQLITECPP_DLL_EXPORT)`, unify `_WIN32`, fix comment | HDR-03, HDR-04 | S |
| [ ] | 27 | Guard/remove MSVC `#define __func__` | HDR-05 | S |
| [ ] | 28 | Add missing headers to umbrella (or document the omission) | HDR-06 | S |
| [ ] | 29 | Version string `"3.03.03"` → `"3.3.3"` (sync with `sqlitecpp-release`) | HDR-07 | S |
| [ ] | 30 | `backup()` `Load` should open source `READONLY`; seed `Exception` extended code from `ret`; doc/typo fixes | DB-11, EXC-01, EXC-06 | S |
| [ ] | 31 | Normalize line endings to LF / address cpplint style set | cpplint | S |
| [ ] | 37 | _(2nd pass)_ Pin GitHub Actions to commit SHAs; add least-privilege `permissions:` blocks | BLD-003, BLD-005 | M |
| [ ] | 38 | _(2nd pass)_ Bump googletest submodule to a current tag (≈1.15.0, matching the wrap) | BLD-002 | S |
| [ ] | 39 | _(2nd pass)_ Drop forced `-fuse-ld=gold`; add a Linux ASan/UBSan CI job (closes the verification ASAN gap) | BLD-006 | M |
| [ ] | 40 | _(2nd pass)_ Remove dead `.travis.yml` (rotate the embedded Coverity token) + retired `appveyor.yml` images | BLD-004 | S |
| [ ] | 41 | _(2nd pass)_ example2: single-quote SQL string literals (DQS-safe) | EXM-003 | S |
| [ ] | 42 | _(2nd pass)_ Friendlier error when binding an unknown parameter name | STMT-11 | S |
| [ ] | 43 | _(2nd pass)_ Document Backup page-counts are 0 before the first `executeStep()` | BKP-07 | S |
| [ ] | 44 | _(2nd pass)_ `defaultPageCacheSizeBytes` → signed `int32_t` (negative = KiB) | DB-13 | S |
| [ ] | 45 | _(2nd pass)_ Raise tests to C++17 or drop C++17 `[[maybe_unused]]` at C++11 | TST-001 | S |
| [ ] | 46 | _(2nd pass)_ `examples/example2/CMakeLists.txt`: drop `FORCE`; `sqlite3` export via `target_compile_definitions(... PRIVATE)` | BLD-007, BLD-008 | S |
| [ ] | 59 | _(3rd pass)_ Document Transaction non-nestability (→ Savepoint); document/reject mid-row `exec()` | TXN-09, STMT-17 | S |
| [ ] | 60 | _(3rd pass)_ Refresh `mColumnCount` after step / clear name map on `reset()` (or document freeze-at-prepare) | STMT-16 | S |
| [ ] | 61 | _(3rd pass)_ Drop `SQLITECPP_PURE_FUNC` from throwing `getIndex()` | STMT-18 | S |
| [ ] | 62 | _(3rd pass)_ Typos: "reseted"→"reset" (×2); `incrementalVaccumMode` (deprecate+rename, **breaking**); COL-10 doc; EM-07 file header | STMT-19, DB-21, COL-10, EM-07 | S |
| [ ] | 63 | _(3rd pass)_ Document `executeStep(0)` no-op (or treat ≤0 as "all remaining pages") | BKP-09 | S |
| [ ] | 64 | _(3rd pass)_ Document `isUnencrypted`/`getHeaderInfo` plain-path-only contract (no URI/:memory:/temp) | DB-17 | S |
| [ ] | 65 | _(3rd pass)_ `#error`/`#warning` instead of silent `OPEN_NOFOLLOW = 0` on pre-3.31 SQLite | DB-18 | S |
| [ ] | 66 | _(3rd pass)_ Replace the `namespace std` filesystem-alias injection with a library-local alias | DB-20 | M |
| [ ] | 67 | _(3rd pass)_ Push/pop MSVC warning-disable pragmas; include MinGW in the dllexport path | HDR-09, HDR-10 | S |
| [ ] | 68 | _(3rd pass)_ Make `SQLITECPP_ASSERT` handler path NDEBUG-consistent; move macros out of `namespace SQLite {}` | HDR-11, HDR-12 | S |
| [ ] | 69 | _(3rd pass)_ Move the tuple/index_sequence bind helper into `detail` (with #25) — **breaking for direct users** | VB-06 | S |
| [ ] | 70 | _(3rd pass)_ Gate sqlite3 install on `SQLITECPP_INSTALL`; add `NAMESPACE SQLiteCpp::` to the export; set SOVERSION policy — **breaking (packaging)** | BLD-009, BLD-010, BLD-011 | M |
| [ ] | 71 | _(3rd pass)_ Align/matrix the C++ standard across CMake and Meson CI | BLD-012, TST-001 | S |
| [ ] | 72 | _(3rd pass)_ Test hygiene: unique/RAII temp db files; fix `Database_test.cpp:551` sign-compare (`12345u`) | TST-002, TST-003 | S |

> **Note on EXC-01 / EXC-related test churn:** seeding the extended code from `ret` changes existing `tests/Exception_test.cpp` expectations (`-1`), so it's a deliberate API decision to confirm before applying.

---
_Per-unit detail: see the `code-review/` folder (`<Unit>-review.md` for passes 1–2, `findings3-*.md` for the 3rd pass). Verification logs: `code-review/verification.md` (Windows), `code-review/verification_2.md`, `code-review/verification_3.md` (Linux + sanitizers). Full pass reports: `CODE_REVIEW_2.md`, `CODE_REVIEW_3.md`._
