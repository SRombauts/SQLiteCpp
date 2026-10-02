---
name: deep-code-review
description: >-
  SQLiteCpp source prioritization and delegated bug or vulnerability review.
  Use when ranking files for review or requesting a deep source audit.
---

# Deep Code Review

Choose the phase requested by the user. A priority inventory only ranks files; do not expand that
request into a bug hunt, runtime reproductions, or fixes. An explicit deep review examines defects
and vulnerabilities separately from the inventory. Neither phase authorizes fixes or publication.

## Priority inventory workflow

1. Read `AGENTS.md` and the branching, coding, testing, and skill-maintenance skills. Check the
   working tree, create a dedicated `deep-code-review` branch from `master` before editing, and
   record the reviewed commit. If that branch already exists, reuse it when appropriate or create
   a uniquely suffixed review branch. Preserve unrelated user changes.
2. Enumerate every source file under `src/` and every header under `include/`, recursively.
   Pair implementations with their corresponding headers. Assess each independent header too.
   Account for unmatched implementations and report generated headers separately if applicable.
3. Ask a sub-agent to evaluate each pair or independent header. An agent may evaluate multiple
   groups, but must return a separate assessment for each. Delegate read-only code inspection;
   agents must not change library code or commit. Limit concurrency to available agent slots.
4. For each group, inspect implementation and public contracts, repository call sites, relevant
   `tests/`, and `examples/`. Evaluate importance and usage, intrinsic complexity, unit-test
   quality, example quality, and vulnerability or other bug risk. Cite paths and line numbers for
   material claims. Separate observed defects, plausible concerns, and documented caller misuse.
5. Score each dimension from 1 to 5 using the rubric below. Tests and examples measure observed
   quality, not instrumented coverage. Do not invent coverage percentages or external usage.
   Record any inspection commands actually run and limitations of the assessment. Do not perform
   runtime reproductions for a priority-only request.
6. Check the delegated evidence and normalize scores across groups. Compute priority as
   `3 * importance + 3 * risk + complexity + (6 - tests)`. Examples remain a separate signal.
   Sort descending by priority, then risk, then importance, then ascending by file-group name.
7. Keep [references/file-importance.md](references/file-importance.md) limited to the ranked
   file groups, all five scores, priority, and brief revision/scope/scoring context. The requested
   single-line rows may exceed the usual Markdown wrapping limit. Store assessment evidence,
   bug findings, documentation defects, plausible concerns, reproductions, and validation limits
   in [references/findings-bugs.md](references/findings-bugs.md), never in the ranking file.
   Link the two references. Preserve the distinction between observations and confirmed defects.
8. Verify every enumerated file occurs in exactly one group, all scores are in range, priorities
   and ordering are correct, and evidence links resolve. Validate the skill and inspect the diff.
   Commit the skill, ranked report, and its `AGENTS.md` discovery entry when requested, excluding
   unrelated changes. Report the leading priorities and assessment limits; keep detailed defect
   findings in the separate deep-review phase.

## Scoring rubric

- **Importance (I):** 1 peripheral support; 2 optional convenience; 3 common supporting API;
  4 major runtime service; 5 foundational API used throughout normal database operations.
- **Complexity (C):** 1 trivial declarations or forwarding; 2 small deterministic logic;
  3 several branches or templates; 4 substantial state/lifetime interactions;
  5 broad stateful API with many contracts and error paths.
- **Tests (T):** 1 absent relevant automated checks; 2 indirect or happy-path checks only;
  3 useful direct tests with material gaps; 4 broad behavior and error tests with some gaps;
  5 thorough direct edge/error/lifetime tests for the actual surface.
- **Examples (E):** 1 no relevant example; 2 incidental or comment-only usage;
  3 one substantive common-use example; 4 several substantive usage patterns;
  5 broad examples including tricky contracts or errors.
- **Risk (R):** 1 little runtime exposure; 2 bounded support logic; 3 meaningful correctness or
  portability exposure; 4 substantial memory/resource/transaction exposure or a concrete concern;
  5 multiple high-impact hazards or highly exposed state/lifetime boundaries needing close review.

Risk is a review-priority estimate, not a confirmed vulnerability rating or exploitability score.
High importance and risk dominate the ranking; test gaps break out further review work.

## Reusing the report

Read the ranked reference when selecting the next file for a detailed review. Re-enumerate and
refresh evidence when the source revision changes. Read the separate findings reference for prior
observations and validation limits. Reproductions and additional testing belong to an explicitly
requested deep review. Fixes require the usual project workflow.


## Deep bug and vulnerability review

1. Reuse the dedicated review branch when continuing a review. Record the current source commit;
   do not assume the priority inventory covers the current revision. Re-enumerate all groups.
   When validating an existing report, compare its source revision with the current source and
   mark already-fixed findings as resolved. Scope delegated work to the reports under review.
2. For a full source audit, assign each source/header pair and independent header its own review.
   Prefer a separate agent per group. If the service limits the total number of agent threads,
   reuse completed reviewers with a fresh assignment for each queued group. Each assignment owns
   one group and returns a separate finding and coverage ledger. For report validation, delegate
   the affected groups. Each reviewer may inspect dependencies. It must read the whole assigned
   implementation/header and relevant tests, not just previous findings. Start from raw source
   rather than the old finding list to reduce confirmation bias.
3. Check ownership/destruction, exception safety, state transitions, moves/copies, callbacks,
   pointer/row/buffer lifetimes, lengths and arithmetic, NULL/empty inputs, SQL construction,
   untrusted data, feature macros, exported ABI, and supported compiler/platform behavior.
   Check the actual public promises and SQLite contracts against bundled headers/source.
4. Delegate review only: agents may create isolated probes and notes under `/tmp`, but must not
   edit project sources, build configuration, tests, or shared review documents, and must not
   commit. Use separate probe filenames/work directories. A coordinator can share immutable
   completed build outputs. Do not rebuild in another agent's directory.
5. Return findings with affected file/line, trigger, expected versus actual behavior, impact,
   severity, confidence, evidence, test gap, and a focused remedy. State which functions/contracts
   were inspected, commands run, and limits even when no defect is found. Distinguish confirmed
   defects from plausible concerns, documentation errors, and documented caller misuse.
6. Validate material findings with minimal C++17 probes when feasible. Run baseline tests and
   meaningful sanitizer checks using the build/testing skills. Keep generated files and probe
   databases under `/tmp`. A test pass does not invalidate an uncovered defect. Use bundled local
   SQLite contracts or authoritative documentation for uncertain semantics. Do not label undefined
   behavior a library vulnerability when the trigger merely violates an explicit caller contract.
7. Independently check and deduplicate agents' findings. Attribute cross-file defects to the
   responsible API. Security impact needs a realistic input path and consequence; do not infer
   exploitability solely from a high risk score. Use high/medium/low severity for actual impact,
   not the inventory's ordinal importance scores. Rank fix urgency by confirmed consequences and
   realistic triggers. Give confirmed bugs stable IDs, status, and focused regression requirements;
   keep documentation defects and unvalidated concerns separate.
8. Save [references/findings-bugs.md](references/findings-bugs.md) with the source revision,
   severity-ranked findings, reproduction evidence, rejected concerns, and validation limits.
   Include a complete per-group ledger for a full audit. For report validation, identify checked
   reports/groups and link retained historical assessments with their original revision. Keep the
   existing priority inventory separate. Preserve useful probe sources in
   `references/review-probes/` with exact build/run instructions when needed to make important
   findings reproducible. Review results do not certify absence of vulnerabilities.
9. Validate skill metadata, report links, complete file coverage, and the diff. Report the most
   important confirmed findings and remaining limits. Commit review artifacts when the user's
   ongoing review workflow already authorizes it; do not commit unrelated changes.
