---
name: windows-shell-commands
description: >-
  Running shell commands on this Windows checkout via the PowerShell and Bash tools, and passing
  multi-line or quoted arguments (commit messages, PR bodies, file content) without corruption.
  Use when running git or gh with multi-line or quoted input, writing a commit message or PR body
  from the command line, reaching for a heredoc or here-string, or after a message or body comes
  out wrapped in stray characters such as a leading and trailing `@`.
---

# Windows Shell Commands

This repo is developed on Windows. Two shells are reachable from tools, and they do **not** share
syntax. Mixing one shell's quoting into the other silently corrupts arguments.

## Default to PowerShell

Prefer the **PowerShell** tool for shell commands on Windows, especially anything with quoting,
multi-line text, or special characters (`git commit`, `gh pr create`, writing file content). It is
the native shell here and its multi-line here-string works as documented.

Use the **Bash** tool for genuine POSIX work: `sed`/`awk` pipelines, `curl` one-liners, shell
scripts, and `.sh` files. It runs Git Bash (POSIX sh), not cmd or PowerShell.

The trap: it is easy to type one shell's heredoc syntax into the other tool. The syntax below is
**not** interchangeable.

## Multi-line text: what works, what does not

### Symptom of getting it wrong

A commit message or PR body comes out wrapped in literal characters, classically a lone `@` line at
the very top and bottom. That means a PowerShell here-string (`@'...'@`) was fed to the Bash tool,
where `@` is just a literal character concatenated onto the quoted string.

### PowerShell tool (preferred)

Single-quoted here-string. The closing `'@` MUST be at column 0 (no indentation) on its own line:

```powershell
git commit -m @'
Subject line

Body paragraph with $literal dollar signs and "quotes" left intact.
'@
```

Use `@'...'@` (literal, no expansion). Only use `@"..."@` if you actually need `$variable`
expansion. Indenting the closing `'@` is a parse error.

### Bash tool

A PowerShell here-string does NOT work here. Use one of these instead:

- A real POSIX heredoc into `git commit -F -`:

  ```sh
  git commit -F - <<'EOF'
  Subject line

  Body paragraph.
  EOF
  ```

- Multiple `-m` flags (each becomes a paragraph): `git commit -m "Subject" -m "Body paragraph."`

`<<'EOF'` (quoted delimiter) keeps the body literal; unquoted `<<EOF` expands `$vars` and backticks.

## PR bodies and other long content: write a file

The most robust approach in **either** shell is to write the text to a file and point the command
at it. This sidesteps all quoting and heredoc differences:

```sh
gh pr create --base master --title "..." --body-file path/to/body.md
gh pr edit <N> --body-file path/to/body.md
```

Write the file with the Write tool (not shell redirection), then pass `--body-file` / `-F`. Delete
the temp file afterward. A scratch file under `.git/` is convenient and never tracked.

## Rule of thumb

Multi-line or special characters: use the PowerShell here-string, or write a file and pass it by
path. Reserve the Bash tool's heredoc for when you are already in a POSIX pipeline, and never paste
`@'...'@` into the Bash tool.
