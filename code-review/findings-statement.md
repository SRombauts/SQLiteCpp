# Findings — Statement unit

Files: `include/SQLiteCpp/Statement.h`, `src/Statement.cpp`, `tests/Statement_test.cpp`.

`SQLite::Statement` wraps a prepared `sqlite3_stmt` (owned through a `shared_ptr` finalizer), exposing the prepare/bind/step/reset lifecycle, the bind overload matrix, and column accessors. It is the most-depended-on data-path unit: `Column`, `VariadicBind`, `ExecuteMany`, `Savepoint`, and `Database::execAndGet`/`tableExists` all build on it.

## Findings (most severe first)

### ST-001 — Defaulted move-assignment leaves the moved-from `Statement` inconsistent
- [ ] **Severity:** medium — **Confidence:** high — **Category:** correctness / resource-safety
- **Location:** `include/SQLiteCpp/Statement.h:83` (`Statement& operator=(Statement&&) = default;`)
- **Impact (failure scenario):** The move *constructor* (`src/Statement.cpp:42-55`) nulls the source (`mpSQLite=nullptr`, `mColumnCount=0`, `mbHasRow=false`, `mbDone=false`). The move *assignment* is `= default`, so it only member-wise moves: `mpPreparedStatement` goes null in the source, but the raw `mpSQLite` and the `mColumnCount`/`mbHasRow`/`mbDone` flags are **copied, not reset**. After `b = std::move(a)`, `a.hasRow()` can return `true` while `a.getColumn()` throws (null prepared statement). The `moveConstructor` test never re-uses the moved-from source, so this is untested.
- **Fix:** Provide a user-defined move-assignment mirroring the move constructor (move each member, reset the source's `mpSQLite`/`mColumnCount`/`mbHasRow`/`mbDone`, with a self-assignment guard).

### ST-004 — `bind`/`bindNoCopy(std::string)` narrow `size()`→`int` with no overflow guard
- [ ] **Severity:** low — **Confidence:** high — **Category:** portability / integer-overflow / UB
- **Location:** `src/Statement.cpp:114-115`, `136-137` (and the query-size cast at `src/Statement.cpp:366`)
- **Impact (failure scenario):** `static_cast<int>(aValue.size())` narrows `size_t`. A string > `INT_MAX` (possible on 64-bit) yields a negative/truncated length to `sqlite3_bind_text`; SQLite treats a negative length as "read until NUL", silently corrupting the bound value rather than erroring.
- **Fix:** Guard `size() > INT_MAX` and throw before the cast in both string overloads and the prepare path.

### ST-002 — Binding by an unknown parameter name reports "out of range" instead of "unknown"
- [ ] **Severity:** low — **Confidence:** high — **Category:** api-footgun / error-reporting
- **Location:** `src/Statement.cpp:78-81` (`getIndex`); named `bind` overloads
- **Impact (failure scenario):** `sqlite3_bind_parameter_index` returns **0** for an unknown name; `getIndex` forwards it verbatim, so `bind("typo", 42)` → `sqlite3_bind_int(stmt, 0, ...)` → `SQLITE_RANGE` → "column index out of range". The user supplied a *name*, so the message is misleading. The read side (`getColumnIndex`) correctly throws "Unknown column name."
- **Fix:** Detect the `0` return in the named bind path and throw `SQLite::Exception("Unknown bind parameter name.")`.

### ST-003 — Column-name map rebuilds every call on a zero-column / NULL-name result
- [ ] **Severity:** low — **Confidence:** medium — **Category:** performance / correctness-edge
- **Location:** `src/Statement.cpp:281-303`
- **Impact (failure scenario):** The build guard is `if (mColumnNames.empty())`. With a zero-column result, or if `sqlite3_column_name` returns NULL (documented OOM), the map stays empty and the loop re-runs on every by-name call; a transient OOM that skips a column caches an incomplete map permanently (that column's by-name lookup then throws forever).
- **Fix:** Track "built" with a separate flag rather than `empty()`; treat NULL `sqlite3_column_name` as an error.

### ST-005 — `tryExecuteStep` synthesizes `SQLITE_MISUSE` after done, diverging from `sqlite3_step`
- [ ] **Severity:** low — **Confidence:** medium — **Category:** api-contract / test-gap
- **Location:** `src/Statement.cpp:206-211`
- **Impact (failure scenario):** When `mbDone`, `tryExecuteStep` returns `SQLITE_MISUSE` without calling `sqlite3_step`. The header documents it as "returning the sqlite result code", but this code never came from sqlite3; a caller looping on the raw code sees a synthesized value past the end.
- **Fix:** Document the post-done `SQLITE_MISUSE` return and add a regression test asserting it (no code change if intended).

### ST-006 — Metadata accessors omit `checkRow()` (verified correct)
- [ ] **Severity:** info — **Confidence:** high — **Category:** api-consistency
- **Location:** `src/Statement.cpp:265-269`, `273-277`, `305-317`
- **Impact (failure scenario):** `getColumnName`/`getColumnDeclaredType`/`getColumnOriginName` call only `checkIndex`, not `checkRow`. This is correct (these are valid after prepare without stepping, and tests rely on it) but asymmetric with value accessors, so it can look like a missing check during audit.
- **Fix:** None.

## Verified non-issues
- `~Statement() = default` — handle owned by the `shared_ptr` finalizer that calls `sqlite3_finalize` and swallows the result; no throwing destructor.
- `bind(uint32_t)` via `sqlite3_bind_int64` preserves the full unsigned range (tested with `4294967295U`).
- `getColumn` returns a `Column` holding a **copy** of the statement `shared_ptr`, so row data outlives the `Statement` per the documented sharing design.
- `SQLITE_TRANSIENT` (bind/copy) vs `SQLITE_STATIC` (bindNoCopy); `bindNoCopy(std::string&&)` is `= delete`d to block temporaries. Correct, tested with embedded NULs.
- `exec()` throws on `SQLITE_ROW` (tested). `reset()` re-throwing the prior step error is intended and tested.
- `getExpandedSQL` unconditionally `sqlite3_free`s and handles a NULL return — no leak/UB.
- Thread-safety: mutable `mColumnNames`/flags are unsynchronized, consistent with the documented "not shareable across threads" contract.
