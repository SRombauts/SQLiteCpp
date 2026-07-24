---
name: sqlitecpp-workflow
description: >-
  SQLiteCpp change workflow checklists for API, tests, build files, and CHANGELOG updates.
  Use when adding a method or class, editing build files, or writing a CHANGELOG entry for a PR.
---

# SQLiteCpp Workflow

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
- Make **small, atomic commits** that each address a single logical change. Do not mix unrelated changes (e.g., bug fix + feature + formatting) in one commit.
- **CHANGELOG.md updates must be in a separate commit** from source code changes, created after the PR is opened so the PR number can be included.
- Each commit should be **complete and self-contained**: it must compile and pass tests independently.
- Before pushing a branch to the remote, **ask the user for explicit permission** stating the branch name and action (e.g., "Push branch `update-sqlite-3.52.2` to origin?"). Push only after receiving approval.

## Add a method
1. Declare in `include/SQLiteCpp/<Class>.h` with Doxygen.
2. Implement in `src/<Class>.cpp`.
3. Add tests in `tests/<Class>_test.cpp`.
4. Commit the changes from steps 1-3.
5. Push the branch and open the PR.
6. Add CHANGELOG entry with the PR number in a separate commit.

## Add a class
1. Create `include/SQLiteCpp/NewClass.h` and `src/NewClass.cpp`.
2. Add files to `CMakeLists.txt` (`SQLITECPP_SRC` and `SQLITECPP_INC`).
3. Add files to `meson.build`.
4. Include in `SQLiteCpp.h` if public API.
5. Create `tests/NewClass_test.cpp`.
6. Add test to `CMakeLists.txt` and `meson.build`.
7. Commit all changes from steps 1-6.
8. Push the branch and open the PR.
9. Add CHANGELOG entry with the PR number in a separate commit.
