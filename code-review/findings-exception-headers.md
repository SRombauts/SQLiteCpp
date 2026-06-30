# Findings — Exception unit + header-only utilities

Files: `Exception.h/.cpp` + test, `VariadicBind.h` + test, `ExecuteMany.h` + test, `Utils.h`, `Assertion.h`, `SQLiteCpp.h`, `SQLiteCppExport.h`.

- **Exception** derives from `std::runtime_error`; `what()` is owned by the base (safe to copy/throw); `mErrcode`/`mExtendedErrcode` are trivially copyable. No correctness defect found.
- **VariadicBind / ExecuteMany** are the public convenience API over `Statement::bind`.
- **Utils / Assertion / Export / umbrella** are macro/utility headers.

## Findings (most severe first)

### UTL-001 — `SQLITECPP_ASSERT` handler branch is an unguarded `if` (dangling-else / shape mismatch)
- [ ] **Severity:** medium — **Confidence:** high — **Category:** correctness / api-footgun
- **Location:** `include/SQLiteCpp/Assertion.h:36-37`
- **Impact (failure scenario):** With `SQLITECPP_ENABLE_ASSERT_HANDLER`, the macro expands to a bare `if (!(expression)) SQLite::assertion_failed(...)`. (1) `if (cond) SQLITECPP_ASSERT(x,"m"); else foo();` reroutes the user's `else` to the macro's hidden `if`. (2) The handler branch is an `if` *statement* while the `assert()` branch (line 45) is a single expression statement, so the macro changes statement shape between configurations. It is invoked from destructors (e.g. `Database::Deleter`), so mis-expansion is configuration-dependent and easy to miss.
- **Fix:** Wrap the handler branch as `do { if (!(expression)) SQLite::assertion_failed(...); } while (0)` (or a `(void)`/ternary expression mirroring the `assert` branch).

### VB-001 — `bind()` takes args by `const&` then `std::forward`s them (dead forward; forces copies, defeats `bindNoCopy`/move selection)
- [ ] **Severity:** medium — **Confidence:** high — **Category:** api-design / performance
- **Location:** `include/SQLiteCpp/VariadicBind.h:48,52`
- **Impact (failure scenario):** The signature is `void bind(Statement&, const Args&... args)`, so `std::forward<decltype(args)>(args)` always yields `const Args&` — the forward is dead code. `Statement::bind` therefore only ever sees const lvalues: an rvalue `std::string`/blob can never select a move/`bindNoCopy` overload and is always deep-copied into SQLite. The C++14 tuple path inherits this. The misleading `std::forward` implies a perfect forwarding that does not exist.
- **Fix:** Make it a true forwarding reference: `template<class... Args> void bind(Statement& query, Args&&... args)` with `std::forward<Args>(args)`. Re-run existing tests (incl. the too-many-args throw case); add a regression test binding an rvalue `std::string`.

### EM-001 — First parameter set bound without the reset/clearBindings applied to all others (asymmetry)
- [ ] **Severity:** low — **Confidence:** high — **Category:** correctness / maintainability
- **Location:** `include/SQLiteCpp/ExecuteMany.h:50-53`
- **Impact (failure scenario):** `bind_exec` handles the first set; `reset_bind_exec` (reset + clearBindings) handles the rest. Correct today only because the `Statement` is freshly constructed; any refactor reusing an existing statement, or a caller copying the pattern, would carry stale state on the first set but not the rest.
- **Fix:** Route all sets (including the first) through `reset_bind_exec`, or make first-set handling identical.

### HDR-003 — `WIN32` vs `_WIN32` used inconsistently in the same file
- [ ] **Severity:** low — **Confidence:** medium — **Category:** portability
- **Location:** `include/SQLiteCpp/SQLiteCppExport.h:21` (`_WIN32`) vs `:35` (`WIN32`)
- **Impact (failure scenario):** `_WIN32` is always defined on Windows; `WIN32` only by some SDKs/flags. The export block keys off `_WIN32` (correct), but the C4251/C4275 warning-suppression block keys off `WIN32`. On a DLL build where `WIN32` is undefined, the `#pragma warning(disable: 4251/4275)` is skipped and consumers get spurious warnings about exporting the `std::runtime_error`-derived `Exception` across the DLL boundary.
- **Fix:** Use `_WIN32` in both blocks.

### VB-002 — Tuple `bind` overloads ambiguous for a tuple whose single element is itself a tuple
- [ ] **Severity:** low — **Confidence:** medium — **Category:** correctness / api-footgun
- **Location:** `include/SQLiteCpp/VariadicBind.h:74-94`
- **Impact (failure scenario):** For ordinary tuples the `const std::tuple<Types...>&` overload wins, but for nested cases (`std::make_tuple(std::make_tuple(1))`) the API cannot express "bind as single value" vs "expand", and `Statement::bind` has no tuple overload, so the single-value reading would not compile. Under-specified corner, not a crash.
- **Fix:** Pin the dispatch behavior with a tuple-of-tuple test (document the rule in the commit message/API docs).

### UTL-002 — Header redefines the standard identifier `__func__` for MSVC
- [ ] **Severity:** low — **Confidence:** medium — **Category:** portability / standards
- **Location:** `include/SQLiteCpp/Assertion.h:32-34`
- **Impact (failure scenario):** `#define __func__ __FUNCTION__` macro-replaces a reserved predefined identifier and is never `#undef`-ed, leaking into every TU that includes `Assertion.h` with the handler enabled; any later `__func__` in user/third-party code is silently rewritten. Modern MSVC (≥VS2015) already provides a conforming `__func__`, so it is unnecessary, and redefining a reserved name is technically UB.
- **Fix:** Drop it for current MSVC, or use `__FUNCTION__` directly in the MSVC path without redefining `__func__`.

### HDR-002 — Doc comment names a macro that does not exist (`SQLITECPP_EXPORT` vs `SQLITECPP_DLL_EXPORT`)
- [ ] **Severity:** low — **Confidence:** high — **Category:** documentation
- **Location:** `include/SQLiteCpp/SQLiteCppExport.h:17`
- **Impact (failure scenario):** The comment tells users to `#define SQLITECPP_EXPORT`, but the code (line 22) and both build systems use `SQLITECPP_DLL_EXPORT`. Following the doc yields `dllimport` while building a DLL → link errors.
- **Fix:** Correct the comment to `SQLITECPP_DLL_EXPORT`.

## Verified non-issues
- **HDR-001 (reconciled — NOT a bug):** One reviewer flagged `#if SQLITECPP_DLL_EXPORT` (`SQLiteCppExport.h:22`) as an ill-formed empty-macro test. **Verified false:** both GCC/Clang `-DNAME` and MSVC `/DNAME` define a value-less macro to `1`, so the directive is `#if 1`. The enclosing block is gated to MSVC-DLL builds (`_WIN32 && !__GNUC__ && SQLITECPP_COMPILE_DLL`), where CMake passes `-DSQLITECPP_DLL_EXPORT`; the MSVC `BUILD_SHARED_LIBS=ON` CI job passes. The only residual nit is stylistic (an explicit `-DSQLITECPP_DLL_EXPORT=0` would silently flip to `dllimport`); using `#ifdef` would be marginally more robust but is not required.
- **Pack evaluation order (VariadicBind.h:52):** the expansion is inside a braced `std::initializer_list<int>{...}` → guaranteed left-to-right (`[dcl.init.list]/4`); `++pos` yields 1,2,3… correctly. This is the canonical fix, not a bug.
- **`Exception::what()` lifetime / copy / assignment:** message owned by the `std::runtime_error` base; all members trivially copyable; implicit special members correct. Tested.
- **Null `const char*` message** substituted with `""` before the base ctor. Tested.
- **`getErrorStr()` uses the primary code** — deliberate; asserted by tests.
- **`clearBindings()` between sets in `execute_many`** is present and regression-tested (`ExecuteMany.decreasingArity`).
- **Version sync:** `SQLITECPP_VERSION "3.03.03"` / `3003003` match `project(... VERSION 3.3.3)`.
- **C++11/14 guards** (`__cplusplus >= 201402L || _MSC_VER >= 1900`) correctly gate the tuple/`index_sequence` machinery while keeping variadic `bind` at C++11.
- **`SQLITECPP_PURE_FUNC` detection (Utils.h):** properly nested behind compiler-family + `__has_attribute` checks with an empty fallback.
