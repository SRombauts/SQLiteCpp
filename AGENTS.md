# SQLiteCpp AI Agents Custom Instructions

## Project Overview
- SQLiteCpp is a C++11 RAII wrapper around SQLite3 C APIs
- Minimal dependencies: C++11 STL + SQLite3
- Cross-platform: Windows, Linux, macOS
- Thread-safe at SQLite multi-thread level

## Maintaining This Documentation

**IMPORTANT:** When the user explains how to do something, corrects your behavior, or points out errors:
1. **Update AGENTS.md** for universal SQLiteCpp constraints and best practices, or to correct guidance in this file.
2. **Update skills** for task-specific guidance (build, testing, documentation, etc.), or to correct and extend
   skill content.

**Goal:** Keep this documentation accurate and comprehensive so future sessions benefit from corrections and
clarifications.

## Core Non-Negotiables (MUST Follow)

1. **RAII only**: Acquire resources in constructors, release in destructors
2. **Never throw in destructors**: Use `SQLITECPP_ASSERT()` instead
3. **C++11 core library**: C++14 only in VariadicBind.h and ExecuteMany.h
4. **Public API isolation**: Headers must NOT include sqlite3.h
5. **Export macros**: Public API must use `SQLITECPP_API` from SQLiteCppExport.h
6. **Threading constraint**: One Database/Statement/Column per thread
7. **Tests required**: New functionality must have tests in tests/
8. **Portability**: Code must work on Windows, Linux, and macOS
9. **Const correctness**: Use `const` for methods that don't modify object state, `const&` for read-only parameters

## Error Handling
- Throw `SQLite::Exception` for errors in throwing APIs
- Use `tryExec()`, `tryExecuteStep()`, `tryReset()` for error codes
- In destructors: use `SQLITECPP_ASSERT()` never throw

## Code Style
- 4 spaces (no tabs)
- Allman braces style
- LF line endings (Unix style)
- Final newline at end of file
- `#pragma once` in headers

## Naming Conventions

| Element | Convention | Example |
|---------|------------|---------|
| Types | PascalCase | `Database`, `Statement`, `TransactionBehavior` |
| Functions/vars | camelCase | `executeStep()`, `getColumn()`, `getErrorCode()` |
| Member variables | `m` prefix | `mDatabase`, `mQuery`, `mStmt` |
| Function arguments | `a` prefix | `aDatabase`, `aQuery`, `aFilename` |
| Boolean variables | `b`/`mb` prefix | `bExists`, `mbDone`, `mbReadOnly` |
| Pointer variables | `p`/`mp` prefix | `pValue`, `mpSQLite`, `mpStmt` |
| Constants | ALL_CAPS | `OPEN_READONLY`, `SQLITE_OK` |

## Documentation Requirements
- Doxygen required for all public API
- ASCII only in code and comments
- Max 120 characters per line

## Repository Structure
```
include/SQLiteCpp/    # Public headers
src/                  # Implementation files
tests/                # Unit tests (*_test.cpp)
sqlite3/              # Bundled SQLite3 source
examples/             # Example applications
```

## Key Classes
- `Database`: Connection management and database operations
- `Statement`: Prepared statement execution
- `Column`: Result column access
- `Exception`: Error handling

## Use skills for task guidance
Skills live under `.claude/skills/`. Load the relevant skill(s) based on the task:
- `sqlitecpp-coding-standards`: core library edits, public API rules, style and naming.
- `sqlitecpp-workflow`: add methods/classes, tests, build file updates, changelog.
- `sqlitecpp-git-branching`: branch creation and naming rules.
- `sqlitecpp-build-cmake`: CMake builds, options, tests.
- `sqlitecpp-build-meson`: Meson builds, options, tests.
- `sqlitecpp-ci-workflows`: CI config updates and build matrices.
- `sqlitecpp-doxygen-guide`: Doxygen standards and public API documentation.
- `sqlitecpp-testing-practices`: GoogleTest patterns and test structure.
- `sqlitecpp-troubleshooting`: compiler/linker/build issues.
- `sqlitecpp-sqlite-flags`: SQLite/SQLiteCpp feature flags.
- `sqlitecpp-update-sqlite`: updating the bundled SQLite3 amalgamation and the Meson wrap.
- `sqlitecpp-release`: how to release (version bump, CHANGELOG finalization, tagging).
- `windows-shell-commands`: Windows PowerShell vs Bash tool usage and safe multi-line arguments
  (commit messages, PR bodies); use before running git/gh with multi-line or quoted text.
- `humanizer`: Remove signs of AI-generated writing from text.
