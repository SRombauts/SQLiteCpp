# ExecuteMany — Review

## Summary
`ExecuteMany.h` is a small, C++14-gated header (92 lines) providing `execute_many()` plus the two helpers `bind_exec()` and `reset_bind_exec()`. It builds directly on `SQLite::bind` (VariadicBind.h) and `Statement::reset()`/`executeStep()`. The core mechanics are sound: the C++14 `#if` guard degrades cleanly, the `std::initializer_list` fold over parameter sets is well-formed and left-to-right ordered, and there is no real use-after-move (the underlying `bind` overloads take their arguments by `const&`, so nothing is consumed). The one substantive defect is a **stale-binding leak**: because `reset_bind_exec()` calls `reset()` but never `clearBindings()`, parameters bound in an earlier tuple persist into later tuples that bind fewer parameters. Counts by severity: Critical 0, High 0, Medium 1, Low 2, Info 3.

## Findings

### [EM-01] Stale bindings leak between parameter sets (reset() does not clear bindings)
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug
- **Location:** `include/SQLiteCpp/ExecuteMany.h:67-72` (`reset_bind_exec`); contract at `src/Statement.cpp:58-69` and `sqlite3/sqlite3.h:5094-5096, 5651-5652`
- **Description:** Each non-first parameter set goes through `reset_bind_exec()`, which does `apQuery.reset()` then `bind_exec()`. `Statement::reset()` calls only `sqlite3_reset()`, and the SQLite contract is explicit that `sqlite3_reset()` "does not change the values of any bindings" (sqlite3.h:5651-5652); `Statement::reset()`'s own Doxygen says the same (Statement.h:92). `SQLite::bind` (VariadicBind.h:50-53) binds only positions `1..N` for the N values in the current tuple — it does not null out higher positions. Therefore, if an earlier tuple binds K parameters and a later tuple binds J < K parameters, positions `J+1 .. K` retain the values bound by the earlier tuple instead of reverting to NULL/their declared default. `clearBindings()` exists (`src/Statement.cpp:72-76`) but is never invoked anywhere in this flow.
  The existing test (`tests/ExecuteMany_test.cpp:31-52`) hides the bug because the tuples are supplied in *increasing* arity (`1`, `make_tuple(2)`, `make_tuple(3,"three")`): position 1 is rebound every iteration and position 2 is only ever set on the last row, so no stale value is observable. Reverse the order — e.g. `execute_many(db, "INSERT INTO test VALUES (?, ?)", std::make_tuple(3, "three"), std::make_tuple(2))` — and row 2 would silently insert `(2, "three")` instead of `(2, <default/NULL>)`, because position 2 still holds `"three"` from the previous iteration.
- **Impact:** Silent data corruption for callers who pass parameter sets of differing arity (a legitimate use, since the docstring example itself mixes a 1-tuple and a 2-tuple). Bound values bleed across rows; the value inserted is neither what the caller passed nor the column default. No crash, no UB — purely incorrect results, and only under the variable-arity usage pattern, hence Medium rather than High.
- **Proposed fix:** Call `clearBindings()` as part of the reset step so every iteration starts from a clean (all-NULL) binding state:
  ```cpp
  template <typename TupleT>
  void reset_bind_exec(Statement& apQuery, TupleT&& aTuple)
  {
      apQuery.reset();
      apQuery.clearBindings();
      bind_exec(apQuery, std::forward<TupleT>(aTuple));
  }
  ```
  Add a regression test that passes parameter sets in *decreasing* arity and asserts the trailing column is the column default/NULL, not the prior row's value. (If the intent is genuinely "reuse the previous binding when omitted", that should be documented explicitly; the current docstring and the column-default test expectation imply the opposite intent.)

### [EM-02] First parameter set is not reset; relies on a freshly constructed Statement
- **Severity:** Low
- **Confidence:** High
- **Category:** bug
- **Location:** `include/SQLiteCpp/ExecuteMany.h:50-51`
- **Description:** `execute_many()` constructs a fresh `Statement query(aDatabase, apQuery)` and runs the first parameter set through `bind_exec()` (which does *not* reset), while every subsequent set goes through `reset_bind_exec()` (which does). This asymmetry is correct *as written* only because the statement is brand-new and has never been stepped — `sqlite3_step` has not been called, so no reset is needed before the first bind/step. It is consistent and not a live bug today. It is flagged Low/structural because the helpers are part of the public API surface and are individually documented: a caller who reuses `bind_exec()` directly on an already-stepped statement (the natural reading of "bind values and execute it") would hit the SQLITE_MISUSE/"needs to be reseted" path from `executeStep()` (`src/Statement.cpp:175`). The two-function split (`bind_exec` vs `reset_bind_exec`) is an implementation detail leaking into the public namespace.
- **Impact:** No incorrect behavior in the intended `execute_many` flow. Minor footgun if `bind_exec`/`reset_bind_exec` are called standalone, and a small maintenance hazard (the first-row special case is implicit).
- **Proposed fix:** Optionally route the first set through `reset_bind_exec()` too (a reset on a never-stepped statement returns SQLITE_OK and is harmless per sqlite3.h:5632-5635), removing the special case and making the loop uniform. At minimum, document that `bind_exec()` assumes a not-yet-stepped (or just-reset) statement.

### [EM-03] Helpers `bind_exec` / `reset_bind_exec` pollute the `SQLite` namespace and are used before declaration
- **Severity:** Low
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/ExecuteMany.h:51, 54, 67-72, 82-87`
- **Description:** `execute_many()` (line 48) calls `bind_exec` (line 51) and `reset_bind_exec` (line 54) which are *defined later* in the same header (lines 67 and 82). This compiles only because they are function templates: the calls are dependent and name lookup for the unqualified calls is deferred to instantiation (two-phase lookup / ADL on the `SQLite`-namespaced `Statement&` argument finds them). It is legal but fragile — reordering or qualifying differently can break it, and ADL is the only thing making the unqualified call resolve. Additionally, `bind_exec` and `reset_bind_exec` are general-sounding public names in namespace `SQLite` that are really private plumbing for `execute_many`; they invite misuse (see EM-02) and risk overload-set collisions with user code. The docstring for `reset_bind_exec` also still has a `@param apQuery Query to use` referring to a `Statement&` named `apQuery` — the `ap` prefix conventionally denotes a pointer in this codebase, but it is a reference.
- **Impact:** Maintainability/readability and a minor API-cleanliness concern; no runtime effect.
- **Proposed fix:** Move the two helpers into a detail namespace (e.g. `SQLite::detail` or an anonymous/`/// @cond` internal block) or make them `static`/local, and/or forward-declare them above `execute_many`. Rename the reference parameter from `apQuery` to `aQuery` to match the codebase's pointer/reference Hungarian convention.

### [EM-04] `execute_many` requires at least one parameter set but does not document/static_assert it
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/ExecuteMany.h:47-56`
- **Description:** The signature `execute_many(Database&, const char*, Arg&&, Types&&...)` mandates at least one parameter set (`Arg`), with `Types...` allowed to be empty. Calling `execute_many(db, query)` with zero sets is a compile error (no matching overload) rather than a documented no-op or a clear diagnostic. The empty-`Types...` case is handled correctly: the `std::initializer_list<int>{ ... }` becomes an empty braced list, which is well-formed, so a single-set call runs exactly the first `bind_exec`. This is benign but undocumented.
- **Impact:** None at runtime; only a slightly cryptic compiler error for the (unusual) zero-set call.
- **Proposed fix:** Document that ≥1 parameter set is required, or accept zero sets via a different overload if that is desirable. Optional only.

### [EM-05] No use-after-move despite the `std::forward` chain (verified non-issue, noted for the record)
- **Severity:** Info
- **Confidence:** High
- **Category:** memory
- **Location:** `include/SQLiteCpp/ExecuteMany.h:51, 54, 71, 85`
- **Description:** The forwarding chain `execute_many` → `bind_exec`/`reset_bind_exec` → `SQLite::bind` looks like it could move-from a tuple and then reuse it, but it cannot: both terminal overloads in VariadicBind.h take their inputs by `const&` — `bind(Statement&, const std::tuple<Types...>&)` (VariadicBind.h:76) and `bind(Statement&, const Args&...)` (VariadicBind.h:48) — and `query.bind(++pos, std::forward<decltype(args)>(args))` forwards a `const&` (i.e. binds as lvalue). Each `aParams` element is forwarded exactly once (one helper call per pack expansion element), and the values are copied into SQLite via `SQLITE_TRANSIENT`/by-value binds. So there is no double-move, no use-after-move, and no dangling-temporary risk across iterations. The `Arg&&`/`Types&&...` forwarding references buy nothing here (the value category is discarded at the const-ref boundary) but are harmless.
- **Impact:** None.
- **Proposed fix:** None required. (If the binding path were ever changed to consume rvalues, re-examine: `bindNoCopy(..., std::string&&)` is `= delete`d in Statement.h:185, which already blocks the most dangerous case.)

### [EM-06] C++14 gating is correct and consistent with the rest of the library
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/ExecuteMany.h:14, 91`
- **Description:** The guard `#if (__cplusplus >= 201402L) || ( defined(_MSC_VER) && (_MSC_VER >= 1900) )` matches the identical guard in VariadicBind.h:17/56 and Statement.h:511, so the feature appears/disappears coherently across the dependency chain. Under C++11 the entire header body (including the `namespace SQLite { ... }` block) is excluded, so it degrades to an empty translation unit cleanly with no partial declarations. The MSVC `_MSC_VER >= 1900` (VS2015) carve-out is needed because old MSVC reported `__cplusplus == 199711L` regardless of the actual standard. The dependency on `std::index_sequence_for` / `std::index_sequence` (used by the tuple `bind` it relies on) is itself C++14, so the gate is necessary, not merely cautious.
- **Impact:** None — correct as-is.
- **Proposed fix:** None.

## Notes / non-issues
- **`(void)std::initializer_list<int>{ ... }` fold is well-defined.** Using a braced-init-list to expand the pack guarantees left-to-right evaluation of the parameter sets (unlike a plain comma-operator fold in a function call), so rows are inserted in argument order. The `(void)` casts and the trailing `, 0` are the standard idiom to discard each `void` result and yield an `int` for the list. Correct.
- **`reset()` failure mid-iteration leaves a sane state.** `reset()` throws `SQLite::Exception` on a non-OK code; `Statement::tryReset()` is `noexcept` and sets `mbHasRow=false`/`mbDone=false` *before* calling `sqlite3_reset` (Statement.cpp:64-69), so even on a thrown reset the flags are consistent and the `Statement`'s RAII destructor will finalize the handle. An exception from any `bind`/`executeStep`/`reset` simply unwinds out of `execute_many`; the local `Statement` is destroyed and the partial inserts already committed by prior `executeStep()` calls remain (expected — there is no implicit transaction, matching the rest of the library; callers wanting atomicity wrap in `SQLite::Transaction`). This is acceptable and not a defect, though worth a doc note.
- **First scalar argument (non-tuple) binds correctly.** In the test, the first set is a bare `1`, not a tuple. `bind_exec(query, 1)` → `SQLite::bind(query, 1)` resolves to the variadic `bind(Statement&, const Args&...)` overload (VariadicBind.h:48), binding it at position 1. The tuple overload is only selected for actual `std::tuple` arguments. Works as intended.
- **No `[[nodiscard]]` is consistent with house style.** These functions return `void`, and the library does not use `[[nodiscard]]` anywhere (grep of `include/` finds none), so its absence is not a finding.
- **Thread safety:** none of these functions add shared state; they operate on a Statement local to the call (or one passed in by the caller). Thread-safety is inherited from `Statement`/`Database`, which document single-thread-per-connection use (Statement.h:45-50). No new concern introduced here.
