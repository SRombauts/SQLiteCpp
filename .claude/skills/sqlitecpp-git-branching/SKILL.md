---
name: sqlitecpp-git-branching
description: SQLiteCpp branch naming and creation rules for starting tasks, issues, or features.
---

# SQLiteCpp Git Branching

## When to create a branch
- User mentions working on a task, issue, or feature.
- User references a GitHub issue number.

## Before starting work
1. Run `git status` to check current branch.
2. If on `master` or wrong branch, create a task-specific branch from `master`.

Create the branch before the first edit. If edits were accidentally made on `master`, create the task
branch with the working tree intact before continuing.

## Branch naming
- With issue: `<issue>-<type>-<short-description>`
  - `123-fix-short-description`
  - `123-feature-short-description`
- Without issue: `<type>-<short-description>`
  - `fix-short-description`
  - `feature-short-description`
- Issue ID is optional; do not use a `000-` prefix.

## Maintenance branch conventions
- Updating the bundled SQLite3: `update-sqlite-X.Y.Z` (e.g. `update-sqlite-3.52.2`).
  See [[sqlitecpp-update-sqlite]].
- Cutting a release: `release-X.Y.Z`. See [[sqlitecpp-release]].

## Branch-only requests
- If the user only requests a branch, create it and stop (no file changes).
