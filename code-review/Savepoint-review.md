# Savepoint — Review

## Summary
The `Savepoint` class is a small RAII wrapper around SQLite's `SAVEPOINT` / `RELEASE` / `ROLLBACK TO` commands. It is the only unit in the library that interpolates a runtime value (the savepoint name) into SQL text, and it does so after escaping the name through a `SELECT quote(?)` statement in the constructor. The escaping is fundamentally correct (`quote()` doubles interior single quotes and wraps the value in single quotes, which neutralizes injection), and the escaped form is used consistently for all three commands — there is no code path that uses the raw, unescaped name. The real weaknesses are: a destructor that issues a `ROLLBACK TO` immediately followed by a `RELEASE` (which generally is *not* the documented commit semantics and discards the work), a documented `quote()` NUL-truncation behavior that silently changes the savepoint name, and several modernization gaps (no move support, no `final`, no const, missing `mbReleased` initialization is fine but design flaws around state flags).

Findings by severity: Critical 0, High 1, Medium 3, Low 2, Info 2.

## Findings

### [SP-01] Destructor `rollback()` + `release()` discards all work even on the non-error / scope-exit path
- **Severity:** High
- **Confidence:** High
- **Category:** bug
- **Location:** `src/Savepoint.cpp:37-52` (destructor), `:33` (constructor SAVEPOINT)
- **Description:** The destructor, when `!mbReleased`, unconditionally runs `rollback()` (`ROLLBACK TO SAVEPOINT <name>`) and then `release()` (`RELEASE SAVEPOINT <name>`). The class doc comment (`Savepoint.h:30-32`) states the savepoint "is rolled back in the destructor (unless committed before)", so on its face the rollback-on-destruction is intentional. However, the ordering and pairing have a subtle consequence: `ROLLBACK TO SAVEPOINT name` rolls back all changes made since the savepoint but **leaves the savepoint on the transaction stack**; the subsequent `RELEASE SAVEPOINT name` then pops it. The net effect is that every implicit (scope-exit) destruction throws away the work. This matches the test's expectation for `sp2`/`sp4` (auto-rollback), but it means there is *no* safe way to let a Savepoint commit by simply leaving scope — you must call `release()` explicitly *and* the class offers no "commit-on-success" guard like a typical RAII transaction would. Compare `Transaction`, whose destructor only rolls back (it does not also call commit). Issuing both a `ROLLBACK TO` and a `RELEASE` in the destructor is unusual; the `ROLLBACK TO` alone undoes the work and the trailing `RELEASE` is only needed to clean the stack. The two-call sequence works but is fragile (see SP-02).
- **Impact:** Behaviorally the auto-rollback semantics are as documented, but the design is a footgun: it differs from `Transaction` and the doubled command in the destructor is the root of the exception-safety subtlety in SP-02. Any future change that makes `release()` mean "commit" (as its own comment claims: "Release the savepoint and commit", `:54`) would silently start committing rolled-back/empty savepoints.
- **Proposed fix:** Keep auto-rollback semantics but make the destructor intent explicit and minimal: in the non-released case, execute a single `ROLLBACK TO SAVEPOINT <name>` followed by `RELEASE SAVEPOINT <name>` only if the first succeeds, and document clearly that scope-exit always rolls back. Alternatively, introduce an explicit state model (Active / RolledBack / Released) so the destructor does the minimum required for the current state (see SP-02). At minimum, reconcile the `release()` comment ("commit") with the actual `RELEASE` semantics.

### [SP-02] Destructor relies on exception-swallowing to skip the second command after the first fails; no independent rolled-back guard
- **Severity:** Medium
- **Confidence:** High
- **Category:** thread/exception
- **Location:** `src/Savepoint.cpp:37-52`, state flag `mbReleased` (`Savepoint.h:95`)
- **Description:** There is exactly one state flag, `mbReleased`. There is no `mbRolledBack` flag (despite the task description assuming one). In the destructor, `rollback()` and `release()` are called in sequence inside a single `try` block. If `rollback()` throws (e.g. the parent transaction was already committed/rolled back, so the savepoint no longer exists), the `catch` swallows it and `release()` is **never reached** — which is correct, because the savepoint is already gone. But the control flow depends entirely on the exception path: there is no explicit check. More importantly, `rollback()`/`release()` both guard only on `mbReleased`, not on whether a rollback already happened. A user who calls `rollback()` manually (test `sp4`) leaves `mbReleased == false`, so the destructor calls `rollback()` *again* (a second `ROLLBACK TO` to the same savepoint — harmless because the savepoint still exists after `ROLLBACK TO`) and then `release()`. This works but only by luck of SQLite semantics; the code has no model of "already rolled back".
- **Impact:** Fragile. The "harmless double rollback" relies on `ROLLBACK TO` not removing the savepoint and on SQLite tolerating a repeat. If the first destructor command (`rollback`) succeeds but `release` throws, the exception is swallowed and the savepoint is left on the stack — leaking a savepoint onto the connection's transaction stack until an outer release/commit. This is hard to observe but can confuse later savepoint bookkeeping.
- **Proposed fix:** Track explicit state (e.g. an enum or a `mbRolledBack` flag). In the destructor, if already rolled back, only `release()`; otherwise `rollback()` then `release()`. Keep the `try/catch(SQLite::Exception&)` but consider catching `...` as well (see SP-03), and ensure `release()` is attempted even if it is the only cleanup still required.

### [SP-03] Destructor only catches `SQLite::Exception`; a different exception type would propagate out of the destructor and call `std::terminate`
- **Severity:** Medium
- **Confidence:** High
- **Category:** thread/exception
- **Location:** `src/Savepoint.cpp:46` (`catch (SQLite::Exception&)`)
- **Description:** The destructor catches only `SQLite::Exception&`. `mDatabase.exec()` ultimately can throw `SQLite::Exception`, but the call chain also constructs `std::string` objects (`std::string("ROLLBACK TO SAVEPOINT ") + msName`) which can throw `std::bad_alloc`, and any user-installed assertion handler or `Statement` machinery could throw something else. Since `~Savepoint()` is `noexcept(true)` by the implicit C++11 rule for destructors, any exception that escapes the `catch` (e.g. `std::bad_alloc`, or a non-`SQLite::Exception` derived error) calls `std::terminate()`.
- **Impact:** A `std::bad_alloc` (or any non-`SQLite::Exception`) during destructor cleanup terminates the process instead of being safely ignored. Low probability but a hard crash when it happens, and it violates the "never throw from a destructor" intent stated in the code comment (`:48`).
- **Proposed fix:** Broaden the handler to `catch (...)` in the destructor (the comment already says the goal is "Never throw an exception in a destructor"). Building the SQL strings outside the try is not necessary if the whole body is guarded; the simplest fix is to change `catch (SQLite::Exception&)` to `catch (...)`.

### [SP-04] `quote()` silently truncates a savepoint name at the first embedded NUL, changing the effective name
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug (robustness) / security (defense-in-depth)
- **Location:** `src/Savepoint.cpp:28-33`
- **Description:** `stmt.bind(1, msName)` binds the full `std::string` including any embedded NUL bytes (`Statement::bind(const int, const std::string&)` uses `sqlite3_bind_text(..., aValue.size(), SQLITE_TRANSIENT)` — `src/Statement.cpp:112-117` — so it does not stop at a NUL). However, the SQLite `quote()` function documentation states: "Strings with embedded NUL characters cannot be represented as string literals in SQL and hence the returned string literal is truncated prior to the first NUL." So `quote("a\0bc")` returns `'a'`. Then `getText()` (`src/Column.cpp:77-81`) returns that C string. The savepoint is therefore created as `SAVEPOINT 'a'`, not the name the caller intended. `RELEASE`/`ROLLBACK TO` reuse the same truncated `msName`, so they remain internally consistent (no mismatch bug between create and release), but the name silently differs from the argument.
- **Impact:** Two consequences. (1) Correctness: a caller passing a name containing a NUL gets a different savepoint name than requested, with no error; two distinct names that share a NUL-truncated prefix collide. (2) Security (positive/defense-in-depth note): because everything after the NUL is dropped, a NUL-smuggled injection payload (`"x\0'; DROP TABLE t; --"`) is discarded by `quote()` — so this truncation does not create an injection, it removes one. The issue is the silent name change, not injection.
- **Proposed fix:** Reject or detect names with embedded NUL bytes before binding (e.g. `if (msName.find('\0') != std::string::npos) throw SQLite::Exception("Invalid savepoint name");`), or document that names are truncated at the first NUL. Given names are programmer-supplied identifiers, throwing on a NUL is the cleanest.

### [SP-05] No move constructor / move assignment for a reference-holding RAII type
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `Savepoint.h:71-73` (copy deleted; no move declared)
- **Description:** Copy is correctly deleted. No move operations are declared. Because copy is user-declared (deleted) and a user-declared destructor exists, the compiler does **not** implicitly generate move operations, so `Savepoint` is non-movable. `Transaction` in this same library has the identical limitation, so this is consistent, but it means a `Savepoint` cannot be returned by value from a factory or stored in containers that require movability. The class holds `Database&` (a reference), which is not itself movable, so a hand-written move would need to transfer the `mbReleased` flag and neutralize the moved-from object's destructor.
- **Impact:** Reduced ergonomics; cannot move a Savepoint out of a function or into a `std::vector`. Not a correctness bug.
- **Proposed fix:** Either explicitly document Savepoint as non-movable (matching Transaction) or add a move constructor/assignment that transfers `msName`/`mbReleased` and sets the source's `mbReleased = true` so the moved-from destructor is a no-op. (Holding `Database*` instead of `Database&` would make a defaulted move possible.)

### [SP-06] `rollbackTo()`/`release()` are not `const`-correct candidates and lack input validation; `release()` doc says "commit"
- **Severity:** Low
- **Confidence:** Medium
- **Category:** api/modernization
- **Location:** `Savepoint.h:83-90`, `src/Savepoint.cpp:54-66`
- **Description:** Minor API observations: (a) The deprecated `rollback()` is declared inline (`Savepoint.h:90`) with a `// @deprecated` line comment but no `[[deprecated]]` attribute, so the compiler emits no deprecation warning; the comment is non-binding. (b) `release()`'s implementation comment (`:54` "Release the savepoint and commit") and the header doc (`:81` "Commit and release the savepoint.") describe `RELEASE` as a commit. `RELEASE` of the outermost savepoint commits, but `RELEASE` of a nested savepoint merely merges it into the parent — it does not durably commit. The doc slightly overstates the semantics. (c) These methods are intentionally non-const (they mutate DB state), which is correct.
- **Impact:** Documentation/usability only. A developer may assume `release()` durably commits when nested.
- **Proposed fix:** Use the `[[deprecated]]` attribute (guarded for C++11/14) on `rollback()`; refine the `release()` docs to note RELEASE only commits when it is the outermost savepoint.

### [SP-07] If the escaping statement throws, the constructor leaves a half-constructed object — acceptable but worth noting
- **Severity:** Info
- **Confidence:** High
- **Category:** thread/exception
- **Location:** `src/Savepoint.cpp:23-34`
- **Description:** The constructor runs `SELECT quote(?)` and then `SAVEPOINT <name>`. If `Statement` construction, `bind`, `executeStep`, or the final `mDatabase.exec("SAVEPOINT ...")` throws, the `Savepoint` constructor throws and the object is never created — so the destructor will **not** run (correct: no savepoint was opened, so nothing to roll back). `mbReleased` defaults to `false` (`Savepoint.h:95`) but is irrelevant on the throwing path. This is the correct RAII contract and matches the header comment ("Exception is thrown in case of error, then the Savepoint is NOT initiated", `:67-68`). No leak, because the temporary `Statement stmt` is itself RAII and finalizes its prepared statement on scope exit.
- **Impact:** None — behavior is correct.
- **Proposed fix:** None. Noted for completeness because the prompt asked what happens if the escaping statement fails.

### [SP-08] SQL injection via savepoint name is correctly mitigated — verification
- **Severity:** Info
- **Confidence:** High
- **Category:** security
- **Location:** `src/Savepoint.cpp:28-33`, reused at `:59`, `:73`
- **Description:** Verified the core security question. The name is escaped once in the constructor via a parameterized `SELECT quote(?)` (the value is bound, not concatenated, so the escaping query itself is injection-safe), and the *quoted* result overwrites `msName`. All three SQL commands (`SAVEPOINT`, `RELEASE SAVEPOINT`, `ROLLBACK TO SAVEPOINT`) concatenate the **same already-quoted** `msName`; there is no code path that uses the original raw argument after line 33. SQLite's `quote()` wraps strings in single quotes and doubles interior single quotes ("Strings are surrounded by single-quotes with escapes on interior quotes as needed"), which neutralizes embedded quotes, semicolons, and `--`/`/* */` comment sequences because they all become part of a single quoted string token. SQLite accepts a single-quoted string literal as a savepoint-name token, so `SAVEPOINT 'evil; DROP TABLE t; --'` creates one savepoint literally named `evil; DROP TABLE t; --` rather than executing the injection. Unicode and very long names are handled as ordinary bytes by `quote()` and `sqlite3_bind_text`. The one residual oddity is the NUL truncation in SP-04, which (if anything) strengthens the safety by discarding post-NUL bytes. Conclusion: no SQL injection vulnerability.
- **Impact:** None (positive finding).
- **Proposed fix:** None. Optionally add a regression test that constructs a Savepoint with a name containing `'`, `;`, `--`, and `"` and asserts it round-trips, to lock in the escaping behavior.

## Notes / non-issues
- **Lifetime of `getColumn(0).getText()` at line 31 is safe.** `getColumn(0)` returns a temporary `Column`; `getText()` returns a `const char*` into the `stmt` buffer. The pointer is consumed by `std::string::operator=(const char*)`, which copies before the full-expression ends and well before the local `Statement stmt` is destroyed. No dangling pointer.
- **`mDatabase` as `Database&` is fine for lifetime** given the documented contract that the Database must outlive the Savepoint (same contract as Statement/Transaction). It does, however, force non-movability (SP-05).
- **`mbReleased` default member initializer (`= false`)** is correct and ensures a defined state on every path, including before the `SAVEPOINT` exec.
- **Consistency of escaped name across commands** was specifically checked: `msName` is the quoted form for the constructor's `SAVEPOINT`, and the identical `msName` is reused by `release()` and `rollbackTo()`. There is no create/release name mismatch bug.
- **Header dependency hygiene:** `Savepoint.h` includes `Exception.h` (needed because `~Savepoint`/`release` reference `SQLite::Exception` semantics) and forward-declares `Database`, keeping `sqlite3.h` out of the public header. Correct.
- **Test coverage gaps (not bugs, but worth flagging):** `Savepoint_test.cpp` only uses simple names (`sp1`..`sp4`). There is no test for a name containing quotes/semicolons (SP-08 escaping), an empty name, a NUL-containing name (SP-04), or nested savepoints. Adding these would harden the most security-relevant behavior in the library.
