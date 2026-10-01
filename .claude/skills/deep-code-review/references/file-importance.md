# Source file importance and review priority

Reviewed on 2026-10-01, on branch `deep-code-review`, against local `master` commit
`cf33ad77d86b1c8292bdf3374855c8020a8c95fa` (SQLiteCpp 4.0.0).

Ranked inventory of 20 files: seven source/header pairs and six independent headers.
Scores and ordering are unchanged from the original assessment. Each file appears once.

I = importance, C = complexity, T = test quality, E = example quality, R = bug risk.
Scores range from 1 to 5; higher T and E mean better tests/examples. Priority is
**3I + 3R + C + (6 - T)**. Sort by descending priority, risk, importance, then group name.
See the [scoring rubric](../SKILL.md#scoring-rubric) for definitions.

Detailed findings, assessment evidence, reproductions, and validation limits are in
[findings-bugs.md](findings-bugs.md). Risk scores are review priorities, not vulnerability ratings.

## Ranked inventory

Each source/header pair occupies one row; file links make these rows exceed normal wrapping.

| Files | I | C | T | E | R | Priority |
|---|---:|---:|---:|---:|---:|---:|
| [src/Database.cpp](../../../../src/Database.cpp) + [include/SQLiteCpp/Database.h](../../../../include/SQLiteCpp/Database.h) | 5 | 5 | 4 | 4 | 5 | 37 |
| [src/Statement.cpp](../../../../src/Statement.cpp) + [include/SQLiteCpp/Statement.h](../../../../include/SQLiteCpp/Statement.h) | 5 | 5 | 4 | 4 | 5 | 37 |
| [src/Column.cpp](../../../../src/Column.cpp) + [include/SQLiteCpp/Column.h](../../../../include/SQLiteCpp/Column.h) | 5 | 3 | 4 | 4 | 4 | 32 |
| [src/Transaction.cpp](../../../../src/Transaction.cpp) + [include/SQLiteCpp/Transaction.h](../../../../include/SQLiteCpp/Transaction.h) | 4 | 3 | 3 | 4 | 4 | 30 |
| [src/Backup.cpp](../../../../src/Backup.cpp) + [include/SQLiteCpp/Backup.h](../../../../include/SQLiteCpp/Backup.h) | 3 | 4 | 3 | 1 | 4 | 28 |
| [src/Savepoint.cpp](../../../../src/Savepoint.cpp) + [include/SQLiteCpp/Savepoint.h](../../../../include/SQLiteCpp/Savepoint.h) | 3 | 4 | 3 | 1 | 4 | 28 |
| [include/SQLiteCpp/Assertion.h](../../../../include/SQLiteCpp/Assertion.h) | 3 | 2 | 2 | 4 | 4 | 27 |
| [src/Exception.cpp](../../../../src/Exception.cpp) + [include/SQLiteCpp/Exception.h](../../../../include/SQLiteCpp/Exception.h) | 5 | 2 | 3 | 2 | 2 | 26 |
| [include/SQLiteCpp/SQLiteCppExport.h](../../../../include/SQLiteCpp/SQLiteCppExport.h) | 3 | 2 | 3 | 2 | 3 | 23 |
| [include/SQLiteCpp/ExecuteMany.h](../../../../include/SQLiteCpp/ExecuteMany.h) | 2 | 3 | 3 | 2 | 3 | 21 |
| [include/SQLiteCpp/SQLiteCpp.h](../../../../include/SQLiteCpp/SQLiteCpp.h) | 3 | 1 | 3 | 4 | 2 | 19 |
| [include/SQLiteCpp/VariadicBind.h](../../../../include/SQLiteCpp/VariadicBind.h) | 2 | 3 | 3 | 3 | 2 | 18 |
| [include/SQLiteCpp/Utils.h](../../../../include/SQLiteCpp/Utils.h) | 1 | 2 | 2 | 1 | 2 | 15 |
