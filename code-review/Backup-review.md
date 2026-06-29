# Backup — Review

## Summary
The `Backup` unit is a small, well-structured RAII wrapper around the `sqlite3_backup` C API. Resource ownership is modeled correctly with `std::unique_ptr<sqlite3_backup, Deleter>` calling `sqlite3_backup_finish`, the NULL-init error path correctly fetches the error from the destination connection, and `executeStep()` filters result codes in agreement with the SQLite contract. No correctness, memory-safety, or security bugs were found. The findings below are minor API/modernization and robustness observations.

Finding counts by severity: Critical 0, High 0, Medium 1, Low 3, Info 2.

## Findings

### [BKP-01] `executeStep()` is not `[[nodiscard]]`, so callers can silently drop BUSY/LOCKED
- **Severity:** Medium
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Backup.h:113` (declaration); `src/Backup.cpp:52-60` (definition)
- **Description:** `executeStep()` deliberately does *not* throw on `SQLITE_OK`, `SQLITE_DONE`, `SQLITE_BUSY`, or `SQLITE_LOCKED` (confirmed correct against `sqlite3.h:9620-9650`: BUSY/LOCKED are transient and "can be retried later"). The return value is the *only* signal that distinguishes "more pages remain" (`SQLITE_OK`), "complete" (`SQLITE_DONE`), and "retry needed" (`SQLITE_BUSY`/`SQLITE_LOCKED`). Nothing marks the return value as must-use. A caller that writes `backup.executeStep();` and assumes success (as `Database::backup()` does at `src/Database.cpp:434`) will silently treat a partial/contended backup as if it finished. `Database::backup()` itself does this: it calls `bkp.executeStep();` with `aNumPage = -1` and ignores the result, so if `SQLITE_BUSY`/`SQLITE_LOCKED` is returned the backup is incomplete yet the caller gets no error.
- **Impact:** Silent data-loss-style bug class for downstream users: an incomplete backup can be mistaken for a successful one when the return code is dropped. The risk is real for BUSY/LOCKED, which return without throwing by design.
- **Proposed fix:** Annotate the method `[[nodiscard]]` to force callers to inspect the result. Because the library targets C++11, gate it behind a feature macro, e.g. `#if defined(__cplusplus) && __cplusplus >= 201703L` define `SQLITECPP_NODISCARD [[nodiscard]]` else empty, then `SQLITECPP_NODISCARD int executeStep(...)`. Separately, consider documenting (or fixing) that `Database::backup()` ignores a possible BUSY/LOCKED return from its single `executeStep()` call.

### [BKP-02] No move constructor/assignment: declaring deleted copy ops suppresses implicit moves, making `Backup` non-movable
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Backup.h:96-98,128`
- **Description:** The class user-declares the copy constructor and copy-assignment as `= delete`. Per the C++11 rules (`[class.copy]`), user-declaring *any* copy operation suppresses the implicit declaration of the move constructor and move-assignment operator. The sole data member is a movable `std::unique_ptr`, so move semantics would be both natural and cheap, but as written `Backup` is neither copyable nor movable. Sibling RAII classes in this library are inconsistent here (e.g. `Statement` and `Column` are explicitly movable), so this is a gap rather than an intentional seal.
- **Impact:** A `Backup` cannot be returned by value from a factory, stored in a `std::vector`, or otherwise relocated. This is a usability limitation, not a bug — current usage (`Database::backup()` constructs it in place) is unaffected.
- **Proposed fix:** If movability is desired for API symmetry, add `Backup(Backup&&) = default;` and `Backup& operator=(Backup&&) = default;`. The defaulted moves are correct because `unique_ptr` move leaves the source holding `nullptr`, and the `Deleter` already null-guards. If non-movability is intentional, add a short comment stating so.

### [BKP-03] `getRemainingPageCount()` / `getTotalPageCount()` are not `[[nodiscard]]` and lack `noexcept`
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Backup.h:116,119`; `src/Backup.cpp:63-72`
- **Description:** Both accessors are pure const getters that forward to `sqlite3_backup_remaining`/`sqlite3_backup_pagecount`. Those C functions cannot fail and do not throw; the C++ wrappers do nothing that can throw. They are correctly `const`, but they are neither `noexcept` nor return-value-enforced. `Database::getHandle()` in the same library is marked `noexcept` (`Database.h:475`), so the convention exists.
- **Impact:** Minor: lost optimization/contract clarity; a discarded page count is almost always a caller bug but is not flagged.
- **Proposed fix:** Mark both `noexcept`. Optionally add the same gated `SQLITECPP_NODISCARD` as in BKP-01.

### [BKP-04] Inconsistent error construction between init failure and step failure
- **Severity:** Low
- **Confidence:** Medium
- **Category:** api/modernization
- **Location:** `src/Backup.cpp:34` vs `src/Backup.cpp:57`
- **Description:** On NULL init, the exception is built from the destination `sqlite3*` handle via `Exception(aDestDatabase.getHandle())`, which captures the real `errmsg`, `errcode`, and crucially the *extended* error code (`Exception.cpp:25-30`). On a fatal `executeStep()` failure the exception is built as `Exception(sqlite3_errstr(res), res)` — a generic string from the bare result code, with `mExtendedErrcode` left at `-1` and no connection-specific message. For backup-step failures the most useful diagnostics (e.g. `SQLITE_IOERR_XXX` extended codes, see `sqlite3.h:9624`) live on the destination connection and are discarded. This is arguably acceptable because `sqlite3_backup_step` does not document that the error is reliably posted to a connection's `errcode`, but it produces strictly poorer diagnostics than the init path.
- **Impact:** Callers catching the step exception get a generic message and no extended error code, hampering diagnosis of I/O errors during a backup.
- **Proposed fix:** Consider constructing the step exception from a stored destination `sqlite3*` handle, e.g. `Exception(pDestHandle, res)`, to recover the extended error code and message. This requires retaining the destination handle as a member (see BKP-05). If the simpler form is kept intentionally, document why.

### [BKP-05] No retained reference to the source/destination `Database`; lifetime relationship is undocumented
- **Severity:** Info
- **Confidence:** High
- **Category:** memory
- **Location:** `src/Backup.cpp:22-49` (constructors); `include/SQLiteCpp/Backup.h:128`
- **Description:** The constructors take the two `Database&` arguments only to extract their raw `sqlite3*` handles into `sqlite3_backup_init`; no reference or shared ownership is retained (the only member is `mpSQLiteBackup`). The `sqlite3_backup` object internally holds the two connection pointers and remains valid only while both connections are open (`sqlite3.h:9716-9728`). If either `Database` is destroyed before the `Backup`, the `Backup`'s handle dangles and `sqlite3_backup_finish` in the `Deleter` operates on freed connections — undefined behavior. Nothing in the wrapper prevents this; correctness relies entirely on caller-enforced ordering.
- **Impact:** Latent use-after-free if a caller outlives a `Database` with a live `Backup`. Current in-library usage (`Database::backup()`) is safe because both `Database` objects outlive the local `Backup`.
- **Proposed fix:** This is inherent to the design and acceptable for a thin wrapper, but it should be documented. Add a note in the class/constructor Doxygen that the `Backup` must not outlive either `Database`. Retaining the destination handle for BKP-04 does not change the lifetime contract.

### [BKP-06] `Deleter` swallows the `sqlite3_backup_finish` return code (correct, but worth a note)
- **Severity:** Info
- **Confidence:** High
- **Category:** thread/exception
- **Location:** `src/Backup.cpp:75-81`
- **Description:** The `Deleter::operator()` calls `sqlite3_backup_finish(apBackup)` and discards its return value. This is correct and required: the deleter runs during `unique_ptr` destruction, which is implicitly `noexcept`, so it must not throw. Per `sqlite3.h:9680-9689`, `finish` returns the result code of the *last step error* (OOM/IOERR), or `SQLITE_OK` otherwise, and BUSY/LOCKED do not affect it. There is genuinely no useful action to take in a destructor, and any per-step error has already been surfaced by `executeStep()` throwing. The null-guard (`if (apBackup)`) is also correct and consistent with the SQLite contract that `sqlite3_backup_finish(NULL)` is a harmless no-op returning `SQLITE_OK`.
- **Impact:** None — this is correct destructor behavior.
- **Proposed fix:** None required. Optionally document that finish errors are intentionally ignored at destruction since they were already reported by `executeStep()`.

## Notes / non-issues

- **NULL-init error handle is correct.** `Backup(Database&, const char*, Database&, const char*)` fetches the error from `aDestDatabase.getHandle()` (the destination), matching the SQLite contract exactly: `sqlite3.h:9599-9604` states the error code/message for a failed `sqlite3_backup_init(D,N,S,M)` are stored in destination connection `D`. The delegating `std::string` and main/main constructors inherit this correctly. The `initException` test (`tests/Backup_test.cpp:24-38`) exercises this (init fails because source==destination, see `sqlite3.h:9592`) and asserts the throw.

- **`executeStep()` result-code filtering matches the contract.** It rethrows for any code other than `OK/DONE/BUSY/LOCKED`. Against `sqlite3.h:9620-9650`: `SQLITE_OK` = more pages remain, `SQLITE_DONE` = complete, `SQLITE_BUSY`/`SQLITE_LOCKED` = transient/retriable. The fatal codes (`SQLITE_READONLY`, `SQLITE_NOMEM`, `SQLITE_IOERR_XXX`) correctly fall through to the throw. The header doc comment (`Backup.h:101-110`) accurately describes this. Tests cover `OK` (`executeStepOne`), `DONE` (`executeStepOne`/`executeStepAll`), and the readonly-destination throw (`executeStepException`, `tests/Backup_test.cpp:106-127`).

- **`SQLITE_DONE` is handled distinctly from `SQLITE_OK`** — both are returned to the caller rather than thrown, and the return value lets the caller distinguish them; the tests rely on this distinction (`ASSERT_EQ(SQLITE_DONE, res)` vs `ASSERT_EQ(SQLite::OK, res)`).

- **`getRemainingPageCount()` / `getTotalPageCount()` cannot hit a NULL handle.** Because the constructor throws on NULL init, a successfully-constructed `Backup` always holds a non-NULL handle, so `mpSQLiteBackup.get()` is never NULL in these accessors. Their "most recent `executeStep()`" semantics (`sqlite3.h:9694-9704`) are reflected accurately in the brief doc comments.

- **`unique_ptr` + custom `Deleter` is the right ownership model.** Exactly one `sqlite3_backup_finish` per successful `sqlite3_backup_init` (required by `sqlite3.h:9577-9578`) is guaranteed: init populates the pointer, the deleter finishes it once at destruction, and a failed init leaves the pointer NULL (deleter no-ops). No double-finish or leak path exists.

- **Thread-safety is appropriately the caller's responsibility.** `sqlite3.h:9731-9734` notes `remaining()`/`pagecount()` must not run concurrently with `step()` on the same backup object; the wrapper adds no synchronization, which is the correct, conventional choice for this library — flagged only as documentation context, not a defect.
