# Code guidelines

Rules for writing and reviewing Jetset code. A rule is here only if a
competent programmer reading three neighbouring files could plausibly get it
wrong:

- the code was inconsistent, so copying a neighbour can mislead;
- the rule isn't local (which page code goes in, which folders may include
  which);
- the reason is invisible (an SDCC limitation, a miscompile);
- only a person can check it.

Formatting (indentation, braces, spacing), and anything the build or a
script already enforces, has no rule here.

## Files

| File | Prefix | Covers | Applies to |
|---|---|---|---|
| [c.md](c.md) | `C-` | Universal C: naming, headers, preprocessor, idioms | all C |
| [house.md](house.md) | `H-` | House standards: types, constant groups, comments, documentation | all C |
| [toolchain.md](toolchain.md) | `T-` | z88dk and zsdcc traps | all code built for the Next |
| [nge.md](nge.md) | `N-` | The engine: naming, layers, initialisation, banking, data placement, tests | `src/nge/`, and banking and data in `src/game/` |
| [game.md](game.md) | `G-` | The game: modules, files, the architecture rules, development aids | `src/game/` |
| [patterns.md](patterns.md) | `P-` | The design patterns, each written up in the code it belongs to | `src/game/`, `src/nge/` |

For a file in `src/nge/`, read `c.md`, `house.md`, `toolchain.md` and
`nge.md`. For a file in `src/game/`, read all six. Tests in `tests/` follow
`c.md` and `house.md` only. They are PC code: `T-` rules don't apply.

## Rule format

```markdown
### C-IDIOM-3 · should · String comparisons are explicit
The rule, in one or two sentences.
Why: the reason, when the code doesn't show it.
Check: how to find breaks (a grep, or what to look at).
```

- **ID:** `<file prefix>-<topic>-<n>`. An ID never changes meaning. A rule
  that is dropped stays, marked *Withdrawn*, so old review comments and
  commit messages still resolve. New rules take the next number in their
  topic.
- **Strength:**
  - **must:** breaking it causes bugs, a broken build on the Next, or
    breaks a rule other code relies on. Fix before merging.
  - **should:** breaking it costs only readability or consistency. Fix when
    the code is touched, or record why not.
- **Exceptions** are in the rule itself. Code that breaks a rule for a reason
  says so in a comment that names the rule ID.

## Already enforced elsewhere

No rule repeats these:

- `make` (z88dk): compile errors, page overflow ("Section PAGE_n overlaps").
- `scripts/check-banks.sh`, run after every link: banked code and literals
  carry their page; no odd code pages; `*_cdata.c` symbols in their page and
  range; nge's `banked_call`, not z88dk's.
- `make test`: the PC tests, and the effects core's include ban
  (`check-effects-core` in `tests/Makefile`).

## Using the guidelines

- **Writing code:** read the files for the folder you are changing before
  you start, then check the result against them before handing it over.
- **Reviewing code:** the `review` skill (`.claude/skills/review/`) checks
  a diff, files or folders against these rules, and reports findings as
  `RULE-ID path:line: what is wrong`.
- **Missing rules:** a review that finds a problem no rule covers lists it
  under "possible new rules". Add it here only if it passes the test at
  the top.
