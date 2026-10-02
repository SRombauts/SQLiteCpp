# Bug report reproduction probes

These probes were revalidated with GCC 13.3 on Linux against library source revision
`15f2a5a714e87b2b616b59cf6462dc21edc0ee6e`. The original B1-B8 validation used
`0074d708370da25e3d18867a58eba5bc28c8824c`; library sources are identical between those revisions.
Run from a repository root containing both the probes and that equivalent library source; the
report update does not change the library, build configuration, or existing unit tests.
These standalone C++17 programs reproduce reported defects; they are not regression tests and
must not be interpreted as passing correctness checks merely because they exit successfully.
They create/remove only their named scratch databases below the supplied work directory.
Use the dedicated scratch path shown here, not a directory containing useful databases.

- [findings_probe.cpp](findings_probe.cpp): B1-B4, B6-B7, the automatic-rollback ownership variant,
  and the failed-COMMIT retry behavior that a B2 fix must preserve.
- [assertion_probe.cpp](assertion_probe.cpp): B5 with a custom assertion handler.
  The unbraced outer if/else is deliberate: it exercises the defective macro expansion.
- [expanded_sql_probe.cpp](expanded_sql_probe.cpp): B8 by failing the next C++ allocation after
  constructing the database/statement. SQLite's separate allocation succeeds before the injected
  string-construction failure. This isolated program overrides global new/delete intentionally.
- [column_allocation_probe.cpp](column_allocation_probe.cpp): B9, deterministic SQLite allocation
  failure during UTF-16-to-UTF-8 conversion. The forwarding allocator and disabled lookaside are
  deliberate; allocation failure is armed only after successful row stepping.
- [extension_error_probe.cpp](extension_error_probe.cpp): B10, missing extension diagnostics.
- [filesystem_disable_probe.cpp](filesystem_disable_probe.cpp): B11, intentional compile failure
  when the experimental-filesystem enable macro overrides disabling.
- [iterator_postfix_probe.cpp](iterator_postfix_probe.cpp) and
  [iterator_arrow_probe.cpp](iterator_arrow_probe.cpp): B12, intentional compile failures for
  advertised C++17 input-iterator operations.
- [savepoint_lifecycle_probe.cpp](savepoint_lifecycle_probe.cpp): B13, parent-invalidated children
  modifying replacement savepoints; also checks quote escaping and NUL name aliasing.
- [savepoint_failed_release_probe.cpp](savepoint_failed_release_probe.cpp): a successful retry
  after failed deferred-constraint release, which a B13 fix must preserve.
- [assertion_expression_probe.cpp](assertion_expression_probe.cpp): B14, changed assignment
  semantics, plus B5/evaluation behavior across default/custom and NDEBUG modes.
- [statement_metadata_probe.cpp](statement_metadata_probe.cpp): B4, reordering, adding and removing
  result columns. Obsolete-index reads illustrate stale validation, not proven memory corruption.
- [getindex_pure_probe.cpp](getindex_pure_probe.cpp): B15, optimized callers discard the throwing
  name lookup when its public declaration is incorrectly marked pure.

## Build and baseline tests

```sh
review_build=/tmp/sqlitecpp-deep-review-15f2a5a
review_probes=.claude/skills/deep-code-review/references/review-probes
cmake -S . -B "$review_build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_STANDARD=17 \
  -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_BUILD_EXAMPLES=OFF \
  -DSQLITECPP_RUN_CPPLINT=OFF -DSQLITECPP_RUN_CPPCHECK=OFF -DSQLITECPP_USE_ASAN=ON
cmake --build "$review_build" -j 4
ctest --test-dir "$review_build" --output-on-failure
```

Observed: 69 tests from 10 suites passed (one CTest UnitTests entry).

## Main findings probe

```sh
c++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer \
  -DSQLITE_ENABLE_COLUMN_METADATA -I include -I sqlite3 \
  "$review_probes/findings_probe.cpp" "$review_build/libSQLiteCpp.a" \
  "$review_build/sqlite3/libsqlite3.a" -lpthread -ldl \
  -o "$review_build/findings_probe"
"$review_build/findings_probe" "$review_build/probe-data"
```

Observed, exit 0 with no sanitizer diagnostic:

```text
missing_load created=1 tables=0
missing_load persisted_tables=0
backup raw_status=5
backup returned tables=0
backup existing_old_row=123 new_table=0
locked_load old_row=123 new_table=0
replacement_transaction rows=0
replacement_transaction commit=cannot commit - no transaction is active
old_wrapper_commit autocommit=1 rows=1
automatic_rollback successor_rows=1
failed_commit still_active=1
failed_commit retry_rows=1
metadata cached=1 actual=2
metadata added_column=Column index out of range.
metadata name_a_value=9 actual_first_name=b
header page_size=1
header bad_magic_accepted=1
```

The automatic-rollback case starts with one committed row and inserts a second in its successor.
A count of 1 demonstrates the successor's insert was lost. Metadata values in the rebuilt table
are `b=9, a=7`, so returning 9 for name `a` is incorrect.

## Assertion probe

```sh
c++ -std=c++17 -DSQLITECPP_ENABLE_ASSERT_HANDLER -I include \
  "$review_probes/assertion_probe.cpp" -o "$review_build/assertion_probe"
"$review_build/assertion_probe"
```

Observed: `assertion outer_else_reached=0` (exit 0). Correct outer-if behavior would reach else.

## Expanded-SQL allocation probe

```sh
c++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer \
  -DSQLITE_ENABLE_COLUMN_METADATA -I include -I sqlite3 \
  "$review_probes/expanded_sql_probe.cpp" "$review_build/libSQLiteCpp.a" \
  "$review_build/sqlite3/libsqlite3.a" -lpthread -ldl \
  -o "$review_build/expanded_sql_probe"
"$review_build/expanded_sql_probe"
```

Observed: `caught bad_alloc; retained SQLite bytes=80`, followed by a LeakSanitizer failure
(exit 1): a direct leak of 88 bytes allocated through `sqlite3_expanded_sql`, originating at
`src/Statement.cpp:370`. SQLite's memory counter excludes its allocator header; LeakSanitizer
includes that overhead. Exact allocation sizes depend on the platform and input string.

No default assertion-handler build was changed, and these probes do not exercise every compiler,
SQLite configuration, error path, or platform.

## Additional full-review probes

Use the build and `review_probes` variables above. Build the runtime probes with the same immutable
ASAN archives used by the coordinator:

```sh
for probe in column_allocation extension_error savepoint_lifecycle savepoint_failed_release \
  statement_metadata; do
  c++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer \
    -DSQLITE_ENABLE_COLUMN_METADATA -I include -I sqlite3 \
    "$review_probes/${probe}_probe.cpp" "$review_build/libSQLiteCpp.a" \
    "$review_build/sqlite3/libsqlite3.a" -lpthread -ldl -o "$review_build/${probe}_probe"
done
"$review_build/column_allocation_probe" 0
"$review_build/column_allocation_probe" 1
"$review_build/extension_error_probe"
"$review_build/savepoint_lifecycle_probe"
"$review_build/savepoint_failed_release_probe"
"$review_build/statement_metadata_probe"
```

Coordinator observations, all without sanitizer diagnostics:

```text
column control: failedCalls=0 threw=0 resultSize=4000 equalsExpected=1
column failure: failedCalls=1 threw=0 resultSize=8000 equalsExpected=0 sqliteError=7
fresh exception=not an error code=1 extended=0
raw code=1 error=<missing library path>: cannot open shared object file: No such file or directory
parent_release successor_rows=0 autocommit=1
successor_release_error=no such savepoint: child
parent_rollback successor_rows=0
old_release successor_rows=1 autocommit=1
quoted_name_table_exists=1
nul_name_aliases_prefix=1
failed_release=FOREIGN KEY constraint failed
retry_rows=1
```

The column control and failure outputs also print mode and expected size; irrelevant allocator
counts or loader path text can vary. The `zeroblob` optional mode did not force an allocation and
does not prove OOM behavior. The metadata probe additionally demonstrates unchanged/reduced cached
counts and obsolete name/index acceptance after dropping a column.

Compile-only probes intentionally fail. Run each separately and inspect diagnostics:

```sh
c++ -std=c++17 -I include -fsyntax-only "$review_probes/iterator_postfix_probe.cpp"
c++ -std=c++17 -I include -fsyntax-only "$review_probes/iterator_arrow_probe.cpp"
c++ -std=c++17 -DSQLITECPP_HAVE_STD_EXPERIMENTAL_FILESYSTEM \
  -DSQLITECPP_DISABLE_STD_FILESYSTEM -I include -fsyntax-only \
  "$review_probes/filesystem_disable_probe.cpp"
```

Observed: postfix reports `void value not ignored`; arrow reports a non-pointer operand;
filesystem reports the intentional `#error`. GCC 13 also reports a conflicting experimental
namespace alias. A disable-only build of that probe succeeds.

Build and run the assertion expression probe separately for each configuration:

```sh
c++ -std=c++17 -I include "$review_probes/assertion_expression_probe.cpp" \
  -o "$review_build/assertion_expression_probe"
"$review_build/assertion_expression_probe"
```

Repeat adding `-DSQLITECPP_ENABLE_ASSERT_HANDLER`, `-DNDEBUG`, and both definitions.
Default: `actualAssignment=1 standardAssignment=7 outerElse=1`.
Custom: `actualAssignment=7 standardAssignment=7 outerElse=0`.
NDEBUG/default evaluates neither expression nor message; NDEBUG/custom remains active, as designed.

## Optimized getIndex attribute probe

Compile against the same library, changing only the consumer's attribute definition:

```sh
c++ -std=c++17 -O2 -g -fsanitize=address -fno-omit-frame-pointer \
  -DSQLITE_ENABLE_COLUMN_METADATA -I include -I sqlite3 \
  "$review_probes/getindex_pure_probe.cpp" "$review_build/libSQLiteCpp.a" \
  "$review_build/sqlite3/libsqlite3.a" -lpthread -ldl -o "$review_build/getindex_pure_probe"
"$review_build/getindex_pure_probe"
c++ -std=c++17 -O2 -g -fsanitize=address -fno-omit-frame-pointer \
  -DSQLITECPP_PURE_FUNC= -DSQLITE_ENABLE_COLUMN_METADATA -I include -I sqlite3 \
  "$review_probes/getindex_pure_probe.cpp" "$review_build/libSQLiteCpp.a" \
  "$review_build/sqlite3/libsqlite3.a" -lpthread -ldl -o "$review_build/getindex_pure_disabled"
"$review_build/getindex_pure_disabled"
```

Default output includes `discard caught=0`. Empty-attribute output includes
`discard exception=Statement was not prepared.` and `discard caught=1`. Both report the used-result
exception and `valid first=1 second=2`. Reviewer and coordinator independently reproduced these GCC
results without ASAN diagnostics; the reviewer also reproduced the default suppression with Clang.
