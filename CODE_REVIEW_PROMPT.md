# Reusable deep code review prompt

A reusable, language-agnostic prompt for a deep, multi-pass code review. The defaults are
baked in, so you can paste the Prompt section into an agent as-is. It scales from a single
file to a large monorepo.

---

## Original prompt (for reference)

Plan a deep code review of the source code of this C++ wrapper.
Task a faster sonnet model to investigate the codebase to produce an intermediate markdown text file at the root of the project, with the list of source code files, with, for each, an eval between 1 and 5 of their complexity, their importance/usage, perceived risk of bugs, and risk of security vulnerabilities. This requires investigating dependencies, unit tests, examples etc.
Then, task opus models to, in parallel, each do a thorough code review of one source code file with its header.
Report all the findings in the markdown file in a comprehensive way, and finally rank the proposed fixes, but don't implement anything yet.
Ask me any questions to flesh out the plan in detail if needed.

---

## Prompt

Run a deep, thorough, multi-pass code review of this repository, with balanced priorities.
Work in phases.

Phase 0, inventory and scope. List the in-scope source files (respect ignore files; skip
vendored, third-party, and generated code, build artifacts, and lockfiles, and say what you
skipped). Map dependencies, entry points, tests, examples, and build/CI tooling. Group files
into logical review units, such as a class with its header and implementation, or a module,
rather than file by file. Report the inventory and the units you propose.

Phase 1, triage matrix, using a fast and cheap model. Score each unit from 1 to 5, with a
one-line reason, on complexity, importance or blast radius, bug risk, security risk, and
test-coverage gap (5 means least covered). Base the scores on the actual dependencies, callers,
tests, and examples. Write the matrix and a recommended review order as section 1 of a single
report at the repo root.

Phase 1b, verification, run once if the project builds. Build with the strictest warnings the
project supports, run the tests, and run any available sanitizers (address, undefined-behavior,
thread, or the language's equivalents), linters, static analyzers, and cheap coverage. Record
the results honestly, including anything that could not run and why; never call a check passed
if it did not run. If time is short, run the most informative check first.

Phase 2, deep reviews, using the strongest model available. Review units in parallel, scaled to
repo size, with each reviewer writing to its own file. On a very large repo, do the
highest-priority units first and sample the rest. Cover at least:

- Correctness and logic: edge cases, error and failure paths, off-by-one, state machines, reentrancy.
- Memory and resource safety: ownership and lifetime, leaks, use-after-free and double-free, dangling references, releasing handles, file descriptors, and locks; for native code also buffer bounds, integer overflow, undefined behavior, alignment, endianness.
- Concurrency: thread-safety contracts, data races, shared mutable state, lock ordering, atomics.
- Security: input validation, injection (SQL, command, path, template), deserialization, path traversal and SSRF, secrets, crypto misuse, authentication and authorization, unsafe defaults, untrusted-data flow.
- Error and exception handling: failure atomicity, partial-state recovery, no throwing from destructors (or the language equivalent), silently swallowed errors.
- API and ABI design: backward and forward compatibility, const-ness and immutability, naming consistency, footguns, easy-to-ignore results.
- Performance: allocations, copies, algorithmic complexity, hot paths, needless work.
- Portability: platform, compiler, architecture, locale, and text-encoding assumptions.
- Maintainability: dead code, duplication, excess complexity, code smells, inconsistent style.
- Test gaps: what is untested, and the regression tests each finding implies.
- Documentation: whether comments and docs match the code.
- Dependencies and supply chain: versions, pinning, vendored drift, known-vulnerable or deprecated APIs.
- Observability: logging, metrics, and tracing where they matter.

Hold each finding to evidence: check it against the real API contract and the existing tests,
and do not report what you only suspect. Give each a stable ID, a severity (critical, high,
medium, low, info), a confidence (high, medium, low), a category, the file and line, the impact
as a failure scenario, and a concrete fix. Also list the verified non-issues (looked wrong, is
correct, why) so nobody re-flags them. Make every finding tickable: start it with a "[ ]" Done
checkbox an agent flips to "[x]" when the fix lands, optionally annotated with the PR or commit
(for example "[x] #552"), so the report works as a live tracker.

Phase 3, consolidate and rank. Merge the findings, call out cross-cutting themes (one fix that
resolves several findings), and produce one ranked fix list grouped P0, P1, and P2 by severity
weighed against likelihood and effort, with confidence and the files touched on each row. Make
each fix tickable too. Flag any fix that is breaking, changes behavior, or changes tests. Then
present the report and stop; make no code changes until I approve.

Output. One report at the repo root that stays easy to scan (matrix, verification results,
per-unit findings, cross-cutting themes, ranked fixes), with the full per-finding detail in a
sibling folder. Every issue and fix must be tickable. Keep reviewers off each other's files.

Phase 4, implement, only after I approve. Make each fix on its own branch following the repo's
conventions, add or extend tests, update the changelog and docs, and re-run the verification
pass to confirm green. Keep each change traceable to a ranked finding.
