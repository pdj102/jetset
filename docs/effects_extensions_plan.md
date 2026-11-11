# Effects extensions: implementation plan

The steps that build what [effects_extensions.md](effects_extensions.md) specifies. The
spec says what and why; this file says in what order, what changes, and how each step is
checked.

Status: step 1 done 2026-11-11, its CSpect checks passed. Step 2 done 2026-11-11, its CSpect checks passed. Step 3 done 2026-11-11: CSpect check 1 passed; check 2 (blinded by a gold snake) not run. Step 4 done 2026-11-11, its CSpect checks passed. Step 5 done 2026-11-11, its CSpect check passed.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game playable.
- **Every step:** `make`, `make test`, and a look at the map for what the step cost. A
  new `__*` or `asm_*` symbol in common memory breaks R24.
- **The CSpect check closes a step.** The user runs it, and the next step waits for the
  result. A step with nothing to see is checked through a later one, as marked.
- **After each step:** anything that differs from the spec goes in an "As built" section
  in `effects_extensions.md`.

```text
 1 fizzles, level teleport, descent and recall
   |
 2 teleport and summon
   |
 3 ATTR_TO_HIT, the table's rows (E3), mana drain
   |
 4 confused, frightened, paralysed; lost turns, waking, blind AI; the status tags
   |
 5 flying --> 6 detect traps and blink --> 7 closing docs
```

Steps 1 and 2 come first because they change only the effects module and the data. Step
3 renumbers the table once, before the statuses add more rows. Steps 4 to 6 each touch a
different set of modules (actions and AI; features; spells and targeting).

## 2. Steps

### Step 1: fizzles, level teleport, descent and recall

R1 to R6, D1 to D3. E1.

- **Before:** note common memory free, and pages 109, 118, 120, 121, 122 and 136, from
  the map.
- **Change:**
  - `EV_FIZZLED` in `game/events.h` (target: the entity, p1: the op). In
    `messages_bank.c`, "Nothing happens." when the target is the player.
  - `OP_LEVEL_TELEPORT` 13 (`OP_COUNT` 14) with `LT_UP`, `LT_DOWN`, `LT_RANDOM` in
    `effects_defs.h`. In `effects_bank.c`, a `level_teleport()` that run_op() calls:
    section 3.2's four rules, with `level_holds()`, `level_request()`, the persistent
    flag and `level_send()`. A `fizzle()` helper that posts the event.
  - Rows: `EF_DESCENT` and `EF_RECALL` (timed, 15 turns × 1, `on_end` only), before
    `EF_TIMED_COUNT`; `EF_LEVEL_TELEPORT` (instant). The instant and worn numbers move by
    two; `effects.h`'s list and `anim_vfx.c`'s colours follow.
  - `IT_SP_LEVEL_TELEPORT` after `IT_BOOK_MASTER` (`IT_COUNT` 164): "teleport level
    parchment", the parchment sprite in a bank no other parchment uses, depth 3 to the
    bottom.
  - `FT_LEVEL_RUNE` after `FT_TRAPDOOR` (`FT_COUNT` 23): "level teleport rune", the trap
    look, hit hook `EF_LEVEL_TELEPORT` 1, depth 4 to the bottom, rarity 0 like the
    other runes. The dev trap key's list (`player_bank.c`) gains the teleport, level
    teleport and summoning runes, for the CSpect checks in this step and the next.
  - A dev key or the dev kit gives the new parchment and a way to start descent, for
    check 2 (whichever costs less in page 114; noted in As built).
- **Tests:** `test_effects`: the new rows are well formed; level teleport at depth 1, in
  the middle and at the bottom, for the player (the request and its depth) and for a
  monster (sent, or immune if persistent, with `EV_FIZZLED`); descent's `on_end` on
  expiry, and nothing while parked. `test_gen` (the item and feature counts),
  `test_ui` (the message).
- **CSpect:** spec section 6, checks 1 and 2, and the level teleport rune from check 3.
- **Depth:** the deepest new chain is a monster's hit on a monster with a level teleport
  hook: `nge_timer_advance()`, `ai_on_turn()`, `act_melee()`, `effect_apply()`,
  `level_send()`: 5 of 8.

### Step 2: teleport and summon

R7, R8, D9. E2.1, E2.6.

- **Change:** in `effects_bank.c`, `OP_TELEPORT` through `level_free_floor()` (the
  spec's `level_random_floor()`: it exists under this name), `nge_location_set()` and
  `EV_MOVED`; on the player, the camera centres as on arrival. `OP_SUMMON`: section
  3.3, with `gen_creature_random()` and `level_can_enter()`. `EF_TELEPORT` and
  `EF_SUMMON` lose "pending" in `effects.h` and `effects.md`.
- **Tests:** `test_effects`: teleport lands on free floor; summon next to the target, at
  most 3, and fizzles with no room.
- **CSpect:** check 3, the teleport and summoning runes, and a teleport parchment.
- **Depth:** summon on a hit: 5 calls to `effect_apply()`, then `gen_creature_random()`
  and its calls into nge, measured against the 8.

### Step 3: `ATTR_TO_HIT`, the table's rows, mana drain

R10, R11, R20, R23, D7, D11. E3, E5.1, E5.4.

- **Change:**
  - `ATTR_TO_HIT`: `stats_to_hit[]` in slot 0, `stats_to_hit_base[]` in page 139, a row
    of the attributes table, `store()`'s clamp to -20 to +20. `attack_actor_to_hit()`
    adds it and drops its blinded case; `EF_BLIND` gains the modifier.
  - Section 3.8's rows, and `EF_BLESS`, `EF_BANE`, `EF_HEROISM`. `EF_STRENGTH` becomes
    `EF_RAISE_STR` everywhere. Each new row gets a VFX colour.
  - `mana_drain()` in `mana.c`; `OP_MANA` takes either sign. `OP_DAMAGE` and `OP_HEAL`
    do nothing with an amount below 1.
- **Tests:** `test_effects` (every row well formed, the clamp, mana drain stops at 0,
  negative damage and healing), `test_act_melee` (blinded still -4, through the
  attribute), `test_attrs`.
- **Memory:** page 110 has 173 bytes free; `mana_drain()` and the clamp go there. If it
  doesn't fit, `mana_drain()` moves to page 120 as a static helper (As built).
- **CSpect:** nothing new to see; the PC screen still shows the abilities, and a blinded
  player still misses more (checked again in step 4).

### Step 4: statuses, lost turns, waking, the tags

R12 to R18, D4 to D6, D10. E4.1, E4.2, E4.3, E4.5, E4.6.

- **Change:**
  - `STATUS_CONFUSED`, `STATUS_FRIGHTENED`, `STATUS_PARALYSED` in `stats.h`, in
    `STATUS_CONDITIONS`. Rows `EF_CONFUSE`, `EF_FEAR`, `EF_PARALYSE`.
  - `act_move()`: the confused stray in the resolve step. `act_melee()`:
    `ACT_R_AFRAID`. The messages for both.
  - `player_on_turn()` and `ai_on_turn()`: lost turns. `ai_on_turn()`'s step away when
    frightened. `ai_in_sight()`'s callers fail while blinded.
  - `game_on_event()`: damage wakes the unconscious.
  - `screen_main`: the condition tags of D6, side by side.
- **Tests:** `test_act_move` (a confused move with a fixed `rand()` sequence),
  `test_act_melee` (refused while frightened), `test_ai` (lost turns, flees, blind),
  `test_pc` or `test_game` (waking), `test_ui` (the tags).
- **CSpect:** checks 4, 5 and 8. A dev way to confuse the player (the dev kit or a key),
  noted in As built.

### Step 5: flying

R19. E4.4.

- **Change:** `STATUS_FLYING` (not a condition). `EF_LEVITATE`, `EF_WORN_FLY`. In
  `feature_event.c`, trap springing and the trapdoor's drop skip a flier.
  `IT_POT_LEVITATE` and `IT_BOOTS_LEVITATE` after the step 1 parchment, with existing
  sprites in other palette banks.
- **Tests:** `test_feature` (a flier walks over a trap and a trapdoor), `test_gen`.
- **CSpect:** check 6.

### Step 6: detect traps and blink

R21, R22, D8, D12. E5.2, E5.3.

- **Change:** `OP_DETECT` with `DETECT_TRAPS`, `EF_DETECT_TRAPS` (instant).
  `effect_apply_at()` and `OP_BLINK`; `EF_BLINK`; `SPELL_TILE` in the spells module and
  the cast and aim paths, aimed with `act_line()`; `SP_BLINK` (level 2, range 6).
- **Tests:** `test_effects` (detect finds only hidden features in range; blink to a free
  tile, fizzles on a blocked one), `test_spell`, `test_target`.
- **CSpect:** check 7, and detect traps beside the dev traps.

### Step 7: closing docs

`effects.md` (the ops, the pending list, "Negative power on an op" closed),
`attributes.md` (`ATTR_TO_HIT`, the statuses), `features.md` (fliers), `spells.md`
(blink), `items.md` (the new items), `todo.md` (E1 to E5 status), and the spec's status.

## 3. Memory

| Step | Common free | Pages |
|---|---|---|
| Start | 4,257 | Page 102 7,987; 104 7,600; 110 8,019; 114 5,469; 116 6,799; 118 6,384; 120 2,922; 121 2,339; 124 1,325; 130 5,805; 136 1,080; 139 1,950. `BANK_56` 14,436 |
| 1 | 4,257 | Page 120 +414; 121 +144; 114 +93 (the dev keys); 118 +37. 136 and `BANK_56` unchanged |
| 2 | -1: 4,256, the fizzle flag | Page 120 +440; 121 +16; 114 +28 (the trap list a local); 102 +8 (`level_free_floor()` banked) |
| 3 | 4,256 | Page 121 +1,854 (25 rows); 138 +264; 50 +150 and 139 +150 (the to-hit tables); 120 +95; 126 +27; 110 +19, 154 left; 104 -20 |
| 4 | -27: 4,229 (lost turns, the waking call, 2 reasons, the Z key's counter) | Page 130 +368; 116 +246 (tags, help); `BANK_56` +240; 121 +218; 120 +80; 114 +64; 118 +59 |
| 5 | 4,229 | Page 121 +108; 114 +57 (the G key); `BANK_56` +17; 126 +2; 109 6,508 with the two items |
