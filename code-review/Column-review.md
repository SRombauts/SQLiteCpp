# Column — Review

## Summary
`Column` is a thin, copyable handle over a column of the current result row. It keeps the
prepared `sqlite3_stmt` alive via a shared `std::shared_ptr<sqlite3_stmt>` (`Statement::TStatementPtr`),
and its accessors forward almost 1:1 to the `sqlite3_column_*` C API. The constructor correctly
rejects a null statement pointer, and `getString()`/`getText()` correctly handle NULL/empty values.
The main risks are structural rather than outright bugs: every `noexcept` getter dereferences the
raw `sqlite3_stmt*` and passes `mIndex` straight to SQLite with no bounds/row-state guard, so misuse
(out-of-range index, use-after-`reset()`/`executeStep()`) is undefined behavior by design — the
"only valid until next step/reset" contract is documented but not enforceable at the `Column` level.
A genuine correctness defect exists in `operator<<` for BLOB columns, and `getUInt()` silently
truncates a 64-bit value. Counts: Critical 0, High 1, Medium 4, Low 3, Info 2.

## Findings

### [COL-01] `operator<<` writes `getText()` data with `getBytes()` length — wrong byte count for BLOB columns and unspecified evaluation order
- **Severity:** High
- **Confidence:** Medium
- **Category:** bug
- **Location:** `src/Column.cpp:117-121` (`operator<<`), interacting with `getText()` `src/Column.cpp:77-81` and `getBytes()` `src/Column.cpp:111-114`
- **Description:** `operator<<` does `aStream.write(aColumn.getText(), aColumn.getBytes())`. Two issues stack here:
  1. **Mismatched length for BLOBs.** `getText()` calls `sqlite3_column_text()` which forces a conversion to a NUL‑terminated UTF‑8 TEXT representation; `getBytes()` calls `sqlite3_column_bytes()` which returns the byte count of the column's *current* representation. For a TEXT column these agree. For a BLOB column they do not necessarily agree: per sqlite3.h:5503/5506, a TEXT→BLOB is "no change" but a BLOB→TEXT is a `CAST to TEXT` that "ensure[s] zero terminator", and the documented rule (sqlite3.h:5547-5549) is to pair `_text()` with `_bytes()` *only after forcing the format you want*. Because the two calls force/measure potentially different representations, the length passed to `write()` may not match the buffer returned by `getText()`.
  2. **Unspecified argument evaluation order.** In C++11/14 the order of evaluation of `getText()` and `getBytes()` as function arguments is unspecified. Per sqlite3.h:5510-5512 and 5538-5539, the safe pattern is to call the value accessor *first* and the bytes accessor *second* against the same forced format. If the compiler evaluates `getBytes()` before `getText()`, the byte count reflects the pre‑conversion (e.g. BLOB) representation while the pointer reflects the post‑conversion TEXT buffer — the pointer may even have been reallocated by the conversion (sqlite3.h:5510-5512 "pointers ... may be invalidated"). This is a latent, compiler-dependent inconsistency.
- **Impact:** Streaming a BLOB column (or a value whose text and blob byte counts differ) via `operator<<` can write the wrong number of bytes — truncated output, trailing garbage, or reading past/short of the converted buffer. The existing `Column.stream` test (`tests/Column_test.cpp:240-263`) only covers a TEXT column with an embedded NUL, where text-bytes == blob-bytes, so it does not exercise the divergent case.
- **Proposed fix:** Force the format once and read the length from the same forced representation, in a defined order. Simplest correct form: build the value locally, e.g.
  ```cpp
  std::ostream& operator<<(std::ostream& aStream, const Column& aColumn) {
      const std::string s = aColumn.getString(); // forces blob+bytes consistently
      aStream.write(s.data(), static_cast<std::streamsize>(s.size()));
      return aStream;
  }
  ```
  or, to preserve TEXT semantics, capture `const char* p = aColumn.getText();` then `int n = aColumn.getBytes();` on separate sequenced statements (text-first per sqlite3.h:5538). Note the current behavior is documented as inserting "the text value ... using getText()" (Column.h:236-238), so `getString()`-based output is a behavior change for non-text columns; either way the length/pointer pairing must be made consistent.

### [COL-02] All `noexcept` getters dereference the raw `sqlite3_stmt*` with no row-state or index guard — UB by design
- **Severity:** Medium
- **Confidence:** High
- **Category:** thread/exception (UB / API safety)
- **Location:** `src/Column.cpp:39-42, 46-49, 53-56, 59-62, 65-68, 71-74, 77-81, 84-87, 105-108, 111-114` (every getter); index field `include/SQLiteCpp/Column.h:232`
- **Description:** Every accessor passes `mStmtPtr.get()` and `mIndex` directly to `sqlite3_column_*`. The SQLite contract (sqlite3.h:5400-5411) states explicitly: "If the SQL statement does not currently point to a valid row, or if the column index is out of range, the result is undefined" and the routines may only be called when the most recent `sqlite3_step()` returned `SQLITE_ROW` and no `reset()`/`finalize()` followed. `Column` itself never validates the row state or the index — the validation lives only in `Statement::getColumn()` (`checkRow()`/`checkIndex()`, Statement.h:680-697) at construction time. Once a `Column` is held, a subsequent `executeStep()`/`reset()` on the owning `Statement` (which shares the same `sqlite3_stmt`) silently invalidates the row, and any later getter call is UB. The library's own tests document this: `tests/Column_test.cpp:289-296` mark `column0.getInt()` after `executeStep()`/`reset()` as "Undefined behavior".
- **Impact:** Use-after-row-change and out-of-range access are silent UB (potential read of stale/garbage memory, no exception). Because the getters are `noexcept`, there is no path to surface an error even in principle. The shared_ptr only guarantees the `sqlite3_stmt` object stays *allocated*; it does not guarantee the row cursor is still valid.
- **Proposed fix:** This is largely inherent to the wrapper's zero-copy design and is documented (Column.h:85-97, Statement.h:472-476). Realistic mitigations: (a) keep the getters as-is but strengthen the doc warning that holding a `Column` across `executeStep()`/`reset()` is UB (currently only on `Statement::getColumn`, not on `Column`'s value getters); (b) optionally, in debug builds, assert `mIndex` against `sqlite3_column_count(mStmtPtr.get())` and that the stmt is positioned on a row. Note the constructor's null check (Column.cpp:32-35) already covers the destroyed-statement case; the residual gap is row-state/index drift after construction.

### [COL-03] `getUInt()` truncates a 64-bit signed value to 32 bits via `getInt64()`
- **Severity:** Medium
- **Confidence:** High
- **Category:** bug
- **Location:** `src/Column.cpp:59-62`
- **Description:** `getUInt()` is implemented as `static_cast<unsigned>(getInt64())`, i.e. it fetches the full 64-bit value then narrows to 32 bits. For a stored value outside `[0, 2^32)` (e.g. a real `uint32_t` that round-tripped through SQLite's signed 64-bit storage, or any large/negative integer), the result is an implementation-defined/modular truncation, not a saturating or error path. The header comment (Column.h:79) acknowledges "SQLite3 does not support unsigned 64bits" but the silent narrowing of the low 32 bits can surprise callers who stored a value via `bind(uint32_t)` and read back with a different code path.
- **Impact:** Silent data corruption for integer values that don't fit in 32 bits when accessed through `getUInt()`/`operator uint32_t()`. Reading a stored `int64` of e.g. `0x1_0000_0001` yields `1`.
- **Proposed fix:** This is the long-standing, intentional 32-bit contract, so a behavior change is out of scope, but two safer options: (a) document that `getUInt()` returns only the low 32 bits of the underlying int64; (b) use `sqlite3_column_int64()` and cast to `uint32_t` is what it already does — the truncation is unavoidable given the 32-bit return type, so the fix is documentation/`[[deprecated]]`-style guidance rather than logic. Confirmed correct for the in-range cases exercised by `tests/Column_test.cpp:94-97,116-117`.

### [COL-04] Implicit conversion operators create ambiguity and silent narrowing hazards
- **Severity:** Medium
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Column.h:163-228`
- **Description:** `Column` exposes a large set of *implicit* conversion operators: `char`, `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, `int32_t`, `uint32_t`, `int64_t`, `double`, `const char*`, `const void*`, and `std::string`. Concerns grounded in the code/tests:
  - **Overload ambiguity / surprising selection.** With both `operator const char*` and `operator std::string` present, `std::string s = column;` and overloaded function calls can be ambiguous or resolve in surprising ways across compilers. The test file itself documents this fragility (`tests/Column_test.cpp:81-85,103-106`): on MSVC 2010-2013 the `std::string` initialization "would fallback to const char* ... witch does not work with the NULL char in the middle", so those assertions are `#if`-guarded by compiler version. That is a real, observed cross-compiler behavior divergence caused by the implicit operators.
  - **Narrowing.** `operator char/int8_t/uint8_t/int16_t/uint16_t` all `static_cast` from a 32-bit `getInt()`, silently truncating. A column holding `300` read into a `char`/`uint8_t` context silently yields a wrapped value with no diagnostic.
  - **`operator const char*` vs `operator const void*`** both being implicit means a `Column` participates in pointer-context overload resolution in ways that are easy to trigger unintentionally.
- **Impact:** Hard-to-spot conversions at call sites, cross-compiler inconsistency (already encoded in the tests), and silent narrowing. These are API-design smells rather than memory-safety bugs.
- **Proposed fix:** A breaking change, so for v3.x at most: document the narrowing operators clearly and prefer the explicit `getXxx()` getters in examples (examples already favor them). Long term (a major version), consider marking the narrowing/pointer operators `explicit` or removing the redundant `operator const char*` in favor of `getText()`. The library targets C++11, so `explicit` conversion operators are available if desired.

### [COL-05] `getString()` calls `sqlite3_column_bytes()` an extra time; relies on call ordering that is correct but fragile
- **Severity:** Low
- **Confidence:** High
- **Category:** bug (robustness) / efficiency
- **Location:** `src/Column.cpp:90-102`
- **Description:** `getString()` calls `sqlite3_column_bytes()` (line 96, result discarded), then `sqlite3_column_blob()` (line 97), then `sqlite3_column_bytes()` again inside the `std::string` constructor (line 101). The intent (per the inline comment) is to force the value into a stable format. Per the SQLite contract the *documented safe order* is blob-first then bytes (sqlite3.h:5538-5539). The code's effective sequence is bytes, blob, bytes:
  - The leading `(void)sqlite3_column_bytes()` (line 96) is largely redundant given the comment claims it ensures format — but `sqlite3_column_bytes()` on a non-blob/non-text value triggers a numeric→TEXT conversion (sqlite3.h:5440-5441), and a subsequent `sqlite3_column_blob()` then returns that TEXT buffer. The final `sqlite3_column_bytes()` (line 101) is sequenced *after* the `data` pointer is read (line 97), which matches blob-then-bytes and is correct.
  - The net result is correct for TEXT, BLOB, NULL, and numeric columns (the `numeric→text` path is exactly what the `Column.basis` test relies on, e.g. `getString()` of an INTEGER, `tests/Column_test.cpp:152`). But the redundant first `_bytes()` call adds a conversion that is then potentially re-done, and the three-call dance obscures the contract.
- **Impact:** Minor: one redundant conversion/measurement per `getString()` call; no correctness defect found. The reliance on three calls in a specific order is fragile to future edits.
- **Proposed fix:** Reduce to the documented two-call pattern, sequenced explicitly:
  ```cpp
  std::string Column::getString() const {
      const char* data = static_cast<const char*>(sqlite3_column_blob(mStmtPtr.get(), mIndex)); // force blob
      const int len = sqlite3_column_bytes(mStmtPtr.get(), mIndex);                              // then measure
      return std::string(data ? data : "", static_cast<size_t>(len)); // data may be null only when len==0
  }
  ```
  This keeps the verified behavior (`std::string(nullptr, 0)` is currently relied upon at line 101 and is well-defined since C++11 when the count is 0) while matching sqlite3.h:5539 exactly and dropping the redundant call.

### [COL-06] `getString()` constructs `std::string(nullptr, 0)` — relies on a subtle standard guarantee
- **Severity:** Low
- **Confidence:** High
- **Category:** memory
- **Location:** `src/Column.cpp:97-101`
- **Description:** For a NULL value or a zero-length BLOB, `sqlite3_column_blob()` returns a NULL pointer (sqlite3.h:5462,5494) and `sqlite3_column_bytes()` returns 0 (sqlite3.h:5443). The code therefore constructs `std::string(nullptr, 0)`. The inline comment (Column.cpp:100) asserts this is OK. Per the C++ standard, `basic_string(const charT* s, size_type n)` requires `[s, s+n)` to be a valid range; for `n == 0` and `s == nullptr` this is the empty range and is well-defined in practice across libstdc++/libc++/MSVC, but it is a frequently cited gray area (some hardened/`_GLIBCXX_ASSERTIONS` builds historically flagged a null pointer here even with length 0). The `Column.basis` test exercises NULL columns via the implicit/`getText` paths but `getString()` on a NULL column is not directly asserted (the empty TEXT column 5 is read via `getText`, `tests/Column_test.cpp:183`).
- **Impact:** Theoretical: a hardened standard library could trip an assertion on `string(nullptr, 0)`. No real-world failure observed.
- **Proposed fix:** Guard the pointer as shown in COL-05 (`data ? data : ""`), eliminating the `nullptr` argument entirely and removing dependence on the empty-range edge case.

### [COL-07] `getText()` default-value pointer lifetime is the caller's responsibility but undocumented for the non-default case
- **Severity:** Low
- **Confidence:** Medium
- **Category:** memory / api
- **Location:** `src/Column.cpp:77-81`; declaration `include/SQLiteCpp/Column.h:91`
- **Description:** `getText(const char* apDefaultValue = "")` returns either the SQLite text pointer or `apDefaultValue` for a NULL column. When the default is taken, the returned pointer's lifetime is that of `apDefaultValue`, not the statement. The default literal `""` is fine (static storage). But a caller passing a temporary or stack buffer as the default gets back a pointer whose validity differs from the documented "valid while the statement is valid" warning (Column.h:85-90). The `reinterpret_cast<const char*>(sqlite3_column_text(...))` (line 79) is correct — `sqlite3_column_text` returns `const unsigned char*` and the cast to `const char*` is the standard idiom.
- **Impact:** Edge-case dangling pointer if a caller supplies a non-static default and then uses the result beyond the default's lifetime. The cast itself is safe and conventional.
- **Proposed fix:** Documentation only: note that when the column is NULL the returned pointer aliases `apDefaultValue`, so its lifetime is the caller's, not the statement's.

### [COL-08] `Column` is implicitly copyable; copies extend `sqlite3_stmt` lifetime but not row validity (documented contract is non-enforceable)
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Column.h:48-233` (no user-declared copy/move; members at 231-232)
- **Description:** `Column` holds a `Statement::TStatementPtr` (shared_ptr) plus an `int` index, and has implicitly-generated copy/move. Copying a `Column` (e.g. `auto column0 = query->getColumn(0);` then `query.reset();` in `tests/Column_test.cpp:276-281`) keeps the `sqlite3_stmt` allocated even after the owning `Statement` is destroyed — that part works and is tested. However this can lull users into thinking a stored `Column` stays *usable*; the row data is only valid until the next `executeStep()`/`reset()` (Statement.h:472-476), which the shared_ptr cannot protect. So the "valid until next step/reset" contract is fundamentally documented-only, not enforceable — confirming the task's hypothesis.
- **Impact:** Conceptual footgun, not a defect: the shared_ptr's lifetime guarantee is narrower than it appears (object stays allocated, row cursor does not stay valid).
- **Proposed fix:** None required. Optionally tighten the class-level doc (Column.h:32-47) to state that a copied/stored `Column` keeps the statement alive but does NOT preserve the row's data across the next step/reset.

### [COL-09] Missing `[[nodiscard]]` on pure value getters
- **Severity:** Info
- **Confidence:** High
- **Category:** api/modernization
- **Location:** `include/SQLiteCpp/Column.h:64,74,78,80,82,84,91,98,104,117,120-143,154,157`
- **Description:** All getters (`getInt`, `getString`, `getBytes`, `isNull`, etc.) are pure read accessors whose return value is the entire point of calling them, yet none are marked `[[nodiscard]]`. Discarding their result (e.g. `column.getString();`) is almost always a bug. The library targets C++11 where `[[nodiscard]]` (C++17) is unavailable, so this would require a feature-gated macro.
- **Impact:** Minor: callers can silently drop results. No runtime issue.
- **Proposed fix:** Introduce a `SQLITECPP_NODISCARD` macro (expanding to `[[nodiscard]]` for C++17+, empty otherwise, consistent with the existing `SQLITECPP_PURE_FUNC` pattern in Utils.h) and apply to the value getters.

## Notes / non-issues
- **Constructor null check is correct and well-placed.** `Column::Column` (Column.cpp:28-36) throws `SQLite::Exception("Statement was destroyed")` if `aStmtPtr` is null before storing it, so the getters can assume `mStmtPtr` itself is non-null (the wrapped raw pointer is what the shared_ptr manages). This is the right place for the throwing check, keeping the getters `noexcept`.
- **`getText()`'s `reinterpret_cast<const char*>` is correct.** `sqlite3_column_text` returns `const unsigned char*` (sqlite3.h:5582); the cast to `const char*` is the conventional, well-defined idiom and matches upstream usage.
- **`getString()` byte-length handling of embedded NULs is correct.** Using `sqlite3_column_blob` + `sqlite3_column_bytes` (rather than `sqlite3_column_text` + `strlen`) correctly preserves embedded NUL bytes; verified by `tests/Column_test.cpp:102-106,124,133-134` ("bl\0b"). The header note (Column.h:100-104) is accurate.
- **`getType()`/`isXxx()` "meaningful only before conversion" caveat is accurate.** Matches sqlite3.h:5425-5430 (after a type conversion, `sqlite3_column_type()` is undefined-but-harmless). The header warnings (Column.h:113-116, 119-143) correctly reflect this.
- **`size()` is a correct alias for `getBytes()`** (Column.h:157-160); the stray space in `getBytes ()` is cosmetic only.
- **`getBytes()` returning 0 for NULL is correct** per sqlite3.h:5443 and asserted at `tests/Column_test.cpp:184`.
- **Forward-declaring `struct sqlite3_stmt`** in the header (Column.h:21) to avoid pulling in `<sqlite3.h>` is good practice and consistent with Statement.h.
