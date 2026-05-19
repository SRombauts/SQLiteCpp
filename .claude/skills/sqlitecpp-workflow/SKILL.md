---
name: sqlitecpp-workflow
description: SQLiteCpp change workflow checklists for API, tests, build files, and changelog updates.
---

# SQLiteCpp Workflow

## Change checklist
- [ ] Public API has Doxygen (`@brief`, `@param`, `@return`, `@throw`).
- [ ] Tests added under `tests/`.
- [ ] Build files updated (`CMakeLists.txt`, `meson.build`).
- [ ] `CHANGELOG.md` updated for user-facing changes.

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
