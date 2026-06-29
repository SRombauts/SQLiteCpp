# Trivial/support headers — Review

## Summary
Reviewed the four support headers: `Utils.h` (`SQLITECPP_PURE_FUNC`), `Assertion.h` (`SQLITECPP_ASSERT` + optional user handler), `SQLiteCppExport.h` (DLL/visibility macros), and `SQLiteCpp.h` (umbrella include + version macros). The code is low-complexity and mostly correct, but there are two genuine defects worth fixing: a **dangling-else / single-evaluation hazard** in the `SQLITECPP_ASSERT` macro when the user handler is enabled (`Assertion.h`), and a **declared-vs-defined signature mismatch** for `assertion_failed` (`int apLine` in the header vs `long apLine` in every example/test/docs definition), which is technically an ODR violation. The remaining items are Low/Info: a fragile `SQLITECPP_DLL_EXPORT` truthiness test, an unconditional `__func__` redefinition under MSVC, `WIN32` vs `_WIN32` inconsistency, an intentional-but-undocumented umbrella-header omission, and a non-canonical version string. No Critical/High findings.

Finding counts by severity: Medium 2, Low 4, Info 2.
- `Assertion.h`: HDR-01 (Medium), HDR-02 (Medium), HDR-05 (Low), HDR-08 (Info)
- `SQLiteCppExport.h`: HDR-03 (Low), HDR-04 (Low)
- `SQLiteCpp.h`: HDR-06 (Low), HDR-07 (Info)
- `Utils.h`: no findings (see Notes)

## Findings

### [HDR-01] `SQLITECPP_ASSERT` (handler path) is an unbraced `if` — dangling-else / scope-capture hazard
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug
- **Location:** `include/SQLiteCpp/Assertion.h`:36-37
- **Description:** When `SQLITECPP_ENABLE_ASSERT_HANDLER` is defined, the macro expands to a bare `if` statement without braces and is **not** wrapped in a `do { ... } while(0)` idiom:
  ```cpp
  #define SQLITECPP_ASSERT(expression, message) \
      if (!(expression))  SQLite::assertion_failed(__FILE__, __LINE__, __func__, #expression, message)
  ```
  This is the classic unsafe function-like-statement-macro pattern. Two concrete failure modes:
  1. **Dangling else.** `if (cond) SQLITECPP_ASSERT(x, "m"); else foo();` binds the user's `else` to the macro's internal `if`, silently changing control flow.
  2. **Body capture in an unbraced branch.** `if (cond) SQLITECPP_ASSERT(x, "m");` followed by an unbraced loop/branch behaves unexpectedly because the macro itself is a compound prefix.

  The non-handler path (`assert(expression && message)`) does not have this problem because `assert` already expands to a single self-contained expression-statement. Note the two existing call sites in this repo (`Database.cpp:92`, `example1/main.cpp:420,453`) happen to be at statement scope followed by `;`, so they are currently safe — but the macro is part of the public API (documented in README, used in user destructors), so any consumer can trip this.
- **Impact:** Silent, hard-to-diagnose control-flow change in user code that uses `SQLITECPP_ASSERT` inside an unbraced `if`/`else`. Only affects the `SQLITECPP_ENABLE_ASSERT_HANDLER` build configuration.
- **Proposed fix:** Wrap in the standard `do { ... } while(0)` guard so the macro is a single statement requiring a trailing `;`:
  ```cpp
  #define SQLITECPP_ASSERT(expression, message) \
      do { if (!(expression)) SQLite::assertion_failed(__FILE__, __LINE__, __func__, #expression, message); } while (0)
  ```

### [HDR-02] `assertion_failed` declared with `int apLine` but defined everywhere with `long apLine` — ODR / link mismatch
- **Severity:** Medium
- **Confidence:** High
- **Category:** build/abi
- **Location:** `include/SQLiteCpp/Assertion.h`:29-30 (declaration) vs definitions in `tests/Database_test.cpp`:29, `examples/example1/main.cpp`:26, `examples/example2/src/main.cpp`:24, and `README.md`/`docs/README.md` (the copy-paste template users follow)
- **Description:** The header declares:
  ```cpp
  void assertion_failed(const char* apFile, const int apLine, const char* apFunc,
                          const char* apExpr, const char* apMsg);
  ```
  Every actual definition the project ships and every documented template uses `const long apLine` instead of `const int apLine`. `int` and `long` are distinct types in C++ (even where they share a width), so the user-provided `SQLite::assertion_failed(const char*, long, ...)` is a **different function** from the one the library's `SQLITECPP_ASSERT` call site references (`const char*, int, ...`). In practice the call site passes `__LINE__` (an `int`), and the library is what calls the function — so the linker needs the symbol matching the *header* signature (`int`). The shipped definitions with `long` therefore define a function the library never calls, and the `int` overload is left undefined. This "works" today only because:
  - In the default config (`SQLITECPP_ENABLE_ASSERT_HANDLER` OFF) `assertion_failed` is never referenced, so no link error surfaces; and
  - The single in-tree consumer of the handler config (`Database.cpp`) is compiled into the same translation-unit set, and assertions rarely fire, so the missing-symbol link error is masked unless the handler path is actually exercised with the handler build flag on.
  This is a latent ODR/linkage bug and a documentation defect: anyone enabling `SQLITECPP_ENABLE_ASSERT_HANDLER` and copying the documented `long` signature gets an unresolved-symbol link error against the `int` declaration (or, with name-mangling differences, a silent mismatch).
- **Impact:** Unresolved-external link failure (or undefined behavior) for downstream projects that enable the assert handler and follow the documented `long apLine` signature. The header and all docs/examples disagree on the public ABI.
- **Proposed fix:** Make the declaration and all definitions/docs use the same type. Recommended: change the header declaration to `const long apLine` to match the existing examples/tests/docs (smaller blast radius, since those are what users copy), OR change all examples/tests/docs to `const int apLine`. Pick one and apply consistently across `Assertion.h`, `tests/Database_test.cpp`, both examples, and both README files.

### [HDR-03] `#if SQLITECPP_DLL_EXPORT` uses value-truthiness, not `defined()` — fragile and warns under `-Wundef`
- **Severity:** Low
- **Confidence:** High
- **Category:** build/abi
- **Location:** `include/SQLiteCpp/SQLiteCppExport.h`:22
- **Description:** Inside the `defined(SQLITECPP_COMPILE_DLL)` block the export decision is `#if SQLITECPP_DLL_EXPORT`. The build systems define this macro with **no value** (CMake `target_compile_definitions(... PRIVATE "SQLITECPP_DLL_EXPORT")` → `-DSQLITECPP_DLL_EXPORT`; meson `-DSQLITECPP_DLL_EXPORT`). A bare `-DSQLITECPP_DLL_EXPORT` makes the macro expand to `1` on most compilers (MSVC/GCC/Clang treat `-DFOO` as `FOO=1`), so `#if SQLITECPP_DLL_EXPORT` currently evaluates true and the build works. However:
  - If `SQLITECPP_DLL_EXPORT` is *not* defined (the import side: CMake/meson define only `SQLITECPP_COMPILE_DLL` for consumers), `#if SQLITECPP_DLL_EXPORT` treats the undefined identifier as `0` — which is the intended import behavior, but relies on the undefined-macro-is-0 rule and emits a warning under `-Wundef`.
  - The header's own doc comment (lines 16-17) refers to a macro named `SQLITECPP_EXPORT`, not `SQLITECPP_DLL_EXPORT` — stale/incorrect documentation of the toggle.
- **Impact:** Works in the sanctioned CMake/meson flows, but is brittle for hand-rolled builds and produces `-Wundef` warnings; the comment misnames the controlling macro.
- **Proposed fix:** Use `#if defined(SQLITECPP_DLL_EXPORT)` for a presence test (matches how the build systems define it), and fix the comment at lines 16-17 to reference `SQLITECPP_DLL_EXPORT` rather than `SQLITECPP_EXPORT`.

### [HDR-04] `WIN32` vs `_WIN32` inconsistency in the warning-suppression block
- **Severity:** Low
- **Confidence:** High
- **Category:** build/abi
- **Location:** `include/SQLiteCpp/SQLiteCppExport.h`:21 vs 35
- **Description:** The export/import selection uses `defined(_WIN32)` (line 21), the canonical predefined macro that MSVC always defines. The warning-disable block (line 35) uses `defined(WIN32)`. `WIN32` (no leading underscore) is **not** an MSVC-predefined macro by default — it is supplied by `<windows.h>` / the Windows SDK / older project settings, so it may or may not be defined depending on include order and build configuration. The two blocks gate on different conditions even though both intend "Windows DLL build". As a result the C4251/C4275 warning suppressions (line 36-37) may silently not apply in builds where `WIN32` is undefined but `_WIN32` is defined.
- **Impact:** Inconsistent; the DLL-interface warning suppression can fail to engage, producing noisy C4251/C4275 warnings on some valid MSVC DLL builds. Functionally harmless otherwise.
- **Proposed fix:** Use `_WIN32` consistently in both `#if` blocks (line 35 → `#if defined(_WIN32) && defined(SQLITECPP_COMPILE_DLL)`).

### [HDR-05] Unconditional `#define __func__ __FUNCTION__` redefines a reserved/standard identifier under MSVC
- **Severity:** Low
- **Confidence:** Medium
- **Category:** bug
- **Location:** `include/SQLiteCpp/Assertion.h`:32-34
- **Description:** Under `_MSC_VER` (and only when `SQLITECPP_ENABLE_ASSERT_HANDLER` is on), the header does `#define __func__ __FUNCTION__` with no `#ifndef` guard. `__func__` is a C++11 standard predefined function-local identifier, and modern MSVC (VS 2015+) supports it; identifiers with leading double underscore are reserved. Unconditionally `#define`-ing it (a) is undefined behavior per the standard for redefining a reserved identifier, (b) leaks into every TU that includes `Assertion.h` after this point (the `#define` is never `#undef`-ed and escapes the `namespace SQLite { ... }` block since macros ignore C++ scope), and (c) replaces standard `__func__` with MSVC's `__FUNCTION__` extension everywhere downstream. Because `SQLiteCpp.h` includes `Assertion.h` first, this macro is active for all subsequently-included SQLiteCpp headers and user code in the handler build.
- **Impact:** Low in practice (only the handler config, MSVC only, and `__FUNCTION__` yields a similar string), but it is technically UB and pollutes the global macro namespace for all downstream code. Could conflict if user code or another library also manipulates `__func__`.
- **Proposed fix:** Guard it (`#if defined(_MSC_VER) && !defined(__func__)`) and ideally restrict to old MSVC that lacks `__func__`; since SQLiteCpp targets VS that supports `__func__`, the redefinition can likely be removed entirely. At minimum wrap in `#ifndef __func__`.

### [HDR-06] Umbrella `SQLiteCpp.h` omits `Backup.h`, `Savepoint.h`, `VariadicBind.h`, `ExecuteMany.h` without documentation
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/SQLiteCpp.h`:20-27
- **Description:** The header comment claims it provides "access to all functionality provided by the wrapper" (lines 5-6, 15), but it includes only `SQLiteCppExport.h`, `Assertion.h`, `Exception.h`, `Database.h`, `Statement.h`, `Column.h`, `Transaction.h`. Four public headers that ship in `include/SQLiteCpp/` are not included: `Backup.h`, `Savepoint.h`, `VariadicBind.h`, `ExecuteMany.h`. This is consistent across the codebase (e.g. `example1/main.cpp:18-19` includes `SQLiteCpp.h` **and** separately `VariadicBind.h`, confirming the umbrella does not pull it in), so the omission appears intentional — likely to keep variadic-template / heavier headers opt-in. But the "all functionality" wording is misleading, and there is no comment explaining the intentional exclusion. A user reasonably expects `#include <SQLiteCpp/SQLiteCpp.h>` to expose `Backup`, `Savepoint`, etc.
- **Impact:** Usability footgun: users get confusing "incomplete type / undeclared identifier" errors for `Backup`, `Savepoint`, `bind(...)` variadic helpers, or `execMany` despite including the "main" header. Documentation overstates coverage.
- **Proposed fix:** Either (a) add the four missing public headers to the umbrella (simplest, matches the "all functionality" promise), or (b) keep them out deliberately and update the header comment to list what is and isn't included and why (e.g. "VariadicBind.h / ExecuteMany.h require explicit inclusion"). Option (b) is lower-risk for compile times.

### [HDR-07] `SQLITECPP_VERSION` string uses zero-padded non-canonical form `"3.03.03"`
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/SQLiteCpp.h`:44-45
- **Description:** The version macros are correct in value and consistent with the project (CMake `project(SQLiteCpp VERSION 3.3.3)`, meson `version: '3.3.3'`): `SQLITECPP_VERSION_NUMBER 3003003` correctly equals `3*1000000 + 3*1000 + 3`. However the string literal is `"3.03.03"` (zero-padded minor/patch) with a trailing comment `// 3.3.3`, whereas the documented format (lines 33-37, mirroring sqlite3.h) is `"X.Y.Z"`. sqlite3 itself uses unpadded `"3.53.2"` style. `"3.03.03"` is unusual and would break any consumer doing a literal string compare or parsing on the `.` boundaries expecting `"3.3.3"`. The need for the `// 3.3.3` clarifying comment is itself a signal the string is non-obvious.
- **Impact:** Cosmetic/compatibility nit. Tools or scripts that read `SQLITECPP_VERSION` as a string get `"3.03.03"` instead of the conventional `"3.3.3"`. The numeric macro is fine.
- **Proposed fix:** Change the string to `"3.3.3"` to match the documented `X.Y.Z` format and the project version, and drop the now-redundant comment. (Note: the release skill / `sqlitecpp-release` should be checked so its sed/replace expectations stay in sync.)

### [HDR-08] `message` argument not used on the handler path; both args lack defensive parenthesization in the handler macro
- **Severity:** Info
- **Confidence:** Medium
- **Category:** bug
- **Location:** `include/SQLiteCpp/Assertion.h`:36-37 (handler path) and :45 (assert path)
- **Description:** Minor macro-hygiene observations:
  - On the **handler path**, `message` is forwarded as a function argument (`..., message)`), which is fine for a single token/string, but it is not parenthesized; a `message` argument containing a top-level comma in a non-parenthesized context would mis-parse. In practice all call sites pass a string literal, so this is theoretical.
  - On the **`assert` path** (line 45): `assert(expression && message)` does **not** parenthesize `expression`. If a caller passes an expression like `a, b` or a low-precedence expression, the `&&` would bind incorrectly. Should be `assert((expression) && (message))`. All in-repo call sites pass simple comparisons so no bug is triggered today.
  - **Single-evaluation:** the handler path evaluates `expression` exactly once (good); the `assert` path also evaluates it once (and zero times under `NDEBUG`). No double-evaluation exists in either path. This is correct and noted here only to confirm the macro-hygiene check.
- **Impact:** No current bug (all call sites pass literals/simple comparisons). Purely a robustness improvement for the public macro.
- **Proposed fix:** Parenthesize defensively: handler path `if (!(expression)) SQLite::assertion_failed(..., (message))`; assert path `assert((expression) && (message))`.

## Notes / non-issues

- **`Utils.h` / `SQLITECPP_PURE_FUNC` is correct.** The feature-detection cascade (Hedley-derived) properly guards `__attribute__((pure))` behind `__has_attribute(pure)` and a compiler allow-list, and falls back to an empty definition (`#if !defined(SQLITECPP_PURE_FUNC) #define SQLITECPP_PURE_FUNC`). The `!defined(SQLITECPP_PURE_FUNC)` inner guard also lets a user pre-define it. Importantly, the **only** application of the attribute is `Statement::getIndex` (declared at `Statement.h`:123, defined at `Statement.cpp`:78-81), which simply returns `sqlite3_bind_parameter_index(getPreparedStatement(), apName)`. That is a genuine pure read (return value depends only on arguments + handle state, no side effects observable to the caller, no writes to globals), so `pure` is a valid annotation and will not cause a miscompile. MSVC gets the empty fallback (no MSVC equivalent attempted), which is safe. No finding.

- **`#pragma once` is used consistently** in all four headers (`Utils.h`:11, `Assertion.h`:11, `SQLiteCppExport.h`:13, `SQLiteCpp.h`:17). No mix of include guards; consistent with the rest of the public headers. No finding.

- **DLL macro static-lib case is handled.** When `SQLITECPP_COMPILE_DLL` is not defined (static lib, the default), `SQLITECPP_API` resolves to GCC `visibility("default")` on `__GNUC__ >= 4` or empty otherwise — correct for static linkage. The `&&` spacing typo on line 21 (`defined(_WIN32)&& !defined(__GNUC__)`) is cosmetic only.

- **`SQLITECPP_ASSERT` under `NDEBUG`** (non-handler path) correctly becomes a no-op via the standard library's `assert`, and the surrounding code accounts for it (e.g. `Database.cpp`:88 does `(void) ret;` to avoid the unused-variable warning in release). Correct by design.

- **Version numeric macro is internally consistent** with the string's intended value and with both build systems' `3.3.3`; only the string *format* is flagged (HDR-07), not the value.
