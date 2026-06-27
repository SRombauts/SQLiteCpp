---
name: sqlitecpp-coding-standards
description: SQLiteCpp coding standards and API rules for core edits, public headers, style, and Doxygen.
---

# SQLiteCpp Coding Standards

## Non-negotiables
- RAII only: acquire in constructors, release in destructors.
- Never throw in destructors; use `SQLITECPP_ASSERT()` instead.
- C++11 only in core library (C++14 only in `VariadicBind.h` and `ExecuteMany.h`).
- Public API headers must not include `sqlite3.h`.
- Public API must use `SQLITECPP_API` from `SQLiteCppExport.h`.
- One `Database`/`Statement`/`Column` per thread.

## Error handling
- Throw `SQLite::Exception` for errors in throwing APIs.
- Use `tryExec()`, `tryExecuteStep()`, `tryReset()` for error codes.

## Documentation and style
- Doxygen required for public API (`@brief`, `@param`, `@return`, `@throw`).
- ASCII only, 4 spaces, Allman braces, max 120 chars, LF line endings, final newline.
- LF line endings are enforced repo-wide by `.gitattributes` (`* text=auto eol=lf`), matching
  `.editorconfig` (`end_of_line = lf`); never commit CRLF. Run `git add --renormalize .` if a file drifts.
- Use `#pragma once` in headers.

## Naming conventions
- Types: PascalCase (`Database`, `Statement`).
- Functions/vars: camelCase (`executeStep()`, `getColumn()`).
- Members: `m` prefix (`mDatabase`).
- Args: `a` prefix (`aDatabase`).
- Booleans: `b`/`mb` prefix (`bExists`, `mbDone`).
- Pointers: `p`/`mp` prefix (`pValue`, `mpSQLite`).
