# Spells: implementation plan

The steps that build what [spells.md](spells.md) specifies. The spec says what a spell
is and why, and this file says in what order, what changes, and how each step is checked.

Status: done 2026-11-03. Steps 1 to 3 passed in CSpect. Step 4: casting at range and
melee when out of MP passed in CSpect; healing itself and holding a bolt are covered by
`test_ai` only. Step 5 closed the docs.

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
