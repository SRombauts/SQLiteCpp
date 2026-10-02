# Bug findings

Reviewed on 2026-10-01 against `15f2a5a714e87b2b616b59cf6462dc21edc0ee6e`
(SQLiteCpp 4.0.0), on branch `deep-code-review`. Original assessment revision:
`cf33ad77d86b1c8292bdf3374855c8020a8c95fa`.

The original file contained useful evidence, but mixed confirmed bugs with test gaps, speculative
concerns, and historical build defects. It also used file importance to suggest fix order.
This report separates those categories and ranks bugs by consequence and realistic triggers.
The recommended first fixes are **B1, B2, B3, B4, and B13**, all high-severity correctness bugs.
Severity describes correctness impact; none of these reports establishes a security exploit.

[File rankings](file-importance.md) remain unchanged. The original assessments, test/example
rationale, and historical validation are preserved in
[review-assessments.md](review-assessments.md).
This pass reviews all seven source/header pairs and six independent headers. Three subagents
received one group per assignment, reading the complete assigned source and relevant tests before
comparing historical findings. The agent service's total thread limit required reusing completed
reviewers for queued groups. The coordinator independently checked material findings and probes.
The per-group ledger below records inspection coverage and limits; this is not a security
certification. Historical validation against `0074d708370da25e3d18867a58eba5bc28c8824c` is retained
in the probe instructions.

## Confirmed bugs, in recommended fix order

Finding IDs are stable across passes. Runtime findings have high confidence and reproductions;
configuration and iterator defects have intentional compile-failure evidence.
Exact probe sources and build/run commands are in [review-probes](review-probes/README.md).

### B1. Loading a nonexistent backup file silently erases the current database

**Severity: high. Status: confirmed, newly identified during report validation.**

Affected code: [Database.cpp:401](../../../../src/Database.cpp#L401).
The [public contract](../../../../include/SQLiteCpp/Database.h#L620) describes loading a database
file and throwing on errors; it does not describe creating a missing restore source.

- **Trigger:** Populate a database, ensure an input path does not exist, then call
  `db.backup(missingPath, SQLite::Database::Load)`.
- **Expected:** Opening the missing source fails before changing the current database.
- **Actual:** `OPEN_READWRITE | OPEN_CREATE` creates an empty source. Copying that empty database
  succeeds and replaces the current schema/data. The call returns normally.
- **Impact:** A mistyped or missing restore path deletes existing tables and data. The probe
  confirms this for both in-memory and file-backed destinations; reopening the latter confirms
  the deletion persists. The missing source file is also created.
- **Remedy:** Use an existing-source open mode for Load, preferably `OPEN_READONLY` without
  `OPEN_CREATE`. Keep destination creation for Save. Do not reject intentionally empty existing
  source databases without a separately defined API policy.
- **Regression requirements:** Missing input throws, input remains absent, destination schema/data
  survive, and existing read-only input can be loaded. Cover memory-backed and disk destinations.

Evidence: `missing_load created=1 tables=0` and `missing_load persisted_tables=0`.
The existing Database backup test covers only a successful save/load round trip.

### B2. A finished Transaction can roll back or commit a successor transaction

**Severity: high. Status: confirmed; broader ownership variant also reproduced.**

Affected code: [rollback](../../../../src/Transaction.cpp#L80),
[destructor](../../../../src/Transaction.cpp#L50), and
[commit](../../../../src/Transaction.cpp#L66). Successful rollback leaves `mbCommited` false.

- **Trigger:** Keep the first wrapper alive after `first->rollback()`. Start a second
  `SQLite::Transaction` on the same connection and insert a row. Destroy the first wrapper.
  This uses only public APIs and respects database lifetime and thread constraints.
- **Expected:** The first wrapper is finished and cannot change the second transaction.
- **Actual:** Its destructor issues another ROLLBACK, discarding the second transaction's insert.
  The second wrapper's `commit()` then throws because no transaction remains active.
  Calling `first.commit()` instead prematurely commits the second transaction.
- **Impact:** Lost writes or unintended commits outside the transaction that owns them.
- **Remedy:** Track successful completion through either COMMIT or ROLLBACK. Finished wrappers
  must stop issuing transaction commands; their destructors must be inert. Update state only
  after successful execution: failed COMMIT can leave a transaction active and retryable.
- **Regression requirements:** Destroy a rolled-back first wrapper while a successor is active;
  the successor must still commit its insert. Old commit/rollback calls must not affect successors.
  Preserve scope rollback, repeated-operation error behavior, and retry/rollback after failed
  commit.

Evidence: `replacement_transaction rows=0`, successor commit throws, and
`old_wrapper_commit autocommit=1 rows=1`.
The [existing test](../../../../tests/Transaction_test.cpp#L96) has no successor transaction.

**Broader variant:** `INSERT OR ROLLBACK` can automatically end the first transaction without
updating its wrapper. Starting a successor and destroying the old wrapper again loses the new
insert. A successful-explicit-rollback state fix alone does not solve this. Connection ownership
tracking or a clear lifecycle contract is needed; checking autocommit only at destruction cannot
identify which transaction is active. A separate probe also confirms failed deferred-foreign-key
COMMIT stays active and can be repaired/retried, so a fix must not mark every failure as finished.

### B3. Backup and restore return normally without copying the requested contents

**Severity: high. Status: confirmed.**

Affected code: [Database.cpp:409](../../../../src/Database.cpp#L409).
`Backup::executeStep()` correctly returns retryable statuses; `Database::backup()` discards them.

- **Trigger:** Save while the source connection has an active write transaction, or Load from a
  source file held under `BEGIN EXCLUSIVE` by another connection.
- **Expected:** A normal return means the copy completed; failure must be reported or handled.
- **Actual:** The probe observes `SQLITE_BUSY` (5), yet the helper returns normally and destroys
  the incomplete backup. A fresh Save target has no source tables; an existing target retains its
  old contents. A contended Load retains the old destination instead of loading the source.
- **Impact:** Callers trust an absent/stale backup or a restore that never happened. Later deletion
  of the source can turn an unnoticed failed backup into unrecoverable data loss.
- **Remedy:** Accept success only after `SQLITE_DONE`. Throw on an unhandled retry status or use
  an explicitly bounded retry policy. Avoid an unbounded busy loop, particularly when the same
  source connection owns the write transaction.
- **Regression requirements:** Fresh/existing Save targets and contended Load must report failure
  or actually complete. Assert copied contents, not just a nonthrowing call. Keep happy-path checks.

Evidence: `backup raw_status=5`, `backup returned tables=0`,
`backup existing_old_row=123 new_table=0`, and `locked_load old_row=123 new_table=0`.

**Impact boundary:** SQLite [rolls back incomplete destination
writes](../../../../sqlite3/sqlite3.h#L9676)
when finishing the handle. This is false success, not demonstrated committed partial corruption.
Checking the finish result alone is insufficient: BUSY/LOCKED need not make it report failure.

### B4. Schema reprepare leaves stale metadata and can silently return the wrong column

**Severity: high. Status: confirmed; stronger impact than the original report.**

Affected code: [cached count](../../../../src/Statement.cpp#L39),
[stepping](../../../../src/Statement.cpp#L222), and
[name lookup](../../../../src/Statement.cpp#L297).
SQLite [supports automatic reprepare](../../../../sqlite3/sqlite3.h#L4562) after schema changes.

- **Trigger:** Prepare `SELECT * FROM t` for columns `(a, b)`, execute and prime `getColumn("a")`,
  reset, then rebuild `t` with columns `(b, a)` and execute the same statement again.
- **Expected:** Named lookup resolves the current result's names/indexes.
- **Actual:** The cached name map still associates `a` with index 0, now named `b`.
  With values `b=9, a=7`, `getColumn("a")` silently returns 9. The column count is unchanged.
  Adding a column also leaves the cached count too small and rejects valid index access.
- **Impact:** Silent wrong-value reads after a migration, or exceptions for valid current results.
- **Remedy:** Refresh column count and invalidate the name map when stepping can reprepare the
  statement, including cases with unchanged counts. Clearing names only on reset is insufficient
  if a caller populates the cache between reset and the next step.
- **Regression requirements:** Add/remove/rename/reorder columns, prime name caches, and check both
  named and indexed reads. Include changed schemas with identical column counts.

Evidence: `metadata cached=1 actual=2`, added-column access throws, and
`metadata name_a_value=9 actual_first_name=b` (the actual `a` value is 7).
Existing Statement tests do not exercise schema reprepare.

### B13. An invalidated child Savepoint can roll back or release a replacement

**Severity: high. Status: confirmed, newly identified by the Savepoint reviewer.**

Affected code: [destructor](../../../../src/Savepoint.cpp#L37),
[release](../../../../src/Savepoint.cpp#L54), and
[rollbackTo](../../../../src/Savepoint.cpp#L68).
The [documented caveats](../../../../include/SQLiteCpp/Savepoint.h#L39) allow parent operations
to remove child savepoints, but child wrappers keep their active state and use only the SQL name
to identify their resource.

- **Trigger:** Keep a child wrapper alive while calling `parent.release()` or
  `parent.rollbackTo()`. Create a replacement savepoint with the child's name and insert a row.
  Destroy the old child or call its `release()`.
- **Expected:** A wrapper whose original savepoint was removed cannot modify the replacement.
- **Actual:** The old destructor rolls back and releases the replacement, deleting its insert.
  Calling the old child's `release()` instead can commit the replacement early. Its own release
  subsequently throws because the named savepoint no longer exists.
- **Impact:** Lost writes or unintended commits. The reproductions use only wrapper APIs,
  one thread, a live Database, and the parent operations explicitly described in the header.
- **Remedy:** Track savepoint identity and ancestor invalidation, or prevent stale wrappers from
  addressing a reused SQL name through a defined identity/lifecycle policy. A name-existence check
  alone cannot distinguish the replacement. Preserve failed-release retryability.
- **Regression requirements:** Parent release and parent rollback-to invalidate children without
  letting old destruction/release/rollback affect replacements. Cover reused names, nested scope
  cleanup, and failed deferred-constraint release followed by repair/retry.

Reviewer and coordinator independently reproduced `parent_release successor_rows=0`,
`successor_release_error=no such savepoint: child`, `parent_rollback successor_rows=0`, and
`old_release successor_rows=1 autocommit=1`. ASAN was clean. Existing Savepoint tests cover
ordinary scope cleanup and direct manual removal but no parent-invalidated child/name reuse.
This is separate from B2: Savepoint has different completion rules and a named resource stack.

### B5. The custom assertion macro captures a caller's else branch

**Severity: medium. Status: confirmed for custom-handler builds.**

Affected code: [Assertion.h:36](../../../../include/SQLiteCpp/Assertion.h#L36).
With `SQLITECPP_ENABLE_ASSERT_HANDLER`, the expansion is an unwrapped `if`.

- **Trigger:** Use `if (condition) SQLITECPP_ASSERT(...); else action();`.
- **Expected/actual:** The outer else should run when condition is false. Instead, it binds to the
  macro's inner if and does not run. Probe output: `assertion outer_else_reached=0`.
- **Impact:** Wrong control flow in user code. Current library assertion call sites do not reproduce
  this pattern, so no current internal data loss is demonstrated.
- **Remedy/tests:** Wrap the expansion in `do { ... } while (false)`; test outer if/else behavior,
  true/false expressions, single evaluation, and both custom/default assertion configurations.

### B9. Failed string conversion returns UTF-16 bytes as a successful getString result

**Severity: medium. Status: confirmed, newly identified by the Column reviewer.**

Affected code: [Column.cpp:94](../../../../src/Column.cpp#L94).
The initial `sqlite3_column_bytes()` forces UTF-8 conversion but discards its result and error.

- **Trigger:** Read TEXT from a UTF-16 database after successfully stepping, with SQLite allocation
  failure armed for the UTF-8 conversion and connection lookaside disabled.
- **Expected:** Return UTF-8 or throw `SQLite::Exception` reporting `SQLITE_NOMEM`.
- **Actual:** Conversion fails; the subsequent blob accessor exposes the original UTF-16 buffer
  and changes its representation flags. The final byte count is the UTF-16 length. Four thousand
  ASCII X characters become an 8000-byte string with interspersed NULs, without an exception.
- **Impact:** Incorrect string content under memory pressure. String casts and streaming inherit
  the behavior. No memory-corruption or exploitability consequence was demonstrated.
- **Remedy:** Detect SQLite accessor conversion errors immediately, before another accessor can
  change the representation or diagnostics. Preserve UTF-8 and binary-safe copying on success.
- **Regression requirements:** Inject the conversion allocation failure after successful stepping;
  require an exception with `SQLITE_NOMEM`. Retain UTF-16 success, NULL/empty TEXT/BLOB, embedded
  NUL, and stream behavior checks.

The reviewer and coordinator independently built the allocator probe. Normal result:
`failedCalls=0 threw=0 resultSize=4000 equalsExpected=1`. Failure result:
`failedCalls=1 threw=0 resultSize=8000 equalsExpected=0 sqliteError=7`, with no ASAN diagnostic.
The [SQLite contract](../../../../sqlite3/sqlite3.h#L5561) explains the immediate error check.
Existing Column tests cover ordinary UTF-16 reads but no allocation failures.

### B6. getHeaderInfo reports one byte for a valid 64 KiB page size

**Severity: low. Status: confirmed.**

Affected code: [Database.cpp:373](../../../../src/Database.cpp#L373).
Create a file with `PRAGMA page_size=65536` before creating its first table.
`pageSizeBytes` reports 1 instead of 65536 because the valid on-disk sentinel is not decoded.
This misleads consumers of header metadata; no database mutation or corruption is demonstrated.
Decode raw value 1 to 65536 and add a real-file regression alongside ordinary page-size cases.
Evidence: `header page_size=1`. Existing tests use the ordinary 4096-byte size.

### B7. getHeaderInfo accepts an invalid final signature byte

**Severity: low. Status: confirmed.**

Affected code: [Database.cpp:358](../../../../src/Database.cpp#L358).
Change byte 15 of a valid file from NUL to `X`. The parser accepts it because it overwrites that
byte and compares only the first 15 bytes. Expected: reject the invalid 16-byte SQLite signature.
Impact is false acceptance by this metadata helper; SQLite itself still rejects the malformed file.
Compare all 16 raw bytes before terminating the display copy. Add a corrupted-byte-15 regression.
Evidence: `header bad_magic_accepted=1`. This does not assert exhaustive database validation.

### B8. getExpandedSQL leaks its SQLite buffer if string construction throws

**Severity: low. Status: confirmed by allocation injection and LeakSanitizer.**

Affected code: [Statement.cpp:370](../../../../src/Statement.cpp#L370).
After SQLite allocates expanded SQL, construction of the returned `std::string` can throw before
`sqlite3_free()` is called. The injected `std::bad_alloc` leaves 80 SQLite-accounted bytes
allocated;
LeakSanitizer reports an 88-byte direct allocation including allocator overhead.
Use immediate RAII ownership with a `sqlite3_free` deleter before constructing the string.
Add focused allocation-failure coverage; preserve successful expansion behavior.
Impact is a leak on an allocation-failure path, with no demonstrated ordinary-path leak.

### B10. Extension-load exceptions report unrelated connection diagnostics

**Severity: low. Status: confirmed, newly identified by the Database reviewer.**

Affected code: [Database.cpp:222](../../../../src/Database.cpp#L222).
Calling `loadExtension()` with a nonexistent library throws `what() == "not an error"`, primary
code 1 and extended code 0 on a fresh connection. Prior operations can produce a different stale
diagnostic. The function discards `sqlite3_load_extension()`'s dedicated error output and instead
uses connection error state that this failure does not update.

The reviewer and coordinator both reproduced this. Calling the C API with an error-output pointer
returns the actual missing-library message. Capture that output with immediate `sqlite3_free`
RAII ownership and construct the exception from it and the returned status; use a suitable fallback
when no diagnostic is available. Do not attach an unrelated extended code. Regressions should test
a fresh connection and prior unrelated errors/ROW results. The existing test asserts only a throw.
This is a diagnostic defect, separate from extension-loading capability policy.

### B11. Experimental filesystem mode bypasses the filesystem-disable option

**Severity: low. Status: confirmed, configuration-specific compile defect.**

Affected code: [Database.h:17](../../../../include/SQLiteCpp/Database.h#L17) and
[Database.h:41](../../../../include/SQLiteCpp/Database.h#L41).
Define both `SQLITECPP_HAVE_STD_EXPERIMENTAL_FILESYSTEM` and `SQLITECPP_DISABLE_STD_FILESYSTEM`.
The experimental branch skips the disable block and enables filesystem support anyway. Both
reviewer preprocessing and coordinator compilation confirm the support macro remains defined.
On GCC 13 C++17 the experimental namespace alias also conflicts with libstdc++'s existing
`std::filesystem` declaration. Default filesystem support is unaffected.

Make disabling take precedence over both detection paths; reconsider the experimental branch on
the C++17 baseline. Compile both ordinary disable and combined experimental/disable configurations,
asserting that neither the support macro nor path overload remains. The preserved probe contains
an intentional `#error` for incorrectly enabled support; compilation failure is the evidence.

### B12. RowIterator does not meet its advertised C++17 input-iterator contract

**Severity: low. Status: confirmed API conformance defect.**

Affected code: [Statement.h:742](../../../../include/SQLiteCpp/Statement.h#L742),
[postfix increment](../../../../src/Statement.cpp#L387), and the missing arrow operator.
The iterator declares `std::input_iterator_tag`, but `visit(*first++)` fails to compile because
postfix increment returns void. `it->getColumn(0)` also fails. These expressions are required by
the [C++17 draft's input-iterator table](https://timsong-cpp.github.io/cppwp/n4659/input.iterators).
Independent reviewer and coordinator compile probes confirm both failures.

Range loops and prefix iteration work. No unsupported independent-copy behavior is alleged:
the header explicitly limits one active iterator per Statement. Either narrow the advertised
contract to supported row iteration or provide a coherent input-iterator value/proxy interface,
including the noncopyable Statement value type. Returning an iterator copy from postfix alone
would still expose the advanced mutable row. Existing tests check traits and discarded postfix
results; add compile/use coverage for the operations actually promised.

### B14. Default assertion expansion changes assignment expressions

**Severity: low. Status: confirmed, newly identified by the Assertion reviewer.**

Affected code: [Assertion.h:45](../../../../include/SQLiteCpp/Assertion.h#L45).
The default macro expands to `assert(expression && message)` without parenthesizing its arguments.
`SQLITECPP_ASSERT(value = 7, "message")` assigns 1 rather than 7: the assignment receives the
result of the appended logical expression. Standard `assert(value = 7)` and custom-handler mode
assign 7. The reviewer and coordinator reproduced this difference; no in-tree trigger was found.

Use `assert((expression) && (message))`. Test assignment/compound-assignment expressions and
single evaluation, including default/custom modes and expected NDEBUG elision. Do not introduce
side effects relied upon in release builds. The tested ordinary bitwise flag expression behaves
correctly and is not a trigger. This defect is separate from B5's custom-handler dangling else.

### B15. getIndex's pure annotation suppresses its exception in optimized callers

**Severity: low. Status: confirmed, newly identified by the Utils reviewer.**

Responsible API: [Statement.h:125](../../../../include/SQLiteCpp/Statement.h#L125).
The [Utils macro](../../../../include/SQLiteCpp/Utils.h#L24) supplies a pure attribute to a
method that calls the throwing `getPreparedStatement()` check.

Construct an accepted empty-query Statement, then discard `getIndex(":name")`'s result inside
a try/catch. At `-O2`, GCC 13 and Clang omit the pure call; the expected
`"Statement was not prepared."` exception disappears. When the caller defines
`SQLITECPP_PURE_FUNC` as empty, it catches that exception. Used-result calls still throw.
The coordinator independently rebuilt both GCC variants against the same library.
The [GCC docs](https://gcc.gnu.org/onlinedocs/gcc-13.3.0/gcc/Common-Function-Attributes.html)
permits treating the return value as the only observable effect; this annotation is inappropriate
for the checked wrapper method.

Remove the attribute from the throwing API or separate its checked and safely pure operations.
Test optimized discarded-result and used-result error behavior, plus ordinary name lookup with
changed name-buffer contents. All in-tree binding consumers use the result; no ordinary binding
failure or security consequence was demonstrated. This is attributed to Statement's annotation
use, rather than a defect in a generic attribute macro's compiler detection.

## Confirmed documentation defects

These are actionable documentation reports, not demonstrated runtime memory vulnerabilities.

- **D1, Column pointer lifetime:** [getText/getBlob
  warnings](../../../../include/SQLiteCpp/Column.h#L88)
  mention finalization but omit step/reset/type-conversion invalidation from the
  [SQLite contract](../../../../sqlite3/sqlite3.h#L5555). State the full borrowed-pointer lifetime.
  Saved Column objects are not snapshots; shared statement ownership does not retain the Database.
  The streaming comment also names getText although the implementation uses getString.
  Database's [execAndGet warnings](../../../../include/SQLiteCpp/Database.h#L370) incorrectly
  prohibit retaining the returned Column after the local Statement dies: shared ownership keeps
  that statement alive. Distinguish owning a Column from borrowing its pointer.
- **D2, no-copy binding lifetime:** [older
  overloads](../../../../include/SQLiteCpp/Statement.h#L178)
  say buffers must remain unchanged only while executing. SQLite and bindNoCopy64 require valid
  storage until rebinding, clearing bindings, or actual finalization. Reset preserves bindings;
  retained Columns can delay finalization. Align all overloads. The original blanket classification
  of early release as documented caller misuse overstated the wrapper documentation's consistency.
- **D3, Backup destination usage:** The public header omits SQLite's
  [init-to-finish restriction](../../../../sqlite3/sqlite3.h#L9716) on using the destination
  connection.
  Three Backup tests query it while the handle still lives, even after SQLITE_DONE. Scope-destroy
  Backup before querying the destination, document both connection lifetimes, and fix
  examples/tests.
  The passing baseline does not establish these unsupported operations are safe.
- **D4, API examples/discovery:** VariadicBind examples omit the required Database constructor
  argument and use invalid SQLite `&&` syntax; use `AND` after fixing construction.
  SQLiteCppExport's comment names `SQLITECPP_EXPORT` instead of `SQLITECPP_DLL_EXPORT`;
  the umbrella header promises all functionality but omits four optional APIs. Correct the examples
  and wording. Its version prose also names a nonexistent header and calls SQLiteCpp's version
  SQLite's version. Actual version values agree with the build/package metadata. These have lower
  urgency than the confirmed data-integrity bugs.
- **D5, file-header helper contracts:**
  [Database.h:585](../../../../include/SQLiteCpp/Database.h#L585)
  and [Database.h:600](../../../../include/SQLiteCpp/Database.h#L600) promise URI input, while both
  helpers open the literal name with `std::ifstream`. A `file:...?...` URI accepted by Database
  with `OPEN_URI` fails in the instance header helper. Clarify filesystem-path-only input and
  instance behavior or deliberately resolve the SQLite filename. Also,
  [defaultPageCacheSizeBytes](../../../../include/SQLiteCpp/Database.h#L133) represents the raw
  header's persisted **page count**, not bytes: `PRAGMA default_cache_size=12` returns field 12
  with 4096-byte pages. Correct its naming/docs and the example without silently changing units.
- **D6, public lifetime and template descriptions:** Transaction and Savepoint RAII prose
  incorrectly implies connection validity needs no attention; both store a Database reference
  and require it to outlive the guard. Column, Statement, Database, Transaction and Savepoint
  thread explanations mention the removed `Statement::Ptr` type/custom pointer. State the current
  ownership mechanism and retain the threading constraint. Statement's `getColumn` notes call
  those methods non-const although both are const.
  [getColumns documentation](../../../../include/SQLiteCpp/Statement.h#L606) describes
  `[0, getColumnCount())`, but the implementation permits `1 <= N <= getColumnCount()`; zero
  throws and a full-width tuple is supported. Correct the documented range.
  Savepoint's parent rollback caveat should state that descendants are removed, and nested release
  merges into the parent rather than independently committing to disk.
- **D7, Exception diagnostic snapshots:** Handle constructors capture the current connection's
  diagnostic state. The `(handle, ret)` overload combines the supplied result code with the
  connection's message/extended code. Document that those must correspond to the same failure;
  APIs with separate error outputs need those outputs. This clarifies the contract and does not
  duplicate Database's confirmed B10.

## Resolved and unconfirmed items

- **Resolved in current source:** The historical Meson assertion-handler macro mismatch is fixed
  in [meson.build](../../../../meson.build#L105), including propagation to consumers. It is not an
  open bug at the revalidated revision. The dangling-else macro defect B5 remains separate.
- **Extension loading:** C-API loading remains enabled, including after a failed load, but the
  public API says it enables module loading. The bundled/current path keeps SQL `load_extension()`
  disabled. The legacy fallback and any security consequence need separate configuration-specific
  validation. This is a hardening concern, not a confirmed SQL-injection vulnerability.
- **Column NULL/empty paths:** NULL-pointer string construction portability remains unconfirmed.
  Reviewer probes pass NULL, empty TEXT/BLOB, embedded NUL, numeric stringification and unsigned
  boundaries on libstdc++ with ASAN. UTF-16 conversion allocation failure is now confirmed as B9,
  rather than an unvalidated concern. Direct invalid-index construction has no promised validation;
  saved Columns are current-row wrappers, not snapshots.
- **Input boundaries:** Negative int blob lengths reach an SQLite API that documents undefined
  behavior for that invalid input. Extreme SQL-length narrowing and embedded-NUL Savepoint names
  remain input-contract/hardening concerns; no exploit or new confirmed failure is reported here.
  The Savepoint reviewer confirmed NUL names alias their prefix, as SQLite's `quote()` documents;
  an ordinary quote/injection-shaped name is escaped and leaves the table intact. B13 reproduces
  with ordinary names and is independent of these boundaries.
- **Helper semantics:** ExecuteMany partial progress without a transaction, discarded result rows,
  and VariadicBind partial/persistent bindings have no promised atomicity/strong exception
  guarantee.
  Clarify usage and test gaps without reporting expected behavior as a confirmed defect.
- **tableExists scope:** The implementation checks only main `sqlite_master`. A TEMP table can be
  queryable while this helper returns false. Clarify main-schema scope or define broader behavior;
  do not classify this as a new runtime defect without a corresponding schema-search promise.

## Complete per-group review ledger

All 20 files appear in exactly one group below. Each assignment read its entire group and relevant
tests/call sites before consulting historical findings. Reviewers A, B and C are the three reused
subagent threads; assignments were separate and read-only. Commands used full `cat`/`nl` reads,
`rg` call-site/contract searches, focused C++17 builds, and the checks summarized below.

1. **Database, reviewer A:** [source](../../../../src/Database.cpp) and
   [header](../../../../include/SQLiteCpp/Database.h). Inspected all constructors/delegation,
   moves/deletion, timeout, SQL execution, counters, diagnostics, callbacks, extension flags,
   key/rekey validation, header parsing, backup directions and filesystem branches. Read all
   `Database_test.cpp`, relevant example1 sections, Backup/Exception/Column ownership and bundled
   SQLite open/backup/extension/header contracts. Retained B1/B3/B6/B7; new B10/B11; D1/D5/D6.
   Fresh ASAN probes reproduced missing-source deletion, BUSY false success, page sentinel and
   signature acceptance. Extension and macro probes were independently checked by the coordinator.
   `execAndGet` retained-Column, URI failure, cache-page units and TEMP lookup were also checked.
   No new constructor, direct-forwarding, callback-registration or key-length bug established.
   Codec execution, successful real extension and legacy SQLite remain untested.

2. **Statement, reviewer B:** [source](../../../../src/Statement.cpp) and
   [header](../../../../include/SQLiteCpp/Statement.h). Inspected construction, moves/assignment,
   shared finalization, all binding families and deleted temporary overloads, reset/step states,
   row/index checks, metadata/name caches, tuples, expanded SQL, diagnostics and every iterator
   operation. Read all `Statement_test.cpp`, relevant examples, Column templates and bundled
   prepare/bind/row/metadata contracts. Retained B4/B8; new B12; D2/D6; B15 was identified during
   Utils review and attributed here. ASAN metadata probes cover reorder/add/remove, including
   stale checks accepting obsolete indices. Allocation injection reproduces B8 and intentional
   LSan failure. Postfix/arrow compile probes fail as reported. Normal
   binding/reset/move-destination
   behavior showed no additional defect. Extreme SQL lengths and optional trace/old SQLite builds
   remain untested; database lifetime and saved-row misuse were rejected as new vulnerabilities.

3. **Column, reviewer C:** [source](../../../../src/Column.cpp) and
   [header](../../../../include/SQLiteCpp/Column.h). Inspected shared ownership/construction,
   all numeric/name/origin/string/blob accessors and casts, predicates, byte size and streaming.
   Read all `Column_test.cpp`, relevant Statement templates/tests/examples and bundled conversion
   contracts/implementation. New B9; retained D1, plus D6. Independent SQLite allocator injection
   and coordinator rerun reproduce wrong UTF-16 output; a normal control returns correct UTF-8.
   ASAN boundary checks pass NULL, empty TEXT/BLOB, embedded NUL, numeric stringification, stream
   agreement and unsigned conversion. No NULL-string portability defect was proven. Direct invalid
   indices and row advancement need caller-precondition clarity; no cross-platform allocator sweep.

4. **Transaction, reviewer C:** [source](../../../../src/Transaction.cpp) and
   [header](../../../../include/SQLiteCpp/Transaction.h). Inspected both constructors, all behavior
   branches, noncopyable/nonmovable ownership, completion state, destructor, commit and rollback.
   Read all `Transaction_test.cpp`, related Savepoint tests, example1 transaction sections and
   SQLite transaction/autocommit/automatic-rollback contracts. Retained B2 and D6; no distinct new
   implementation bug. ASAN probes cover old destruction/commit/rollback after explicit or implicit
   rollback; all can finish the successor. Failed deferred-FK commit is repairable/retryable and
   nested BEGIN failure preserves the original transaction. I/O/authorizer failures and locking
   stress were not injected.

5. **Backup, reviewer A:** [source](../../../../src/Backup.cpp) and
   [header](../../../../include/SQLiteCpp/Backup.h). Inspected all three constructors/schema
   delegation, failed acquisition, unique ownership, step statuses, counters and finalization.
   Read all five `Backup_test.cpp` tests, Database helper usage and bundled init/step/finish
   contracts.
   No new runtime bug; retained D3 and attributed B3 to Database. Supported-scope ASAN probes verify
   partial cancellation preserves old destination data, complete copy, BUSY then retry, same-source
   and missing-schema errors, and read-only destination failure. Destination queries were performed
   only after Backup destruction. Shared-cache concurrency, attached/temp schemas and injected
   I/O/allocation failures were not exercised; returning retry statuses is intentional.

6. **Savepoint, reviewer B:** [source](../../../../src/Savepoint.cpp) and
   [header](../../../../include/SQLiteCpp/Savepoint.h). Inspected quote construction, acquisition
   exceptions, ownership/copy/move, release/rollback state, destructor, ancestor removal and names.
   Read all five `Savepoint_test.cpp` tests and bundled savepoint opcode/quote implementation;
   no substantive executable example exists. New B13, with independent coordinator reproduction;
   D6 ancestor-removal/commit wording. All five existing tests pass. ASAN probes verify parent
   release/rollback removing children, successor loss or premature commit, safely escaped malicious
   quote text, NUL-prefix aliasing and retry after failed deferred-FK release. Normal rollback-to
   remains active and correctly supports later scope rollback. Name boundaries stay separate
   concerns; no SQL injection, lock-contention or allocation-failure campaign was established.

7. **Exception, reviewer A:** [source](../../../../src/Exception.cpp) and
   [header](../../../../include/SQLiteCpp/Exception.h). Inspected all constructors/delegation,
   null handling, owned message, both codes, generic strings and copy/assignment/destruction.
   Read all four `Exception_test.cpp` tests, all wrapper throwing call sites and bundled error-state
   contracts. No new implementation defect; D7 clarifies snapshots and B10 stays with Database.
   ASAN probes verify constraint primary/extended codes, extended-results mode, message/code
   persistence after successful calls and database close, null failed-open handle, null explicit
   message and unavailable-code sentinels. Copy/assignment/destruction are noexcept on this
   toolchain.
   External SQLite null-return variants and allocation injection remain untested.

8. **Assertion, reviewer C:** [header](../../../../include/SQLiteCpp/Assertion.h). Inspected
   both expansions, callback, MSVC function macro, all assertion/handler call sites,
   README/examples,
   and CMake/Meson propagation. Retained B5; new B14. Default/custom and NDEBUG matrix probes verify
   dangling else, assignment precedence, single evaluation, conditional message evaluation and
   custom callback counts; coordinator reran all four configurations. Ordinary bitwise assertions
   work. Meson's old macro mismatch is resolved. Throwing destructor callbacks violate the existing
   contract; no new handler vulnerability. Native MSVC/shared callback linkage was not tested.

9. **Exports, reviewer A:** [header](../../../../include/SQLiteCpp/SQLiteCppExport.h). Inspected
   every DLL/static/visibility branch, producer/consumer flags and warning pragmas, actual
   CMake/Meson propagation, consumer target and relevant CI matrices. No new linkage bug; D4
   documents the wrong producer macro. Native GCC and Clang preprocessing and simulated non-GNU
   Windows branches match intended annotations. A tiny actual Linux hidden-visibility shared
   library exports only the annotated symbol; `nm -D` and a linked consumer pass. Windows checks
   are preprocessing simulations, not native linkage proof; MinGW/native macOS remain untested.

10. **ExecuteMany, reviewer B:** [header](../../../../include/SQLiteCpp/ExecuteMany.h). Inspected
    all three templates, overload lookup, forwarding, sequencing, reset/clear, binding/execution
    failures, result draining and transaction boundaries. Read all `ExecuteMany_test.cpp`, full
    VariadicBind dependencies and relevant Statement/SQLite contracts. No confirmed bug. Both
    direct tests pass; ASAN probes check mixed/scalar/const/lvalue/rvalue/empty tuples, omitted
    parameters becoming NULL, embedded NULs, excess fields, stopped execution after errors and
    caller-transaction rollback. Partial autocommit progress and discarded results are intentional.
    Usage docs could explain these semantics and scalar sets; no atomicity guarantee is promised.
    No ADL campaign or RETURNING-specific runtime check was performed.

11. **Umbrella, reviewer B:** [header](../../../../include/SQLiteCpp/SQLiteCpp.h). Inspected
    all includes/group/version comments, complete header inventory, package/build/doc versions,
    installation configuration, examples and consumer source. No runtime bug; retained/extended D4.
    Compile probes confirm the four optional APIs require extra headers and version macros agree
    with CMake/Meson/package metadata. Unchanged consumer runs successfully with C++17 and C++20
    against the C++17 ASAN library. Existing version tests check underlying SQLite rather than
    wrapper macros. No installed-package, disabled-feature or C++20-library build was performed.

12. **VariadicBind, reviewer C:** [header](../../../../include/SQLiteCpp/VariadicBind.h).
    Inspected scalar/tuple/index-sequence overloads, sequencing, conversions, lifetimes, persistence
    and bounds; read all direct tests, ExecuteMany tests/header, Statement bind contracts and real
    example1 usage. No implementation bug; D4 additionally records invalid `&&` SQL. ASAN probes
    pass typed numeric/NULL/embedded-NUL/temporary/reference tuple values, empty packs, source
    changes
    after copying, left-to-right conversion and partial bindings after range failure. Documented
    examples fail SQLite preparation even after their constructor is repaired. Preserved or partial
    bindings match sequential bind semantics. No exhaustive user-defined conversion/compiler sweep.

13. **Utils, reviewer A:** [header](../../../../include/SQLiteCpp/Utils.h). Inspected every
    detection/override/fallback branch, the sole getIndex annotation, throwing handle guard,
    accepted empty-query preparation, all getIndex callers and bundled read-only name lookup.
    Macro generation itself has no confirmed bug; new B15 is attributed to Statement. Preprocessing
    verifies default attribute, empty/custom overrides and unsupported-compiler fallback. Optimized
    GCC/Clang ASAN probes suppress discarded-call exceptions; empty-attribute control restores them.
    Changed name-buffer contents still produce indices 1 then 2. Coordinator independently confirms
    both GCC variants. No native Intel/ARM/TI/MSVC or LTO run; existing binding tests use the
    result.

Important positive checks and rejected concerns above are bounded observations. Supporting scratch
probes under `/tmp/sqlitecpp-review-*` are not durable regression tests. Material defect probes and
failed-release/metadata controls are preserved with exact commands in
[review-probes/README.md](review-probes/README.md).

## Validation and limits

The coordinator built the current revision with GCC 13.3, C++17, Debug, bundled SQLite, column
metadata, and AddressSanitizer. All **69 existing unit tests passed**. The main reproduction and
assertion probes completed; the expanded-SQL probe intentionally caused a LeakSanitizer failure
that confirms B8. All seven newly accepted findings were independently reproduced or compiled by
the coordinator, including optimized/default-versus-empty-attribute controls for B15. Saved runtime
probes were rebuilt after formatting and produced the reported results. Exact commands, sources,
and observed output are preserved in
[review-probes/README.md](review-probes/README.md).

No library source or regression tests were changed. Fixes and the regression requirements above
remain work to do. No Windows/macOS builds, codec runs, older external SQLite builds, exhaustive
allocation-failure campaign, fuzzing, or instrumented coverage measurement were performed.
Successful existing tests do not invalidate defects in scenarios they do not cover.
The skill description covers both full review and report validation; no trigger change is needed.
The procedure now explicitly permits separate assignments on reused reviewer threads when the
service limits their total number. No commit or publication was requested or performed.
