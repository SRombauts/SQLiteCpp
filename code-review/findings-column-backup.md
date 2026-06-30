# Findings — Column and Backup units

- **Column** (`include/SQLiteCpp/Column.h`, `src/Column.cpp`, `tests/Column_test.cpp`): a thin value wrapper around a single result cell. Holds a `shared_ptr<sqlite3_stmt>` + index, forwarding accessors to `sqlite3_column_*`; the shared pointer keeps the statement alive for the `Column`'s lifetime. Returned by every `getColumn`/`getColumns`.
- **Backup** (`include/SQLiteCpp/Backup.h`, `src/Backup.cpp`, `tests/Backup_test.cpp`): RAII over a `sqlite3_backup` handle (`unique_ptr` + `sqlite3_backup_finish` deleter). Used by `Database::backup()`.

## Findings (most severe first)

### BK-001 — `executeStep()` discards SQLite's detailed message and extended error code
- [ ] **Severity:** low — **Confidence:** high — **Category:** error-handling / diagnostics
- **Location:** `src/Backup.cpp:57`
- **Impact (failure scenario):** On a fatal `sqlite3_backup_step` result (`SQLITE_READONLY`, `SQLITE_IOERR_*`, `SQLITE_NOMEM`, `SQLITE_FULL`) it throws `SQLite::Exception(sqlite3_errstr(res), res)`. `sqlite3_errstr()` returns only the generic string for the primary code and the resulting `Exception` has `mExtendedErrcode == -1`. Every other throw site in the library uses `Exception(sqlite3*, ret)` and captures `sqlite3_errmsg`/extended code; this one is strictly worse.
- **Fix:** Retain the destination `sqlite3*` handle and throw `SQLite::Exception(destHandle, res)`, matching the rest of the codebase.

### COL-001 — `getString()` calls `sqlite3_column_bytes()` before `sqlite3_column_blob()` (against the documented safe order) and does redundant work
- [ ] **Severity:** low — **Confidence:** medium — **Category:** correctness / api-contract
- **Location:** `src/Column.cpp:96-101`
- **Impact (failure scenario):** SQLite's "safest policy" is `sqlite3_column_blob()` *then* `sqlite3_column_bytes()`. Here the order is a throwaway `bytes()` (line 96), `blob()` (line 97), `bytes()` again (line 101). The leading `bytes()` does not force BLOB format (`blob()` does) and is redundant. No observable corruption today (UTF-8 / embedded-NUL tests pass), but the ordering does not match the API's stated guarantee and is fragile against future in-place-conversion changes. **Corroborated by the verification build:** this line is the sole `-Wsign-conversion` hit in the library (`int`→`size_type`).
- **Fix:** Drop the leading `bytes()`; capture `data = sqlite3_column_blob(...)` first, then `len = sqlite3_column_bytes(...)`, and build `std::string(static_cast<const char*>(data), static_cast<size_t>(len))`.

### BK-002 — Page-count getters return 0 (meaningless) before the first `executeStep()`
- [ ] **Severity:** low — **Confidence:** medium — **Category:** api-design / documentation
- **Location:** `src/Backup.cpp:63-72`, `include/SQLiteCpp/Backup.h:115-119`
- **Impact (failure scenario):** Per `sqlite3.h`, these values are only updated by `sqlite3_backup_step()`. Calling `getTotalPageCount()`/`getRemainingPageCount()` right after construction returns 0 — a caller driving a progress bar before the first copied page can divide by zero or conclude "empty database". The Doxygen never states the pre-step value is 0.
- **Fix:** Tighten the header Doxygen to state both values are meaningful only after the first `executeStep()`; optionally add a regression test for the pre-step getters.

### COL-002 — `getText()` cannot distinguish SQL NULL from out-of-memory
- [ ] **Severity:** info — **Confidence:** medium — **Category:** error-handling / api-design
- **Location:** `src/Column.cpp:77-81`
- **Impact (failure scenario):** `sqlite3_column_text()` returns NULL for both a real SQL NULL and an OOM during conversion; `getText` collapses both to `apDefaultValue`. Inherent to the `noexcept` design; flagged for awareness.
- **Fix:** None required. `getString()` (length-driven) is the recommended path and is already documented as such.

## Verified non-issues
- **Column keeps the statement alive correctly** — stores the `shared_ptr` by value; survives owner `Statement` destruction (tested by `Column_test.cpp::shared_ptr`). The only UB cases (use after a further `executeStep()`/reset/finalize) are correctly documented as UB.
- **Null statement pointer rejected** in the ctor (tested by `invalidStatementPtr`).
- **`getUInt()` truncation is intentional/correct** — reads via `getInt64()` then casts to `unsigned`, so `[0, 2^32)` round-trips; SQLite has no unsigned 64-bit type.
- **`operator<<` handles BLOB/TEXT/embedded-NUL/multibyte UTF-8** — uses `getString()` + `aStream.write(data, size)` (tested).
- **`std::string(nullptr, 0)`** for NULL/zero-length cells is well-defined.
- **No throwing from destructors** — `Backup::Deleter` calls only `sqlite3_backup_finish` (return ignored); `Column` has no user destructor.
- **Backup handle ownership / single-finish** — owned by `unique_ptr`, freed once; on `init` failure the pointer is NULL, deleter no-ops, ctor throws. Self-backup failure covered by `initException`.
- **Backup is correctly non-copyable and (implicitly) non-movable** — user-deleted copy ops suppress implicit moves, so the `unique_ptr` can't be silently moved into a stale handle.
