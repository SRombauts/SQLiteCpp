# SQLiteCpp 3rd-Pass Triage (Post-P0)

_Generated 2026-07-02 on the branch following P0/P1 fix merges (#552–#559)._

## Scoring Matrix (Updated)

| File | Complexity | Importance | Bug Risk | Security Risk | Test-Coverage Gap | Notes |
|------|:----------:|:----------:|:--------:|:-------------:|:-----------------:|-------|
| Database.h + Database.cpp | 4 | 5 | 3 | 4 | 2 | P0 null-handling fixed; DB-12 (move/Statement orphan) remains; encryption/extension loading still high-surface. |
| Statement.h + Statement.cpp | 4 | 5 | 3 | 2 | 2 | STMT-01/02 NULL guards fixed; STMT-04 (move-assign) and STMT-05 (blob size truncation) remain. |
| Column.h + Column.cpp | 3 | 5 | 2 | 2 | 3 | COL-01 operator<< fixed; COL-02/03/04 (noexcept deref, UInt truncation, narrowing) remain. |
| Transaction.h + Transaction.cpp | 1 | 4 | 2 | 1 | 3 | TXN-08 catch(...) merged; TXN-01 (no SQLITECPP_ASSERT) and TXN-02/03/04 (behavior switch, error messaging) remain. Sparse test coverage of error paths. |
| Savepoint.h + Savepoint.cpp | 2 | 3 | 2 | 2 | 3 | SP-02/03 fixed (mbRolledBack flag + catch(...)). SP-01/04/05 remain. No destructor-error tests. |
| Backup.h + Backup.cpp | 2 | 3 | 1 | 1 | 4 | Simple; BKP-01 (executeStep not [[nodiscard]]) remains; page-count 0 case undocumented. Good test coverage. |
| Exception.h + Exception.cpp | 1 | 4 | 1 | 1 | 2 | EXC-02 null-message fixed. EXC-01 (extended code -1) and EXC-03/04 ([[nodiscard]]) remain but are low-risk. |
| VariadicBind.h | 4 | 3 | 1 | 1 | 2 | Header-only; VB-01/04 doc/forward style issues; no resource risk. Adequate test coverage. |
| ExecuteMany.h | 4 | 2 | 1 | 1 | 2 | EM-01 (clearBindings) fixed; EM-02/03 footguns remain but are doc-level. Regression test added. |
| Assertion.h | 1 | 3 | 1 | 1 | 1 | HDR-01 unbraced if, HDR-02 int/long mismatch — low risk, style-level. |
| Utils.h | 1 | 2 | 1 | 1 | 1 | Trivial macro. |
| SQLiteCppExport.h | 1 | 2 | 1 | 1 | 1 | Trivial DLL/warning macros. |
| SQLiteCpp.h | 1 | 3 | 1 | 1 | 1 | Umbrella header; no logic. |

## Recommended Review Order

### Tier 1 (Most Critical) — Security & Orphan-State Bug
1. **Database** — DB-12 (move orphans Statements); security-sensitive (key, extension, header parsing).
2. **Statement** — STMT-04/05 (move-assign, truncation); owns prepared-statement lifetime.

### Tier 2 (High Importance, Remaining Correctness)
3. **Column** — COL-02/03/04 (noexcept UB, narrowing, truncation); touched by every SELECT.
4. **Savepoint** — SP-01/04 (destructor semantics, NUL truncation); only interpolating SQL unit.
5. **Transaction** — TXN-01/02/03/04 (error swallowing, enum exhaustiveness, messaging); destructor discipline.

### Tier 3 (Medium Importance, Robustness)
6. **Backup** — BKP-01/03 (executeStep nodiscard, page-count docs); thin wrapper, low-risk.
7. **Exception** — EXC-01/03/04 (extended code, nodiscard); used everywhere but trivial.

### Tier 4 (Low Priority, Templates & Macros)
8. **VariadicBind / ExecuteMany** — VB-01/04, EM-02/03 (doc/style/namespace); header-only convenience layers.
9. **Assertion / Trivial Headers** — HDR-01/02, Utils, SQLiteCppExport, SQLiteCpp — macro/umbrella style issues.

## Key Scoring Deltas vs. CODE_REVIEW.md Section 1

| Unit | Prior Bug Risk | New Bug Risk | Change | Reason |
|------|:---:|:---:|:---:|---------|
| Database | 4 | 3 | ↓ | DB-01/04 null-handling + DB-02/03 signed/width fixed; DB-12 move remains high-risk. |
| Statement | 4 | 3 | ↓ | STMT-01/02 NULL guards fixed; STMT-04/05 remain medium-risk. |
| Column | 3 | 2 | ↓ | COL-01 operator<< fixed; COL-02/03/04 are design-level, lower incident risk. |
| Transaction | 2 | 2 | = | TXN-08 catch(...) fixed; TXN-01/02/03/04 remain, but primarily robustness/messaging. |
| Savepoint | 3 | 2 | ↓ | SP-02/03 fixed (flag + catch(...)); SP-01/04/05 remain but less critical. |
| Exception | 2 | 1 | ↓ | EXC-02 null-message fixed; remaining issues are low-risk. |

**Overall:** P0 fixes reduced immediate UB/null-pointer risk substantially. Remaining issues cluster around move semantics (DB-12, STMT-04), truncation/narrowing (STMT-05, COL-03), error handling discipline (TXN-01), and API modernization (nodiscard, const-correctness). No unpatched correctness defects remain at **High** severity.
