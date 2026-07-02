# SQLiteCpp Deep Code Review — 3rd independent pass

_Generated 2026-07-02. Scope: wrapper sources (include/SQLiteCpp/*.h, src/*.cpp), build/CI, examples, tests; bundled sqlite3 amalgamation and googletest excluded (vendored). Baseline: current tree with all fixes through #560 (`a6537fe`) merged — `git diff` of src/include/tests/examples since the 2nd pass is empty except those fixes._

> **Relationship to prior passes:** this pass (a) verified every previously ticked fix in the actual code, (b) re-confirmed every still-open finding, and (c) hunted **new** issues only. Full per-unit detail in `code-review/findings3-*.md`; triage in `code-review/triage_3.md`; verification log in `code-review/verification_3.md`. Every finding/fix row starts with a `[ ]` Done checkbox — flip to `[x]` (with PR/commit) when the fix lands.

**Headline results**
- **All 13 previously ticked fixes verified genuinely fixed in the tree** (#552, #553, #554, #556, #558, #559/5e4908a, afa51d3/4ee4be1, #560) — with **one behavioral regression found in the afa51d3 Savepoint fix (SP-09, High)**.
- **New verification coverage:** first-ever ASan+UBSan+LeakSanitizer run (Linux) — **clean** over 60/60 tests at 99.3–100 % line coverage of src/*.cpp; closes the ASAN gap recorded by both prior passes.
- **39 new findings:** 1 High, 8 Medium, 20 Low, 10 Info. No critical issues; no SQL injection; no leaks. The new Mediums cluster around **moved-from/null-pointer holes missed by the #552 sweep**, **connection-handle close semantics**, and **Windows/encoding portability of raw file probes**.

## 1. Triage matrix (delta vs prior passes)

Full matrix: `code-review/triage_3.md`. Post-P0-fix risk deltas vs the CODE_REVIEW.md matrix: Database bug-risk 4→3, Statement 4→3, Column 3→2, Savepoint 3→2, Exception 2→1 (P0/UB items fixed). Review order used: Database → Statement → Column → Transaction+Savepoint → Backup+Exception → templates/support headers → build/CI/examples/tests. A test-coverage-gap axis was added this pass; coverage is excellent overall (gcov ≈100 % of src) — gaps are semantic, not line-level (e.g. no test writes after `rollbackTo()`, none exercises multi-statement SQL in `Statement`).

## 2. Verification results (Linux, this pass)

Environment: Ubuntu 22 sandbox, gcc 11.4. CMake unavailable and package installs blocked (proxy) — build replicated with a hand-written out-of-tree Makefile matching the CMake flags (bundled sqlite3.c + src at C++11, tests at C++14 for gtest 1.16, `-DSQLITE_ENABLE_COLUMN_METADATA`). Full log: `code-review/verification_3.md`.

| Check | Result |
|-------|--------|
| Build (gcc 11.4, `-Wall -Wextra -Wpedantic -Wshadow -Wswitch-enum`) | **OK** |
| Warnings — wrapper (src/ + include/) and example1 | **0** |
| Warnings — tests | **1** benign `-Wsign-compare` (`Database_test.cpp:551`, → TST-003) |
| Unit tests (run directly; ctest unavailable) | **60/60 passed** (10 suites) |
| **ASan + UBSan** (`-fsanitize=address,undefined -fno-sanitize-recover=all`, uniform; repo's broken `SQLITECPP_USE_ASAN` avoided) | **Clean** — 60/60 pass, zero reports, zero leaks (`detect_leaks=1`); example1 clean. First sanitizer run in any pass. |
| Coverage (gcov) | src/*.cpp line coverage **99.3–100 %** per file |
| cppcheck / clang-tidy | **Could not run** — not installed, installs blocked by proxy. Honest gap. |
| Not covered | C++17/std::filesystem configuration build; branch coverage; Meson build |

## 3. Fix verification of previously ticked items

| Item(s) | Status in current tree |
|---------|------------------------|
| DB-01/02/03/04 (#553, #558, #552) | **Fixed** (src/Database.cpp:273-291, 332-360, 67; Database.h:123-146) with regression tests. Residue: DB-01 fix returns false (not throw) on short files and copies the DB-09 close-before-gcount pattern. |
| STMT-01/02 (#552) | **Fixed** (src/Statement.cpp:354-356, 288-292). STMT-01 chose silent-empty fallback (defined behavior). |
| COL-01 (#556) | **Fixed** — `operator<<` now materializes once via `getString()` (src/Column.cpp:117-122) + 3 regression tests. Residue: doc comment still says "using getText()" (→ COL-10). |
| SP-02/SP-03 (afa51d3/4ee4be1) | **Fixed as described** — but the `mbRolledBack` skip in the destructor introduced regression **SP-09 (High, below)**. |
| TXN-08 (#559/5e4908a) | **Fixed** — `~Transaction` fully `catch(...)`-guarded; no statement outside the try (SQL is a `const char*` literal). |
| EXC-02 (#552) | **Fixed** (src/Exception.cpp:19) + regression test. |
| EM-01 (#554) | **Fixed** — `reset()` + `clearBindings()` (ExecuteMany.h:68-70) + decreasing-arity regression test. |
| BLD-001 (#560) | **Fixed** — meson.build:132 uses `sqlitecpp_args`. |

All still-open prior findings (DB-05..13, STMT-03..11, COL-02..09, SP-01/04..08, TXN-01..07, BKP-01..07, EXC-01/03..07, VB-*, EM-02..06, HDR-01..08, BLD-002..008, EXM-001..003, TST-001) were re-checked: **all remain valid as recorded**. Notable refinements: BLD-005 is broader (none of the 6 workflows has `permissions:`); HDR-01's dangling-else is now compile-confirmed (`g++ -Wdangling-else` fires); BKP-04's *proposed* fix is invalid (`sqlite3_backup_step` never posts errors to the destination connection — reading the dest handle would fetch stale state); and open ranked fix #23 is a latent regression (→ COL-12).

## 4. New findings (this pass)

### Savepoint / Transaction — `code-review/findings3-transaction-savepoint.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | SP-09 | **High** | High | **Regression introduced by afa51d3:** work written after a manual `rollbackTo()` is silently **committed** on scope exit — `~Savepoint` skips `ROLLBACK TO` when `mbRolledBack` is set (src/Savepoint.cpp:43-47) and goes straight to `RELEASE`. Pre-fix behavior (and the documented auto-rollback contract) discarded it. Breaks the rollback-retry pattern. Fix: destructor should always `ROLLBACK TO` before `RELEASE` (rolling back twice is harmless), or clear `mbRolledBack` — plus a write-after-rollbackTo regression test. |
| [ ] | TXN-09 | Low | High | Nested `Transaction` (BEGIN inside BEGIN) throws at construction with a raw SQLite error; non-nestability and the `Savepoint` alternative are undocumented in Transaction.h. |

### Database — `code-review/findings3-database.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | DB-14 | Med | High | Deleter uses `sqlite3_close` (not `close_v2`) — if any `Statement` outlives the `Database`, close returns `SQLITE_BUSY`, the code swallows it, and the connection + file locks **leak permanently** (no zombie-mode cleanup). |
| [ ] | DB-16 | Med | High | Windows: `isUnencrypted()`/`getHeaderInfo()` open the UTF-8 filename via ANSI-codepage `std::ifstream` — for non-ASCII paths they probe a **different file** than the connection uses. |
| [ ] | DB-15 | Low | High | `getHeaderInfo()` magic check compares only 15 bytes and forces `headerStr[15]='\0'` — accepts invalid headers; inconsistent with the fixed `isUnencrypted()`. |
| [ ] | DB-17 | Low | High | `isUnencrypted()`/`getHeaderInfo()` docs claim "path/uri" but `ifstream` fails on URI/`:memory:`/temp filenames with misleading errors. |
| [ ] | DB-18 | Low | High | `OPEN_NOFOLLOW` silently defined as 0 on pre-3.31 SQLite — symlink protection silently absent instead of a compile error. |
| [ ] | DB-19 | Low | High | `SQLITECPP_DISABLE_STD_FILESYSTEM` is ignored when `SQLITECPP_HAVE_STD_EXPERIMENTAL_FILESYSTEM` is defined (disable block sits inside the wrong `#ifndef` branch; unreachable `#undef` at Database.h:42-44). |
| [ ] | DB-22 | Low | High | `pageSizeBytes` doesn't decode the file-format special value 1 = 65536 — 64 KiB-page databases report page size 1 (refines DB-13). |
| [ ] | DB-20 | Info | High | `namespace std { namespace filesystem = experimental::filesystem; }` injection into `std` is formally UB and leaks into every consumer TU. |
| [ ] | DB-21 | Info | High | Public `Header` field `incrementalVaccumMode` typo ("Vaccum") locked into the API. |

### Statement — `code-review/findings3-statement.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | STMT-12 | Med | High | `sqlite3_prepare_v2(..., nullptr)` pzTail: multi-statement SQL is **silently truncated to the first statement** — `Statement(db, "DELETE FROM a; DELETE FROM b;")` drops the rest with no error, undocumented, while `Database::exec()` runs all (silent data loss for migrating users). |
| [ ] | STMT-13 | Med | High | `getChanges()` on a moved-from `Statement` calls `sqlite3_changes(nullptr)` → null deref inside a `noexcept` method (API_ARMOR not defined in this repo's builds); the one inconsistent hole in the class's otherwise verified moved-from safety. |
| [ ] | STMT-14 | Med | High | `Statement(db, (const char*)nullptr)` constructs `std::string` from NULL → UB. The exact #552 defect class (DB-04/EXC-02), missed in the sweep. |
| [ ] | STMT-15 | Low | High | `getColumnIndex(nullptr)` / `getColumn(nullptr)` / `isColumnNull(nullptr)` build a `std::string` map key from NULL → UB (bind-by-name path is safe by contrast). |
| [ ] | STMT-16 | Low | High | `mColumnCount`/`mColumnNames` frozen at prepare time — schema-change auto-reprepare (prepare_v2 semantics) can change SELECT * column count/names; stale guard throws for genuinely present columns. |
| [ ] | STMT-17 | Low | High | `exec()` called on the last pending row of a SELECT succeeds and returns an unrelated connection-wide change count (misuse detected only when another row follows; compounds STMT-03). |
| [ ] | STMT-18 | Low | Med | `SQLITECPP_PURE_FUNC` on throwing `getIndex()` violates GCC `pure` contract — optimizer may CSE/delete calls, eliding the expected exception (latent miscompilation trap). |
| [ ] | STMT-19 | Info | High | Exception-message typo "needs to be reseted" (×2, src/Statement.cpp:175,198). |

### Column — `code-review/findings3-column.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | COL-11 | Med | High | `getText()`/`getBlob()` `@warning` understates pointer lifetime: omits invalidation by subsequent **type conversions** (getBlob-then-getText can realloc) and by `executeStep()`/`reset()` — a silent use-after-free enabler. |
| [ ] | COL-12 | Med | High | **Open ranked fix #23 (COL-05's 2-call `getString()`) is a latent regression**: the leading `sqlite3_column_bytes()` is load-bearing — it forces UTF-8 conversion on UTF-16 databases (verified against amalgamation internals; `Column.basis16` test would fail). Amend #23 before anyone implements it. |
| [ ] | COL-10 | Low | High | `operator<<` doc comment (Column.h:235-244) still says "using getText()" — stale after #556 switched to `getString()`. |
| [ ] | COL-13 | Low | Med | `getName()`/`getOriginName()` can return nullptr (OOM) and are invalidated by reprepare/next call — undocumented; the library's own test idiom `std::string(getName())` is UB on that path. |

### Backup / Exception — `code-review/findings3-backup-exception.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | BKP-08 | Med | High | `executeStep()` throw puts SQLite **extended** codes (e.g. `SQLITE_IOERR_WRITE`=778) in `getErrorCode()` while `getExtendedErrorCode()` stays -1 — primary/extended slots inverted vs the rest of the library. |
| [ ] | BKP-09 | Low | High | `executeStep(0)` is an undocumented no-op returning `SQLITE_OK` forever — never `SQLITE_DONE`; infinite retry-loop hazard. |
| [ ] | BKP-10 | Info | High | Redundantly qualified `SQLite::Backup::Deleter::operator()` definition inside `namespace SQLite` (src/Backup.cpp:75). |
| [ ] | EXC-08 | Info | High | `std::string` ctors truncate messages at embedded NUL (delegate via `.c_str()` instead of `std::runtime_error(const std::string&)`). |

### Templates / support headers — `code-review/findings3-templates-headers.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | VB-06 | Low | High | Tuple+`index_sequence` helper is a public `SQLite::bind` overload — a user-supplied wrong sequence silently binds wrong positions (not covered by EM-03/fix #25). |
| [ ] | HDR-09 | Low | High | SQLiteCppExport.h `#pragma warning(disable:4251/4275)` lacks push/pop — suppression leaks into all consumer TUs (activates once HDR-04 is fixed). |
| [ ] | HDR-10 | Low | Med | MinGW shared builds excluded from the `__declspec` path (`!defined(__GNUC__)`); PE-meaningless visibility attribute — works only via `--export-all-symbols` default. |
| [ ] | HDR-11 | Low | High | `SQLITECPP_ASSERT` evaluation divergence: handler path always evaluates the expression (even NDEBUG), assert path evaluates nothing under NDEBUG — side-effect/config hazard (sole current use is pure). |
| [ ] | HDR-12 | Info | High | Assertion.h macros defined lexically inside `namespace SQLite {}` — false scoping impression (macros ignore namespaces). |
| [ ] | EM-07 | Info | High | ExecuteMany_test.cpp:2 Doxygen header mislabeled `@file VariadicBind_test.cpp`. |

### Build / CI / tests — `code-review/findings3-build-examples-tests.md`
| Done | ID | Sev | Conf | Title |
|:--:|----|-----|------|-------|
| [ ] | BLD-009 | Low | High | `sqlite3/CMakeLists.txt` installs the internal sqlite3 lib + bare `sqlite3.h` into the include root **unconditionally** — ignores `SQLITECPP_INSTALL=OFF`, can shadow/clash with a system SQLite of a different version. |
| [ ] | BLD-010 | Low | High | `install(EXPORT)` has no `NAMESPACE` — consumers get bare global targets `SQLiteCpp`/`sqlite3` instead of `SQLiteCpp::SQLiteCpp`; `sqlite3` name collisions are common. (Breaking to fix — flag.) |
| [ ] | BLD-011 | Low | Med | `SOVERSION` hardcoded 0 with no ABI policy while 3.x changes public headers (#558 changed `Header` layout) — silent ABI drift under one soname. |
| [ ] | BLD-012 | Info | High | Build-system default divergence: Meson forces C++17/warning_level=3, CMake defaults C++11/hand-rolled flags — CI never cross-covers the standards (masks TST-001). |
| [ ] | TST-002 | Info | Med | Suites hardcode relative db filenames (44× `"test.db3"`) with success-path-only `remove()` — stale files poison later runs; blocks parallel execution from one directory. |
| [ ] | TST-003 | Info | High | `Database_test.cpp:551` signed/unsigned `EXPECT_EQ` — the only warning in an otherwise clean strict build; breaks `-Werror`. |

### Key verified non-issues (this pass — do not re-flag)
- `createFunction` has **no** std::function trampoline (raw C pointers) — no wrapper-added lifetime/exception risk across the C boundary.
- Internal-sqlite CMake export is complete and correct (`sqlite3` is in the same export set; Config.cmake `find_dependency` logic is right).
- `bind(idx, (const char*)nullptr)` binds SQL NULL per the C API — defined, no guard needed.
- `tryExecuteStep()` synthesizes `SQLITE_MISUSE` itself when done — never relies on version-dependent step-after-DONE semantics; `sqlite3_step(NULL)` is defined (`MISUSE_BKPT`).
- `bindNoCopy(std::string&&)` is `= delete`d in all three forms — dangling-temporary trap already closed.
- VariadicBind 1-based `++pos` indexing correct; `initializer_list` evaluation order guaranteed by [dcl.init.list]/4; empty packs compile clean; headers are self-sufficient.
- `SQLITECPP_VERSION_NUMBER` 3003003 is formula-consistent with CMake/meson/package.xml 3.3.3 (only the display string "3.03.03" is off — HDR-07).
- All moved-from `Statement` raw-handle accessors are null-safe **except** `getChanges()` (STMT-13).

## 5. Cross-cutting themes (this pass)

- **The #552 null-guard sweep missed two spots in Statement** (STMT-14, STMT-15) and one moved-from hole (STMT-13) — one more pass of the same one-line pattern closes the class for good.
- **Fix-induced regressions are now the top risk** — SP-09 (from afa51d3) and COL-12 (latent, in open ranked fix #23). Regression tests around destructor semantics and UTF-16 databases should gate future fixes; the fix process, not the original code, produced this pass's only High.
- **Raw file probes vs SQLite's VFS** (DB-15/16/17 + prior DB-01/09): `isUnencrypted`/`getHeaderInfo` re-implement file access with `ifstream` and diverge from the connection (encoding, URI, special names). Consider one shared, documented raw-header reader — or deriving header info via `PRAGMA` queries where possible.
- **Error-code slot consistency** (BKP-08 + prior EXC-01): primary vs extended code conventions differ per throw site; one helper that fills both from a connection or a return code would fix the family.
- **Packaging polish** (BLD-009/010/011): install/export behavior is correct for the happy path but leaks targets/headers and lacks an ABI policy — cheap fixes, but two are breaking for downstream consumers and need a CHANGELOG note.

## 6. Ranked new fixes (continues central numbering from #46)

**Flags:** (B) breaking / behavior change, (T) changes existing tests.

### P0 — fix first
| Done | # | Fix | Finding(s) | Files | Effort | Flags |
|:--:|---|-----|-----------|-------|--------|-------|
| [ ] | 47 | `~Savepoint`: always `ROLLBACK TO` before `RELEASE` (drop the `mbRolledBack` skip or reset the flag); add write-after-rollbackTo regression test | **SP-09** | `src/Savepoint.cpp`, `tests/Savepoint_test.cpp` | S | (B — restores pre-afa51d3 semantics) |
| [ ] | 48 | Null-guard `Statement` ctor query (`apQuery ? apQuery : ""` or throw) and `getColumnIndex(nullptr)` → completes the #552 class | STMT-14, STMT-15 | `src/Statement.cpp` | S | |
| [ ] | 49 | `getChanges()`: `return mpSQLite ? sqlite3_changes(mpSQLite) : 0;` | STMT-13 | `src/Statement.cpp` | S | |

### P1 — should fix
| Done | # | Fix | Finding(s) | Files | Effort | Flags |
|:--:|---|-----|-----------|-------|--------|-------|
| [ ] | 50 | Switch the `Database` Deleter to `sqlite3_close_v2` (zombie-mode close; no lock leak with outstanding statements) — pairs with DB-12/#36 | DB-14 | `src/Database.cpp` or `Database.h` | S | (B — moved-from/dangling-statement behavior changes) |
| [ ] | 51 | Detect trailing SQL via a real `pzTail` in `prepareStatement()`; throw (or at minimum document single-statement truncation); add 2-statement regression test | STMT-12 | `src/Statement.cpp`, `include/SQLiteCpp/Statement.h` | S | (B if throwing) |
| [ ] | 52 | Open raw-header probes with a wide/`std::filesystem::path` stream on Windows (UTF-8 → UTF-16) so they probe the same file as the connection | DB-16 | `src/Database.cpp` | M | |
| [ ] | 53 | `Backup::executeStep()`: put the primary code in `getErrorCode()`, extended in `getExtendedErrorCode()` (shared Exception helper with EXC-01) | BKP-08 | `src/Backup.cpp`, `src/Exception.cpp` | S | (T — Exception_test expectations) |
| [ ] | 54 | `getHeaderInfo()`: compare all 16 magic bytes (memcmp, no NUL overwrite) | DB-15 | `src/Database.cpp` | S | |
| [ ] | 55 | Decode page-size special value 1 → 65536 (fold into DB-13/#44 signed fix) | DB-22, DB-13 | `src/Database.cpp`, `include/SQLiteCpp/Database.h` | S | |
| [ ] | 56 | Rewrite `getText`/`getBlob` lifetime `@warning`s: invalidated by type conversion, `executeStep()`, `reset()`, destruction | COL-11 | `include/SQLiteCpp/Column.h` | S | |
| [ ] | 57 | **Amend open ranked fix #23**: keep the leading `sqlite3_column_bytes()` (UTF-16 conversion is load-bearing) — guard-null only | COL-12 | CODE_REVIEW.md (#23), later `src/Column.cpp` | S | |
| [ ] | 58 | Fix `SQLITECPP_DISABLE_STD_FILESYSTEM` to also disable the experimental-filesystem branch; remove unreachable `#undef` | DB-19 | `include/SQLiteCpp/Database.h` | S | |

### P2 — nice to have
| Done | # | Fix | Finding(s) | Effort | Flags |
|:--:|---|-----|-----------|--------|-------|
| [ ] | 59 | Document Transaction non-nestability (+ point to Savepoint); document `exec()`-vs-pending-row and reject mid-row `exec()` | TXN-09, STMT-17 | S | (B if throwing) |
| [ ] | 60 | Refresh `mColumnCount` after step / clear name map on `reset()` (or document freeze-at-prepare) | STMT-16 | S | |
| [ ] | 61 | Drop `SQLITECPP_PURE_FUNC` from throwing `getIndex()` | STMT-18 | S | |
| [ ] | 62 | Typos: "reseted"→"reset" (×2), `incrementalVaccumMode` (deprecate+rename), COL-10 doc, EM-07 file header | STMT-19, DB-21, COL-10, EM-07 | S | (B for the Header field rename) |
| [ ] | 63 | Document `executeStep(0)` no-op (or treat ≤0 as "all remaining pages") | BKP-09 | S | |
| [ ] | 64 | Document `isUnencrypted`/`getHeaderInfo` plain-path-only contract (no URI/:memory:/temp) | DB-17 | S | |
| [ ] | 65 | `#error` (or `#warning`) instead of silent `OPEN_NOFOLLOW = 0` on pre-3.31 SQLite | DB-18 | S | |
| [ ] | 66 | Replace the `namespace std` filesystem-alias injection with a library-local alias (`SQLite::fs`) | DB-20 | M | |
| [ ] | 67 | Wrap MSVC warning-disable pragmas in push/pop; include MinGW in the dllexport path | HDR-09, HDR-10 | S | |
| [ ] | 68 | Make `SQLITECPP_ASSERT` handler path NDEBUG-consistent; move macro defs out of `namespace SQLite {}` | HDR-11, HDR-12 | S | |
| [ ] | 69 | Move the tuple/index_sequence bind helper into a `detail` namespace (with EM-03/#25) | VB-06 | S | (B for direct users) |
| [ ] | 70 | Gate sqlite3 install rules on `SQLITECPP_INSTALL`; add `NAMESPACE SQLiteCpp::` to the export; set SOVERSION policy | BLD-009, BLD-010, BLD-011 | M | (B — packaging) |
| [ ] | 71 | Align/matrix the C++ standard across CMake and Meson CI | BLD-012, TST-001 | S | |
| [ ] | 72 | Test hygiene: unique/RAII temp db files; fix `Database_test.cpp:551` sign-compare (`12345u`) | TST-002, TST-003 | S | |

No code changes have been made; this report and its `code-review/findings3-*.md` detail files are the only writes.

---
_Detail: `code-review/findings3-{database,statement,column,transaction-savepoint,backup-exception,templates-headers,build-examples-tests}.md` · Triage: `code-review/triage_3.md` · Verification: `code-review/verification_3.md`_
