# Findings — Transaction and Savepoint units

- **Transaction** (`include/SQLiteCpp/Transaction.h`, `src/Transaction.cpp`, `tests/Transaction_test.cpp`): RAII over a top-level transaction. Ctor runs `BEGIN [DEFERRED|IMMEDIATE|EXCLUSIVE]`; `commit()`/`rollback()` run `COMMIT`/`ROLLBACK`; destructor rolls back when not committed. Single `mbCommited` flag.
- **Savepoint** (`include/SQLiteCpp/Savepoint.h`, `src/Savepoint.cpp`, `tests/Savepoint_test.cpp`): RAII over a named, nestable SAVEPOINT. Ctor escapes the name via `SELECT quote(?)` then runs `SAVEPOINT <name>`. Reworked destructor uses `mbReleased`+`mbRolledBack`. **This is the unit the current branch (`fix-savepoint-destructor-exception-safety`, commit afa51d3) just fixed; its rollback-then-release destructor is correct.**

## Findings (most severe first)

### TX-001 — `~Transaction()` catches only `SQLite::Exception`, so a non-SQLite throw escapes → `std::terminate`
- [ ] **Severity:** medium — **Confidence:** high — **Category:** error-handling (throw-from-destructor)
- **Location:** `src/Transaction.cpp:58`
- **Impact (failure scenario):** `mDatabase.exec("ROLLBACK TRANSACTION")` constructs a `SQLite::Exception`, whose `std::runtime_error` base copies the message and can throw `std::bad_alloc` under memory pressure. `bad_alloc` is **not** a `SQLite::Exception`, so it escapes the implicitly-`noexcept` destructor → `std::terminate`. **This is the exact defect just fixed in `Savepoint::~Savepoint` (afa51d3 broadened `catch (SQLite::Exception&)` to `catch (...)`); `Transaction` was left with the narrow handler.**
- **Fix:** Broaden the handler to `catch (...)` in `~Transaction()`, matching the Savepoint destructor. Add a regression test (or at least a note) covering the parallel.

### SP-001 — Destructor abandons `release()` if `rollbackTo()` throws
- [ ] **Severity:** low — **Confidence:** medium — **Category:** correctness / failure-atomicity
- **Location:** `src/Savepoint.cpp:43-48`
- **Impact (failure scenario):** If `rollbackTo()` throws, `release()` is never attempted (intended swallow). Conversely, if `rollbackTo()` succeeds but `release()` fails, the savepoint frame is left on the stack until a parent unwinds it. Benign under SQLite nesting semantics (a parent COMMIT/RELEASE/ROLLBACK cleans it up), but a destructor "success" does not guarantee the frame is popped.
- **Fix:** Optionally attempt `release()` in its own try block independent of `rollbackTo()`. No functional change strictly required.

### TX-002 — `mbCommited` misspelled (should be `mbCommitted`)
- [ ] **Severity:** low — **Confidence:** high — **Category:** maintainability / naming
- **Location:** `include/SQLiteCpp/Transaction.h:95`; uses at `src/Transaction.cpp:52,68,71,82`
- **Impact (failure scenario):** Typo for "committed". Private member (no external references), so cosmetic, but copy-paste-prone. Cheap to fix while internal.
- **Fix:** Rename the member and its uses to `mbCommitted`. Private-symbol rename only; no API change.

### SP-002 — `rollback()` documented as deprecated but not marked `[[deprecated]]`; tests still call it
- [ ] **Severity:** low — **Confidence:** high — **Category:** api-design / maintainability
- **Location:** `include/SQLiteCpp/Savepoint.h:89-90`
- **Impact (failure scenario):** A plain `// @deprecated` line comment (not even a Doxygen `@deprecated`) produces no compiler warning, and the tests themselves call `savepoint.rollback()` (`tests/Savepoint_test.cpp:43,90`), so the API is effectively first-class despite the note.
- **Fix:** Decide: mark it with the project's deprecation macro and migrate test callers to `rollbackTo()`, or drop the misleading note.

### SP-003 — Test gap: no nested / child-savepoint coverage
- [ ] **Severity:** low — **Confidence:** high — **Category:** test-gap
- **Location:** `tests/Savepoint_test.cpp`
- **Impact (failure scenario):** The headline nesting feature and its documented cascading caveats are never exercised. A regression in rollback-then-release ordering or name handling with two live savepoints would go uncaught.
- **Fix:** Add a test with outer+inner `Savepoint`, release/rollback the outer, and assert the inner's destructor does not throw and the data outcome is correct.

### SP-004 — Test gap: hostile / quote-containing savepoint name (injection boundary) unverified
- [ ] **Severity:** low — **Confidence:** high — **Category:** test-gap / security
- **Location:** `tests/Savepoint_test.cpp`
- **Impact (failure scenario):** The `SELECT quote(?)` escaping (`src/Savepoint.cpp:28-33`) is the injection boundary, but no test passes a name like `sp'; DROP TABLE test; --`. A refactor dropping `quote()` would silently reintroduce injection with no failing test.
- **Fix:** Add a test constructing `Savepoint(db, "weird'; name")` (and a double-quote variant), then `release()`, asserting no throw and the schema intact.

### TX-003 — Test gap: a failing `commit()` must leave the transaction open for destructor rollback
- [ ] **Severity:** low — **Confidence:** medium — **Category:** test-gap
- **Location:** `tests/Transaction_test.cpp`
- **Impact (failure scenario):** `commit()` sets `mbCommited=true` only after `exec` returns without throwing — correct, since a failed COMMIT leaves the transaction active and the destructor must roll back. This load-bearing ordering is untested.
- **Fix:** Add a deferred-FK-constraint test that makes COMMIT throw, then assert the row was not persisted.

## Verified non-issues
- **Savepoint name injection:** `SELECT quote(?)` binds the raw name and returns a quote-doubled literal; `msName` is reassigned to the escaped form, so `release()`/`rollbackTo()` stay consistent. Safe.
- **No dangling pointer from `getColumn(0).getText()`:** result is copied into `std::string msName` before `stmt` is destroyed (`src/Savepoint.cpp:31`).
- **Constructor failure atomicity (both units):** a throw in the ctor means the object is never constructed, the destructor doesn't run, and no orphaned savepoint/transaction remains.
- **Savepoint destructor rollback-then-release ordering:** matches SQLite (`ROLLBACK TO` rewinds but keeps the frame; `RELEASE` pops it); `mbRolledBack` correctly avoids a redundant second `ROLLBACK TO`. Verified against the new `rollbackToThenRelease` test.
- **Double-release / release-after-rollback guards:** throw "Savepoint already released" only when `mbReleased`; covered by tests. No missed/double RELEASE.
- **`TransactionBehavior` invalid value:** `default:` throws before any `exec`; covered by the `static_cast<TransactionBehavior>(-1)` test.
