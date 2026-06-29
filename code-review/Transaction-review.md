# Transaction — Review

## Summary
The `Transaction` class is a small, well-behaved RAII wrapper over `BEGIN`/`COMMIT`/`ROLLBACK` with only fixed SQL literals (no interpolation, so no injection surface). Copy is correctly deleted and the destructor swallows exceptions to remain `noexcept`. The `mbCommited` guard correctly prevents the destructor from acting after a `commit()`. Findings are minor: a few semantic/diagnostic gaps (silent loss of real rollback errors in the destructor, a `default:` case that defeats `-Wswitch-enum`, a misleading exception message, and incomplete Rule-of-5/`explicit` hygiene). No critical or high-severity issues.

Counts by severity: Critical 0, High 0, Medium 2, Low 3, Info 2.

## Findings

### [TXN-01] Destructor silently swallows ALL rollback failures, including real errors (no assert / no diagnostic)
- **Severity:** Medium
- **Confidence:** High
- **Category:** thread/exception
- **Location:** `src/Transaction.cpp`:50-63 (`~Transaction`)
- **Description:** The destructor wraps `mDatabase.exec("ROLLBACK TRANSACTION")` in `try { ... } catch (SQLite::Exception&) {}` and discards every exception with the rationale "error if already rollbacked, but no harm is caused by this." That rationale only covers the benign "no active transaction" case (`SQLITE_ERROR` "cannot rollback - no transaction is active"). It also swallows genuinely meaningful failures such as `SQLITE_BUSY`/`SQLITE_LOCKED` (another connection holds a lock) or an I/O error, where the rollback did **not** happen and the transaction is left open on the connection. The user is given no signal at all. Notably, `Assertion.h` documents that `SQLITECPP_ASSERT()` exists specifically "to be used in destructors, where exceptions shall not be thrown" and provides an optional user `assertion_failed()` handler hook — yet this destructor uses neither `SQLITECPP_ASSERT` nor any logging, unlike the design intent. (The `Savepoint` destructor at `src/Savepoint.cpp`:37-52 has the identical pattern, so this is a library-wide habit rather than a one-off.)
- **Impact:** A failed automatic rollback (lock contention, disk-full, I/O error) is invisible. The connection can be left with an open transaction after the `Transaction` object is destroyed, and subsequent code may behave unexpectedly (e.g. a later `BEGIN` failing, or unintended data persistence) with no diagnostic to trace it to.
- **Proposed fix:** Inside the `catch`, route the swallowed error through `SQLITECPP_ASSERT(false, e.what())` so that debug builds and any user-provided `assertion_failed` handler are notified, while release builds still never throw. For example:
  ```cpp
  catch (SQLite::Exception& e)
  {
      // Never throw from a destructor; surface the error via the assert hook instead.
      (void)e;
      SQLITECPP_ASSERT(false, e.what());
  }
  ```
  This keeps the `noexcept` guarantee while restoring observability per the documented `Assertion.h` contract.

### [TXN-02] `default:` case in the behavior switch defeats `-Wswitch-enum` coverage for future enum values
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug
- **Location:** `src/Transaction.cpp`:26-38; enum at `include/SQLiteCpp/Transaction.h`:26-30
- **Description:** The `switch (behavior)` handles `DEFERRED`/`IMMEDIATE`/`EXCLUSIVE` and then has a `default: throw`. Because a `default` label is present, `-Wswitch`/`-Wswitch-enum` will **not** warn if a new enumerator is later added to `TransactionBehavior` — the new value would silently fall through to `default` and throw `SQLITE_ERROR` at runtime instead of being caught at compile time. The runtime guard is reachable today only via a deliberately invalid cast (`static_cast<TransactionBehavior>(-1)`), which the test at `tests/Transaction_test.cpp`:56 exercises, so the `default` is not dead code; the concern is purely the lost compile-time exhaustiveness check.
- **Impact:** A future maintainer adding a 4th behavior (e.g. `CONCURRENT`) gets no compiler help and ships a transaction type that always throws at runtime.
- **Proposed fix:** Keep correctness but restore the warning: after the `switch` (with no `default`), handle the invalid value with an explicit `if`/`throw`, or initialize `stmt = nullptr;` and throw when it is still null. With no `default` label, `-Wswitch-enum` will flag any unhandled enumerator. Alternatively, leave as-is and document the deliberate trade-off; this is a low-cost robustness improvement, not a defect in current behavior.

### [TXN-03] `rollback()` after `commit()` throws a misleading "already committed" message
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `src/Transaction.cpp`:80-90 (`rollback`)
- **Description:** When `mbCommited` is true, `rollback()` throws `SQLite::Exception("Transaction already committed.")`. The behavior (refusing to roll back a committed transaction) is correct, but the message is the same string used by `commit()` for the genuine double-commit case, and "already committed" is a slightly odd thing to report from a *rollback* call. A caller catching this generically cannot distinguish a double-commit from a post-commit rollback attempt.
- **Impact:** Minor diagnostic ambiguity; no functional defect. The `Exception` is built via the single-arg `explicit Exception(const std::string&)` ctor (Exception.h:51), giving `getErrorCode() == -1`, which is consistent with `commit()`.
- **Proposed fix:** Use a message specific to the call site, e.g. `"Cannot rollback a transaction that has already been committed."`, leaving `commit()`'s "Transaction already committed." intact.

### [TXN-04] Manual `rollback()` then destructor issues a second `ROLLBACK` that is expected to fail
- **Severity:** Low
- **Confidence:** High
- **Category:** bug
- **Location:** `src/Transaction.cpp`:80-90 (`rollback`) and 50-63 (`~Transaction`)
- **Description:** `rollback()` executes `ROLLBACK TRANSACTION` but does **not** set any state flag (`mbCommited` stays `false`, and there is no separate "rolled back" flag). Consequently, when the object goes out of scope after a manual `rollback()`, the destructor's `if (false == mbCommited)` is still true and it runs `ROLLBACK TRANSACTION` a second time. That second statement fails ("cannot rollback - no transaction is active") and the exception is swallowed by the dtor's `catch`. The test at `tests/Transaction_test.cpp`:93-106 explicitly documents and relies on this ("the automatic rollback should not raise an error because it is harmless"). So this is intended and currently harmless, but it (a) couples correctness to the error-swallowing in TXN-01 — if TXN-01 is changed to assert/log, this benign path would start firing the assert — and (b) wastes a round-trip to SQLite on every manually-rolled-back transaction.
- **Impact:** No data-correctness problem today, but it is fragile: the "double rollback is harmless" assumption is load-bearing for both the dtor design and TXN-01. If a future change makes the dtor surface errors, this path becomes a false positive.
- **Proposed fix:** Track terminal state explicitly. Replace the single `mbCommited` flag with an enum/`bool mbFinished` (set true by both `commit()` and `rollback()`), and gate the destructor on it. This makes "transaction already finished" the single source of truth, eliminates the redundant second `ROLLBACK`, and lets TXN-01's assert distinguish real failures from already-finished ones. (Mirrors the same latent issue in `Savepoint`.)

### [TXN-05] Move constructor / move assignment are not explicitly declared (incomplete Rule of 5)
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Transaction.h`:74-76
- **Description:** The class declares the destructor and `= delete`s the copy constructor and copy assignment, but says nothing about move operations. Per the C++ rules, user-declaring the copy operations (and the destructor) means the move constructor and move assignment are **not** implicitly declared, so `Transaction` is correctly non-movable — which is the desired behavior for a type holding a `Database&` and a "live transaction" responsibility. The result is correct, but the intent is implicit rather than expressed, so a reader must reason about the Rule of 5 to confirm `Transaction t2 = std::move(t1);` is rejected. This matches the existing convention in `Backup.h`:97-98 and `Savepoint.h`:72-73 (all delete copy only), so it is a library-wide style point, not a regression.
- **Impact:** None functionally; readability/maintainability only.
- **Proposed fix:** For C++11 clarity, optionally add explicit `Transaction(Transaction&&) = delete;` and `Transaction& operator=(Transaction&&) = delete;` (and likewise for `Savepoint`/`Backup` to stay consistent). Purely a documentation-of-intent change.

### [TXN-06] Lifetime risk: `Transaction` stores `Database&` with no protection against the Database outliving it
- **Severity:** Info
- **Confidence:** High
- **Category:** memory
- **Location:** `include/SQLiteCpp/Transaction.h`:94 (`Database& mDatabase`)
- **Description:** `Transaction` keeps a reference to the `Database` it was constructed from and dereferences it in `commit()`, `rollback()`, and the destructor. If the `Database` is destroyed before the `Transaction` (e.g. wrong declaration order, or a `Database` on the heap freed while a `Transaction` is still alive), the destructor's `mDatabase.exec(...)` is undefined behavior (use-after-free). This is the standard SQLiteC++ ownership contract — `Statement`, `Savepoint`, and `Backup` all hold references/handles into the `Database` and require it to outlive them — and the class doc (Transaction.h:39-41) frames the RAII guarantee around "the validity of the underlying SQLite Connection." There is no defect; this is an inherent, documented constraint worth recording for the consolidated review.
- **Impact:** Undefined behavior only on misuse (Database destroyed first). Cannot occur with the documented stack-scoped usage pattern shown in the tests.
- **Proposed fix:** None required. Optionally strengthen the class-level Doxygen to state explicitly that the referenced `Database` must outlive the `Transaction`.

### [TXN-07] No `[[nodiscard]]` on the type / constructors (C++11 target makes this non-actionable)
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Transaction.h`:52-72
- **Description:** A common RAII footgun is constructing a scope-guard as a temporary (`SQLite::Transaction(db);`) that is destroyed immediately, beginning and instantly rolling back a transaction. `[[nodiscard]]` on the class could catch a discarded-temporary in some compilers. However, the library targets C++11 (per the review brief and the codebase's use of `enum class`/default member initializers without C++17 features), and `[[nodiscard]]` is C++17. The constructors are correctly marked `explicit` (Transaction.h:62, 72), which prevents the most likely accidental conversions. No change is appropriate under the C++11 constraint.
- **Impact:** None; informational, and not applicable given the language standard.
- **Proposed fix:** None under C++11. If the minimum standard is ever raised, consider a guarded `SQLITECPP_NODISCARD` macro on the class.

## Notes / non-issues
- **No SQL injection surface.** All four executed statements (`BEGIN TRANSACTION`, `BEGIN DEFERRED/IMMEDIATE/EXCLUSIVE`, `COMMIT TRANSACTION`, `ROLLBACK TRANSACTION`) are fixed string literals chosen by an internal `enum class` switch. No user-supplied value is ever concatenated into SQL (contrast with `Savepoint`, which interpolates a name and mitigates it via `SELECT quote(?)`). Security risk is correctly rated 1 in the scoring matrix.
- **Double-commit is correctly guarded.** `commit()` checks `mbCommited` and throws on the second call (`src/Transaction.cpp`:68-76); the test at `tests/Transaction_test.cpp`:42 confirms `EXPECT_THROW(transaction.commit(), SQLite::Exception)`. After a successful `commit()`, `mbCommited == true` so the destructor's `if (false == mbCommited)` is false and the destructor does nothing — there is no "commit then destructor also rolls back" bug.
- **Constructor exception safety is correct.** In the behavior constructor the `default:` `throw` happens *before* `mDatabase.exec(stmt)`, and `exec()` itself may throw; in either case no `Transaction` object is fully constructed, so no transaction is left dangling and the destructor does not run for a throwing constructor. Matches the doc "Exception is thrown in case of error, then the Transaction is NOT initiated."
- **`BEGIN TRANSACTION` vs `BEGIN DEFERRED` equivalence.** The default constructor uses `"BEGIN TRANSACTION"`, which SQLite treats as `DEFERRED` by default — consistent with `TransactionBehavior::DEFERRED`. This is intended and matches SQLite's documented default; not a discrepancy.
- **`enum class TransactionBehavior` is scoped and C++11-clean.** No implicit int conversion; the only way to reach the invalid `default:` path is a deliberate `static_cast`, which the test covers.
- **Destructor `catch (SQLite::Exception&)` by reference is correct** (catches the library's own exception type; avoids slicing). It does not catch `std::exception`, but `Database::exec` only throws `SQLite::Exception`, so the narrower catch is appropriate and intentional.
