# Column — 3rd-pass Deep Review (findings3-column.md)

_Scope: `include/SQLiteCpp/Column.h`, `src/Column.cpp`, `tests/Column_test.cpp`. Date: 2026-07-02._
_Focus: verify the COL-01 fix (#556), re-check COL-02..COL-09, and hunt new issues against the SQLite column-access API contract (sqlite3.h:5380-5560 conversion/invalidation rules, verified against the bundled amalgamation implementation in sqlite3.c)._

---

## 1. COL-01 fix verification (#556) — VERIFIED COMPLETE (one doc remnant, see COL-10)

Current `operator<<` (`src/Column.cpp:117-122`):

```cpp
std::ostream& operator<<(std::ostream& aStream, const Column& aColumn)
{
    const std::string value = aColumn.getString();
    aStream.write(value.data(), static_cast<std::streamsize>(value.size()));
    return aStream;
}
```

Both defects from the original finding are gone:

1. **Byte-count/pointer mismatch for BLOBs** — fixed. The value is materialized once via `getString()`, which forces a single representation (`sqlite3_column_bytes` -> `sqlite3_column_blob` -> `sqlite3_column_bytes`, `src/Column.cpp:96-101`) and pairs the pointer with the length of *that same* representation. `write()` then uses `value.data()`/`value.size()` from one `std::string` — they cannot diverge.
2. **Unspecified argument evaluation order** — fixed. There is now exactly one function call producing the buffer; the old two-argument `write(getText(), getBytes())` race is gone.

Test coverage landed with the fix (`tests/Column_test.cpp`):
- `Column.streamUtf8Text` (:267-292) — multibyte UTF-8 TEXT, byte-exact round-trip, cross-checked against `getString()`.
- `Column.streamUtf8EmbeddedNul` (:295-316) — UTF-8 with embedded NUL.
- `Column.streamUtf8Blob` (:319-344) — **the divergent BLOB case the finding flagged**, asserts `isBlob()`, byte-exact equality, and `streamed == getString()`.
- Pre-existing `Column.stream` (:240-263, embedded-NUL TEXT) still present.

Residual nit: the Doxygen comment above the `operator<<` declaration still documents the *old* behavior ("using getText()") — tracked below as COL-10. Also note the fix makes `operator<<` allocate (may throw `std::bad_alloc` where the old form could not) — acceptable and standard for stream inserters; not a finding.

**Verdict: COL-01 can be ticked `[x] #556` in CODE_REVIEW.md.**

---

## 2. Status of still-open COL-02..COL-09 (one line each)

| ID | Status |
|----|--------|
| COL-02 | **Still valid, unchanged** — every `noexcept` getter still passes `mStmtPtr.get()`/`mIndex` straight to `sqlite3_column_*` with no row/index guard (`src/Column.cpp:39-114`). |
| COL-03 | **Still valid, unchanged** — `getUInt()` is still `static_cast<unsigned>(getInt64())`, silent low-32-bit truncation (`src/Column.cpp:59-62`). |
| COL-04 | **Still valid, unchanged** — full implicit conversion-operator set (char..int64_t, double, `const char*`, `const void*`, `std::string`) intact (`include/SQLiteCpp/Column.h:163-228`); MSVC `#if` guards still in tests (:81-85, :103-106). |
| COL-05 | **Still valid as written** (3-call `bytes,blob,bytes` dance unchanged, `src/Column.cpp:96-101`) — **but its proposed 2-call fix is now shown to be a latent regression; see COL-12 before applying ranked fix #23.** |
| COL-06 | **Still valid, unchanged** — `std::string(data, len)` with `data == nullptr` when NULL/empty (`src/Column.cpp:101`); `data ? data : ""` guard still missing. |
| COL-07 | **Still valid, unchanged** — `getText(apDefaultValue)` returns the caller's default pointer for NULL columns with no lifetime note (`src/Column.cpp:77-81`, `include/SQLiteCpp/Column.h:91`). |
| COL-08 | **Still valid, unchanged** — `Column` implicitly copyable; shared_ptr keeps the stmt allocated but not row-valid (`include/SQLiteCpp/Column.h:230-233`; exercised by `Column.shared_ptr` test :346-380). |
| COL-09 | **Still valid, unchanged** — no `[[nodiscard]]`/`SQLITECPP_NODISCARD` anywhere in the unit. |

---

## 3. NEW findings (COL-10 onward)

| Done | ID | Severity | Confidence | Category | Location | Impact | Concrete fix |
|:--:|----|----------|------------|----------|----------|--------|--------------|
| [ ] | COL-10 | Low | High | docs (stale after #556) | `include/SQLiteCpp/Column.h:235-244` | The `operator<<` Doxygen block still says "Insert the text value of the Column object, **using getText()**, into the provided stream" — the implementation (`src/Column.cpp:119`) has used `getString()` since #556. Doc-vs-code drift misleads readers about BLOB semantics (the exact case the fix changed) and about which conversion is forced on the column. | Update the comment to: "Insert the text or binary value of the Column object, using getString(), into the provided stream" (and optionally note it writes exactly `size()` bytes, embedded NULs included). |
| [ ] | COL-11 | Medium | High | docs / memory (UAF enabler) | `include/SQLiteCpp/Column.h:88-90` (getText warning) and `:95-97` (getBlob warning); contract: `sqlite3/sqlite3.h:5510-5532` | Both `@warning` blocks claim the returned pointer "is only valid while the statement is valid (ie. not finalized)". That overstates the guarantee: per sqlite3.h:5510-5512 pointers from prior `column_blob()`/`column_text()` calls **may be invalidated by a later type conversion on the same cell** — concretely, `const void* b = col.getBlob(); const char* t = col.getText();` on a BLOB column performs a "CAST to TEXT, ensure zero terminator" (sqlite3.h:5506, 5517-5519) that can realloc the buffer, leaving `b` dangling while the statement is perfectly alive. `executeStep()`/`reset()` also invalidate, mentioned only on `Statement::getColumn`, not here. Users following the current warning to the letter can write a silent use-after-free. | Rewrite both warnings to list all three invalidation triggers: (1) any subsequent getXxx() on the same Column that forces a type conversion (mixing getBlob/getText/getString on non-TEXT/non-BLOB or BLOB columns), (2) the next `executeStep()`/`reset()`, (3) statement finalization. Recommend "call the pointer getter last, or use getString()". |
| [ ] | COL-12 | Medium | High | review-correction (latent regression in an open proposed fix) | `src/Column.cpp:96` (the load-bearing leading `(void)sqlite3_column_bytes()`); refutes the fix text of COL-05 (`code-review/Column-review.md`) and ranked fix #23 in `CODE_REVIEW.md` | COL-05/fix #23 propose simplifying `getString()` to plain blob-then-bytes and dropping the leading `sqlite3_column_bytes()` as "redundant". It is **not redundant for UTF-16-encoded databases**: `sqlite3_value_blob` (`sqlite3/sqlite3.c:93739-93751`) returns the string `p->z` in its **current encoding** and sets `MEM_Blob`; a subsequent `sqlite3_value_bytes` (`sqlite3/sqlite3.c:87728-87746`) then takes the `MEM_Blob` branch and returns `p->n` — the **UTF-16** byte count of the **UTF-16** buffer. So the simplified form returns UTF-16-encoded bytes from `getString()` on a `PRAGMA encoding='UTF-16'` database, silently changing results (the `Column.basis16` test, `tests/Column_test.cpp:207-210` via `:118,124`, would catch it). The existing leading `bytes()` call runs while the value is still `MEM_Str`-only, falls through to `valueBytes(pVal, SQLITE_UTF8)` (`sqlite3.c:87745`) and forces the UTF-8 conversion first — that is exactly what makes the current 3-call sequence correct. | Amend COL-05's proposed fix and ranked fix #23: keep the leading `(void)sqlite3_column_bytes()` (or replace the whole body with `sqlite3_column_text()` first then `sqlite3_column_bytes()`), and only add the `data ? data : ""` null guard (COL-06). Add a code comment stating the first `bytes()` call forces UTF-8 conversion for UTF-16 databases and must not be removed; `Column.basis16` is the regression test. |
| [ ] | COL-13 | Low | Medium | docs / api | `include/SQLiteCpp/Column.h:59-64` (`getName`), `:66-75` (`getOriginName`); `src/Column.cpp:39-49`; contract: `sqlite3/sqlite3.h:5128-5136` | `getName()` forwards `sqlite3_column_name()`, which (a) **returns NULL if `sqlite3_malloc()` fails** (sqlite3.h:5134-5136) and (b) returns a pointer valid only until the statement is finalized, **automatically reprepared by the first `sqlite3_step()` after a schema change, or the next `column_name`/`column_name16` call on the same column** (sqlite3.h:5128-5132). The header documents neither; the library's own idiom `const std::string name0 = query.getColumn(0).getName();` (`tests/Column_test.cpp:224`) is UB on the (OOM-only) NULL return — same defect class as the fixed STMT-02. | Either mirror `getText()`'s pattern (`return pName ? pName : "";`) keeping `noexcept`, or document "may return nullptr on out-of-memory; pointer invalidated by re-preparation or the next getName() call". The one-line null-coalesce is the cheaper, safer fix. |

---

## 4. Systematic conversion-operator / getter audit vs SQLite call-order semantics (evidence)

Every accessor was checked against the sqlite3.h "Result Values From A Query" contract (conversion table sqlite3.h:5490-5507, invalidation rules :5510-5532, safe pairings :5534-5540):

- `getInt`/`getInt64`/`getDouble`/`getUInt` (`src/Column.cpp:53-74`) — value (not pointer) returns; conversions from TEXT/BLOB are *defined* (atoi/atof-style CAST, sqlite3.h:5495-5505). `getInt64` on a TEXT column is defined behavior (best-effort numeric CAST), not UB — no new finding beyond the known COL-03 truncation.
- `getText` (`:77-81`) — `text()` only; NULL-safe via default; the `reinterpret_cast` from `const unsigned char*` is the standard idiom. Pointer-lifetime doc defect -> COL-11.
- `getBlob` (`:84-87`) — single call, may return the value **converted in place**: for an INTEGER/FLOAT column `sqlite3_value_blob` falls through to `sqlite3_value_text` (`sqlite3.c:93748-93750`), i.e. `getBlob()` on a numeric column returns its **text representation** and allocates — defined and consistent with `getBytes()`, but part of why COL-11's warning rewrite matters.
- `getString` (`:90-102`) — the final `blob -> bytes` pair matches the documented safe order (sqlite3.h:5539) and the leading `bytes()` is load-bearing for UTF-16 (COL-12). `data==nullptr` only when `len==0` (NULL or zero-length value), so COL-06 remains the only residual (hardened-STL) concern.
- `getType`/`isXxx` (`:105-108`, header :117-143) — "meaningful only before conversion" caveat present and accurate.
- `getBytes`/`size` (`:111-114`, header :145-160) — safe standalone; pairing guidance covered by COL-11.
- Conversion operators (header :163-228) — all delegate to the getters above; no operator performs a second SQLite call after taking a pointer, so no *internal* pointer-invalidating sequence exists in any single conversion. The dangerous sequences are all *user-composed* across multiple getter calls (COL-11) or covered by COL-04.
- `getColumns<T,N>` glue (header :250-263) — `checkRow()`/`checkIndex(N-1)` precede construction; each `Column(mpPreparedStatement, Is)` shares the same shared_ptr; per-element access happens through the same audited getters. No new issue.

## 5. Verified non-issues (3rd pass)

- **`operator<<` on a NULL column** — `getString()` yields an empty string; `write(ptr, 0)` writes nothing. Correct.
- **`const char* s = db.execAndGet(q);` dangling temporary** — real footgun (temporary `Column` is the sole owner; the stmt is finalized at end of full-expression), but **already loudly documented** with a WARNING on both `execAndGet` overloads (`include/SQLiteCpp/Database.h:396-398`: "make a COPY OF THE result, else it will be destroy before the next line"). The silent-implicit-conversion aspect is COL-04's existing scope; no new ID assigned.
- **`Column` copy semantics** — copies share `mStmtPtr` (keeping the `sqlite3_stmt` allocated after the owning `Statement` dies) and copy `mIndex`; verified by `Column.shared_ptr` (`tests/Column_test.cpp:346-380`) including `getName()` after `Statement` destruction. Row-validity gap is exactly COL-02/COL-08; nothing new.
- **Index validity after `reset()`** — `mIndex` itself cannot go stale for the same prepared statement (column count is fixed at prepare time); what goes stale is the *row*, which is COL-02. `sqlite3_column_name` after reset is legal (name pointers survive reset; only reprepare/finalize kill them — sqlite3.h:5128-5132), consistent with the test at :379.
- **UTF-16 databases through `getText`/`getBytes`** — `text()` forces UTF-8 then `bytes()` measures UTF-8 (`sqlite3.c:87731` enc-match branch); the header's getText-then-getBytes pairing (`Column.h:149`) is the documented safe order. Exercised by `Column.basis16`.
- **`Column.streamUtf8*` tests** — assert byte-exact identity and `getString()` agreement; they would catch any future reintroduction of the COL-01 pattern.
- **Constructor null-check** (`src/Column.cpp:32-35`) still correct and now directly unit-tested (`Column.invalidStatementPtr`, `tests/Column_test.cpp:382-387`).
- **Conversion operators are not `noexcept` while the getters are** — cosmetic asymmetry only; `operator std::string()` genuinely can throw (allocation), so blanket `noexcept` would be wrong. Not a finding.
