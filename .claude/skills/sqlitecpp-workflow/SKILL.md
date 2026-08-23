---
name: sqlitecpp-workflow
description: >-
  SQLiteCpp workflow for branches, implementation, tests, commits, pull requests, and CHANGELOG
  updates. Use when changing the repository.
---

# SQLiteCpp Workflow

## Required workflow
For an implementation request, complete the local Git workflow before handing the work back:

1. Load `sqlitecpp-git-branching`, inspect `git status`, and create the task branch before editing.
2. Implement the change and its tests, public API documentation, and any required build-file updates.
3. Build and run the relevant tests.
4. Commit the completed work according to the atomic and independent commit rules below.
5. Report the branch name, commit hashes, and validation results.

If work was accidentally started on `master`, create the correctly named branch immediately while
preserving the working tree, then continue the workflow there. Do not leave completed implementation
changes uncommitted unless the user explicitly asks for an uncommitted patch.

## Change checklist
- [ ] Public API has Doxygen (`@brief`, `@param`, `@return`, `@throw`).
- [ ] Tests added under `tests/`.
- [ ] Build files updated (`CMakeLists.txt`, `meson.build`).
- [ ] `CHANGELOG.md` updated for user-facing changes (in a **separate commit** after opening the PR).

## CHANGELOG conventions
Update `CHANGELOG.md` in the same PR that makes the change, but **in a separate commit** created after
the PR is opened so the PR number is known. Add one line per PR under the current unreleased
version heading (`Version X.Y.Z - <year> ???`). Create that heading if it does not exist yet.

- One bullet per PR: `- <description> (#NNN)`. The PR number is the **last token**, in parentheses.
- Write in the imperative mood, present tense: "Add", "Fix", "Update", "Remove". Not "Added",
  "Fixes", or "Adding".
- Keep each entry to a single line that names the user-facing effect, not the internal mechanics.
- Put the SQLite version bump first when the release includes one (see [[sqlitecpp-update-sqlite]]).
- Order the rest roughly as features, fixes, build/CI, then docs and tooling.
- A change merged straight to master without a PR still gets a bullet; omit the `(#NNN)` and note
  it was committed directly to master.
- ASCII only, no em dashes. Run the entry through the `humanizer` skill before committing so the
  prose stays plain and free of AI tells.

Finalizing the version heading and tagging belong to the release process: see
[[sqlitecpp-release]].

**Workflow for CHANGELOG updates:**
1. Commit source code changes (tests, implementation, etc.) in one or more atomic commits
2. Push the branch and open the PR to obtain the PR number
3. Create a separate commit adding only the CHANGELOG entry with the PR number
4. Push the CHANGELOG commit to the same branch

This keeps commits atomic (CHANGELOG is separate from code) and allows the PR number to be
included in the CHANGELOG entry.

## Pull requests
- Open PRs with `gh pr create` against `master`.
- The maintainer wants a **short and tight** PR description: one or two sentences on what the PR
  does and why, plus a brief bullet list only when it genuinely helps review. No filler, no
  restating the diff, no marketing. ASCII only, no em dashes; run it through `humanizer` if unsure.

## Git commits and pushing
- Make commits **atomic and independent**. Each commit must have exactly one purpose and be reviewable
  on its own. Include only the implementation, tests, and documentation required for that purpose.
- Do not mix distinct bug fixes, API additions, refactoring, formatting, workflow changes, or other
  unrelated work in one commit. A task containing separable outcomes, such as fixing existing behavior
  and adding a new API, requires separate commits.
- **CHANGELOG.md updates must be in a separate commit** from source code changes, created after the PR is opened so the PR number can be included.
- Each commit must leave the repository in a valid state: it must compile and pass its relevant tests
  when checked out at that point in the branch history.
- Before pushing a branch to the remote, **ask the user for explicit permission** stating the branch name and action (e.g., "Push branch `update-sqlite-3.52.2` to origin?"). Push only after receiving approval.

## Add a method
Follow the required workflow above. Include:

- A declaration with Doxygen in `include/SQLiteCpp/<Class>.h`.
- The implementation in `src/<Class>.cpp`.
- Tests in `tests/<Class>_test.cpp`.

## Add a class
Follow the required workflow above. Include:

- `include/SQLiteCpp/NewClass.h`, `src/NewClass.cpp`, and `tests/NewClass_test.cpp`.
- The new files in `CMakeLists.txt` and `meson.build`.
- The public header in `SQLiteCpp.h` when the class is public API.
