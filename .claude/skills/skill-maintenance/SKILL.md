---
name: skill-maintenance
description: >-
  Editing conventions for files under `.claude/skills/**` (markdown style, frontmatter format,
  SKILL.md vs `references/` split, upstream-vendored skills). Use when creating a new skill,
  editing any `SKILL.md` or `references/*.md` file, refactoring a skill description,
  or after adding/removing a skill.
---

# Skill Maintenance

Editing conventions for files under `.claude/skills/**` (including `SKILL.md` and any
`references/*.md`). Does not apply to markdown elsewhere in the repo.
Apply on every edit. Do not retroactively rewrite skills that drift from these rules. Refactor
only when you are already editing the skill for another reason.

For the upstream specification, see Anthropic's
[Skill authoring best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices).
The conventions below are repo-local extensions and reinforcements of that guide.

## Markdown style

Wrap markdown at **100 characters**. In rendered Markdown, single newlines fold into spaces, so
wrapping does not affect the output, but it makes side-by-side diffs and review readable.

**ASCII only.** No emoji, no symbolic icons (warning signs, check marks, decorative arrows,
prohibition signs, etc.), no emoticons. Plain ASCII keeps `git diff` and GitHub's diff/blame views
readable and copy-pasteable, and stays consistent with the repo's `.editorconfig` (UTF-8 encoding,
but ASCII content avoids review surprises). For emphasis use **bold**, headings, or a leading word
like **NEVER**.

**No em dashes or en dashes.** Em dashes (`—`) and en dashes (`–`) are non-ASCII, so the
ASCII-only rule above already bans them. This line restates it because they slip in through
copy-paste and autocorrect. Do not use `--` or `-` as a substitute; restructure instead:
a period (new sentence), a comma (a tight aside), a colon (an explanation), or parentheses (a true
aside). This keeps skill markdown consistent with the `humanizer` skill, which bans em dashes in
all published prose.

## Frontmatter

Only `name` and `description` fields. No `version`, `allowed-tools`, or other keys unless a tool
actively reads them.

Use the YAML folded block scalar (`>-`) for any `description` longer than one line. Wrap lines at
100 chars inside the scalar; YAML folds them into one logical string.

```yaml
description: >-
  SQLiteCpp coding standards and API rules.
  Use when editing core sources, changing public headers, adding Doxygen comments,
  or enforcing RAII and naming conventions.
```

Avoid:
- `description: |` (literal: preserves newlines; the trigger string becomes multi-line)
- A single 400-char unwrapped line (works, but unreadable in diffs)

### Description anatomy: WHAT + WHEN

A description has two parts:

1. **WHAT**: one topic sentence covering what the skill knows. No triggers, no rule content.
2. **WHEN**: concrete user phrases or task contexts that should trigger the skill ("Use when...").

Annotated example:

```
WHAT:  SQLiteCpp CMake build configuration.
WHEN:  Use when configuring CMake builds, toggling build options, running
       tests via CTest, or editing `CMakeLists.txt` and build scripts.
```

### Token economy

The description is the trigger surface, always loaded into the system prompt for every conversation.
Every word costs.

**Hard ceiling: the folded `description` MUST be under 1024 characters. This is a hard limit.**
**After writing or editing any `description`, measure it before saving:**
count the length of the single string YAML produces (wrapped lines folded together with spaces,
blank lines becoming newlines), and confirm it is under 1024. A description that
reaches 1024 chars is a broken skill. Do not save it; summarize or cut triggers until it fits.
When in doubt, measure rather than eyeball.

Optimize:

- **Drop synonyms.** "Edit / modify / update" is one trigger, not three. Pick the verb users type.
- **Drop duplicates.** If the topic sentence already says "for SQLiteCpp", the trigger list does
  not need "in SQLiteCpp".
- **Drop quality words.** "Comprehensive", "robust", "powerful": none affect when the skill loads.
- **Drop hedges.** "May be useful for", "can help with". Replace with concrete triggers.
- **Drop body content.** Rules, syntax details, code snippets belong in the SKILL.md body, not the
  description.

### Trigger precision

Goal: load when needed, not when not.

- **Too narrow:** skill never triggers; capability is dead weight.
- **Too broad:** skill loads on unrelated tasks; wastes context budget.

Heuristic: a trigger phrase is well-calibrated when (a) you can imagine a user typing it verbatim
or in a near-paraphrase, and (b) you cannot imagine the phrase appearing in a task where this
skill is irrelevant.

### When to refactor a description

Rewrite a description on the next edit pass to that skill when:
- It reaches the 1024-char hard ceiling (e.g. a needed new trigger would push the folded length
  over the limit). This is the **only** length-based trigger: do not trim a description merely for
  being near the ceiling. A long description that still fits under 1024 and reads clearly is fine
  and needs no refactor, even at 950+ chars.
- It restates the same trigger in three different ways.
- It contains marketing or hedging language.
- The body has gained or lost a capability the description does not reflect.

Do not refactor pre-emptively. Proximity to the ceiling, on its own, is never a reason to act;
only an edit that would actually cross 1024 forces the length-driven rewrite.

## After Editing a SKILL.md Body

Whenever you change a SKILL.md body, re-read its `description` and decide:

- **Add triggers** if the body now covers a capability whose user-facing phrasing is not in the
  description.
- **Remove triggers** if the body no longer covers something the description still claims.
- **Tighten triggers** if a phrase has become a duplicate or synonym of another.
- **State explicitly that no change is needed** if the description still covers the body. Do not
  skip the check silently.

The same check applies when editing a `references/*.md` file if the reference introduces a new
capability or removes one.

## SKILL.md vs `references/`

SKILL.md is always-loaded once the skill triggers. Keep it lean: rules, decisions, pointers.

Push to `references/<topic>.md`:
- Setup, install, troubleshooting (loaded on demand only when broken)
- Verbose examples, full schemas, edge-case tables
- Project-specific build/test commands

If a rule is needed *only* during a sub-workflow that already loads a reference, put the rule in
the reference. SKILL.md should not pre-empt it.

## Canonical guidelines (Anthropic)

The repo-local rules above defer to the upstream
[Skill authoring best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices).
Highlights worth re-stating:

- **Hard limits.** `name` <= 64 chars (lowercase letters, digits, hyphens only).
  `description` < 1024 chars, a hard ceiling. Measure the folded length before saving
  (see "Token economy").
- **Body size.** Keep SKILL.md under ~500 lines / ~5000 tokens. Split into `references/` past that.
- **Description structure.** WHAT (what the skill does) **plus** WHEN (concrete triggers).
  Use the imperative ("Use when ...") rather than vague hedges.
- **Avoid undertriggering.** Claude tends to skip skills that *should* fire. Make triggers explicit
  enough, and slightly "pushy" when the topic is one the user will not name directly.
- **Test trigger coverage.** When adding or refactoring a description, run a few realistic user
  phrasings through it (literal, paraphrased, abbreviated, casual). Confirm the skill would load
  for the ones that should match and would not load for the ones that should not.
- **Progressive disclosure, three tiers:**
  1. Frontmatter: always loaded (cheap, every word counts).
  2. SKILL.md body: loaded when triggered.
  3. `references/`, `scripts/`, `assets/`: loaded only when SKILL.md instructs.

## Third-party / upstream skills

Some skills are vendored from an external source and tracked verbatim so they can be re-synced from
upstream. **`humanizer`** is one of these: leave its file untouched, including its frontmatter,
which carries upstream keys (`version`, `license`, `compatibility`, `allowed-tools`) that these
conventions would otherwise strip. Editing it locally would create merge friction the next time it
is pulled from upstream. If it genuinely needs a change, push the change upstream or fork it under a
new name rather than diverging the vendored copy in place.

## Validating a skill change

A skill's deliverable is agent behavior, not the Markdown files. Validate a change by **exercising
its procedure**: prompt a Coding Agent (ideally a fresh session/subagent on the most powerful model
available, `opus`) with a trigger phrase, and confirm the skill loads and the agent follows it to
the intended outcome. Re-reading the edited files is *review*, not validation.
