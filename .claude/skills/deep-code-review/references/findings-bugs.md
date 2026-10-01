# Bug findings and review evidence

Reviewed on 2026-10-01, on branch `deep-code-review`, against local `master` commit
`cf33ad77d86b1c8292bdf3374855c8020a8c95fa` (SQLiteCpp 4.0.0).

These notes were moved from the original priority inventory without re-running the review.
They preserve reproduced bugs, static observations, documentation defects, plausible concerns,
and documented caller misuse with their original confidence and validation limits.
The ranked file groups and scores remain in [file-importance.md](file-importance.md).
Assessment scores below explain that historical ranking; they are not bug severity ratings.

## Scope and method

The inventory contains **20 files: seven implementations and 13 headers**, grouped into seven
source/header pairs and six independent headers. Every file under `src/` and every header under
`include/` is represented once in the ranked inventory. No unmatched implementation or generated
header was
found in these directories. Bundled SQLite, build files, tests, and examples supplied supporting
evidence; they are not separately ranked here.

Three sub-agents supplied separate assessments for every group: one reviewed Database, Statement,
and Column; one reviewed the four remaining pairs; one reviewed the six independent headers.
The primary agent checked material evidence, retained the rubric-consistent scores, and reproduced
selected findings in an isolated build. Usage means observed repository dependencies and examples,
not measured usage by downstream projects.

Scores use the [skill rubric](../SKILL.md#scoring-rubric), from 1 to 5:

- I: importance, from peripheral support to foundational API.
- C: intrinsic complexity, from trivial forwarding to broad state/lifetime interactions.
- T: test quality, from absent checks to thorough direct edge/error/lifetime checks.
- E: example quality, from absent examples to broad examples of tricky contracts/errors.
- R: vulnerability or other bug risk, from low exposure to high-impact exposed boundaries.

Higher T and E mean better coverage/examples; higher I, C, and R mean greater review pressure.
Priority is **3I + 3R + C + (6 - T)**. Examples are reported separately rather than driving
priority.
Rows sort by descending priority, risk, importance, then ascending group name. Scores are ordinal
judgment, not measured coverage, CVSS ratings, or evidence that a vulnerability is exploitable.

## Assessment evidence

### Database

I5/C5: Foundational connection owner used by Statement, Transaction, Savepoint, and Backup.
It combines execution, callbacks, extension loading, codec flags, file parsing, and connection
cleanup ([implementation](../../../../src/Database.cpp#L64)).
T4: Broad direct tests cover opening, moving, SQL errors, callbacks, malformed headers, and codec
arguments ([tests](../../../../tests/Database_test.cpp#L81)); codec execution is conditional.
The backup test is an uncontended round trip (line 186); extension success remains TODO (line 525).
E4: Multiple substantive open/query/blob/header/transaction examples
([example1](../../../../examples/example1/main.cpp#L97),
[example2](../../../../examples/example2/src/main.cpp#L45)).
R5: Reproduced incomplete backup success and two header parsing defects (see reproduction section).
[Backup call](../../../../src/Database.cpp#L409) ignores retry statuses;
[header parsing](../../../../src/Database.cpp#L358) overwrites the signature's final byte and
[line 373](../../../../src/Database.cpp#L373) leaves the 64 KiB page-size encoding undecoded.
Additional review concern: extension loading stays enabled after the call
([code](../../../../src/Database.cpp#L216)); the older fallback also enables SQL extension loading.
This is not a demonstrated exploit. Database destruction while Statements/Columns remain alive is
caller misuse under the [lifetime contract](../../../../include/SQLiteCpp/Database.h#L253).

### Statement

I5/C5: Foundational prepared execution and result access, also used by scalar queries, savepoint
quoting, and binding/batching helpers. Typed bindings, reset/step state, shared statement ownership,
borrowed database ownership, metadata caches, expansion allocation, and iteration interact
([implementation](../../../../src/Statement.cpp#L34)).
T4: Extensive binding/error/state/metadata/iterator tests
([tests](../../../../tests/Statement_test.cpp#L27)); schema reprepare, allocation failures,
move assignment, and actual contention recovery remain gaps.
E4: Preparation, binding, reset, named results, iteration, and blobs appear in
[example1](../../../../examples/example1/main.cpp#L163).
R5: Reproduced stale column count after automatic schema reprepare. The count is set at
[construction](../../../../src/Statement.cpp#L39); stepping does not refresh count/name caches
([step](../../../../src/Statement.cpp#L222), [name cache](../../../../src/Statement.cpp#L297)).
Observed exception-safety gap: [getExpandedSQL](../../../../src/Statement.cpp#L370) frees the SQLite
allocation only after potentially throwing string construction; no allocation-failure probe ran.
Older no-copy warnings say only "while executing"
([header](../../../../include/SQLiteCpp/Statement.h#L177)); SQLite requires the buffer until rebind
or finalization ([bundled contract](../../../../sqlite3/sqlite3.h#L4946)).
Negative blob lengths and extreme SQL-length narrowing deserve input-contract review
([binding](../../../../src/Statement.cpp#L129), [preparation](../../../../src/Statement.cpp#L425)).
No-copy buffers released too early and saved Columns used after row advance are caller misuse;
[Statement explicitly warns](../../../../include/SQLiteCpp/Statement.h#L554) that Columns are not
snapshots. No memory exploit was demonstrated.

### Column

I5/C3: Standard result access across query examples and Database::execAndGet. Short conversion
forwarders still involve shared statement lifetime, borrowed pointers, implicit casts, byte
preservation, and streaming ([implementation](../../../../src/Column.cpp#L28)).
T4: Broad getters/types/null/UTF-8/UTF-16, embedded-NUL streaming, and retained-statement tests
([tests](../../../../tests/Column_test.cpp#L22),
[lifetime](../../../../tests/Column_test.cpp#L346)).
Gaps include NULL/empty getString/streaming, numerical extremes, direct invalid indexes, and
allocation failures. E4: Typed, named, streamed, and blob access in
[example1](../../../../examples/example1/main.cpp#L173).
R4: Observed documentation gap: [pointer warnings](../../../../include/SQLiteCpp/Column.h#L88)
mention finalization but omit step/reset/conversion invalidation described by
[SQLite](../../../../sqlite3/sqlite3.h#L5512).
The [streaming comment](../../../../include/SQLiteCpp/Column.h#L238) also says getText, while
[implementation](../../../../src/Column.cpp#L119) uses getString and preserves exact bytes.
Plausible concerns: allocation failure may resemble empty/NULL data, and NULL/empty results reach
std::string construction with a possibly null pointer ([code](../../../../src/Column.cpp#L90));
C++17 library portability needs further verification. Direct construction checks the statement
pointer but not index or row validity. Row advance/reset invalidation is documented caller misuse;
shared statement ownership does not retain the Database or preserve an earlier row.

### Transaction

I4/C3: Major atomicity service with small logic but database-wide state interactions
([implementation](../../../../src/Transaction.cpp#L22)).
T3: Direct tests cover commit, duplicate commit, rollback after commit, all behavior enums, invalid
enum, scope/error rollback, and explicit rollback
([tests](../../../../tests/Transaction_test.cpp#L21)); failed commit, actual locking behavior, and
replacement transactions are missing. E4: Commit and exception rollback with result verification
([examples](../../../../examples/example1/main.cpp#L356)).
R4: Reproduced an old rolled-back wrapper rolling back a newer transaction.
[rollback](../../../../src/Transaction.cpp#L80) does not mark the wrapper finished;
[destruction](../../../../src/Transaction.cpp#L50) still issues ROLLBACK.
The existing [double-rollback test](../../../../tests/Transaction_test.cpp#L96) has no intervening
transaction. Other concerns are failed commit/automatic rollback state and non-owning Database
lifetime. Cross-thread sharing violates the public contract.

### Backup

I3/C4: Optional backup API also implementing Database::backup; two connections, incremental steps,
retry statuses, completion/abandonment, and cleanup create substantial lifetime complexity
([implementation](../../../../src/Backup.cpp#L22)).
T3: Direct constructor, page-count/content, overload, full-copy, and read-only error tests
([tests](../../../../tests/Backup_test.cpp#L24)); BUSY/LOCKED retry, abort, and lifetime tests are
missing. Tests access the destination while Backup remains alive (lines 63, 93, 124), against the
[bundled init-to-finish restriction](../../../../sqlite3/sqlite3.h#L9716).
E1: No relevant executable example found. R4: The public header omits this destination-use
restriction and connection-lifetime details. Both connections must outlive the handle.
[executeStep](../../../../src/Backup.cpp#L52) correctly returns retryable statuses; false success
belongs to Database::backup. Test/contract mismatch is observed; no deadlock or exploit was
reproduced. Review abort/finalization behavior and clarify safe usage.

### Savepoint

I3/C4: Optional nested transaction API with SQL-name quoting, rollback/release transitions, parent
state, and scope cleanup ([implementation](../../../../src/Savepoint.cpp#L23)).
T3: Useful single-savepoint release/double-release, rollback, post-rollback changes, automatic
cleanup, and externally released cleanup tests
([tests](../../../../tests/Savepoint_test.cpp#L21)); actual nesting, repeated/quoted/NUL names,
parent invalidation, and failed release are missing. E1: No relevant example found.
R4: Nested state and name identity deserve close review. Names are bound through SELECT quote(?),
so ordinary quotes are escaped; no SQL injection was established. Embedded NUL truncation in
[SQLite quote](../../../../sqlite3/sqlite3.c#L135196) may produce unexpected name collisions;
no name-edge probe ran. Parent operations changing child validity are already
[documented](../../../../include/SQLiteCpp/Savepoint.h#L37); thread sharing and an invalid Database
lifetime are caller misuse. No confirmed runtime defect was found in this group.

### Assertion

I3/C2: Common destructor diagnostics, with a small macro tree depending on build flags and user
callbacks ([header](../../../../include/SQLiteCpp/Assertion.h#L15)).
T2: Indirect configured Debug/Release checks
([CI](../../../../.github/workflows/cmake.yml#L9)); the
[test handler](../../../../tests/Database_test.cpp#L29) leaves invocation testing TODO.
E4: Both examples define aborting handlers
([example1](../../../../examples/example1/main.cpp#L22),
[example2](../../../../examples/example2/src/main.cpp#L20)); example1 also calls assertions.
R4: Observed custom-macro control-flow defect: its
[unwrapped if](../../../../include/SQLiteCpp/Assertion.h#L36) captures a caller's following else.
Current Database usage is standalone and safe. Adjacent build defect at this reviewed master:
[Meson](../../../../meson.build#L101) defines SQLITE_ENABLE_ASSERT_HANDLER, while the
[header](../../../../include/SQLiteCpp/Assertion.h#L22) checks SQLITECPP_ENABLE_ASSERT_HANDLER.
[CMake](../../../../CMakeLists.txt#L225) maps this correctly. These findings were inspected, not
runtime-tested here. Release-mode removal of ordinary assert is documented; throwing from a
callback during destruction violates the no-throw contract. Shared-library callback linkage was
not verified. The prior working branch's Meson fixes are outside this master-based snapshot.

### Exception

I5/C2: Foundational throwing API across Database, Statement, and resource wrappers; constructor
forwarding, copied messages, and scalar codes keep logic bounded
([implementation](../../../../src/Exception.cpp#L18)).
T3: Direct copy/assignment/base-catch/string/null-message/code tests
([tests](../../../../tests/Exception_test.cpp#L18)); handle-backed exceptions occur indirectly,
but their primary/extended code and message snapshots are not directly asserted.
E2: Examples catch std::exception and print what()
([example1](../../../../examples/example1/main.cpp#L370)); no SQLite-specific diagnostic example.
R2: No owned SQLite resource or confirmed defect. The constructor combining a supplied return
code with connection-derived message/extended code
([code](../../../../src/Exception.cpp#L32)) deserves consistency tests. Dangling external SQLite
handles are a caller lifetime concern.

### SQLiteCppExport

I3/C2: Common ABI support for all public classes and exported constants/functions; small platform
and compiler macro tree ([header](../../../../include/SQLiteCpp/SQLiteCppExport.h#L20)).
T3: Indirect configured MSVC/MinGW/Linux/macOS static/shared build checks
([CI](../../../../.github/workflows/cmake.yml#L50)); no isolated macro tests.
Meson tests use a static variant even for a shared main library
([build](../../../../meson.build#L230)). E2: Examples consume exports indirectly.
R3: Observed documentation mismatch: line 17 names SQLITECPP_EXPORT, while line 22 actually
checks SQLITECPP_DLL_EXPORT. _WIN32 versus WIN32 and the GNU Windows visibility path are plausible
portability concerns, not demonstrated linkage failures. CMake/Meson separate import/export flags
([CMake](../../../../CMakeLists.txt#L245), [Meson](../../../../meson.build#L202)).
No confirmed vulnerability was found.

### ExecuteMany

I2/C3: Optional batching helper used primarily in its tests; variadic forwarding, Statement reuse,
reset/clear, and result draining interact
([header](../../../../include/SQLiteCpp/ExecuteMany.h#L44)).
T3: Direct scalar/tuple and increasing/decreasing arity checks
([tests](../../../../tests/ExecuteMany_test.cpp#L21)); decreasing arity explicitly verifies NULL
rather than stale bindings. Partial-progress/error and result-query cases are missing.
E2: A [comment example](../../../../include/SQLiteCpp/ExecuteMany.h#L31), no executable example.
R3: A later failure can leave earlier writes committed without a caller transaction; atomicity is
not promised, so this is a contract/example gap rather than a confirmed bug. The helper drains and
discards result rows ([code](../../../../include/SQLiteCpp/ExecuteMany.h#L74)); SELECT/RETURNING
behavior needs clarification and tests. No stale-binding defect remains in the inspected code.

### SQLiteCpp

I3/C1: Common umbrella header and version macros, directly used by both examples and the consumer
([header](../../../../include/SQLiteCpp/SQLiteCpp.h#L20)).
T3: [Consumer smoke checks](../../../../tests/consumer/main.cpp#L7) and configured
[cross-standard CI](../../../../.github/workflows/cmake_standards.yml#L16) exercise its ordinary
API; completeness/version synchronization checks are absent.
E4: Multiple substantive consumer patterns in
[example1](../../../../examples/example1/main.cpp#L49) and
[example2](../../../../examples/example2/src/main.cpp#L40).
R2: Observed API-discovery gap: line 6 promises all functionality, but lines 21-27 omit Backup,
Savepoint, VariadicBind, and ExecuteMany. Version macros currently agree numerically with the
CMake project version. Manual synchronization is a maintenance concern, not a current mismatch.

### VariadicBind

I2/C3: Optional scalar/tuple helper used by ExecuteMany and example1; overload resolution and pack
expansion dominate complexity ([header](../../../../include/SQLiteCpp/VariadicBind.h#L43)).
T3: Direct scalar/tuple, missing/excess parameter, exception, and persisted-value checks
([tests](../../../../tests/VariadicBind_test.cpp#L37)); zero-argument/empty-tuple, broader types,
and decreasing arity are missing. E3: Substantive bind/execute/readback usage
([example1](../../../../examples/example1/main.cpp#L485)).
R2: Incremental binding leaves earlier bindings changed when a later bind throws; existing tests
explicitly exercise this, and no strong exception guarantee is promised. Rebinding fewer values
preserves omitted old values, matching [reset
contract](../../../../include/SQLiteCpp/Statement.h#L90). ExecuteMany clears them explicitly.
Observed documentation defect: [comment examples](../../../../include/SQLiteCpp/VariadicBind.h#L33)
construct Statement without the required Database argument. No runtime defect was established.

### Utils

I1/C2: Peripheral pure-function annotation with compiler/attribute guards and an empty fallback
([header](../../../../include/SQLiteCpp/Utils.h#L14)); its single observed consumer is
[Statement::getIndex](../../../../include/SQLiteCpp/Statement.h#L124), forwarding parameter lookup.
T2: Indirect [named-binding tests](../../../../tests/Statement_test.cpp#L489), no isolated macro or
optimization checks. E1: No annotation-specific example.
R2: No observed defect. Conservative omission loses optimization; future side effects in the
annotated method would require reassessing the pure contract.

## Reproduced issues and follow-up order

The top two groups tie at 37; Database sorts first alphabetically. Begin detailed review with
Database and Statement, then Column and Transaction. Transaction has a reproduced data-integrity
issue despite its smaller implementation. Backup/Savepoint need targeted lifetime/state tests.

Five behaviors were reproduced against the unchanged reviewed sources with GCC 13.3, C++17,
Debug, bundled SQLite, and column metadata enabled. Probes were isolated under
`/tmp/sqlitecpp-deep-review-cf33ad7`; no library or unit-test source was edited.

1. **Database backup false success:** Create an in-memory source table, execute BEGIN and INSERT,
   then call Backup::executeStep with another in-memory destination. It returns SQLITE_BUSY (5).
   Calling Database::backup to a new file with Database::Save on that source returns normally.
   Follow-up: require confirmed completion or expose an explicit retry/error outcome.
2. **Statement metadata after schema change:** Prepare SELECT * on a one-column table, execute,
   populate name lookup, reset, ALTER TABLE ADD COLUMN, then execute again. getColumnCount stays 1
   and getColumn(1) throws "Column index out of range". Follow-up: refresh metadata after reprepare;
   add rename/name-cache and added/removed-column tests.
3. **Transaction rollback ownership:** Within the first wrapper's scope, call first.rollback(),
   then execute BEGIN and INSERT using the same connection. Destroy the first wrapper before
   committing the newer transaction. Its destructor removes the newer insert; row count becomes 0.
   Follow-up: represent successful explicit rollback as a finished wrapper state.
4. **Header page-size sentinel:** Create a file with PRAGMA page_size=65536 before its first table.
   getHeaderInfo reports pageSizeBytes=1. Follow-up: decode the valid sentinel and add a test.
5. **Header magic validation:** Replace byte 15 of a valid header with X. getHeaderInfo still
   accepts it. Follow-up: validate all 16 bytes before terminating the returned display string.

Other findings above are static observations or plausible concerns. None was verified as an
exploitable security vulnerability. Fix design and regression tests belong to subsequent review.

## Validation and limitations

The build succeeded using:

```sh
cmake -S . -B /tmp/sqlitecpp-deep-review-cf33ad7 \
  -DCMAKE_BUILD_TYPE=Debug -DSQLITECPP_BUILD_TESTS=OFF \
  -DSQLITECPP_BUILD_EXAMPLES=OFF -DSQLITECPP_RUN_CPPLINT=OFF \
  -DSQLITECPP_RUN_CPPCHECK=OFF
cmake --build /tmp/sqlitecpp-deep-review-cf33ad7 -j 4
```

A temporary standalone C++17 probe linked libSQLiteCpp.a and the bundled sqlite3 library and
produced:

```text
backup raw status=5
Database::backup returned normally
post-alter cached column count=1
second column threw: Column index out of range.
rows after old Transaction destruction=0
decoded page size=1
bad magic byte accepted
```

Unit tests and examples were inspected, not executed. No instrumented coverage, sanitizers,
allocation-failure injection, Windows/macOS builds, Meson builds, or codec-backed runs were
performed. CI references establish configured checks, not successful executions in this review.
The probe results apply to this Linux Debug bundled-SQLite build; they do not establish identical
behavior for every configuration. Importance/complexity/risk and coverage quality remain review
judgments. Generated artifacts outside the enumerated source directories were not assessed.

At the original review, the skill validator, inventory completeness/uniqueness check, score/order
check, local evidence
link/line checks, ASCII/wrapping checks, and git diff whitespace checks passed before commit.
The skill description still covers the workflow and reference; no trigger change was needed.
