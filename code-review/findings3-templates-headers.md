# Findings 3 — Templates & Support Headers (3rd independent pass)

_Reviewed 2026-07-02. Unit: `include/SQLiteCpp/VariadicBind.h`, `ExecuteMany.h`, `Assertion.h`, `Utils.h`, `SQLiteCpp.h`, `SQLiteCppExport.h` + `tests/VariadicBind_test.cpp`, `tests/ExecuteMany_test.cpp`._
_Verification tooling: g++ 11.4 (Linux workspace), `-std=c++14 -fsyntax-only -Wall -Wextra -Wpedantic` against the repo includes._

## 1. EM-01 fix verification (PR #554)

**Complete.**
- `include/SQLiteCpp/ExecuteMany.h:68-70` — `reset_bind_exec()` now calls `apQuery.reset();` **then** `apQuery.clearBindings();` before `bind_exec()`. Order is correct (reset first, then clear), and `Statement::clearBindings()` exists (`Statement.h:104`).
- Regression test present: `tests/ExecuteMany_test.cpp:55-93` (`TEST(ExecuteMany, decreasingArity)`) — inserts `(1,"first")` then arity-1 `(2)` and asserts via `Column::isNull()` that row 2's value is **NULL**, not the stale `"first"` (lines 89-91). This is exactly the decreasing-arity case EM-01 described.
- The pre-existing `ExecuteMany.invalid` test (line 50) also pins the fixed behavior: set `std::make_tuple(2)` following scalar set `1` yields NULL/`""`, not a leaked binding.

## 2. Status of still-open items

### VariadicBind (VB-01..VB-05)
- **VB-01** — still valid: `VariadicBind.h:52` still does `std::forward<decltype(args)>(args)` on `const Args&` parameters; a no-op cast to `const&` that misleads readers into thinking rvalues are forwarded (all values are bound by const-ref and copied by `Statement::bind`).
- **VB-02/VB-05** — still valid (Info): `void` returns unchanged; the C++14 gate expression is still duplicated three times (`VariadicBind.h:17`, `:55`, `ExecuteMany.h:14`).
- **VB-03** — still valid: single-tuple call (`:75`) and tuple+`index_sequence` helper (`:91`) still resolve against the fully-generic variadic `bind` purely via partial ordering. Compile-verified today that lvalue/rvalue/const/empty tuples all pick the tuple overload (g++ 11, clean) — latent fragility, currently functioning.
- **VB-04** — still valid: doc examples at `:37` and `:64` still use non-SQL `&&`; still no SQLITE_TRANSIENT/always-copy caveat.

### ExecuteMany (EM-02..EM-06)
- **EM-01** — **fixed** (#554), see section 1.
- **EM-02** — still valid: `execute_many` (`ExecuteMany.h:51`) still runs the first set through `bind_exec` with no reset/clear; public `bind_exec` on a reused Statement remains a stale-binding footgun.
- **EM-03** — still valid: `bind_exec`/`reset_bind_exec` are still public in namespace `SQLite` (no `detail`), still called at `:51`/`:53` before their definitions (found only by ADL at instantiation), and `apQuery` is still a mis-prefixed reference parameter (`ap` = pointer convention).
- **EM-04** — still valid (Info): signature `(Database&, const char*, Arg&&, Types&&...)` still requires >=1 parameter set, undocumented.
- **EM-05** — still verified non-issue: each `aParams` element is forwarded exactly once inside the fold; `aArg` forwarded once; no use-after-move.
- **EM-06** — still verified non-issue: C++14 gating (`__cplusplus >= 201402L || _MSC_VER >= 1900`) correct and consistent with VariadicBind.h.

### Trivial/support headers (HDR-01..HDR-08)
- **HDR-01** — still valid, now **compile-confirmed**: wrapping `SQLITECPP_ASSERT` (handler path, `Assertion.h:36-37`) in an outer `if/else` triggers `-Wdangling-else` on g++ 11 and the `else` silently binds to the macro's hidden `if`, inverting logic. `do { } while(0)` fix still needed.
- **HDR-02** — still valid: `Assertion.h:29` declares `const int apLine`; every definition uses `long` (`examples/example1/main.cpp:26`, `examples/example2/src/main.cpp:24`, `tests/Database_test.cpp:29`, `README.md:429`, `docs/README.md:319`). Those are distinct overloads — a handler copied from the docs is never called; the library's `int` declaration stays unresolved at link time in handler builds.
- **HDR-03** — still valid: `SQLiteCppExport.h:22` uses `#if SQLITECPP_DLL_EXPORT` (value-truthiness; `-Wundef` noise; `#define SQLITECPP_DLL_EXPORT` with empty value = compile error); comment at `:17` still misnames the macro `SQLITECPP_EXPORT`.
- **HDR-04** — still valid: `:21` tests `_WIN32`, `:35` tests `WIN32` — the pragma block frequently never fires on MSVC (compiler predefines `_WIN32` only).
- **HDR-05** — still valid: `Assertion.h:32-34` unconditionally `#define __func__ __FUNCTION__` under `_MSC_VER` (reserved identifier; leaks into every downstream TU of handler builds; unnecessary since VS2015, the minimum supported).
- **HDR-06** — still valid: `SQLiteCpp.h:21-27` still omits `Backup.h`, `Savepoint.h`, `VariadicBind.h`, `ExecuteMany.h` despite the "all functionality" claim.
- **HDR-07** — still valid, extra evidence: `SQLiteCpp.h:44` `"3.03.03"` vs CMake `project(SQLiteCpp VERSION 3.3.3)` (`CMakeLists.txt:9`) and `meson.build:7` `version: '3.3.3'` — the string disagrees with both build systems and with the sqlite3 unpadded `"X.Y.Z"` convention. (Side nit: the same doc block calls the file "the SQLiteC++.h header", a filename that doesn't exist.)
- **HDR-08** — still valid: assert path `assert(expression && message)` leaves both args unparenthesized (an `expression` with a low-precedence operator changes meaning); handler path parenthesizes `expression` but not `message`.

## 3. NEW findings

| Done | ID | Sev | Conf | Category | Location | Impact | Fix |
|:--:|----|-----|------|----------|----------|--------|-----|
| [ ] | VB-06 | Low | High | API hygiene | `include/SQLiteCpp/VariadicBind.h:90-94` | The tuple + `std::index_sequence<Indices...>` helper is an implementation detail exposed as a public overload of `SQLite::bind`. It is directly callable with a user-supplied sequence; a wrong-length or reordered sequence (e.g. `index_sequence<1,0>`) silently binds values to the wrong SQL positions with no error — bind positions come from `++pos` order, not from `Indices`. Sibling of EM-03 (ranked-fix #25 covers only the ExecuteMany helpers, not this one). | Move the 3-arg overload into a `SQLite::detail` namespace (or rename to a non-overload `bind_impl`) and have the 2-arg tuple overload call it explicitly. |
| [ ] | EM-07 | Info | High | Docs | `tests/ExecuteMany_test.cpp:2` | Doxygen header says `@file VariadicBind_test.cpp` — copy-paste error; the file is `ExecuteMany_test.cpp`. Same defect class as EXC-06 (Exception_test mislabeled). Misleads doc generation and navigation. | Change line 2 to `@file    ExecuteMany_test.cpp` (and `@brief` to "Test of execute_many"). |
| [ ] | HDR-09 | Low | High | Header hygiene | `include/SQLiteCpp/SQLiteCppExport.h:35-38` | `#pragma warning(disable: 4251)` / `4275` are issued with **no `#pragma warning(push)`/`pop`** in a public header: when the block fires, C4251/C4275 are disabled for the remainder of *every consumer translation unit* including any SQLiteCpp header, hiding real DLL-interface bugs in the user's own code. Today it is mostly inert because the guard tests `WIN32` (HDR-04) — but fixing HDR-04 without adding push/pop would activate the leak. | Scope with `push`/`pop`, or drop the pragmas entirely; fix together with HDR-04. |
| [ ] | HDR-10 | Low | Med | Portability | `include/SQLiteCpp/SQLiteCppExport.h:21,28-29` | `defined(_WIN32) && !defined(__GNUC__)` excludes MinGW/GCC-on-Windows from the `__declspec(dllexport/dllimport)` path even when `SQLITECPP_COMPILE_DLL` is defined; those builds fall through to `__attribute__((visibility("default")))`, which has no effect in PE object files. A MinGW DLL build works only via binutils' `--export-all-symbols` default (which switches off as soon as any object contains an explicit dllexport). | Use `__declspec` on all `_WIN32` compilers when `SQLITECPP_COMPILE_DLL` is set (GCC/Clang on Windows support `__declspec`); reserve the visibility attribute for non-Windows GCC >= 4. |
| [ ] | HDR-11 | Low | High | Macro semantics | `include/SQLiteCpp/Assertion.h:36-37` vs `:45` | Divergent evaluation semantics between the two configurations of `SQLITECPP_ASSERT`: the handler path **always** evaluates `expression` (including `NDEBUG` release builds), while the default path expands to `assert(...)`, which evaluates **nothing** under `NDEBUG` (the header's own comment at `:44` says so). An `expression` with side effects would run in release+handler builds but be compiled out of release+default builds — a config-dependent behavior change. Currently latent: the only in-tree use, `src/Database.cpp:92`, is side-effect-free (`SQLITE_OK == ret`). Also undocumented that the handler fires in release builds at all. | Document both properties ("expression must be side-effect free; the user handler is invoked in release builds too"), and/or make the handler path honor `NDEBUG` if release-checking is unintended. Combine with the HDR-01 `do{}while(0)` rewrite. |
| [ ] | HDR-12 | Info | High | Style/clarity | `include/SQLiteCpp/Assertion.h:25-39` | The `#define __func__` and `#define SQLITECPP_ASSERT` directives sit lexically *inside* `namespace SQLite { }`. The preprocessor ignores namespaces, so this creates a false impression of scoping. Purely misleading structure, no behavior change. | Close the namespace after the `assertion_failed` declaration; define the macros at file scope (natural to do alongside HDR-01/HDR-05/HDR-11). |

## 4. Verified non-issues (this pass)

- **Bind index base**: `VariadicBind.h:50-52` uses `int pos = 0` with pre-increment `++pos` inside the expansion — first bind index is 1, matching SQLite's 1-based parameter indexing. No off-by-one.
- **`initializer_list` evaluation order**: both fold sites (`VariadicBind.h:52`, `ExecuteMany.h:53`) rely on braced-init-list element ordering, which is **guaranteed** left-to-right by [dcl.init.list]/4 — a standards-conformant idiom, not a fragility.
- **Empty-pack / empty-tuple instantiation**: `SQLite::bind(q)` (zero args) and `SQLite::bind(q, std::make_tuple())` both compile warning-clean (g++ 11, `-Wall -Wextra -Wpedantic`, C++14) and correctly bind nothing; the empty `initializer_list` and `index_sequence<>` chains are well-formed.
- **Tuple lvalue vs rvalue**: non-const lvalue, const lvalue, and rvalue `std::tuple` arguments all resolve to the tuple overload (compile-verified); no ambiguity with the variadic overload in practice. Rvalue tuples gain no move benefit (elements re-bound as `const&`), but `Statement::bind` copies via SQLITE_TRANSIENT anyway — only VB-01's misleading `forward` remains as a readability issue.
- **Header self-sufficiency**: a TU including only `<SQLiteCpp/ExecuteMany.h>` (or only `<SQLiteCpp/VariadicBind.h>`) compiles standalone; `Statement.h` forward-declares `class Database` (`Statement.h:30`), which suffices for `execute_many`'s `Database&` parameter. `Assertion.h` self-contains via `<cassert>`; `Utils.h` and `SQLiteCppExport.h` are freestanding.
- **Include guards**: all six headers use `#pragma once` consistently; no missing/duplicated-guard defects (the `#pragma once`-vs-`#ifndef` point is a cpplint style item already logged).
- **Version-number formula**: `SQLITECPP_VERSION_NUMBER 3003003` (`SQLiteCpp.h:45`) correctly equals `X*1000000 + Y*1000 + Z` for 3.3.3, matching the sqlite3 `SQLITE_VERSION_NUMBER` convention and both build systems. Only the *string* form is non-canonical (HDR-07).
- **`Utils.h` attribute gating**: `SQLITECPP_PURE_FUNC` is correctly double-guarded (`defined(__has_attribute)` outer, `__has_attribute(pure)` inner) with an empty-definition fallback; safe on all compilers.
- **C++14 gate on MSVC**: `_MSC_VER >= 1900` correctly compensates for MSVC's default `__cplusplus == 199711L` (no `/Zc:__cplusplus`); the gated features are available on VS2015+. (EM-06 re-confirmed.)
