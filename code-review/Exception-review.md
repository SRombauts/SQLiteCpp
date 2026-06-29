# Exception — Review

## Summary
`SQLite::Exception` is a small, well-behaved `std::runtime_error` subclass storing two `int` error codes; the stored message lifetime is correctly delegated to the base class (no dangling-pointer or ownership bug), the getters are correctly `noexcept`, and the copy/move/assignment semantics are exercised by the tests. The most-publicized concern — constructing from a null/failed `sqlite3*` — is in fact **safe**: the SQLite C API defines `sqlite3_errmsg/errcode/extended_errcode(NULL)` to return valid static strings / `SQLITE_NOMEM`, not UB (verified against the bundled amalgamation). Remaining findings are an API-consistency bug in `getExtendedErrorCode()` for the non-`sqlite3*` constructors, a real (if hard-to-hit) UB path if a null `const char*` message is ever passed, and several modernization opportunities.

Finding counts by severity: Critical 0, High 0, Medium 2, Low 3, Info 2.

## Findings

### [EXC-01] Extended error code is silently `-1` whenever a result code is known but no `sqlite3*` handle is supplied
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug
- **Location:** `src/Exception.cpp:18-23` (constructor `Exception(const char*, int)`); contrasted with `src/Exception.cpp:32-37`. Reached e.g. from `src/Backup.cpp:57` (`SQLite::Exception(sqlite3_errstr(res), res)`), `src/Statement.cpp:175,198` (`Exception("...", ret)`), `src/Transaction.cpp:37`, `include/SQLiteCpp/Statement.h:673`.
- **Description:** `Exception(const char* aErrorMessage, int ret)` always sets `mExtendedErrcode(-1)`, regardless of `ret`. The documented contract for the getter (`Exception.h:77` "Return the extended numeric result code (if any, otherwise -1)") and the symmetry with the `sqlite3*,int` overload imply that when a real result code is known, the extended code should reflect it. For all error paths that have a result code but construct via `(const char*, int)` — notably `Backup::executeStep()` (`Backup.cpp:57`) which passes the genuine `res` — `getExtendedErrorCode()` returns `-1` while `getErrorCode()` returns the real code. The two getters therefore disagree for the *same* logical error depending only on which constructor the call site happened to pick.
- **Impact:** Callers that branch on `getExtendedErrorCode()` (e.g. to distinguish `SQLITE_IOERR_*` / `SQLITE_CONSTRAINT_*` subtypes) get `-1` for many real errors, even when the primary code is present. Inconsistent/unreliable API behavior rather than a crash.
- **Proposed fix:** When no handle is available, the most defensible behavior is to seed the extended code from the primary code: `mExtendedErrcode(ret)` in the `(const char*, int)` constructor (the primary code *is* a valid extended code at the low byte). At minimum, document explicitly that the extended code is only populated by the `sqlite3*` constructors and stays `-1` otherwise. Note this would change the existing test expectations in `tests/Exception_test.cpp:39,65,73,79,86` (which assert `-1`), so the tests must be updated in lockstep — flagging it as a deliberate API decision for the maintainer.

### [EXC-02] `Exception(const char*, int)` forwards a possibly-null pointer to `std::runtime_error`, which is undefined behavior
- **Severity:** Medium
- **Confidence:** Medium
- **Category:** bug
- **Location:** `src/Exception.cpp:18-23` (`std::runtime_error(aErrorMessage)`), reachable via `Exception.h:35,47-48`.
- **Description:** The constructor passes `aErrorMessage` straight into `std::runtime_error(const char*)`. Constructing `std::runtime_error` (or `std::string`) from a null `const char*` is undefined behavior (the standard library calls `strlen`/`char_traits::length` on it). The public constructor `Exception(const char*, int)` and `explicit Exception(const char*)` are part of the API surface, so a caller (internal or external) passing `nullptr` triggers UB. Internally the closest risk is `Backup.cpp:57` `SQLite::Exception(sqlite3_errstr(res), res)`: `sqlite3_errstr` is verified to never return null (it defaults to the literal `"unknown error"`, `sqlite3.c:188882`), so today no in-tree call site actually passes null. This is a latent robustness/contract gap rather than an active in-tree crash.
- **Impact:** UB (typically a crash via null deref in `strlen`) if any current or future caller — or an external user of the public header — passes a null message. The `sqlite3*` overloads are *not* affected because `sqlite3_errmsg(NULL)` is defined to return a valid static string (see Notes).
- **Proposed fix:** Guard the message in the `const char*` constructor, e.g. `std::runtime_error(aErrorMessage ? aErrorMessage : "")` (or substitute `sqlite3_errstr`/"unknown error"). This is a one-line defensive change with no downside and removes the only UB path in the unit.

### [EXC-03] Getters and `getErrorStr()` are good candidates for `[[nodiscard]]` (purely informational accessors)
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Exception.h:72,78,84`.
- **Description:** `getErrorCode()`, `getExtendedErrorCode()`, and `getErrorStr()` are pure observers whose return value is the entire point of the call; discarding it is almost always a bug. The library targets C++11 where `[[nodiscard]]` (C++17) is unavailable, but the project already gates newer features by `__cplusplus`/feature macros (e.g. C++14 helpers, `SQLITECPP_PURE_FUNC` in `Utils.h`). A conditionally-defined `SQLITECPP_NODISCARD` macro would be consistent with existing practice.
- **Impact:** Missed compile-time diagnostics for accidental value-discarding; no runtime effect.
- **Proposed fix:** Add a `SQLITECPP_NODISCARD` macro (expanding to `[[nodiscard]]` only when `__cplusplus >= 201703L`) and apply it to the three accessors. Optional, low priority.

### [EXC-04] `getErrorStr()` could be `const noexcept` and marked pure; consider documenting the static-storage lifetime of the returned pointer
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Exception.h:84`; `src/Exception.cpp:40-43`.
- **Description:** `getErrorStr()` is already `const noexcept` (good, and correct: `sqlite3_errstr` is `noexcept` in practice and never throws). The returned `const char*` points into SQLite's process-lifetime static `aMsg[]` table / string literals (`sqlite3.c:188847-188900`), so it is safe to hold indefinitely — but the Doxygen comment ("Return a string, solely based on the error code") does not state the lifetime/ownership, unlike the documented caveats elsewhere in the library for raw pointers. The function is also a candidate for the existing `SQLITECPP_PURE_FUNC` attribute (depends only on `mErrcode`).
- **Impact:** Documentation gap only; behavior is correct.
- **Proposed fix:** Extend the doc comment to note the returned pointer has static storage duration and must not be freed (mirroring the `sqlite3_errstr` contract), and optionally tag with `SQLITECPP_PURE_FUNC`.

### [EXC-05] `mErrcode` / `mExtendedErrcode` are not `const`, and the class relies on implicitly-generated copy/move
- **Severity:** Low
- **Confidence:** Medium
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Exception.h:87-89`.
- **Description:** The error codes are logically immutable after construction but are non-`const` data members. They are left mutable specifically so the compiler can still generate the copy-assignment operator that `std::exception` requires to be available (the test `Exception_test.cpp:29-41` deliberately exercises `operator=`). This is the correct trade-off — making them `const` would delete copy/move assignment and break that contract — but it is a subtle, intentional decision that is undocumented. The implicitly-defaulted copy/move/assignment are correct here because both members are trivially copyable `int`s and the base `std::runtime_error` provides its own.
- **Impact:** None functionally; a maintainer could "tidy up" by adding `const` and silently break the assignability contract.
- **Proposed fix:** Add a short comment explaining the members are intentionally non-`const` to preserve copy-assignability required of exception types. No code change otherwise.

### [EXC-06] Header formatting/typo nits in Doxygen comments
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Exception.h:56` (mis-indented `/**` for the `sqlite3*` constructor), and `tests/Exception_test.cpp:1-10` (file header still titled `Transaction_test.cpp`), `tests/Exception_test.cpp:28` ("avaiable" typo).
- **Description:** The Doxygen block at line 56 has a stray leading space (`   /**`) breaking alignment; the test file's header block was copy-pasted from `Transaction_test.cpp` and never retitled. Cosmetic only.
- **Impact:** None; minor readability/doc-generation tidiness.
- **Proposed fix:** Fix indentation and the test-file header/typo when next touching these files.

### [EXC-07] Two `const char*` constructors and two `std::string` constructors duplicate the "-1 default code" magic value
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Exception.h:48,52`.
- **Description:** The sentinel `-1` ("0 would be SQLITE_OK, which doesn't make sense") is hard-coded in two delegating constructors. The intent is well-commented, but the magic number is repeated. Constructor delegation is already used well here (string overloads delegate to the `const char*` overloads, which delegate to the master constructor), so this is minor.
- **Impact:** None; minor maintainability.
- **Proposed fix:** Optionally introduce a named constant (e.g. a private `static constexpr int NO_ERROR_CODE = -1;`) and reuse it. Low value.

## Notes / non-issues

- **Null / failed `sqlite3*` handle is NOT undefined behavior (the headline concern is a non-issue).** `Database::Database` (`src/Database.cpp:69-75`) can reach `throw SQLite::Exception(handle, ret)` with `handle == nullptr`, because `sqlite3_open_v2` writes `NULL` into `*ppDb` on allocation failure (documented at `sqlite3.h:3719-3723`). I verified the bundled amalgamation: `sqlite3_errmsg(NULL)` returns `sqlite3ErrStr(SQLITE_NOMEM_BKPT)` (a valid static `"out of memory"` string, `sqlite3.c:189909-189913`), `sqlite3_errcode(NULL)`/`sqlite3_extended_errcode(NULL)` both return `SQLITE_NOMEM` (`sqlite3.c:190015-190032`), and `sqlite3ErrStr` always returns a non-null static string (`sqlite3.c:188846-188900`). So the `sqlite3*` constructors are fully defined for a null handle and never pass null to `std::runtime_error`. The scoring-matrix note "constructors dereference `sqlite3*` ... (assumes non-null)" overstates the risk — no guard is required for the SQLite functions themselves.

- **Stored message lifetime is correct.** `Exception` stores no `const char*`; it forwards the message to the `std::runtime_error` base, which copies it into its own internally ref-counted `std::string`. `what()` (inherited, `noexcept`) returns a pointer owned by the base for the lifetime of the exception (and copies). There is no dangling-pointer risk even though `sqlite3_errmsg`'s buffer is transient — the copy happens during construction, before the buffer can be overwritten. Verified by `Exception_test.cpp:18-25,43-54`.

- **`getErrorCode()` / `getExtendedErrorCode()` capture-at-throw-time is correct for the `sqlite3*` constructors.** Both codes are read in the constructor initializer list (`src/Exception.cpp:27-28,35`) at throw time, not lazily later, so they reflect the connection state at the moment of the error and are immune to later API calls mutating `db->errCode`. This matches the SQLite contract that the error code "might change with each API call" (`sqlite3.h:4193-4194`) — capturing eagerly is the right design.

- **`noexcept` correctness.** `getErrorCode()`/`getExtendedErrorCode()` (`Exception.h:72,78`) trivially cannot throw; `getErrorStr()` (`Exception.h:84`) calls only `sqlite3_errstr`, which does not throw. The `noexcept` annotations are all sound. `what()` is inherited and already `noexcept`.

- **Security: no attack surface.** The unit performs no parsing, allocation it owns, or I/O; it only stores ints and forwards a string. Confirmed no security concern (consistent with the matrix's Security Risk 1).

- **`Exception(sqlite3*, int ret)` intentionally mixes sources.** It stores the caller-supplied `ret` as the primary code but pulls the message and extended code from the live handle (`src/Exception.cpp:32-37`). This is deliberate and correct: the caller usually has the precise function return value, while the handle yields the richer extended code and human-readable message. Not a bug.
