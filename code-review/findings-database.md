# Findings — Database unit

Files: `include/SQLiteCpp/Database.h`, `src/Database.cpp`, `tests/Database_test.cpp`.

`SQLite::Database` is the RAII owner of a `sqlite3*` connection (`std::unique_ptr<sqlite3, Deleter>`). It opens via `sqlite3_open_v2()` and closes via `sqlite3_close()` in the deleter. It is the root of the ownership graph: `Statement` (a `friend`), `Backup`, `Column`, `Transaction`, and `Savepoint` all depend on a live `Database&`.

## Findings (most severe first)

### DB-001 — Defaulted move can orphan live `Statement`s / close a busy handle
- [ ] **Severity:** medium — **Confidence:** high — **Category:** resource-safety / lifetime
- **Location:** `include/SQLiteCpp/Database.h:254-255` (defaulted move ctor/assign); interacts with `Statement` friend access (`Statement.h`)
- **Impact (failure scenario):** A `Statement` captures the raw `sqlite3*` at construction. `Database` is `default`-movable. On move-**assignment** into a `Database` that still owns live `Statement`s, the target's previous connection is closed by `unique_ptr` while its statements are not finalized: `sqlite3_close()` returns `SQLITE_BUSY` (the `Deleter` assert fires in debug; handle leaks in release) and the orphaned statements dangle (use-after-free on next use).
- **Fix:** Document the "no outstanding statements" precondition on the move operations, or enforce it (track outstanding statements and `SQLITECPP_ASSERT` count==0 on move/close). Add a regression test that moves a `Database` with a live `Statement`.

### DB-005 — `backup(..., Load)` opens the source `READWRITE|CREATE`, silently creating+restoring an empty DB
- [ ] **Severity:** medium — **Confidence:** medium — **Category:** correctness / data-loss
- **Location:** `src/Database.cpp:365-379`
- **Impact (failure scenario):** For `BackupType::Load` (restore *from* `apFilename`), the source file is opened `OPEN_READWRITE | OPEN_CREATE`. A missing or mistyped source path is therefore **created as an empty database** and then "restored" over the live destination — **silent data loss, no error**. Loading from a read-only file/FS also fails needlessly. The peer is also opened with the default VFS, ignoring any custom VFS the original connection used.
- **Fix:** Make the open flags direction-dependent — `Load` should open the source `OPEN_READONLY`; only `Save` needs `RW|CREATE`. Add a test that `Load` from a non-existent file throws rather than wiping the destination.

### DB-004 — `getHeaderInfo().defaultPageCacheSizeBytes` is unsigned, but the field is signed in the file format
- [ ] **Severity:** low — **Confidence:** medium — **Category:** correctness / api-design
- **Location:** `include/SQLiteCpp/Database.h:138`; filled at `src/Database.cpp:353`
- **Impact (failure scenario):** File-format offset 48 ("default page cache size") is a **signed** 32-bit big-endian int (negative ⇒ size in KiB). `readBE32()` returns `uint32_t`, so `PRAGMA default_cache_size = -2000` surfaces as `0xFFFFF830` (~4.29e9). The `...SizeBytes` name is also misleading (it is a page count, or KiB when negative).
- **Fix:** Type the field `std::int32_t` and reinterpret the bytes as signed. Add a round-trip test with a negative `default_cache_size`.

### DB-002 — `rekey()` narrows `size_type`→`int` without a cast (inconsistent with `key()`)
- [ ] **Severity:** low — **Confidence:** high — **Category:** portability / integer-narrowing
- **Location:** `src/Database.cpp:248`
- **Impact (failure scenario):** `int passLen = aNewKey.length();` is an implicit `size_t`→`int` narrowing; `key()` (line 229) uses an explicit `static_cast<int>`. Produces a `-Wconversion` warning and is inconsistent. (Only compiled under `SQLITE_HAS_CODEC`, so not seen in the default build.)
- **Fix:** `const int passLen = static_cast<int>(aNewKey.length());`

### DB-003 — `key()` / `rekey()` declared `const` but mutate connection / file encryption state
- [ ] **Severity:** low — **Confidence:** medium — **Category:** api-design / const-correctness
- **Location:** `include/SQLiteCpp/Database.h:537,553`; defs `src/Database.cpp:227,245`
- **Impact (failure scenario):** `rekey()` rewrites every page of the database yet is callable on a `const Database&` — a footgun advertising a non-mutating operation; asymmetric with `setBusyTimeout`/`createFunction`/`loadExtension` (correctly non-const). Changing the signature is a source-compatibility break.
- **Fix:** Prefer making `rekey()` non-`const` (document the break). `key()`'s `const` is more defensible (unlock-only).

### DB-006 — `tableExists()` case-sensitivity / error path untested (test gap)
- [ ] **Severity:** info — **Confidence:** medium — **Category:** test-gap
- **Location:** `src/Database.cpp:132-138`
- **Impact (failure scenario):** Logic is correct; no test exercises the documented case-sensitivity or the throwing path on an errored connection.
- **Fix:** Add a case-sensitivity test. No code change.

## Verified non-issues
- **Null `apFilename`** (`Database.cpp:67`): guarded for the `std::string`; `sqlite3_open_v2(nullptr,...)` is a valid temp-DB open. Tested (`Database.nullFilename`).
- **`Deleter` on `nullptr`** (`Database.cpp:83-93`): `sqlite3_close(nullptr)` is a documented no-op; asserts only on `SQLITE_BUSY`; never throws from the destructor.
- **`readBE32` shifts** (`Database.cpp:332-360`): unsigned operands built from `unsigned char`, so `<<24` is well-defined (this is the `6c7a96f`/#558 fix). Short-read `< 100` check is correct.
- **`isUnencrypted()`** (`Database.cpp:273-290`): `gcount() != 16` rejects short files (tested); `header` value-initialized; `memcmp` against the 16-byte magic correct.
- **`execAndGet()` ignoring the `executeStep()` bool** (`Database.cpp:124-129`): intentional — empty result throws in `getColumn(0)`, tested by `execException`.
- **`SQLITE_DETERMINISTIC` fallback `0x800`** (`Database.cpp:22-24`): matches SQLite's constant.
