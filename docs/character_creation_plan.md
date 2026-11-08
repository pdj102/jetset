# Character creation: implementation plan

This plan gives the steps that build what
[character_creation.md](character_creation.md) specifies. The spec says
what the feature is and why. This file says in what order it is built,
what changes, and how each step is checked.

Status: step 1 done (2026-11-08): its PC tests and CSpect checks passed.
Step 2 done (2026-11-08): its PC tests and CSpect checks 1-6 passed.
Hardware check 7 is still to do.
Step 3 is still to do.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game
  playable.
- **Every step:** run `make` and `make test`, and check the map for what
  the step cost. A new `__*` or `asm_*` symbol in common memory is a fault.
- **The CSpect check closes a step.** The user runs it, and the next step
  waits for the result.
- **After each step:** update the docs the spec lists (section 6) for what
  the step built. Anything that differs from the spec goes in an "As built"
  section in `character_creation.md`.

```text
 1 classes and making a character (fixed class, no screen)
   |
 2 the creation screen --> 3 closing docs
```

## 2. Steps

### Step 1: classes and making a character

Covers R6-R9 and D2-D7, D9 and D10. There is no screen yet. The game
starts as Dev in debug builds and as Fighter in release builds.

- **Change:**
  - `CR_FIGHTER` and `CR_MAGE` in `gen_kinds.h` and
    `gen_creature_pdata.c`, with their sequence rows in `anim_pdata.c`.
    `CR_PLAYER` (Dev) gets 12 in every ability, and the test
    expectations that followed from today's figures are updated.
  - `PC_CLASS_MAGE` and `PC_CLASS_DEV`, and `pc_set_class()`.
  - `src/game/chargen/` (pair 140/141, added to the bank list) with the
    class table, the kits, `chargen_make()` and `chargen_class_name()`.
  - `game_reset_world()` taken out of `play_start()`.
  - `actors_create()` becomes `actors_create_persistents()`, and
    `dev_kit[]` moves to Dev's kit.
  - `play_start()` calls `chargen_make()` with a fixed class and the name
    the title still asks for.
  - The PC screen gets its class name from `chargen_class_name()`.
- **Tests:** `test_chargen` (new: everything except the screen),
  `test_gen`, `test_pc`, and `test_ui` (the PC screen's title). The test
  Makefile gets the new module's sources.
- **CSpect (debug):** a new game starts as before, but with the sword,
  plate armour +2, short bow and arrows equipped, the rest in the pack,
  and no "You wield" messages. The PC screen reads "Rogue the Dev".
- **Map:** common memory goes down (`dev_kit[]` moves out), and page 140
  holds the new code.

### Step 2: the creation screen

Covers R1-R5, R10-R13, D1, D8, D11 and D12.

- **Change:**
  - `chargen_run()`: the panels, the list and cursor, the preview's rows,
    and the keys: up/down, `n` (change name) and ENTER. Each move calls
    `chargen_make()`.
  - The class default names.
  - `ask_name()` moves from the title. `screen_title_run()` loses its
    parameter and its messages.
  - `game.c`'s `name` starts empty, and `play_start()` calls
    `chargen_run(last_class, name)`.
  - The welcome messages move to the end of `chargen_run()`.
- **Tests:** `test_ui` (the screen's rows, the keys, the default and typed
  names, ENTER, the class remembered, the title with only a key to wait
  for).
- **CSpect:** checks 1 to 6 of spec section 7.
- **Hardware:** check 7, a release build: only two classes, and no
  development kit.

### Step 3: closing docs

- `src/game/README.md`: a `chargen/` row, and the `actors` and `dev.h`
  rows.
- `docs/memory_map.md`: pages 140/141 and the common memory figures.
- `game/dev.h`'s comment: the kit is now Dev's.
- `character_creation.md`: the status line and the "As built" section.
- `docs/todo.md`: C1, C1.1 and C1.2 marked `[done]`.
