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

## Branch naming
- With issue: `<issue>-<type>-<short-description>`
  - `123-fix-short-description`
  - `123-feature-short-description`
- Without issue: `<type>-<short-description>`
  - `fix-short-description`
  - `feature-short-description`
- Issue ID is optional; do not use a `000-` prefix.

## Branch-only requests
- If the user only requests a branch, create it and stop (no file changes).
