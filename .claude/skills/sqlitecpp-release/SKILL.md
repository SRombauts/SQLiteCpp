---
name: sqlitecpp-release
description: >-
  SQLiteCpp release procedure: bump the version across all files, finalize the CHANGELOG, open the
  Release PR, and tag. Use when cutting a new release, bumping the project version, or tagging.
---

# SQLiteCpp Release

How to cut a new SQLiteCpp release once the changes for it have landed on master. The per-PR
CHANGELOG and branching rules live in [[sqlitecpp-workflow]] and [[sqlitecpp-git-branching]].

## 1. Start a release branch
Branch from an up-to-date master: `release-X.Y.Z` (see [[sqlitecpp-git-branching]]).

## 2. Finalize the CHANGELOG
The entries should already be there from each PR (see [[sqlitecpp-workflow]]). For the release:
- Rename the heading from `Version X.Y.Z - <year> ???` to the real release date,
  e.g. `Version 3.4.0 - 2026 Jun 22`.
- Confirm every merged PR since the previous tag has a bullet, including any change committed
  directly to master.

## 3. Bump the version
Keep all of these in sync. The four-place version (e.g. `3.4.0`) and the zero-padded macro form
(e.g. `"3.04.00"` / `3004000`) must match.

- `CMakeLists.txt`: `project(SQLiteCpp VERSION X.Y.Z)`
- `Doxyfile`: `PROJECT_NUMBER = X.Y.Z`
- `package.xml`: `<version>X.Y.Z</version>`
- `meson.build`: `version: 'X.Y.Z',`
- `include/SQLiteCpp/SQLiteCpp.h`:
  - `#define SQLITECPP_VERSION "X.0Y.0Z"` (each component zero-padded to two digits)
  - `#define SQLITECPP_VERSION_NUMBER XYYZZ` (e.g. `3.4.0` -> `3004000`)

The header carries a `WARNING: shall always be updated in sync with PROJECT_VERSION` comment. Honor
it: a mismatch between the header macros and `CMakeLists.txt` is the most common release mistake.

Commit as `Update project version to X.Y.Z`.

## 4. Open the Release PR
Open a PR titled `Release X.Y.Z`. GitHub's "Generate release notes" lists the merged PRs and new
contributors; that summary is for the GitHub Release body, while `CHANGELOG.md` stays the
hand-curated record.

## 5. Tag after merge
After the Release PR merges, create the lightweight tag `X.Y.Z` (no `v` prefix, matching existing
tags) on the merge commit and publish the GitHub Release with the generated notes.

## Version numbering
- Patch (`Z`) for fixes and build tweaks only.
- Minor (`Y`) when adding features, public API, or CMake/Meson options, or for a non-trivial SQLite
  bump.
- Major (`X`) for breaking API changes or a raised C++/CMake baseline.
