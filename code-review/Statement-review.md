# Statement — Review

## Summary
The `Statement` unit (`include/SQLiteCpp/Statement.h`, `src/Statement.cpp`) is a solid, well-tested RAII wrapper. The prepared-statement ownership model (`shared_ptr<sqlite3_stmt>` with a finalizing deleter lambda) is exception-safe and the bind overload set is comprehensive. Two genuine defects exist around possibly-NULL C pointers: `getExpandedSQL()` constructs a `std::string` from a possibly-NULL `sqlite3_expanded_sql()` result (UB), and the lazy column-name map inserts a possibly-NULL `sqlite3_column_name()` pointer. Several lower-severity issues concern an inconsistent / wrong-handle `getChanges()`, a defaulted move-assignment that leaves the moved-from object in a partially-live state (inconsistent with the move constructor), and missing `[[nodiscard]]`/`const`/`noexcept` opportunities.

Finding counts by severity: Critical 0, High 1, Medium 4, Low 4, Info 2.

## Findings

### [STMT-01] `getExpandedSQL()` constructs `std::string` from possibly-NULL pointer (UB)
- **Severity:** High
- **Confidence:** High
- **Category:** bug / security (null-deref / UB)
- **Location:** `src/Statement.cpp:347-356` (the `getExpandedSQL()` body, line 351-352)
- **Description:** The code is:
  ```cpp
  char* expanded = sqlite3_expanded_sql(getPreparedStatement());
  std::string expandedString(expanded);   // UB if expanded == nullptr
  sqlite3_free(expanded);
  return expandedString;
  ```
  Per the SQLite C API contract (`sqlite3/sqlite3.h:4661-4667`), `sqlite3_expanded_sql()` "returns NULL if insufficient memory is available to hold the result, or if the result would exceed the maximum string length determined by the `SQLITE_LIMIT_LENGTH`", and the `SQLITE_OMIT_TRACE` compile-time option "causes sqlite3_expanded_sql() to always return NULL". Constructing `std::string(const char*)` from `nullptr` is undefined behavior (`std::char_traits::length(nullptr)`). `sqlite3_free(nullptr)` is itself harmless, but the `std::string` construction one line earlier already triggered UB.
- **Impact:** A crash (or worse) when expanding very large bound parameters (exceeds `SQLITE_LIMIT_LENGTH`), under memory pressure, or when built against an SQLite compiled with `SQLITE_OMIT_TRACE`. The size/limit path is reachable with attacker-influenced data sizes, so this is partly a robustness/DoS concern, not purely OOM.
- **Proposed fix:** Guard the pointer before constructing the string, e.g.:
  ```cpp
  char* expanded = sqlite3_expanded_sql(getPreparedStatement());
  if (!expanded)
      throw SQLite::Exception("Failed to expand SQL (out of memory or exceeds SQLITE_LIMIT_LENGTH)");
  std::string expandedString(expanded);
  sqlite3_free(expanded);
  return expandedString;
  ```
  (Or `std::string expandedString(expanded ? expanded : "");` if a silent empty result is preferred — but throwing is more consistent with the rest of the class.)

### [STMT-02] Lazy column-name map inserts possibly-NULL `sqlite3_column_name()` pointer (UB)
- **Severity:** Medium
- **Confidence:** Medium
- **Category:** bug / security (null-deref / UB)
- **Location:** `src/Statement.cpp:281-300` (`getColumnIndex`, line 288-289)
- **Description:**
  ```cpp
  const char* pName = sqlite3_column_name(getPreparedStatement(), i);
  mColumnNames[pName] = i;   // std::string key constructed from pName; UB if NULL
  ```
  The SQLite contract (`sqlite3/sqlite3.h:5134-5136`) states `sqlite3_column_name()` returns a NULL pointer "If sqlite3_malloc() fails during the processing of either routine". `std::map<std::string,int>::operator[]` constructs a `std::string` key from `pName`; a NULL argument is UB. The same raw return is used (without dereference into a `std::string`) in `getColumnName()` (line 268), which merely returns the pointer — that is safe — but the map-build path stores it as a `std::string` key.
- **Impact:** Crash / UB on an out-of-memory edge case during column-name lookup-by-name. Rare, but the wrapper otherwise prides itself on shielding callers from raw C pitfalls.
- **Proposed fix:** Skip / substitute NULL names when building the map:
  ```cpp
  const char* pName = sqlite3_column_name(getPreparedStatement(), i);
  if (pName)
      mColumnNames[pName] = i;
  ```
  (Names returned NULL cannot be matched by a caller-supplied name anyway, so skipping is correct.)

### [STMT-03] `getChanges()` reads connection-wide change count, not statement-specific, and is inconsistent with `exec()`
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug / api
- **Location:** `src/Statement.cpp:316-320` (`getChanges`), `include/SQLiteCpp/Statement.h:619-620`
- **Description:** `getChanges()` calls `sqlite3_changes(mpSQLite)`, which returns the number of rows modified by the most recent INSERT/UPDATE/DELETE *on the database connection*, not on this prepared statement. Because a single `Database` connection is shared by many `Statement` objects, `stmt.getChanges()` can report changes made by an unrelated statement that ran more recently on the same connection. The doc comment ("Get number of rows modified by last INSERT, UPDATE or DELETE statement") does not disclose this connection-wide scoping. SQLite ≥ 3.37.0 provides `sqlite3_stmt_status(..., SQLITE_STMTSTATUS_...)` / the connection-scoped `sqlite3_changes64`, but there is no per-statement "rows changed" call; the more precise pattern is to capture `sqlite3_changes()` right after this statement's own step (which `exec()` already does at line 203). The result: `getChanges()` is only meaningful immediately after this statement's own `exec()`/`executeStep()`.
- **Impact:** Misleading change counts in multi-statement workflows; subtle data-integrity bugs if callers branch on `getChanges()`.
- **Proposed fix:** At minimum, document that the value is connection-scoped and only valid immediately after this statement executes. Optionally cache the change count inside `exec()`/`tryExecuteStep()` (right after the step succeeds) and return the cached value, so `getChanges()` reflects this statement's own last execution.

### [STMT-04] Defaulted move-assignment leaves moved-from object partially live, inconsistent with move constructor
- **Severity:** Medium
- **Confidence:** High
- **Category:** api / bug (state-machine / lifetime)
- **Location:** `include/SQLiteCpp/Statement.h:83` (`Statement& operator=(Statement&&) = default;`) vs the hand-written move constructor `src/Statement.cpp:42-55`
- **Description:** The move *constructor* deliberately scrubs the moved-from source: it sets `mpSQLite = nullptr`, `mColumnCount = 0`, `mbHasRow = false`, `mbDone = false` (and `mpPreparedStatement` is emptied by the `shared_ptr` move). The move *assignment* operator is `= default`, which member-wise move-assigns: `mpPreparedStatement` becomes empty (good), `mQuery`/`mColumnNames` are moved (left empty/unspecified), but the scalar members (`mpSQLite`, `mColumnCount`, `mbHasRow`, `mbDone`) are *copied* and left unchanged in the source. So after `b = std::move(a)`, `a.mpSQLite` still points at a live connection while `a.mpPreparedStatement` is empty. Using `a` then takes the throwing path in `getPreparedStatement()` (line 375-383) — which is the same defensive behavior the moved-from-via-constructor case relies on (and which the `moveConstructor` test exercises at `tests/Statement_test.cpp:139-140`). So it is not a crash, but the two move operations leave inconsistent state, and a future method that reads `mpSQLite`/`mColumnCount` *without* going through `getPreparedStatement()` (e.g. `getChanges()`, `getErrorCode()`, `getColumnCount()`) would observe stale-but-live data on a moved-from object.
- **Impact:** Inconsistent moved-from state; `getColumnCount()`/`getErrorCode()`/`getChanges()` on a moved-from (via assignment) `Statement` return stale values rather than a clean zero/throw. Latent foot-gun if invariants change.
- **Proposed fix:** Provide a hand-written `operator=(Statement&&) noexcept` that mirrors the move constructor (scrub the source scalars), or have the move constructor delegate to a shared helper. Mark it `noexcept` to match the constructor.

### [STMT-05] `bind(int, const void*, int)` / `bindNoCopy` do not validate buffer-vs-size and pass through to `sqlite3_bind_blob` with a possibly-negative size
- **Severity:** Medium
- **Confidence:** Medium
- **Category:** bug / security (untrusted input)
- **Location:** `src/Statement.cpp:127-131` (`bind` blob), `149-153` (`bindNoCopy` blob); also the `std::string`→`int` size casts at `114-115`, `136-137`
- **Description:** The blob/text binds cast `aValue.size()` (a `size_t`) to `int`: `static_cast<int>(aValue.size())`. For a `std::string`/buffer larger than `INT_MAX`, this truncates to a negative or wrong value. `sqlite3_bind_text` treats a negative length as "read until NUL" (wrong for embedded-NUL strings), and `sqlite3_bind_blob` with a negative `n` is documented as yielding undefined behavior. The blob `bind(aIndex, apValue, aSize)` overload also forwards `aSize` verbatim, so a caller passing a negative `aSize` reaches `sqlite3_bind_blob` directly. SQLite will typically return `SQLITE_TOOBIG` for oversize via the limit check, but the *sign* truncation happens in this wrapper before SQLite sees it.
- **Impact:** Silent data truncation or UB for >2 GB strings/blobs (rare but reachable with untrusted input sizes); incorrect length for embedded-NUL text when the cast goes negative.
- **Proposed fix:** Guard the size before casting, e.g. throw `SQLite::Exception` (or clamp) when `aValue.size() > static_cast<size_t>(std::numeric_limits<int>::max())`, and consider rejecting negative `aSize` in the public blob overloads. (The 64-bit `sqlite3_bind_text64`/`sqlite3_bind_blob64` APIs exist if true large-blob support is desired.)

### [STMT-06] `executeStep()`/`exec()` error classification compares `ret` against `sqlite3_errcode()` — fragile
- **Severity:** Low
- **Confidence:** Medium
- **Category:** bug
- **Location:** `src/Statement.cpp:164-204` (`executeStep` lines 169-176, `exec` lines 192-199)
- **Description:** When the step result is neither ROW nor DONE, the code distinguishes a "real" SQLite error from a stale-statement misuse by testing `if (ret == sqlite3_errcode(mpSQLite))`. `sqlite3_step` returns the primary result code, while `sqlite3_errcode()` returns the primary result code of the most recent failed connection-level call; these usually agree on the failing step, but the heuristic is indirect. In particular `tryExecuteStep()` synthesizes `SQLITE_MISUSE` itself when `mbDone` (line 208-211) *without* calling SQLite, so `sqlite3_errcode(mpSQLite)` will generally NOT equal `SQLITE_MISUSE`, and the code correctly falls into the "Statement needs to be reseted" branch — which is the intended message. So the behavior is correct for the tested cases (`tests/Statement_test.cpp:87,98,185`), but the mechanism (matching against a connection-global errcode) is brittle: a concurrent/interleaved failed call on the same connection could change `sqlite3_errcode()` between the step and the comparison.
- **Impact:** Possible mis-classification (wrong exception message / wrong error code wrapped) under unusual interleavings on a shared connection. Functionally minor; messaging only.
- **Proposed fix:** Branch directly on `ret`: treat `SQLITE_MISUSE` (the synthesized reset-needed case) explicitly, and otherwise wrap `Exception(mpSQLite, ret)`. This removes the dependency on the connection-global `sqlite3_errcode()`.

### [STMT-07] `getColumnName()` / `getColumnDeclaredType()` / `getColumnOriginName()` do not call `checkRow()` and may return stale pre-step metadata
- **Severity:** Low
- **Confidence:** High
- **Category:** api / correctness
- **Location:** `src/Statement.cpp:265-269`, `271-278`, `302-314`
- **Description:** These metadata accessors call only `checkIndex()` (not `checkRow()`), which is intentional and correct — column metadata is available after prepare, before any step (the `getColumnDeclaredType`/`getName` tests call them with/without a prior `executeStep`). This is consistent with SQLite, which exposes `sqlite3_column_name`/`_decltype` on a prepared (un-stepped) statement. Noting it here only because it contrasts with `getColumn()`/`isColumnNull()`, which *do* require a row; the asymmetry is by design but undocumented in code. `getColumnName()` can also return NULL (malloc failure) per the C contract, which the wrapper passes straight through to the caller (documented as a raw pointer, acceptable).
- **Impact:** None functionally; documentation/consistency only.
- **Proposed fix:** No code change required. Optionally add a brief comment that metadata accessors are valid post-prepare (no row required), unlike value accessors.

### [STMT-08] Missing `[[nodiscard]]` / `const` / `noexcept` annotations on query/accessor methods (C++11-compatible subset)
- **Severity:** Low
- **Confidence:** High
- **Category:** api / modernization
- **Location:** `include/SQLiteCpp/Statement.h` — `getIndex` (124), `executeStep` (414), `exec` (449), `getColumn` (478,509), `getColumns` (532), `isColumnNull` (554,565), `getColumnName` (576), `getColumnIndex` (600), `getExpandedSQL` (632), `getBindParameterCount` (651)
- **Description:** Many value-returning, side-effect-free accessors are not marked `[[nodiscard]]`, so callers can silently drop results (e.g. ignoring `executeStep()`'s bool, or `exec()`'s change count). `getExpandedSQL()` is `const` but could be `noexcept(false)` documented; `getBindParameterCount()` is correctly `noexcept`. The library targets C++11, where `[[nodiscard]]` (C++17) is unavailable, but the project already defines compiler-attribute macros (`SQLITECPP_PURE_FUNC` in `Utils.h`); a `SQLITECPP_NODISCARD` macro could be added analogously and applied to `executeStep`/`exec`/`getColumn`/`getColumnIndex` etc. `getIndex()` is marked `SQLITECPP_PURE_FUNC` and `const` (good).
- **Impact:** Reduced misuse protection; ignored error/return values compile silently.
- **Proposed fix:** Add a `SQLITECPP_NODISCARD` macro (expanding to `[[nodiscard]]` when `__cplusplus >= 201703L`, empty otherwise) and apply to the pure accessor/return-bearing methods. Optional, non-breaking.

### [STMT-09] `bind(const int aIndex, const uint32_t aValue)` widening to `int64` is correct but the doc/contrast with `getUInt()` could mislead
- **Severity:** Info
- **Confidence:** High
- **Category:** api
- **Location:** `src/Statement.cpp:91-95`
- **Description:** `bind(uint32_t)` forwards to `sqlite3_bind_int64`, which is the correct way to preserve the full unsigned 32-bit range (values > INT32_MAX) since SQLite has no unsigned type. The matching `Column::getUInt()` reads it back. Verified by `tests/Statement_test.cpp:374-390` (`uint32 = 4294967295U` round-trips). This is correct, not a bug — flagged only because the int64 promotion is easy to misread as a sign-extension hazard. No `uint64_t` overload exists (deliberate: SQLite stores signed 64-bit), so binding a large `uint64_t` would select the `int64_t` overload via conversion and could change sign — but there is no implicit `uint64_t→int64_t` ambiguity issue here because no `uint64_t` overload is declared.
- **Impact:** None.
- **Proposed fix:** None. (Optionally document that `uint64_t` values are bound as signed `int64_t`.)

### [STMT-10] Shared-ownership model (`shared_ptr<sqlite3_stmt>` + finalize deleter) — verified correct
- **Severity:** Info
- **Confidence:** High
- **Category:** memory
- **Location:** `src/Statement.cpp:360-372` (`prepareStatement`), `include/SQLiteCpp/Statement.h:87` (defaulted dtor), `Column.h:231` (Column holds a copy of `TStatementPtr`)
- **Description:** `prepareStatement()` calls `sqlite3_prepare_v2` into a raw `sqlite3_stmt*`, throws on non-OK (so on the error path nothing is leaked — the raw pointer was never allocated, SQLite sets `*ppStmt` to NULL on failure per the C contract), and otherwise wraps the pointer in a `shared_ptr` with a lambda deleter calling `sqlite3_finalize`. Construction order in the `Statement` constructor (`Statement.cpp:34-40`) is: `mQuery`, then `mpSQLite`, then `mpPreparedStatement(prepareStatement())`. If `prepareStatement()` throws, no `shared_ptr` was constructed and no `Statement` exists — exception-safe. The `Column` objects copy the `shared_ptr`, extending the `sqlite3_stmt` lifetime beyond the `Statement` if a `Column` outlives it; finalize happens only when the last owner drops. This correctly prevents use-after-finalize for `Column` while the documented "Column is only valid until next executeStep()" caveat covers data-pointer staleness (a `Column` issue, not a `Statement` one). One subtlety: if wrapping the raw pointer in the `shared_ptr` itself threw `std::bad_alloc` (control-block allocation), the already-prepared `sqlite3_stmt*` would leak. This is the standard `shared_ptr(ptr, deleter)` caveat and is essentially unreachable in practice; noting for completeness.
- **Impact:** None in practice.
- **Proposed fix:** None required. (To close the theoretical leak window one could use `std::shared_ptr<sqlite3_stmt>(statement, deleter)` constructed via a small RAII guard, but it is not warranted.)

## Notes / non-issues
- **`tryReset()` / `reset()` state reset (`Statement.cpp:64-69`)**: clears `mbHasRow`/`mbDone` *before* calling `sqlite3_reset`; even if `sqlite3_reset` returns the deferred error from the previous step (as the `insert.reset()` test at `tests/Statement_test.cpp:191` expects), the flags are already reset, which is the intended semantics (a reset statement is no longer "done"/"has row"). Correct.
- **`tryExecuteStep()` synthesizing `SQLITE_MISUSE` when `mbDone` (`Statement.cpp:206-211`)**: this guards against calling `sqlite3_step` on a DONE statement (which the C API otherwise treats as misuse / auto-reset depending on prepare variant). It is `noexcept` and never touches SQLite in this branch — correct and matches `tests/Statement_test.cpp:87`.
- **`check()` / `checkRow()` / `checkIndex()` helpers (`Statement.h:669-697`)**: bounds and row guards are correct; `checkIndex` rejects `< 0` and `>= mColumnCount`, matching the `[0, getColumnCount())` doc and the negative-index tests (`tests/Statement_test.cpp:49-56,768`).
- **`getPreparedStatement()` null check (`Statement.cpp:375-383`)**: throws `SQLite::Exception("Statement was not prepared.")` for a moved-from statement; this is what makes operations on moved-from objects throw rather than deref NULL (exercised at `tests/Statement_test.cpp:139-140`). Good defensive design.
- **`bindNoCopy(std::string&&) = delete` (`Statement.h:185,283,384`)**: correctly prevents binding a temporary with `SQLITE_STATIC`, which would dangle. Good.
- **`bind(...)` text/blob using `SQLITE_TRANSIENT`**: matches the documented copy semantics and the embedded-NUL test (`tests/Statement_test.cpp:433-448`). `bindNoCopy` using `SQLITE_STATIC` with the documented caller-lifetime contract is correct.
- **Thread-safety**: the class documents (header lines 45-50) that a `Statement` must not be shared across threads and that SQLite "Serialized" mode is unsupported due to the shared `sqlite3_stmt`; consistent with the implementation. No locking is attempted, which is the intended design.
