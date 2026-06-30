# Findings — Build system, CI/CD, packaging & examples

Files: `CMakeLists.txt`, `sqlite3/CMakeLists.txt`, `cmake/*`, `meson.build`, `meson_options.txt`, `examples/example2/CMakeLists.txt`, `.github/workflows/*`, `appveyor.yml`, `.travis.yml`, `build.sh/.bat`, `package.xml`, `subprojects/*.wrap`, `.gitmodules`, `examples/example1/main.cpp`, `examples/example2/src/main.cpp`.

Version strings are in sync (CMake `3.3.3`, `SQLiteCpp.h` `"3.03.03"`/`3003003`, `meson.build` `3.3.3`, `package.xml` `3.3.3`); the vendored amalgamation (`sqlite3.h` `3.53.2`) matches the Meson `sqlite3.wrap` (`3.53.2-1`).

## Findings (most severe first)

### BLD-001 — Meson build aborts when `SQLITECPP_DISABLE_STD_FILESYSTEM` is enabled
- [ ] **Severity:** high — **Confidence:** high — **Category:** build-correctness
- **Location:** `meson.build:132`
- **Impact (failure scenario):** The block does `sqlitecpp_cxx_flags += ['-DSQLITECPP_DISABLE_STD_FILESYSTEM']`, but the variable is `sqlitecpp_args` everywhere else and `sqlitecpp_cxx_flags` is never declared (verified: it appears exactly once in the file). `meson setup -DSQLITECPP_DISABLE_STD_FILESYSTEM=true` fails with "Unknown variable 'sqlitecpp_cxx_flags'". The option is unusable via Meson, and it only breaks for the users who need it (older toolchains). No Meson CI job sets the flag, so it is invisible in CI.
- **Fix:** Change line 132 to append to `sqlitecpp_args`.

### EXM-001 — example1 writes one byte past the end of the read buffer
- [ ] **Severity:** medium — **Confidence:** high — **Category:** example-correctness (footgun)
- **Location:** `examples/example1/main.cpp:415-419`
- **Impact (failure scenario):** `char buffer[16*1024];` then `const int size = fread(blob, 1, 16*1024, fp); buffer[size] = '\0';`. `fread` can return the full 16384, making `buffer[16384] = '\0'` an out-of-bounds stack write. The `static_assert` only checks the hard-coded logo size, not the `fread` cap, so it is safe only because the bundled logo is 12581 bytes. Users copy this blob pattern.
- **Fix:** Cap the read at `sizeof(buffer) - 1`, or drop the NUL write (a binary blob doesn't need it; `size` is passed explicitly to `bind`).

### EXM-002 — example1 leaks the `FILE*` on the read-failure / exception paths
- [ ] **Severity:** medium — **Confidence:** high — **Category:** example-correctness / resource-leak (footgun)
- **Location:** `examples/example1/main.cpp:441-462`
- **Impact (failure scenario):** `fp = fopen("out.png", "wb");` then `fclose(fp)` is only reached inside `if (query.executeStep())`. If `executeStep()` returns false, or any exception is thrown between `fopen` and `fclose`, the handle leaks. As the canonical blob example, users copy the leaky raw-`FILE*` idiom.
- **Fix:** Use RAII (`std::unique_ptr<FILE, decltype(&fclose)>`) or make `fclose` unconditional after the block.

### BLD-002 — googletest submodule pinned to a stale 2018-era commit, divergent from CI's wrap
- [ ] **Severity:** medium — **Confidence:** high — **Category:** supply-chain / maintainability
- **Location:** `.gitmodules:1`; pinned commit `6910c9d9…` (`release-1.8.0-3518-g6910c9d9`)
- **Impact (failure scenario):** The CMake test build (no system GTest) pulls this untagged snapshot 3518 commits past release-1.8.0, while the Meson path uses `gtest.wrap` v1.15.0 — the two build systems test against very different googletest versions, and the submodule receives no fixes. `.github/dependabot.yml` only tracks `github-actions`, so neither submodules nor wraps are auto-updated.
- **Fix:** Bump the submodule to a current tagged release (e.g. v1.15.0 to match the wrap); optionally add a `gitsubmodule` Dependabot ecosystem.

### BLD-003 — GitHub Actions pinned to mutable tags, not commit SHAs (secret-bearing workflow affected)
- [ ] **Severity:** medium — **Confidence:** high — **Category:** ci-security / supply-chain
- **Location:** `.github/workflows/cmake.yml:56`, `coverage.yml:51`, `coverity.yml:19,36`, `meson.yml:52,55`, plus `actions/checkout@v4` across all workflows
- **Impact (failure scenario):** Every action uses a floating tag (`actions/checkout@v4`, `coverallsapp/github-action@v2`, `vapier/coverity-scan-action@v1`, `ilammy/msvc-dev-cmd@v1`). Tags are mutable. `coverity.yml` is the highest-value target: it runs on push to `master` and passes `secrets.COVERITY_SCAN_TOKEN` to the third-party `vapier/...@v1`, so a hijacked tag executes with that secret. No workflow declares a top-level `permissions:` block.
- **Fix:** Pin actions to full commit SHAs (Dependabot still bumps them). Add a least-privilege `permissions: { contents: read }`, elevating only where needed.

### BLD-004 — Dead Travis config still ships an embedded Coverity token; AppVeyor lists retired images
- [ ] **Severity:** low — **Confidence:** medium — **Category:** ci / supply-chain hygiene
- **Location:** `.travis.yml:47` (encrypted `secure:` token); `appveyor.yml:15`
- **Impact (failure scenario):** travis-ci.org is shut down and Coverity now runs from `coverity.yml`, but `.travis.yml` still distributes an encrypted Coverity token and misleads contributors. `appveyor.yml` still references Visual Studio 2015. Stale CI definitions rot silently; the embedded token cannot be rotated through an active pipeline.
- **Fix:** Remove `.travis.yml` (and rotate/retire the token); remove or trim `appveyor.yml`; update the `SQLITECPP_SCRIPT` list in `CMakeLists.txt:198-199`.

### BLD-005 — `coverage.yml` / `coverity.yml` lack a `permissions:` block
- [ ] **Severity:** low — **Confidence:** medium — **Category:** ci-security
- **Location:** `.github/workflows/coverage.yml:1-3,50-54`, `coverity.yml:6-15`
- **Impact (failure scenario):** `coverage.yml` runs on `pull_request` and uses `secrets.GITHUB_TOKEN` with no explicit `permissions:`, inheriting the repo-default scope. Combined with the unpinned third-party action (BLD-003), the over-broad default widens the blast radius if that action is compromised.
- **Fix:** Add `permissions: { contents: read }` to both workflows; elevate only the scope the Coveralls upload needs.

### BLD-006 — ASAN config forces GCC `gold` linker and is no longer exercised in CI
- [ ] **Severity:** low — **Confidence:** medium — **Category:** build-portability / ci-gap
- **Location:** `CMakeLists.txt:263-273`, `build.sh:14`
- **Impact (failure scenario):** With GCC, `SQLITECPP_USE_ASAN=ON` forces `-fuse-ld=gold`; `gold` is deprecated and absent on many recent toolchains, so the build can fail with "cannot find ld.gold". `build.sh` enables ASAN by default, so the documented local build breaks on a gold-less GCC. No Actions workflow sets `SQLITECPP_USE_ASAN`, so the sanitizer path has no active CI coverage. (Independently, this review could not run ASan on Windows — see `verification.md` — making the missing Linux CI sanitizer job more valuable.)
- **Fix:** Drop the forced `-fuse-ld=gold` (or make it conditional on gold being present); add a sanitizer job to the Actions matrix.

### EXM-003 — example2 uses double-quoted string literals in SQL
- [ ] **Severity:** low — **Confidence:** high — **Category:** example-correctness (footgun)
- **Location:** `examples/example2/src/main.cpp:53,57,61`
- **Impact (failure scenario):** `VALUES (NULL, "test")` and `value="second-updated"` use double quotes for string values. In SQL, double quotes denote identifiers; SQLite's literal fallback is a deprecated misfeature disabled under `SQLITE_DQS=0`. Users copying this into a strict-DQS build get "no such column: test". example1 correctly uses single quotes.
- **Fix:** Use single quotes for string literals.

### BLD-007 — `examples/example2/CMakeLists.txt` clobbers options with `FORCE`
- [ ] **Severity:** low — **Confidence:** medium — **Category:** build-maintainability
- **Location:** `examples/example2/CMakeLists.txt:17-20`
- **Impact (failure scenario):** `set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)` etc. before `add_subdirectory(../..)` overwrites whatever the consuming project set, silently disabling cpplint/cppcheck/static-runtime for the whole tree. As a copy-paste "how to embed SQLiteCpp" template, it teaches users to clobber options.
- **Fix:** Drop `FORCE` (provide defaults only).

### BLD-008 — `sqlite3/CMakeLists.txt` shared-build export macro is directory-global and export-only
- [ ] **Severity:** low — **Confidence:** medium — **Category:** build-correctness / portability
- **Location:** `sqlite3/CMakeLists.txt:14-18`
- **Impact (failure scenario):** For `BUILD_SHARED_LIBS` on Windows it uses `add_definitions("-DSQLITE_API=__declspec(dllexport)")` — directory-global (applies to later targets) and export-only (never import). Within this project sqlite3 is linked statically into the SQLiteCpp DLL so it works, but the placement is fragile and non-idiomatic.
- **Fix:** Replace with `target_compile_definitions(sqlite3 PRIVATE "SQLITE_API=__declspec(dllexport)")`.

### TST-001 — Tests use C++17 `[[maybe_unused]]` while the project targets C++11
- [ ] **Severity:** low — **Confidence:** high — **Category:** portability / test-hygiene
- **Location:** all `tests/*_test.cpp` (e.g. `tests/Database_test.cpp:39`, `tests/Column_test.cpp:202`, …)
- **Impact (failure scenario):** The default standard is C++11 (`CMakeLists.txt:13`), but tests use the C++17 `[[maybe_unused]]` attribute. clang at C++11 emits `-Wc++17-attribute-extensions` for every `TEST(...)` (observed in the verification build); a compiler treating that as an error (`-Werror`) would fail the test build at C++11. g++ accepts it silently.
- **Fix:** Either raise the test target to C++17, or replace `[[maybe_unused]]` with a C++11-compatible mechanism in the test macros.

## Verified non-issues
- **Version strings are in sync** across CMake / `SQLiteCpp.h` / `meson.build` / `package.xml` (`3.3.3`). `CHANGELOG.md` has a `Version 3.4.0 - 2026 ???` unreleased accumulator — normal on a dev branch.
- **Vendored sqlite3 matches the Meson wrap** (`3.53.2`); both wraps carry `source_hash`/`patch_hash` integrity checks.
- **`#if SQLITECPP_DLL_EXPORT` is not a broken empty-macro bug** — a bare `-D`/`/D` defines the macro to `1`; the MSVC `BUILD_SHARED_LIBS=ON` CI job passes. (See HDR-001 in `findings-exception-headers.md`.)
- **`coverity.yml` fork-secret handling is correct** — gated `if: github.repository == 'SRombauts/SQLiteCpp'` and runs only on push to `master`/dispatch, so forks cannot read the token. (Pinning + permissions still tracked in BLD-003/BLD-005.)
- **No `${{ }}` shell-injection sink** — interpolations come from workflow-defined `matrix.config.*` / `github.ref_name`, not attacker-controlled PR fields.
- **`SQLITECPP_USE_STATIC_RUNTIME` vs gtest CRT handling is internally consistent** (`CMakeLists.txt:44-68`).
