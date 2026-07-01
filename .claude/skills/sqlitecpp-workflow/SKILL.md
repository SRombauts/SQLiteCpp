---
name: sqlitecpp-workflow
description: >-
  SQLiteCpp change workflow checklists for API, tests, build files, and CHANGELOG updates.
  Use when adding a method or class, editing build files, or writing a CHANGELOG entry for a PR.
---

# SQLiteCpp Workflow

## Task branch, commits, and publishing

Every change follows the same shape. Do NOT commit fixes onto `master`, `code-review`, or
whatever branch you happen to be on.

1. **Branch.** Create a dedicated task branch from `master` (see [[sqlitecpp-git-branching]] for
   naming). One branch per issue/fix, even when several fixes are in flight.
2. **Code fix commit.** Commit the code (and its tests) as its own commit. No `CHANGELOG.md` in
   this commit. Bug-fix diffs stay comment-free.
3. **CHANGELOG commit.** Commit the `CHANGELOG.md` entry separately (see below). Keep it out of the
   code-fix commit.
4. **Publish.** Only push the branch and open the PR when the user says to **handle it auto** (or
   has otherwise pre-authorized publishing). Otherwise stop after the local commits and **ask for
   permission to publish** (push + `gh pr create`). Committing locally is not publishing.

### Why the code fix and the CHANGELOG are separate commits
The CHANGELOG entry needs the PR number (`(#NNN)`), which you can guess upfront but do not truly
know until the PR exists. Keeping it in its own commit means:
- the code fix is ready to review without waiting on a PR number, and
- CHANGELOG lines do not collide when several bug fixes are tackled in parallel (every PR touches
  the same few lines of `CHANGELOG.md`, so shared code-fix commits would conflict constantly).

## Change checklist
- [ ] Public API has Doxygen (`@brief`, `@param`, `@return`, `@throw`).
- [ ] Tests added under `tests/`.
- [ ] Build files updated (`CMakeLists.txt`, `meson.build`).
- [ ] `CHANGELOG.md` updated for user-facing changes.

## CHANGELOG conventions
Update `CHANGELOG.md` in the same PR that makes the change, not in a later batch, but in its **own
commit** separate from the code fix (see the publishing section above). Add one line per PR under
the current unreleased version heading (`Version X.Y.Z - <year> ???`). Create that heading if it
does not exist yet.

- One bullet per PR: `- <description> (#NNN)`. The PR number is the last token, in parentheses.
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

## Pull requests
- Open PRs with `gh pr create` against `master`.
- The maintainer wants a **short and tight** PR description: one or two sentences on what the PR
  does and why, plus a brief bullet list only when it genuinely helps review. No filler, no
  restating the diff, no marketing. ASCII only, no em dashes; run it through `humanizer` if unsure.

## Add a method
1. Declare in `include/SQLiteCpp/<Class>.h` with Doxygen.
2. Implement in `src/<Class>.cpp`.
3. Add tests in `tests/<Class>_test.cpp`.
4. Update `CHANGELOG.md`.

## Add a class
1. Create `include/SQLiteCpp/NewClass.h` and `src/NewClass.cpp`.
2. Add files to `CMakeLists.txt` (`SQLITECPP_SRC` and `SQLITECPP_INC`).
3. Add files to `meson.build`.
4. Include in `SQLiteCpp.h` if public API.
5. Create `tests/NewClass_test.cpp`.
6. Add test to `CMakeLists.txt` and `meson.build`.
