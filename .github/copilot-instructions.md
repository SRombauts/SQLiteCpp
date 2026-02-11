# SQLiteCpp LLM Coding Guide

## Non-Negotiables (MUST Follow)
- RAII only. Acquire in constructors, release in destructors.
- NEVER throw in destructors. Use `SQLITECPP_ASSERT()` instead.
- Errors: use `SQLite::Exception` for throwing APIs; `tryExec()`, `tryExecuteStep()`, `tryReset()` return SQLite codes.
- C++11 only in core library. C++14 only in `VariadicBind.h` and `ExecuteMany.h`.
- Public API headers must NOT include `sqlite3.h`. Use `SQLite::OPEN_*` flags from `Database.h`.
- Export public API with `SQLITECPP_API` from `SQLiteCppExport.h`.
- Threading: one `Database`/`Statement`/`Column` per thread.
- Public API must have Doxygen: `@brief`, `@param`, `@return`, `@throw`.
- Tests required for new functionality in `tests/`.
- Portability: Windows, Linux, macOS.
- Style: ASCII only, 4 spaces, Allman braces, max 120 chars, LF line endings, final newline, `#pragma once`.

## Workflow (Common Tasks)
### Add a Method
1. Declare in `include/SQLiteCpp/<Class>.h` with Doxygen.
2. Implement in `src/<Class>.cpp`.
3. Add tests in `tests/<Class>_test.cpp`.
4. Update `CHANGELOG.md`.

### Add a Class
1. Create `include/SQLiteCpp/NewClass.h` and `src/NewClass.cpp`.
2. Add to `CMakeLists.txt` (`SQLITECPP_SRC` and `SQLITECPP_INC`).
3. Add to `meson.build`.
4. Include in `SQLiteCpp.h` if public API.
5. Create `tests/NewClass_test.cpp`.
6. Add test to `CMakeLists.txt` and `meson.build`.

### Git Workflow (GitHub Issues)
**When to create a new branch:**
- User explicitly mentions working on a task, issue, or feature (e.g., "work on issue #123", "implement feature X")
- User references a GitHub issue number

**Before starting work:**
1. Run `git status` to check current branch.
2. If on `master` or wrong branch, create a task-specific branch from `master`.

**Branch naming:** `<issue>-<type>-<short-description>`
- `123-fix-short-description` for bug fixes
- `123-feature-short-description` for new features

**Commits:**
- Imperative mood, ~50 char first line, body wrapped at 72 chars.
- Reference issue: `Closes #123` or `Fixes #123`.

```bash
git fetch origin
git checkout -b 123-fix-short-description origin/master
```

## Common Pitfalls
- `bind()` is 1-based; `getColumn()` is 0-based.
- Reusing `Statement` requires `reset()` and `clearBindings()`.
- `bindNoCopy()` only if data lifetime exceeds statement execution.
- `execAndGet()` returns a temporary `Column`, copy immediately.
- `exec()` returns changes for DML, 0 for DDL.

## Build and Test
### Quick Build Commands
**Windows (Visual Studio 2022):**
```batch
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_BUILD_EXAMPLES=ON ..
cmake --build . --config Release
```

**Windows (using build.bat):**
```batch
build.bat
```

**Unix/macOS:**
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_BUILD_EXAMPLES=ON ..
cmake --build .
```

**Meson:**
```bash
meson setup builddir -DSQLITECPP_BUILD_TESTS=true -DSQLITECPP_BUILD_EXAMPLES=true
meson compile -C builddir
```

### Essential CMake Options
| Option | Default | Description |
|--------|---------|-------------|
| `SQLITECPP_BUILD_TESTS` | OFF | Build unit tests |
| `SQLITECPP_BUILD_EXAMPLES` | OFF | Build examples |
| `SQLITECPP_INTERNAL_SQLITE` | ON | Use bundled sqlite3/ |
| `BUILD_SHARED_LIBS` | OFF | Build as DLL/shared library |
| `SQLITE_ENABLE_COLUMN_METADATA` | ON | Enable `getColumnOriginName()` |

### Running Tests
```bash
cd build
ctest --output-on-failure
ctest --output-on-failure -V
ctest --output-on-failure -R "Database"
```
```bash
meson test -C builddir
meson test -C builddir -v
```

### Style Checking
```bash
python cpplint.py src/*.cpp include/SQLiteCpp/*.h
```

## Troubleshooting
**MSVC:**
```
c:\path\to\file.cpp(42): error C2065: 'undeclaredVar': undeclared identifier
c:\path\to\file.cpp(50,15): error C2039: 'foo': is not a member of 'SQLite::Database'
```

**GCC/Clang:**
```
src/Database.cpp:42:5: error: use of undeclared identifier 'undeclaredVar'
include/SQLiteCpp/Database.h:100:10: error: no member named 'foo' in 'SQLite::Database'
```

**Linker:**
```
error LNK2019: unresolved external symbol "SQLite::Database::exec"
```

Usually means missing SQLiteCpp library or missing source file in build.

## Reference
### Project Snapshot
- SQLiteCpp is a lean C++11 RAII wrapper around SQLite3 C APIs.
- Minimal dependencies (C++11 STL + SQLite3).
- Cross-platform, thread-safe at SQLite multi-thread level.
- Keep naming close to SQLite.
- Public headers avoid `sqlite3.h`; use `SQLite::OPEN_*` flags from `Database.h`.

### Repository Structure
```
SQLiteCpp/
+-- include/SQLiteCpp/    # Public headers
|   +-- SQLiteCpp.h       # Umbrella include + version macros
|   +-- Database.h        # Connection + open flags
|   +-- Statement.h       # Prepared statements
|   +-- Column.h          # Result column access
|   +-- Transaction.h     # RAII transaction
|   +-- Savepoint.h       # RAII savepoint
|   +-- Backup.h          # Online backup
|   +-- Exception.h       # SQLite::Exception
|   +-- Assertion.h       # SQLITECPP_ASSERT macro
|   +-- VariadicBind.h    # Bind helper (C++11/14)
|   +-- ExecuteMany.h     # Batch execute (C++14)
|   +-- SQLiteCppExport.h # SQLITECPP_API macro
+-- src/                  # Implementations
+-- tests/                # Unit tests (*_test.cpp)
+-- sqlite3/              # Bundled SQLite3 source
+-- examples/             # Example applications
+-- CMakeLists.txt / meson.build / build.bat / build.sh
```

### Naming Rules
| Element | Convention | Example |
|---------|------------|---------|
| Types | PascalCase | `Database`, `Statement`, `TransactionBehavior` |
| Files | Named like contained class | `Database.h`, `Statement.cpp` |
| Functions and variables | camelCase | `executeStep()`, `getColumn()` |
| Member variables | Prefix `m` | `mDatabase`, `mQuery` |
| Function arguments | Prefix `a` | `aDatabase`, `aQuery` |
| Boolean variables | Prefix `b`/`mb` | `bExists`, `mbDone` |
| Pointer variables | Prefix `p`/`mp` | `pValue`, `mpSQLite` |
| Constants | ALL_CAPS | `OPEN_READONLY`, `SQLITE_OK` |

### File Header Template
```cpp
/**
 * @file    ClassName.h
 * @ingroup SQLiteCpp
 * @brief   Brief description of the file.
 *
 * Copyright (c) 2012-2025 Sebastien Rombauts (sebastien.rombauts@gmail.com)
 *
 * Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
 * or copy at http://opensource.org/licenses/MIT)
 */
#pragma once

#include <SQLiteCpp/SQLiteCppExport.h>
// other includes...
```

### Doxygen Format
```cpp
/**
 * @brief Execute a step of the prepared query to fetch one row of results.
 *
 * @param[in] aIndex    Index of the column (0-based)
 *
 * @return - true  (SQLITE_ROW)  if there is another row ready
 *         - false (SQLITE_DONE) if the query has finished executing
 *
 * @throw SQLite::Exception in case of error
 */
```

### Error Handling
- Use `SQLite::Exception` for errors (inherits `std::runtime_error`).
- Use `check(ret)` after SQLite C API calls.
- In destructors, use `SQLITECPP_ASSERT()` instead of throwing.

### Minimal API Example
```cpp
SQLite::Database db("example.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

SQLite::Statement query(db, "SELECT * FROM test WHERE id > ? AND name = :name");
query.bind(1, 5);
query.bind(":name", "John");
while (query.executeStep())
{
    int id = query.getColumn(0);
    std::string value = query.getColumn(1);
}

SQLite::Statement insert(db, "INSERT INTO test VALUES (?, ?)");
insert.bind(1, 42);
insert.bind(2, "value");
int changes = insert.exec();
```
