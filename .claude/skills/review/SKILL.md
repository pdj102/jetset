---
name: review
description: Review C code against docs/guidelines/. Run with /review [nothing for the uncommitted diff | a commit range | files | folders | all].
disable-model-invocation: true
---

# Review against the Jetset guidelines

The rules are in `docs/guidelines/`. This skill holds only the procedure: never
apply a rule from memory, read it.

## 1. Scope

From the arguments:

- nothing: the uncommitted changes (`git diff HEAD`, plus untracked files
  under `src/` and `tests/`);
- a commit or range (`HEAD~3..`, `main..dev`): that diff;
- files or folders: those files, all of each one, not only the changes;
- `all`: every `.c`, `.h` and `.asm` file under `src/`, and `tests/`. Split
  the work by folder and report per folder.

**Check the size before reading any rules.** Count the files in scope and,
for a diff, the changed lines (`git diff --stat`), and give the size in one
line.
- No argument (the uncommitted diff) and more than 10 files or 400 changed
  lines: stop. Ask whether to review it all or to narrow it (one folder, one
  commit), and wait for the answer.
- An explicit scope (`all`, a range, files or folders): the user chose it,
  so go on after giving the size.

For a diff, review the changed lines and anything they break nearby (a
header comment that no longer matches, a table row or switch case the change
needs). Don't review untouched code, except to say a rule is broken in a
place the diff copies from.

## 2. Read the rules

1. Read `docs/guidelines/README.md`.
2. Read the files that apply (README "Files" table): `src/nge/` needs
   `c.md`, `house.md`, `toolchain.md` and `nge.md`; `src/game/` needs all
   six; `tests/` needs `c.md` and `house.md`.
3. For each `P-` rule that matches code in scope (a new action, generator,
   component, effect, event, timer kind, feature, screen, state, level
   change), read the write-up it points to.

## 3. Check

Go through each rule that applies, and use its **Check:** line where there is
one. For each break, confirm it by reading the code around it:

- the rule's own exceptions, and any comment in the code citing the rule's
  ID to explain a deliberate exception;
- whether the thing is really what the rule covers (a capacity rather than
  an enumeration, PC test code rather than Next code).

Report only breaks you have confirmed. A grep hit is a lead, not a finding.

## 4. Report

Findings first, grouped by strength (**must** before **should**), each one
line:

```
RULE-ID path:line: what is wrong; the fix in a few words
```

Then:

- **Repeated breaks:** when a rule is broken in many places, give the count
  and up to five examples, not every line.
- **Possible new rules:** problems no rule covers (bugs, design problems,
  inconsistencies). Each with a line on whether it passes the test at the top
  of `docs/guidelines/README.md`.
- **Rules that didn't fit:** a rule that is unclear, contradicts another,
  or that the code shows to be wrong. These are changes to the guidelines,
  not to the code.

End with a count of findings per strength. Say plainly if there were none.
Don't fix anything unless asked. When asked to fix, fix the **must**
findings first, then build (`make`) and run `make test`, and report both
results.
