---
name: sqlitecpp-testing-practices
description: GoogleTest patterns and testing practices for SQLiteCpp test coverage and structure.
---

# SQLiteCpp Testing Practices

## Scope
- Uses GoogleTest (system if found, otherwise `googletest/` fallback).
- Test files live in `tests/*_test.cpp` (see `tests/Database_test.cpp`).
- File headers use `@ingroup tests`.

## Test File Structure
```cpp
#include <gtest/gtest.h>
#include <SQLiteCpp/Database.h>

TEST(DatabaseTest, CanOpenReadWriteDatabase)
{
    // Arrange: setup test database
    SQLite::Database db(":memory:", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    // Act: perform operation
    db.exec("CREATE TABLE test (id INTEGER PRIMARY KEY)");

    // Assert: verify behavior
    EXPECT_EQ(1, db.exec("INSERT INTO test DEFAULT VALUES"));
}
```

## Adding a New Test File
- Create `tests/NewClass_test.cpp`.
- Register it in `CMakeLists.txt` under `SQLITECPP_TESTS`.
- Register it in `meson.build` under `sqlitecpp_test_srcs`.

## Common Patterns
- Prefer `:memory:` databases when possible.
- Use file-based databases when testing file I/O behavior (see `tests/Database_test.cpp`).
- Use `TEST()` for independent tests; use `TEST_F()` only when a shared fixture is needed.

## Running Tests
```bash
# CMake (from build dir)
ctest --output-on-failure
bin/SQLiteCpp_tests --gtest_filter=Database.*

# Meson
meson test -C builddir
meson test -C builddir --test-args="--gtest_filter=Database.*"
```

## Canonical References
- Test style: `tests/Database_test.cpp`
- CMake test list: `CMakeLists.txt` (`SQLITECPP_TESTS`)
- Meson test list: `meson.build` (`sqlitecpp_test_srcs`)

## Cross-References
- Workflow checklist: `sqlitecpp-workflow`
- Build configuration: `sqlitecpp-build-cmake`, `sqlitecpp-build-meson`
