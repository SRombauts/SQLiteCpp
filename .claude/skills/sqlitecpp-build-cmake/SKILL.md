---
name: sqlitecpp-build-cmake
description: Build SQLiteCpp with CMake. Use for CMake builds, tests, options, or build scripts.
---

# SQLiteCpp CMake Build

## Quick builds
- Windows (VS 2026):
  - `mkdir build`
  - `cd build`
  - `cmake -G "Visual Studio 18 2026" -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_BUILD_EXAMPLES=ON ..`
  - `cmake --build . --config Release`
- Unix/macOS:
  - `mkdir build && cd build`
  - `cmake -DCMAKE_BUILD_TYPE=Debug -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_BUILD_EXAMPLES=ON ..`
  - `cmake --build .`

## Build scripts
- `build.bat` (Windows) enables shared libs, tests, examples, and runs `ctest`.
- `build.sh` (Unix) enables ASAN, shared libs, tests, examples, and runs `ctest`.

## Common options
- `SQLITECPP_BUILD_TESTS` (OFF): build unit tests.
- `SQLITECPP_BUILD_EXAMPLES` (OFF): build examples.
- `BUILD_SHARED_LIBS` (OFF): build shared libs (DLLs).
- `SQLITECPP_INTERNAL_SQLITE` (ON): use bundled sqlite3 source.
- `SQLITE_ENABLE_COLUMN_METADATA` (ON): enable `getColumnOriginName()`.
- `SQLITECPP_RUN_CPPLINT` (ON): run cpplint target.
- `SQLITECPP_RUN_CPPCHECK` (ON): run cppcheck target.
- `SQLITECPP_RUN_DOXYGEN` (OFF): generate docs.
- `SQLITECPP_USE_ASAN` (OFF): address sanitizer.
- `SQLITECPP_USE_GCOV` (OFF): GCov coverage.
- `SQLITECPP_DISABLE_STD_FILESYSTEM` (OFF): disable std::filesystem support.
- `SQLITECPP_DISABLE_EXPANDED_SQL` (OFF): disable sqlite3_expanded_sql support.

## Tests
- `ctest --output-on-failure`
- `ctest --output-on-failure -V`
- `ctest --output-on-failure -R "Database"`

## Notes
- Tests can use a system `GTest`; otherwise they fall back to the `googletest` submodule.
- If the submodule is needed: `git submodule update --init --recursive`.
