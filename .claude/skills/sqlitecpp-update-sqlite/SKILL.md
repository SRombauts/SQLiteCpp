---
name: sqlitecpp-update-sqlite
description: >-
  How to update the bundled SQLite3 amalgamation (sqlite3/sqlite3.c and sqlite3.h), the Meson
  wrap, README.md, and CHANGELOG.md. Use when upgrading SQLite, refreshing the vendored
  amalgamation, or bumping the sqlite3 wrap.
---

# Updating the bundled SQLite3

SQLiteCpp ships a copy of the SQLite3 amalgamation under `sqlite3/` so the CMake build works with
no system dependency. The Meson build instead pulls SQLite from wrapdb via a wrap file. An upgrade
touches both.

Do this on a dedicated branch named `update-sqlite-X.Y.Z` (for example `update-sqlite-3.52.2`),
matching the SQLite version you are moving to.

## 1. Download the amalgamation
Get `sqlite-amalgamation-NNNNNNN.zip` from the [SQLite download page](https://www.sqlite.org/download.html).
`NNNNNNN` encodes the version with two digits per component: `3.51.3` is `3510300`.

## 2. Replace the vendored files
Extract and overwrite, verbatim, with the files from the zip:
- `sqlite3/sqlite3.c`
- `sqlite3/sqlite3.h`

Do not hand-edit them. Build-time feature flags (`SQLITE_OMIT_LOAD_EXTENSION`,
`SQLITE_ENABLE_COLUMN_METADATA`, and the rest in [[sqlitecpp-sqlite-flags]]) are passed as compile
definitions, never by patching the amalgamation.

## 3. Update `sqlite3/README.md`
- The amalgamation line: filename, version, and release date, e.g.
  `"sqlite3.c" and "sqlite3.h" files from sqlite-amalgamation-3510300.zip (SQLite 3.51.3 2026-03-13)`
- The copyright year, if the year has rolled over.

## 4. Update the copyright year
Bump the year in `sqlite3/CMakeLists.txt` if it changed (it tracks the same `2012-<year>` range).

## 5. Update the Meson wrap (separate concern)
`subprojects/sqlite3.wrap` points at a wrapdb release and can lag behind the CMake amalgamation
version. Bump it to the matching `sqlite3_X.Y.Z-N` package (the `directory`, `source_url`,
`source_filename`, `source_hash`, `patch_*`, and `wrapdb_version` fields). This is often a separate
PR from the amalgamation drop.

## 6. Build, test, and record
- Build and run the tests on both CMake and Meson (see [[sqlitecpp-build-cmake]] and
  [[sqlitecpp-build-meson]]).
- Update `CHANGELOG.md` following [[sqlitecpp-workflow]] conventions:
  Add one line (or update the existing one) under the current unreleased version heading:
  `- Update SQLite from A.B.C to X.Y.Z (YYYY-MM-DD) (#NNN)`.

The SQLite version bump is independent of the SQLiteCpp version. Bumping the SQLiteCpp version and
tagging are the release process: see [[sqlitecpp-release]].
