---
name: sqlitecpp-doxygen-guide
description: SQLiteCpp Doxygen standards and templates for public API docs and file headers.
---

# SQLiteCpp Doxygen Guide

> **For general style rules, see `AGENTS.md` and `sqlitecpp-coding-standards`.**

## Scope
- Doxygen runs on both `include/` and `src/` (see `Doxyfile`).
- Public API must be documented in headers.
- Source files still use Doxygen file headers.

## Basic Example (Format Only)
```cpp
/**
 * @file    Example.h
 * @ingroup SQLiteCpp
 * @brief   One-line summary of the file.
 */
class SQLITECPP_API Example
{
public:
    /**
     * @brief Do the thing.
     * @param[in] aValue  Value to use
     * @return Result value
     * @throw SQLite::Exception in case of error
     */
    int doThing(int aValue);
};
```
Keep the full MIT license block in real file headers.

## Canonical Examples (Use These)
- File header + class + methods: `include/SQLiteCpp/Database.h`
- File header in `src/`: `src/Database.cpp`

## File Header Rules
- Keep `@file`, `@ingroup`, `@brief`.
- Keep the MIT license block and copyright line.
- Keep `#pragma once` in headers.
- Match the existing header of the file you edit.

## API Comment Rules
- `@brief` for every public class/method.
- `@param[in|out]` for each parameter.
- `@return` for non-void return values.
- `@throw SQLite::Exception` for throwing APIs.
- Use `@note`/`@warning` only when needed.

## Generate Docs
```bash
cmake -DSQLITECPP_RUN_DOXYGEN=ON ..
cmake --build . --target SQLiteCpp_doxygen
```
Output: `doc/html/index.html`

## Cross-References
- Workflow checklist: see `sqlitecpp-workflow`
