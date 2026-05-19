---
name: sqlitecpp-troubleshooting
description: Diagnose SQLiteCpp build, linker, compiler, and sqlite3 integration issues.
---

# SQLiteCpp Troubleshooting

## Common compiler errors
- MSVC: `error C2065` or `C2039` usually means a missing include or wrong symbol.
- GCC/Clang: `use of undeclared identifier` or `no member named` indicates missing includes or API mismatch.

## Common linker errors
- `LNK2019 unresolved external symbol "SQLite::Database::exec"`:
  - Missing SQLiteCpp library or missing source file in build.

## Missing sqlite3 on Linux
- Errors like `undefined reference to sqlite3_xxx`:
  - Install `libsqlite3-dev`, or enable internal sqlite3.
  - CMake option: `-DSQLITECPP_INTERNAL_SQLITE=ON`.

## Column metadata error
- `undefined reference to sqlite3_column_origin_name`:
  - Your sqlite3 library lacks `SQLITE_ENABLE_COLUMN_METADATA`.
  - Options:
    - Rebuild sqlite3 with that flag.
    - Disable `SQLITE_ENABLE_COLUMN_METADATA`.
    - Use internal sqlite3 (`SQLITECPP_INTERNAL_SQLITE=ON`).

## Destructors and assertions
- Do not throw in destructors; use `SQLITECPP_ASSERT()` instead.
