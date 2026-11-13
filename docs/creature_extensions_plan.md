# Creature extensions: implementation plan

The steps that build topic M ([todo.md](todo.md)) as
[creature_extensions.md](creature_extensions.md) specifies it. The spec says what and why;
this file says in what order, what changes, and how each step is checked.

Status: planned (2026-11-13). Step 1 done the same day.

## 1. How the steps work

- **One step, one commit,** made when asked. Each step leaves the game playable.
- **Every step:** `make`, `make test`, `make balance`, and a look at the map for what the
  step cost. Note each page's free bytes before the step (section 3). A new `__*` or
  `asm_*` symbol in common memory is a library routine linked by mistake.
- **Every step that adds kinds** reports `make balance` before and after (spec R14), in
  the spec's "As built".
- **zsdcc:** read the asm of every changed function that reads a struct field in a loop,
  returns a 16-bit mask, or uses a new bit constant (the known miscompiles, and the
  signed-shift trap). PC tests don't catch these.
- **The CSpect check closes a step.** The user runs it, and the next step waits for the
  result. The dev kit (`level_dev_bandit()`, `GAME_DEV_KIT`) places each step's new kinds
  out of sight on depth 1, so they can be met without going deep. That code goes when the
  step's check has passed, except for the bandit and shaman it places today.
- **After each step:** anything that differs from the spec goes in an "As built" section
  at the end of creature_extensions.md.

### 1.1 Tight spots

- **`BANK_56` (the actions) has 104 bytes free.** The steps change the actions only by
  a call or two (steps 2, 9, 10). The work goes in the pages that own it: gen (106),
  effects (120), AI (130). If a step still doesn't fit, it stops and asks. Making room
  in `BANK_56` (a file moved to a second section) is a change of its own.
- **Page 102 (the level generator) has 175 bytes free.** Step 11 picks bands in page
  106. If the placing doesn't fit in page 102, `place_random()` and the bands move to
  the level travel pair (136/137), which has room.
- **The event queue (16).** Generating posts events: 2 for each rooted creature today,
  and more for each kit item equipped (step 2). `test_level`'s queue check runs at every
  step that adds kinds or kits.

```text
Phase 1:  1 balance fix --> 2 kit, natural ranged --> 3 skirmishers --> 4 ability spells
                                                                              |
                            5 the six shapes (art) ----------------------> 6 kinds, depths 1-6
                                                                              |
                                                                         7 kinds, depths 5-10
Phase 2:  8 families, summon undead --> 9 death, struck hooks --> 10 ally spells --> 11 bands
                                                                                       |
                                                                                 12 closing docs
```

Step 5 (art) can run alongside steps 2 to 4. It needs your review of the previews
before anything goes into the `.aseprite` file.

## 2. Steps

### Step 1: `balance.py` reads melee damage `[done]` (2026-11-13)

Spec Q7, section 10.

- **Change:** `scripts/balance.py` reads `.to_damage` (the attack part's field, which was
  `.bonus`), and stops with `ParseError` if a kind has a `.melee` part it can't read
  (difficulty.md R12).
- **Result:** C5 now shows monster damage 1.8 to 8.3 a hit. 4.4 hits to kill the player
  at depth 1 (warns), 5.8 to 8.9 elsewhere (ok). C1 still fails at depths 2 to 8, as
  before.

### Step 2: the kit and the natural ranged attack (M1.1, M1.2)

Spec M1.1, M1.2, R6, R7, R12, R15, D2 to D4.

- **Change:**
  - `gen_internal.h`: `uint8_t kit[GEN_KIT_MAX]` (2) and `attack_kind_t ranged` in
    `gen_creature_kind_t`. A kit entry holds the item kind **plus 1**, written
    `GEN_KIT(IT_SHORT_BOW)`, and 0 means none. An entry left out of a row is 0, and 0
    is `IT_CLUB`, so the +1 lets the rows without a kit (most of them) say nothing.
  - `gen_creature_bank.c` (page 106), `create()`:
    - The ranged part gives `ATTACK_RANGED` (`attack_add()`), as the melee part does.
    - Each kit entry is made with `gen_item(kind, id)`, owned by the creature, then
      equipped with `act_equip(&a, ACT_DO)`, as `chargen_bank.c` does. A refused equip
      leaves the item carried, and asserts in a debug build.
  - `act_shared.c` (`BANK_56`), `act_strike()`: the hook it reads is `GEN_HOOK_AIM`
    when `use` is `ATTACK_RANGED` and `hooks` is a creature (`kind_class[]`), else
    `GEN_HOOK_HIT` as now. About 15 bytes.
  - `gen_kinds.h`: `CR_GOBLIN_ARCHER` 23, `CR_COUNT` 24.
  - `gen_creature_pdata.c`:
    - The goblin archer, from spec 6.1, with its kit (short bow, arrows) and slots
      `SLOT_WEAPON | SLOT_RANGED | SLOT_AMMO`.
    - The gold snake's spit (range 3, d3 poison). Its `EF_BLIND` moves from
      `GEN_HOOK_HIT` to `GEN_HOOK_AIM`.
  - Dev kit: a goblin archer and a gold snake out of sight.
- **Tests:**
  - **`test_gen`:**
    - Every kit entry is a real item kind, and fits a slot the kind has or is ammo
      for its kit launcher (R6).
    - A generated goblin archer has the bow in `SLOT_RANGED` and the arrows in
      `SLOT_AMMO`.
    - Kit, innate effects and mana regeneration fit in `LEVEL_CREATURE_OWNED` for
      every kind (R12).
    - A ranged part has range 2 or more and gives `ATTACK_RANGED`.
    - A creature's `GEN_HOOK_AIM` is set only with a ranged part.
  - **`test_act`:** the gold snake's shot uses no ammo, and applies `EF_BLIND` (its
    `GEN_HOOK_AIM`) and nothing from its bite. An archer's arrow still applies the
    arrow's hook.
  - **`test_level`:** levels at every depth over many seeds don't fill the event queue
    (R15). The events allowed while generating include the equip's.
- **Memory:** page 107 grows by 8 bytes a kind plus the new row (about 0.3K). Page 106
  by about 0.1K. `BANK_56` by about 15 bytes.
- **CSpect (check 2a):** on depth 1, the goblin archer wakes with its bow, shoots, and
  its body drops the bow and the arrows left. The gold snake spits from 3 tiles, and a
  hit blinds.

### Step 3: skirmishers, and `ai_can_t` to 16 bits (M1.3)

Spec M1.3, D7, R5, R16.

- **Change:**
  - `ai.h`: `typedef uint16_t ai_can_t`. `AI_CAN_SKIRMISH 0x0100`, `AI_CAN_SUPPORT
    0x0200`, with `AI_CAN_STEAL 0x0400` and `AI_CAN_PHASE 0x0800` reserved in a
    comment. The existing bits keep their values. `ai_kind_t` gains `uint8_t traits`,
    with `AI_TRAIT_SKIRMISH 0x01`.
  - `ai_pdata.c` (page 131): `ai_traits[NGE_ENTITY_MAX]`. `ai_add()` copies it from the
    kind, and `ai_remove()` clears it.
  - `ai_bank.c`, `ai_can()`: sets `AI_CAN_SKIRMISH` when `ai_traits[id]` has the
    trait, and the mask has `AI_CAN_MOVE` and `AI_CAN_RANGED` or `AI_CAN_CAST`.
  - `ai_think.c`:
    - `opt_stand_off()`, first in `think_hunt()`. With `AI_CAN_SKIRMISH`, seeing the
      target, and the target next to it, it takes the step that `opt_flee()`'s
      scoring likes best among the free tiles from which it still sees the target
      (`ai_sees()`, R16). If none, it does nothing, and the turn goes on to cast,
      attack and the rest.
    - `opt_flee()`'s step search becomes a helper both options call, so the code
      isn't repeated.
  - `ai_internal.h`, `ai_act.c`, `ai_think.c`: the context's `can` and the two locals
    become `ai_can_t`. They are already declared by the type.
  - `gen_creature_pdata.c`: `AI_TRAIT_SKIRMISH` on the goblin archer and the goblin
    shaman.
- **Tests:**
  - **`test_ai`:**
    - The existing exact `ai_can()` masks pass unchanged at 16 bits.
    - A skirmisher next to the player steps to a tile that sees them, and shoots
      next turn.
    - Rooted, or with no such tile, it attacks.
    - With its ammo gone, it loses `AI_CAN_SKIRMISH` and closes to melee.
    - A non-skirmisher with a ranged attack (the gold snake) never steps back.
  - **`test_gen`:** the masks for generated kinds.
- **Asm:** `ai_can()` returns its mask in HL, and `opt_stand_off()`'s tests on the high
  byte are right.
- **Memory:** page 130 about 0.2K, page 131 150 bytes.
- **CSpect (check 1):** the goblin archer backs away when the player closes, then
  shoots. The shaman does the same and casts.

### Step 4: ability spells, erratic movers and waking (M1.4, M2.1)

Spec M1.4, M2.1, R10, R11.

- **Change:**
  - `spells.h`:
    - `spell_def_t` gains `uint8_t flags`: `SPELL_F_MONSTER 0x01`, `SPELL_F_ALLIES
      0x02` (the second unused until step 10).
    - `SPELL_AI_SEEN` 3, `SPELL_AI_COUNT` 4.
    - `SP_FIRE_BREATH` 12, `SP_FROST_BREATH` 13, `SP_CONFUSE` 14, `SP_WAIL` 15,
      `SP_SHRIEK` 16, `SPELL_COUNT` 17.
  - `spells_pdata.c` (page 135): the five rows, all `SPELL_F_MONSTER`, level 1 or more
    so they cost MP. No book lists them.
  - `spells_bank.c`: `spell_learn()` refuses a `SPELL_F_MONSTER` spell for the player,
    and asserts.
  - `effects.h`, `effects_defs.h`, `effects_pdata.c`, `effects_bank.c` (pages
    120/121):
    - `EF_ERRATIC`: timed, for ever, innate, `STATUS_CONFUSED`. It goes at the end of
      the timed block, so `EF_TIMED_COUNT` moves and the instant effects are numbered
      again: nothing stores an `EF_*`, as the item tables name them by constant.
    - `EF_WAKE` (instant), with `OP_WAKE`: every AI entity within `amount` tiles of the
      target gets `ai_alert()`.
  - `ai.h`, `ai_bank.c`: `ai_alert(id)` sends `AI_STIM_NOISE`, waking a sleeper to idle,
    as hearing does.
  - `ai_think.c`, `opt_cast()`:
    - The start pick uses `rand() & 31`, folded below `SPELL_COUNT` (R11).
    - A third pass for `SPELL_AI_SEEN`: when it sees the target, a self spell is cast
      as it is, and a burst only with the target within `ACT_BURST_RADIUS`
      (`nge_map_within()`).
  - `gen_creature_pdata.c`: the bat's innate `EF_ERRATIC`. The dragon gets 12 MP and
    `SP_FIRE_BREATH`, and the shrieker 4 MP and `SP_SHRIEK`.
  - Dev kit: a dragon (it is depth 9's) and a shrieker beside a sleeping goblin, out of
    sight.
- **Tests:**
  - **`test_spell`:**
    - No book lists a `SPELL_F_MONSTER` spell, and `spell_learn()` refuses one for the
      player (R10).
    - Every monster-only spell has level 1 or more.
    - The start pick reaches every spell at 17 (R11).
  - **`test_effects`:**
    - `EF_ERRATIC` is innate, and the "innate is timed, for ever, no save" test
      covers it.
    - `EF_WAKE` wakes sleepers within range and not beyond.
  - **`test_ai`:**
    - A shrieker that sees the player shrieks.
    - A banshee-like test kind wails only with the target within 2 tiles.
    - A dragon breathes at the player.
- **Memory:** page 135 about 0.1K (a byte a row, 5 rows, names). Page 121, 2 rows.
  Pages 120 and 130 a little.
- **CSpect (check 2b):** the dragon breathes, catching itself without harm. The shriek
  wakes the goblin. A bat sometimes flies off its line.

### Step 5: the six shapes (M6.3)

Spec 6.5, sprite_style.md section 7. Can run alongside steps 2 to 4.

- **Grids:** `kobold.idle`, `dog.idle`, `bug.idle`, `zombie.idle`, `robe.idle` and
  `lizard.idle`, 2 frames each, in their default banks, to the masters and size classes
  in spec 6.5. Written once, in a grid file for `scripts/sprite_grids.py`.
- **Preview:** at 8x, next to the masters, on the floor and wall colours, in the flash
  and shadow banks, and in each bank its stand-ins use. **The step waits for your
  review here.**
- **Into the file:** frames at the end of `assets/Sprites.aseprite` with Aseprite's Lua
  command line. Then `make` regenerates `sprite_data.h` (patterns 86 to 97).
- **Check:** `make` passes the importer's hardware rules (sprite_import.md R1 to R14).
  You open the file in Aseprite. The shapes show in CSpect from step 6.

### Step 6: phase 1 kinds, depths 1 to 6 (M6.1)

Spec 6.1, 6.3, R1 to R4.

- **Before:** `make balance` noted.
- **Change:**
  - `gen_kinds.h`: `CR_KOBOLD` 24 to `CR_SKELETON_ARCHER` 37 (14 kinds), `CR_COUNT` 38.
  - `gen_creature_pdata.c`:
    - The 14 rows from spec 6.1 (all but the goblin archer and the gas spore), with
      the `SHAPE_KOBOLD`, `SHAPE_DOG`, `SHAPE_BUG` and `SHAPE_ZOMBIE` macros.
    - The narrower depths of every older kind (spec 6.3).
  - Families and mindless wait for step 8. Until then, zombies, skeletons and the
    shadow can be put to sleep, confused or frightened.
  - Dev kit: one of each new shape out of sight.
- **Tests:** **`test_gen`:** every new kind generates with its components; kits as in
  step 2. `test_level` over many seeds, with the queue check.
- **Memory:** page 107 about 1.0K, page 127 84 bytes.
- **CSpect (check 6):** the new shapes in their banks on the dungeon floor. A kobold
  pack and a stirge at depth 1. A harpy's song confuses, a ghoul's claws paralyse, and
  an orc archer skirmishes.

### Step 7: phase 1 kinds, depths 5 to 10 (M6.1)

Spec 6.2, R1, R2.

- **Change:**
  - `gen_kinds.h`: `CR_OWLBEAR` 38 to `CR_VAMPIRE` 50 (13 kinds), `CR_COUNT` 51.
  - `gen_creature_pdata.c`: the 13 rows from spec 6.2 (all but the orc warlord,
    necromancer, salamander, black pudding and lich), with `SHAPE_LIZARD`, `SHAPE_ROBE`
    and the `fx.orb` pattern for the will-o'-wisp.
- **Tests:**
  - **`test_gen`:** every depth has 6 to 22 random kinds (R1). From depth 2, every
    depth has a kind with a ranged part, a ranged kit or an offensive spell (R2).
  - **`test_ai`:** a hell hound's breath doesn't hurt its packmate (immune).
- **Memory:** page 107 about 1.0K, page 127 78 bytes.
- **CSpect (check 2):** a hell hound breathes, the player and its packmate are caught,
  and the packmate takes no damage. The basilisk's gaze paralyses from range. The hill
  giant throws.

### Step 8: families, mindless and summon undead (M4.3, part of M2.1)

Spec M4.3, M2.1's summons row, R13, Q4.

- **Change:**
  - `gen_internal.h`: `CREATURE_UNDEAD 0x02`, `CREATURE_BEAST 0x04`, `CREATURE_GIANT
    0x08`, `CREATURE_DRAGON 0x10`, `CREATURE_MINDLESS 0x20`.
  - `gen.h`, `gen_creature_bank.c`: `gen_creature_flags(kind)`, beside
    `gen_creature_hook()`.
  - `effects_bank.c` (page 120), `effect_apply()`: refuses `EF_SLEEP`, `EF_CONFUSE` and
    `EF_FEAR` on a creature whose kind is `CREATURE_MINDLESS`, as a save would, with no
    event. Check the bank depth from `act_cast()` (effects_extensions.md 3.9).
  - `EF_SUMMON_UNDEAD` (instant, `OP_SUMMON` with an undead-only argument), and
    `gen_creature_random_flags(depth, owner, flags)` in page 106, which picks only
    kinds with those flags. `OP_SUMMON` stops when fewer than 5 of the reserved
    entities are free (R13).
  - `SP_SUMMON_UNDEAD` 17 (self, `SPELL_AI_SEEN`), `SPELL_COUNT` 18.
  - `gen_creature_pdata.c`: the families in spec 6.1 to 6.3 on every kind. The
    necromancer and the lich rows (`CR_NECROMANCER` 51, `CR_LICH` 52, `CR_COUNT` 53).
- **Tests:**
  - **`test_effects`:** a mindless creature refuses sleep, confusion and fear, and still
    gets held and paralysed (Q4). `EF_SUMMON_UNDEAD` makes only undead kinds, and stops
    at the reserve.
  - **`test_gen`:** the families as spec 6.3.
- **CSpect (check 8):** a necromancer at depth 1 (dev kit) summons undead, casts Hold,
  and keeps its distance. A zombie can't be put to sleep.

### Step 9: death and struck hooks (M4.1, M4.2)

Spec M4.1, M4.2, D3.

- **Change:**
  - `effects.h`, `effects_bank.c` (page 120): `effect_apply_around(ef, power, x, y,
    radius, source)`, the effect on every creature within radius of a tile. A loop over
    the entities in use, as the level does.
  - Death: in `game_on_event()` (`game.c`), on `EV_DIED` of a creature whose
    `GEN_HOOK_USE` is set, it calls `effect_apply_around()` at the corpse's tile,
    radius 1, before the corpse goes. The hook and the location are read first, while
    the entity is still alive.
  - Struck: `act_strike()` (`BANK_56`) calls a new `effect_on_struck(target, attacker)`
    (page 120) after a melee hit that leaves the attacker alive. It applies the target's
    `GEN_HOOK_WORN`. That is one call and its test, about 15 bytes in `BANK_56`.
  - `gen_creature_pdata.c`:
    - The floating eye's struck `EF_PARALYSE` 2.
    - The gas spore, salamander and black pudding rows (`CR_GAS_SPORE` 53 to
      `CR_BLACK_PUDDING` 55, `CR_COUNT` 56).
- **Tests:**
  - **`test_act`:** hitting a floating eye in melee paralyses the attacker. A shot at
    it doesn't. A salamander struck sets its attacker burning.
  - **`test_effects`:** a gas spore's death burns every creature next to it, and none
    further away.
  - **`test_gen`:** a creature's `GEN_HOOK_USE` and `GEN_HOOK_WORN` name instant or timed
    effects.
- **Memory:** `BANK_56` about 15 bytes, page 120 about 0.2K.
- **CSpect (check 3):** hitting a floating eye in melee paralyses the player. A gas spore
  shot from range burns the goblins next to it.

### Step 10: spells on allies (M2.2)

Spec M2.2, D6, R9.

- **Change:**
  - `spells.h`, `spells_pdata.c`:
    - `SPELL_AI_ALLY` 4, `SPELL_AI_COUNT` 5.
    - `SP_WAR_CRY` 18 (burst, `SPELL_F_ALLIES`, `EF_BLESS`, `SPELL_AI_SEEN`).
    - `SP_MEND` 19 (direct, `EF_HEALING`, `SPELL_AI_ALLY`, monster-only): healing for
      a hurt ally.
    - `SPELL_COUNT` 20.
  - `act_shared.c` (`BANK_56`): when the cast's spell has `SPELL_F_ALLIES`, the area
    apply skips creatures with no AI component. The flag reaches it through
    `act_shape_t`, which `act_cast()` fills. About 15 bytes. If that doesn't fit, the
    skip goes in `effect_apply()`, with a flag argument.
  - `ai_bank.c`: `ai_can()` sets `AI_CAN_SUPPORT` when the caster knows a
    `SPELL_AI_ALLY` spell (`spell_next()`).
  - `ai_think.c`: `opt_cast()` tries its ally spells first. It aims them at the most
    hurt monster in sight at half its HP or less (a loop over the AI entities in
    sight), skipping itself.
  - `gen_creature_pdata.c`: the goblin shaman learns `SP_MEND`, and the orc warlord row
    goes in (`CR_ORC_WARLORD` 56, `CR_COUNT` 57: the full roster).
- **Tests:** **`test_ai`:** a shaman mends a hurt goblin it sees, and not the player. A
  war cry blesses the monsters around and never the player (R9). A caster with no ally
  spell has no `AI_CAN_SUPPORT`.
- **Memory:** `BANK_56` about 15 bytes, page 130 about 0.2K, page 135 2 rows.
- **CSpect (check 10):** a shaman mends a goblin the player has hurt. A warlord's war
  cry shows on its party (the glow, if V1.3 is built; otherwise their hits land more).

### Step 11: bands (M3)

Spec M3, R8, D5.

- **Change:**
  - `gen_internal.h`: `gen_band_t { leader, member[3], min[3], max[3], where }`.
  - `gen_creature_pdata.c` (page 107): the 12 bands in spec M3, `GEN_BANDS`.
  - `gen.h`, `gen_creature_bank.c`: `gen_band_pick(depth, kinds, max)` fills up to max
    kinds (the leader first) and returns how many. It picks a band 1 time in 4 when one
    is in the depth's pool, and otherwise returns 1 random kind. Members are rolled
    between their min and max, and are placed whatever their own depths.
  - `level_gen.c` (page 102): `place_random()` for monsters asks for a pick within the
    draws left. It places the first kind on free floor and the rest on free floor in
    the same room, with the leader's start state. If page 102 overflows, this moves to
    page 136 (section 1.1).
  - `scripts/balance.py`, `per_level()`: a monster draw is a band with the pool's
    chance. Its members' XP counts, and its size uses up draws (difficulty.md D10, M1
    and M2 in its model, updated).
- **Tests:**
  - **`test_gen`:** every band's members are real kinds. A band's largest size fits
    `LEVEL_MONSTERS_MAX` (R8). A band's leader is placeable at the band's depths.
  - **`test_level`:**
    - A band's creatures are in one room.
    - Monster counts stay within `LEVEL_MONSTERS_MAX`.
    - The entity budget and the event queue hold at every depth over many seeds.
- **Memory:** page 107 about 0.2K, page 106 about 0.2K, page 102 or 136 about 0.1K.
- **CSpect (checks 4, 5):** a goblin war band at depth 2 or 3: the archer stays back,
  and the shaman casts and mends. A shriek wakes a sleeping band in the next room.
  Depth 10 with the dev seed: about 13 kinds over several levels, and the frame time
  with a full band holds.

### Step 12: closing docs

- **creature_extensions.md:** status, and "As built" with every step's differences and
  balance numbers.
- **creatures.md:** the roster table (all 54 random kinds, depths, mechanics), and the
  "Later" column, now that many Laters are built.
- **ai.md:** the capability list at 16 bits, traits, `ai_alert()`. **ai_default_brain.md:**
  `opt_stand_off()` and the ally pass in the hunt state.
- **spells.md:** the monster-only spells, `SPELL_F_*`, `SPELL_AI_SEEN` and
  `SPELL_AI_ALLY`.
- **effects.md:** `EF_ERRATIC`, `EF_WAKE`, `EF_SUMMON_UNDEAD`, `OP_WAKE`, mindless refusal,
  and the creature meanings of `GEN_HOOK_USE`, `WORN` and `AIM`. **items.md** 4.4, the
  hooks table.
- **difficulty.md:** 5.1's kind count, and the band model in 5.2.
- **memory_map.md:** pages 102 or 136, 106, 107, 120, 127, 130, 131 and 135, and
  `BANK_56`. **sprite_style.md** 6.8, the pattern count.
- **todo.md:** M1 to M4 and M6 `[done]` with dates.

## 3. Memory

Free bytes before the plan (2026-11-13), and the expected total cost:

| Page | Used now | Free | Expected cost | Steps |
|---|---|---|---|---|
| `BANK_56` (actions) | 16,280 | 104 | about 45 bytes | 2, 9, 10 |
| 102 (level generator) | 8,017 | 175 | about 0.1K, or 0 if moved to 136 | 11 |
| 106 (creature generator) | 1,578 | 6.6K | about 0.5K | 2, 8, 11 |
| 107 (creature kinds) | 1,524 | 6.7K | about 3K | 2 to 11 |
| 120 (effects) | 4,774 | 3.4K | about 0.4K | 4, 8, 9 |
| 121 (effect definitions) | 5,597 | 2.6K | 3 rows | 4, 8 |
| 127 (sequences) | 144 | 8K | 204 bytes | 2 to 11 |
| 130 (AI) | 6,173 | 2.0K | about 0.6K | 3, 4, 10 |
| 131 (AI tables) | 900 | 7.3K | 150 bytes | 3 |
| 134 (spells) | 2,374 | 5.8K | a little | 4 |
| 135 (spell definitions) | 975 | 7.2K | about 0.2K | 4, 8, 10 |
| Common memory, slot 0 | | | nothing | |
| Sprite patterns | 86 | 42 | 12 | 5 |
