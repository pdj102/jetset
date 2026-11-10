# Spells: implementation plan

The steps that build what [spells.md](spells.md) specifies. The spec says what a spell
is and why, and this file says in what order, what changes, and how each step is checked.

Status: done 2026-11-03. Steps 1 to 3 passed in CSpect. Step 4: casting at range and
melee when out of MP passed in CSpect; healing itself and holding a bolt are covered by
`test_ai` only. Step 5 closed the docs.

Part 2, spell slots and spell books (spells.md section 10, to-do S1 to S3): done
2026-11-10, steps 6 to 9 in section 4. Steps 6 to 8 passed their CSpect checks; step 9
closed the docs.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game playable and adds
  no half-built command.
- **Every step:** `make`, `make test`, and a look at the map for what the step cost. A
  new `__*` or `asm_*` symbol in common memory breaks R30.
- **The CSpect check closes a step.** The user runs it, and the next step waits for the
  result. A step with nothing to see is checked through a later one, as marked.
- **After each step:** update the docs listed in spells.md, section 4.13, for what the
  step built. Anything that differs from the spec goes in an "As built" section in
  `spells.md`.

```text
 1 mana: queue, ATTR_MP_MAX, regeneration, level-up MP
   |
 2 the spells module: table, known spells, learning
   |
 3 ACT_CAST and the m key --> 4 monsters that cast --> 5 closing docs
```

## 2. Steps

### Step 1: mana

R19 to R21, D10 to D12. No spell yet.

- **Change:** `NGE_EVENT_QUEUE` 32, in the game's and the tests' build flags.
  `ATTR_MP_MAX`, with `mana_mp_max_base[]` in slot 0, a row of `effects_game_attrs[]`
  and a part of `effects_game_store()` behind `MANA_IS()`. `EF_MANA_REGEN`, timed and
  innate, applied by `create()` to every kind with mana. `level_up()` raises maximum
  and current MP.
- **Tests:** `test_mana`, `test_effects`, `test_gen`, `test_pc`.
- **CSpect:** `E` raises maximum MP on the PC screen. Regeneration is checked in step 3,
  when MP can first be spent.

### Step 2: the spells module

R1 to R11, D1 to D8.

- **Change:** `src/game/spells/`, pair 134/135. `spell_defs[]` with section 4.12's rows,
  `COMPONENT_SPELLS`, the functions of section 4.3. `EV_CAST` and `EV_SPELL_LEARNED`.
  `gen_creature_kind_t` gains `spells[]`: the player knows Magic Bolt, the shaman Magic
  Bolt, Cure Wounds and Enfeeble. `level_up()` calls `spell_learn_level()`, and the
  message says what was learned.
- **Tests:** `test_spell` (new), `test_pc`, `test_gen`.
- **CSpect:** `E` says "You learn Cure Wounds.", and common memory free from the map.

### Step 3: `ACT_CAST` and the `m` key

R12 to R18, R22 to R24.

- **Change:** `act_cast.c` in `BANK_56`; `ACT_CAST`, `ACT_R_NO_MANA`, `ACT_R_NO_SPELL`;
  the messages; `EV_CAST`'s stimulus in `ai_on_event()`. `cast_command()` on `m`, the
  spell list in page 104, and `TARGET_CAST`.
- **Tests:** `test_act`, `test_ai`, `test_ui`, `test_target`.
- **CSpect:** spells.md section 7, checks 1 to 4 and 7.

### Step 4: monsters that cast

R25 to R28, D9.

- **Change:** `AI_CAN_CAST` in `ai_can()`, and `opt_cast()` first in `think_hunt()`. A dev
  shaman near the start, for the check only.
- **Tests:** `test_ai`.
- **CSpect:** check 6, and check 8: common memory free, before and after.

### Step 5: closing docs

The rest of section 4.13, and spells.md's status.

## 3. Memory

| Step | Common free | Pages |
|---|---|---|
| Start | 4,611 | BANK_56 10,484. Page 104 4,271; 110 7,008; 114 3,607; 116 5,541; 118 5,127; 120 5,327; 121 2,564; 124 1,268; 130 5,258 |
| 1 | -83: 4,528, the queue's 80 | Page 110 +148; 120 +100; 121 +41; 106 +32. Slot 0 +100 |
| 2 | 4,528 | Page 134 1,542; 135 598. Page 110 +62; 107 +84; 106 +28; 118 +123 |
| 3 | -99: 4,429 | BANK_56 +612. Page 104 +1,664; 114 +164; 118 +278; 124 +57; 116 +7 |
| 4 | 4,429 | Page 130 +545; 102 +115, 714 left in debug |

## 4. Part 2: spell slots and spell books

The steps that build spells.md section 10. Section 1's rules hold: one step a commit, made
when asked; `make`, `make test` and the map every step; the user's CSpect check closes a
step; differences go in an "As built" part of section 10.

```text
 6 spell book items: subclass, kinds, sprite, kits, bands
   |
 7 slots and book lists; level-ups stop teaching and announce slots
   |
 8 ACT_LEARN, the b key, the book screen --> 9 closing docs
```

Each step leaves the game playable. Between steps 7 and 8 a player can't learn anything
new, which is a gap in play, not a half-built command: nothing offers learning yet.

### Step 6: spell book items

R40 to R45, D28's items.

- **Before:** note common memory free, and pages 104, 109, 110, 116, 134, 135 and
  `BANK_56`, from the map. Run `make balance`.
- **Change:** `ITEM_BOOK` and `ITEM_COUNT` 17 in `gen_kinds.h`; `IT_BOOK_BASIC`,
  `IT_BOOK_ADEPT`, `IT_BOOK_MASTER` after `IT_QUEST`, and `IT_COUNT` 163. The book sprite,
  frame 84 and tag `book` in `assets/Sprites.aseprite`, drawn with the Aseprite command
  line (a first version, for the user to redraw if they like). The three rows in
  `gen_item_pdata.c`, with `.group` the `SPELL_BOOK_*` numbers, which this step adds to
  `spells.h` with `SPELL_BOOK_COUNT` and `SPELL_BOOK_MAX`. `{ IT_BOOK_BASIC, 0 }` in the
  Mage's and the Dev's kits.
- **Tests:** `test_gen` (the kinds, the subclass shares), `test_chargen` (the kits).
- **CSpect:** a new Mage carries a basic spellbook, shown in the inventory and on the
  creation screen's kit list; `u` says it can't be used; dropped, its sprite shows in blue.
  The `F` key down a few depths finds books in their colours (check 7).

### Step 7: slots, book lists, and the level-up

R33 to R39, D17 to D20, D26, D27.

- **Change:** in the spells module, `spell_slots_table[]` (D27) and `spell_books[]` (D28)
  in page 135; `spell_slots()`, `spell_free()`, `spell_slots_new()`, `spell_book()`,
  `spell_in_book()` in page 134. `learn` leaves `spell_def_t` and the rows;
  `spell_learn_level()` goes, and `level_up()`'s call to it. The level-up message in
  `messages_bank.c`.
- **Tests:** `test_spell` (section 10.6; the `learn` tests go), `test_pc` (no learning, 5
  events), `test_ui` (the message).
- **CSpect:** a Mage, `E` twice: "Welcome to experience level 2.", "You can learn more
  spells from a book." (as built, 10.9), and the same for level 3; no "You learn". A Fighter's `E`
  says only the welcome.

### Step 8: `ACT_LEARN`, the `b` key and the book screen

R46 to R52, D21 to D25.

- **Change:** `act_learn.c` in `BANK_56`; `ACT_LEARN` 20, `ACT_COUNT` 21, the 6 reasons and
  `ACT_R_COUNT` 51, their kinds in `act.c` and texts in `messages_bank.c`. The book mode
  in `screen_spell_bank.c`, `screen_book_run()` in `screen_spell.h`. In
  `screen_inv_bank.c`, the `ACT_LEARN` mode, `LIST_BOOKS`, `b` and ENTER in browse mode.
  `b` in `player_command()` and `item_command()`'s case. `b` on the help screen's keys
  page. In debug builds, a basic spellbook placed by the start (`level_dev_*`, as the dev
  shaman is), for the Fighter's check.
- **Tests:** `test_act`, `test_ui`, as section 10.6 lists.
- **CSpect:** spells.md section 10.6, checks 1 to 6, and check 8 (common memory).

### Step 9: closing docs

Section 10.5.10's spec changes; spells.md's status and "As built"; `make balance` after;
todo.md: S1, S2, S3 `[done]`.

### Memory, part 2

| Step | Common free | Pages |
|---|---|---|
| Start | 4,289 | Used: page 104 6,478; 109 6,248; 110 7,677; 116 6,630; 134 1,542; 135 798; `BANK_56` 13,672. `make balance`: 0 failed, 4 warnings |
| 6 | 4,289 | Page 109 +128. Sprites 85 of 128. `make balance` unchanged, also after the book rarities rose to 4, 10, 10 and items a level to 9-15 (check 4) |
| 7 | 4,289 | Page 134 +832: 2,374; 135 +163: 961; 110 -65: 7,612; 118 6,117 |
| 8 | -32: 4,257 | `BANK_56` +690: 14,362. Page 104 +1,072: 7,550 (642 free); 116 +169: 6,799; 118 +267: 6,384; 102 7,987 (205 free) |
