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
- Visual Studio 2022/2015, Debug/Release, Win32/x64.
- CMake config: `-DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON -DSQLITECPP_RUN_CPPCHECK=OFF`.
- Build and run `ctest --output-on-failure`.

## Travis CI
- Multiple GCC/Clang versions across Linux and macOS.
- Variants for ASAN, GCov, Valgrind, shared libs, external sqlite3.
