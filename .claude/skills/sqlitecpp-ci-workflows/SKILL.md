---
name: sqlitecpp-ci-workflows
description: SQLiteCpp CI workflow patterns. Use for GitHub Actions, AppVeyor, Travis, matrices, or test steps.
---

# SQLiteCpp CI Workflows

## Common steps
- Checkout code.
- Initialize submodules: `git submodule update --init --recursive`.
- Configure build directory (`build` or `builddir`).
- Build and run tests with verbose output.

## GitHub Actions (CMake)
- Matrix across Windows (MSVC/MinGW), Ubuntu, macOS.
- CMake config includes:
  - `-DBUILD_SHARED_LIBS=ON`
  - `-DSQLITECPP_BUILD_TESTS=ON`
  - `-DSQLITECPP_BUILD_EXAMPLES=ON`
  - `-DSQLITECPP_RUN_CPPCHECK=OFF`
  - `-DSQLITECPP_RUN_CPPLINT=OFF`
- Tests: `ctest --verbose --output-on-failure`.

## GitHub Actions (Meson)
- Use `pipx install meson ninja`.
- Set `CC`, `CXX`, and optional linkers.
- Setup with tests/examples and sqlite3 fallback:
  `meson setup builddir -DSQLITECPP_BUILD_TESTS=true -DSQLITECPP_BUILD_EXAMPLES=true --force-fallback-for=sqlite3`.
- Build: `meson compile -C builddir`.
- Test: `meson test -C builddir`.

## AppVeyor
- Visual Studio 2022/2019, Release, Win32/x64. GitHub Actions provides the overlapping Debug coverage.
- CMake config: `-DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_RUN_CPPCHECK=OFF`.
- Build and run `ctest --output-on-failure`.
- Configure `branches.only: master` in `appveyor.yml`; its presence overrides branch filtering from the AppVeyor
  project UI. Pull requests targeting `master` are still built, while pushes to task branches are skipped.

## Travis CI
- Multiple GCC/Clang versions across Linux and macOS.
- Variants for ASAN, GCov, Valgrind, shared libs, external sqlite3.

## Coverage (Coveralls)
The `Coverage` workflow (`.github/workflows/coverage.yml`) builds with `-DSQLITECPP_USE_GCOV=ON`
(GCC only), runs `ctest` (both `UnitTests` and `Example1Run`), captures with `lcov` while
excluding `/usr`, `googletest`, `sqlite3`, `examples` and `tests`, and uploads to Coveralls.

To find which lines are still uncovered without rebuilding locally, query the Coveralls JSON API
for the master repo (the build id comes from the repo summary):

```sh
# Overall percentage and the latest build id
curl -s https://coveralls.io/github/SRombauts/SQLiteCpp.json   # covered_percent, badge, build id

# Per-file missed counts for a build (source_files is a JSON-encoded string inside the payload)
curl -s "https://coveralls.io/builds/<BUILD_ID>/source_files.json?per_page=100"

# Exact missed line numbers for one file (array of hit counts; null/0 entries are uncovered)
curl -s "https://coveralls.io/builds/<BUILD_ID>/source.json?filename=src/Statement.cpp"
```

Before giving up on a miss, work out the root cause; some look like artifacts but are fixable:

- **Multi-line pack expansion** (`(void)std::initializer_list<int>{ ... };` split across lines for
  variadic `bind`/`execute_many`). When the per-element call can throw, gcov puts the post-call
  edge on the standalone `initializer_list` line and reports it uncovered even though the calls run.
  Collapse the expansion onto a single line so gcov attributes it to the tested call expression.
- **A guard that looks unreachable** may still be reachable through a public constructor. The
  `Column` "Statement was destroyed" check is shadowed by `checkRow()` on the normal path, but
  `Column`'s constructor and `Statement::TStatementPtr` are public, so a test can construct a
  `Column` from a null pointer directly. Check the public surface before assuming dead code.

Genuinely not worth chasing:

- **Closing-brace lines** that gcov marks uncovered as exception-unwinding landing pads. These are
  compiler-version specific (a local gcc may not even reproduce what CI's gcc reports), and removing
  them means contorting the function. Leave them.
- **Platform-specific success paths**, e.g. the normal return of `loadExtension`, which the only
  portable test cannot reach without a real loadable extension binary.

Cover the genuinely reachable lines and leave the rest.
