# Help screen: implementation plan

This plan gives the steps that build what [help_screen.md](help_screen.md)
specifies. The spec says what the feature is and why. This file says in
what order it is built, what changes, and how each step is checked.

Status: done (2026-11-09). Steps 1-3 built, and their CSpect checks
passed. The hardware check (the page turn time) is still to do.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game
  playable.
- **Every step:** run `make` and `make test`, and check the map for what
  the step cost. A new `__*` or `asm_*` symbol in common memory is a fault.
- **The CSpect check closes a step.** The user runs it, and the next step
  waits for the result.
- **After each step:** update the docs the spec lists (section 6) for what
  the step built. Anything that differs from the spec goes in an "As
  built" section in `help_screen.md`.

```text
 1 the screen: pages, turning, ? in play (headings only)
   |
 2 the four pages' text --> 3 closing docs
```

## 2. Steps

### Step 1: the screen, turning pages, and `?` in play

Covers R1-R6, R13-R15 and D2-D4, D6 and D8. Each page has its title and a
one-line placeholder body.

- **Change:**
  - `game/ui/screen_guide.h`: `screen_guide_run()`.
  - `game/ui/screen_guide_bank.c` (page 104): the panels, `turn()`,
    `draw()` and the run loop (spec 5.1).
  - `game/ui/screen_guide_pdata.c` (page 105, `constseg PAGE_105`):
    `guide_pages[]`, with the titles and placeholder bodies, and the `H`,
    `K` and `T` macros.
  - `screen_internal.h`: `turn()` as `screen_guide_turn()`, and
    `GUIDE_PAGES` and `guide_pages[]` for the tests. Add the new files to
    the list at the top.
  - `player_bank.c`: `case '?'` beside `'C'`.
  - `game.c`: `game_play_help()`'s new line (R6).
- **Tests:** `test_ui`: turning (every key in spec section 7) and the
  screen with fake keys (pages shown, buffer 0 untouched on close). If
  `player_command()` can be driven in the PC tests, also `?` uses no
  turn. Otherwise, CSpect check 6 covers that. The test Makefile gets the
  new sources.
- **CSpect (debug):** checks 1, 2, 3 and 6 of the spec, and a first look
  at check 4 (the page turn's repaint).
- **Map:** page 104 grows by about 0.4K and must stay under 8K. Page 105
  is used for the first time. Common memory doesn't change (or changes
  only by a banked-call entry). If 104 overflows, move the guide to pair
  142/143 (D3) and record it as built.

### Step 2: the pages' text

Covers R7-R12 and D1, D5, D7 and D9.

- **Change:**
  - `screen_guide_pdata.c`: the four pages' text from spec 5.2. Check
    each line against the code first:
    - the Commands keys against `player_command()`;
    - the screens' keys against `screen_inv_bank.c`,
      `screen_spell_bank.c`, `target_bank.c` and the direction prompts;
    - the Goal against `winning.md` and `LEVEL_DEPTH_BOTTOM`;
    - the abilities against `effects_pdata.c` (the saves), `feature.c`,
      `pc.c`, and whether gold gives XP (`pc_gain_gold()`).

    Any line that changes from the spec's draft goes in "As built".
  - D9's one-line comments beside `player_command()`'s `switch`, the
    effect saves in `effects_pdata.c`, the checks in `feature.c`, and the
    level gains in `pc.c`.
- **Tests:** `test_ui`: the text fits (R11). For each page: at most 29
  lines and 40 characters a line, only INK and `\n` control codes, and no
  trailing spaces. The screen test's headings are updated if they
  changed.
- **CSpect (debug):** checks 4, 5 and 7: every page is read on screen,
  and each Commands key is tried in play.
- **Map:** page 105 holds about 3K. Nothing else moves.

### Step 3: closing docs

- `help_screen.md`: status implemented, with the dates, and the "As
  built" section.
- `ui_screens.md`: R24 built (link here), R13 still later, and the help
  screen in section 1's list of screens.
- `memory_map.md`: pages 104/105.
- `src/game/README.md`: the new screen.
- `todo.md`: U1 and U1.1-U1.4 `[done]` with the date, and the new items
  (U1.5, U1.6, topic G with G1) if they were agreed.
- Hardware check: the page turn time, recorded when the user has run it.
