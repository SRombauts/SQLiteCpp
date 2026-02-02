---
name: sqlitecpp-build-meson
description: Builds SQLiteCpp with Meson. Use when configuring Meson builds, tests, or Meson options.
---

# SQLiteCpp Meson Build

## Quick builds
- Default build:
  - `meson setup builddir`
  - `meson compile -C builddir`
- With tests and examples:
  - `meson setup builddir -DSQLITECPP_BUILD_TESTS=true -DSQLITECPP_BUILD_EXAMPLES=true`
  - `meson compile -C builddir`
  - `meson test -C builddir`

## CI-style setup (GitHub Actions)
- `meson setup builddir -DSQLITECPP_BUILD_TESTS=true -DSQLITECPP_BUILD_EXAMPLES=true --force-fallback-for=sqlite3`
- `meson compile -C builddir`
- `meson test -C builddir`

## Options
- `SQLITECPP_BUILD_TESTS`: build unit tests.
- `SQLITECPP_BUILD_EXAMPLES`: build examples.
- `SQLITE_ENABLE_COLUMN_METADATA`: enable column origin metadata.
- `SQLITE_ENABLE_ASSERT_HANDLER`: enable assert handler.
- `SQLITE_HAS_CODEC`: enable codec API (requires SQLCipher).
- `SQLITE_USE_LEGACY_STRUCT`: legacy `sqlite3_value` struct.
- `SQLITECPP_DISABLE_STD_FILESYSTEM`: disable std::filesystem.
- `SQLITECPP_DISABLE_SQLITE3_EXPANDED_SQL`: disable sqlite3_expanded_sql.
- `SQLITE_OMIT_LOAD_EXTENSION`: omit load extension.

## Notes
- Meson defaults to `cpp_std=c++17` to support newer gtest versions.
- On Windows, Meson enforces at least C++14 if needed by headers.
