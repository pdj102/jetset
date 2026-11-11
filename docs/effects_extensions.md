# Effects extensions: design specification

Status: agreed (2026-11-11): D1 to D12 accepted as written. Covers to-do items E1 to E5 ([todo.md](todo.md)).
Built in steps ([effects_extensions_plan.md](effects_extensions_plan.md)): steps 1 to 5 built 2026-11-11, their CSpect checks passed (step 3: the effect list; blindness covered by tests only).

This spec adds level teleport, the teleport and summon ops, the gaps in the effect table,
four new statuses, and a to-hit attribute with the ops that go with it. It builds on
[effects.md](effects.md) and [attributes.md](attributes.md), and changes only what is
listed here.

## 1. Scope

| In | Later (why) |
|---|---|
| E1 Level teleport: the op, its limits, monsters, a parchment and a trap, delayed descent and recall | |
| E2.1 Teleport within the level, E2.6 Summon | E2.2 Light, E2.3 Map: there is no fog of war or light radius for them to change |
| | E2.4 Identify: needs S10 (an identified flag, hidden names) |
| | E2.5 Enchant: needs a bonus stored on each item; hooks come from the kind today |
| E3 The table's gaps (data) | |
| E4.1 Confused, E4.2 Frightened, E4.3 Paralysed, E4.4 Flying, E4.5 Waking, E4.6 Unconscious and blinded read by the rules | E4.7 Silenced: no status bit is left (low). E4.8 Invisible (low) |
| E5.1 `ATTR_TO_HIT`, E5.2 Detect traps, E5.3 Blink, E5.4 Mana drain | E5.2 Detect monsters: every monster is already visible. E5.5 to E5.9 (low) |

## 2. Requirements

### Level teleport (E1)

- **R1** `OP_LEVEL_TELEPORT` moves its target up, down or a random way, by the amount
  times the power in levels. The player arrives anywhere on the new level.
- **R2** The player's change of level is a request (`level_request()`), never made inside
  the op, because an effect can run inside an event handler or a timer (G-FLOW-1).
- **R3** Going up stops at depth 1, so a level teleport never ends the game
  (level_transitions.md R2). Going down stops at `LEVEL_DEPTH_BOTTOM`. Asked to go up at
  depth 1, or down at the bottom, it fizzles with a message.
- **R4** A monster leaves the level with `level_send()`. A persistent entity (the boss, the
  quest item's holder) is immune: the op fizzles.
- **R5** A parchment of teleport level, and a level teleport rune beside the teleport rune.
- **R6** Delayed descent and recall are timed effects with one firing, whose `on_end` is a
  level teleport. Recall at power 10 reaches depth 1 from anywhere.

### Pending ops (E2)

- **R7** `OP_TELEPORT` moves its target to random free floor on the level. On the player,
  the view follows.
- **R8** `OP_SUMMON` makes a random creature for the depth next to its target, 1 per power,
  at most 3.
- **R9** Light, map, identify and enchant stay pending (section 1).

### The table (E3)

- **R10** New rows, using only attributes and ops that already work: timed Dex, Con, Int
  and Wis buffs; poison, acid and magic wards; Aid; Shield; Hex; ability drains; burning,
  bleeding and corrosion; cure blindness; a worn Cha bonus; worn immunities.
- **R11** `EF_STRENGTH` is renamed `EF_RAISE_STR`.

### Statuses (E4)

- **R12** Four new statuses: confused, frightened and paralysed (conditions), and flying
  (not a condition).
- **R13** A confused creature's move sometimes goes in a random direction.
- **R14** A frightened creature can't melee, and an AI that is frightened moves away from
  its target.
- **R15** An unconscious or paralysed creature loses its turns.
- **R16** Damage wakes an unconscious creature, but not a paralysed one.
- **R17** A blinded AI can't see its target.
- **R18** The main screen shows every condition, not only poisoned.
- **R19** A flying creature sets off no trap, and doesn't fall through a trapdoor. A potion
  and boots of levitation grant flying.

### Ops and attributes (E5)

- **R20** A new attribute, `ATTR_TO_HIT`, is added to the attacker's roll. Blinded's -4
  becomes a modifier of `EF_BLIND`, not a special case in `attack_actor_to_hit()`.
  Bless, Bane and Heroism use it.
- **R21** `OP_DETECT` with `DETECT_TRAPS` finds hidden features near its target.
- **R22** Blink teleports the caster to a tile they aim at, in line of sight.
- **R23** `OP_MANA` with a negative amount drains MP. A negative amount on `OP_DAMAGE` or
  `OP_HEAL` does nothing (it closes effects.md, Known risk "Negative power on an op").

### Constraints

- **R24** No routine added to common memory (effects.md R20). Banked call depth stays
  within 8 (T-STACK-2).
- **R25** Every new branch that reads struct fields copies them into locals first
  (T-ZSDCC-1).

## 3. Design

### 3.1 New ops

| Op | `arg` | Amount | What it does |
|---|---|---|---|
| `OP_LEVEL_TELEPORT` | `LT_UP`, `LT_DOWN`, `LT_RANDOM` | Levels per power | Section 3.2 |
| `OP_DETECT` | `DETECT_TRAPS` | Radius per power | Each hidden feature within the radius (Chebyshev distance) is found: `feature_find()` and `EV_FOUND`, with the target as finder. Fizzles if none is found |
| `OP_BLINK` | - | - | Moves the target to the tile given to `effect_apply_at()` (section 3.6) |
| `OP_TELEPORT` (built) | - | - | `level_random_floor()`, then `nge_location_set()` and `EV_MOVED`. On the player, the camera centres on them, as on arrival |
| `OP_SUMMON` (built) | - | Creatures per power, at most 3 | Section 3.3 |
| `OP_MANA` (changed) | - | MP per power | Positive: `mana_restore()`. Negative: the new `mana_drain()`, which stops at 0 |

**Fizzles.** An op that can't act posts `EV_FIZZLED` (target: the entity, p1: the op). The
messages module tells the player "Nothing happens." if the target is the player, or
nothing otherwise.

### 3.2 Level teleport

1. If the target isn't on the level (`!level_holds()`), nothing happens. This is the case
   for `on_end` when a monster is parked (D3).
2. The direction: `LT_RANDOM` picks up or down, 50/50. If that way is closed (depth 1 going
   up, the bottom going down), it takes the other way.
3. The new depth is the current one plus or minus the amount, clamped to 1 to
   `LEVEL_DEPTH_BOTTOM`. If it equals the current depth, the op fizzles (R3).
4. Player: `level_request(depth, LEVEL_ARRIVE_ANY)`. A monster: if persistent, it fizzles
   (R4); otherwise `level_send(target, depth)`.

New rows:

| Row | Duration | Period × cycles | Op |
|---|---|---|---|
| `EF_LEVEL_TELEPORT` | Instant | | `on_start` LT_RANDOM 1 |
| `EF_DESCENT` | Timed | 15 turns × 1 | `on_end` LT_DOWN 1 |
| `EF_RECALL` | Timed | 15 turns × 1 | `on_end` LT_UP 1 (power 10: depth 1) |

`on_end` runs whenever an effect ends. Neither row grants a status, so no cure ends them
early. The player is never parked, so their delayed descent survives the stairs and
fires on the new level.

### 3.3 Summon

For each creature (1 per power, at most 3): `gen_creature_random(level_depth, LEVEL_AREA)`,
placed on the first free tile of the 8 around the target, checked with
`level_can_enter()` in a random starting order. With no free tile, the creature is
destroyed and the rest are skipped. If none was placed, the op fizzles. Summoned
creatures behave like any other of their kind.

### 3.4 Statuses

| Bit | Status | Condition | Read by |
|---|---|---|---|
| `0x10` | `STATUS_CONFUSED` | Yes | `act_move()`: in the resolve step (never `ACT_CHECK`), 1 in 3 replaces the direction with a random one and validates again; a blocked random step is a failed move ("You stumble.") |
| `0x20` | `STATUS_FRIGHTENED` | Yes | `act_melee()` refuses with `ACT_R_AFRAID` ("You are too afraid."); the AI's step goes to the free neighbouring tile furthest from its target |
| `0x40` | `STATUS_PARALYSED` | Yes | Loses its turns (3.5) |
| `0x80` | `STATUS_FLYING` | No | Trap springing (`feature_on_event()`) and the trapdoor's drop skip a flier |

`STATUS_UNCONSCIOUS` now loses turns (3.5) and wakes when damaged. `STATUS_BLINDED` fails
the AI's sight check (`ai_in_sight()`'s callers).

New rows: `EF_CONFUSE` (Wis save 12, 2 turns × 4), `EF_FEAR` (Cha save 12, 2 turns × 4),
`EF_PARALYSE` (Con save 14, 1 turn × 3), `EF_LEVITATE` (timed, 10 turns × 3) and
`EF_WORN_FLY` (worn status). Items: a levitation potion and levitation boots, with
existing sprites in another palette bank.

### 3.5 Lost turns and waking

- `player_on_turn()`: with `STATUS_UNCONSCIOUS` or `STATUS_PARALYSED`, the turn passes
  as a wait would, with no input read.
- `ai_on_turn()`: the same, before the brain runs.
- `game_on_event()`: on `EV_DAMAGED` with taken above 0 to an unconscious target,
  `effect_cure(target, STATUS_UNCONSCIOUS)`. A handler may apply effects (effects.md R5).

### 3.6 Blink

`effect_apply_at(ef, power, target, source, x, y)` stores (x, y) in page 120's data for
`OP_BLINK`, then calls `effect_apply()`. The cast and aim paths call it for a spell or item
whose effect is marked to need a tile. A spell with the new targeting mode `SPELL_TILE`
is aimed at a free tile within range and line (`act_line()`), never a creature. `OP_BLINK`
checks the tile can still be entered (`level_can_enter()`), or fizzles. New: `EF_BLINK`
(instant) and `SP_BLINK` (level 2, range 6).

### 3.7 `ATTR_TO_HIT`

| Attribute | Component | Combines | Range |
|---|---|---|---|
| `ATTR_TO_HIT` | stats | Added | -20 to +20, signed |

`stats_to_hit[]` (value, slot 0) and `stats_to_hit_base[]` (page 139), base 0 at creation.
`store()` clamps it. `attack_actor_to_hit()` adds `stats_to_hit[actor]` for an attacker
with stats, and drops its blinded case. `EF_BLIND` gains the modifier `ATTR_TO_HIT -4`
(its power is 1, so it is still -4). New rows: `EF_BLESS` (+1, 10 turns × 3), `EF_BANE`
(-1, Cha save 12) and `EF_HEROISM` (to-hit +1, HP max +2, 10 turns × 3).

### 3.8 The table's new rows (E3)

Timed (before `EF_TIMED_COUNT`; the instant and worn numbers move):

| Row | What it does | Save |
|---|---|---|
| `EF_GRACE`, `EF_ENDURANCE`, `EF_INSIGHT`, `EF_INTUITION` | Dex, Con, Int, Wis +1 per power, 5 turns × 4 (like Might) | |
| `EF_RESIST_POISON`, `EF_RESIST_ACID`, `EF_RESIST_MAGIC` | Resist, 10 turns × 3 (like the fire ward) | |
| `EF_AID` | HP max +2 per power, 10 turns × 3 | |
| `EF_SHIELD` | AC +1 per power, 5 turns × 4, no speed penalty | |
| `EF_HEX` | Vulnerable to magic, 5 turns × 4 | Wis 12 |
| `EF_DRAIN_STR` to `EF_DRAIN_CHA` | The ability -1 per power, 20 turns × 5 | Con 12 |
| `EF_BURNING` | 1 fire per power each turn, 4 times | Dex 12 |
| `EF_BLEEDING` | 1 slash per power every 2 turns, 5 times | Con 12 |
| `EF_CORROSION` | 1 acid per power every 2 turns, 4 times | |

Instant: `EF_CURE_BLIND`. Worn: `EF_WORN_CHA`, `EF_WORN_IMMUNE_FIRE`,
`EF_WORN_IMMUNE_COLD` and `EF_WORN_IMMUNE_POISON`. No item uses the E3 rows yet (D11).
Each new row gets a VFX colour in `anim_vfx.c`.

### 3.9 Bank depth

The deepest chain now: `nge_timer_advance()`, `ai_on_turn()`, `act_melee()`,
`effect_apply()`, then `gen_creature_random()` (summon on a hit) and its calls into nge,
or `level_send()` and `effect_end()` (level teleport on a hit). The build is checked
against the 8 allowed.

## 4. Decisions

- **D1 Level teleport encoding: direction in `arg`, levels in the amount.** One op covers
  random, delayed descent and recall. *Rejected:* separate ops for up and down.
- **D2 Fizzles post `EV_FIZZLED`.** The op knows why it failed, and the messages module
  owns the text. *Rejected:* a message call from the op, which would cross into another
  module's bank.
- **D3 Level teleport does nothing silently off the level.** `level_park()` ends limited
  effects and runs their `on_end`. *Rejected:* running `on_end` only on expiry, which
  changes the one ending path (effects.md D7).
- **D4 The four free bits: confused, frightened, paralysed, flying.** Silenced waits for a
  16-bit mask (E5.8).
- **D5 Blinded: the AI can't see, and the player keeps aiming.** *Rejected:* the player
  can't aim, fire or cast while blinded, which needs a refusal in three actions.
- **D6 Conditions on the main screen as short tags**, Psn Bln Slp Rt Cnf Frt Par,
  side by side on the status row. They are needed if more than one shows at a time.
- **D7 `ATTR_TO_HIT` in the stats component.** Only creatures attack with a roll that
  matters. It costs 100 bytes in slot 0 and 100 in page 139. *Rejected:* the attack
  component, which traps have too.
- **D8 Blink's tile is stored by `effect_apply_at()`.** *Rejected:* passing x and y through
  every `run_op()` call, for one op.
- **D9 Summon: random creatures for the depth, next to the target, hostile.** A summon
  that serves the caster needs factions (later).
- **D10 Frightened uses Charisma for its save,** giving Cha a use (G1). Bane uses it too.
- **D11 E3 is rows only,** with no items yet. Items named by E1.4 and E4.4 are added.
  *Alternative:* add potions or spells for the new buffs now.
- **D12 Detect traps only.** Detect monsters needs fog of war, as map does.

## 5. Tests (PC)

- `test_effects`: every new row is well formed; level teleport at depth 1, in the middle
  and at the bottom, for the player (the request) and a monster (sent, or immune if
  persistent); delayed descent's `on_end` firing on expiry and doing nothing while parked;
  teleport lands on free floor; summon next to the target, and with no room; detect finds
  only hidden features in range; mana drain stops at 0; negative damage and healing do
  nothing; `ATTR_TO_HIT` clamps.
- `test_act_move`: a confused move goes astray with a fixed `rand()` sequence.
- `test_act_melee`: refused while frightened; blinded still -4, now through the
  attribute.
- Lost turns, and waking on damage; a flier walks over a trap and it doesn't go off.

## 6. CSpect checks

1. Read a teleport level parchment at depth 1 (never up), in the middle, and at the bottom.
2. Delayed descent: start it, take the stairs, and see it fire on the new level.
3. Step on a teleport rune, a level teleport rune and a summoning rune.
4. Confused: walk and see some steps go astray.
5. Sleep from the gas trap, then take a hit: wake up.
6. Levitate over a known trap: it doesn't go off.
7. Blink to a tile in sight.
8. The main screen shows two conditions at once.

## 7. As built

Differences from the design, by plan step.

### Step 1: level teleport

- The parchment is the "stairway parchment": "teleport level parchment" is longer than an
  item name's 19 characters. It is red, a bank no other parchment uses, depth 3 to the
  bottom, rarity 3. The rune is `FT_LEVEL_RUNE`, "level teleport rune", DC 15, depth 4 to
  the bottom, rarity 0 like the other runes.
- A monster not owned by the level's area directly (one held by another entity) fizzles,
  as `level_send()` can't send it alone. An amount below 1 fizzles too.
- Persistent means `NGE_ENTITY_PERSISTENT`: the boss is immune, but a monster carrying
  the quest item isn't. It leaves, and the item, which is persistent, drops where it was
  (`level_send()`).
- Dev aids: the T key's traps include the teleport, level teleport and summoning runes;
  G puts a stairway parchment at the player's feet, and J starts a delayed descent.
- Tests: the generators' tests now link the level module, as the effects call it. The
  test support has a weak `screen_map_view` for the tests that don't define one.

### Step 2: teleport and summon

- The spec's `level_random_floor()` is the existing `level_free_floor()`, now banked and
  public in `level.h`. Its last fallback can share a tile with an item or a creature,
  when no floor is free.
- The camera isn't moved by the op: it follows the player (`nge_render_follow()`), so
  it centres on them where they land. `EV_MOVED` has (0, 0), so nothing slides.
- Teleport fizzles for a target off the map (held or parked).
- An instant effect whose op fizzled posts no `EV_EFFECT_STARTED`, so the player hears
  "Nothing happens." and not the effect's start text as well. A flag in common memory
  (1 byte) records the fizzle.
- The 8 directions round a tile are a table in page 121 (`effects_around`): a table in
  page 120 has the page in its address's high bits, which the assembler warns about.
  For the same reason the dev trap list in `player_bank.c` (step 1) is now a local
  array.
- Bank depth: a summon from a monster's hit would reach 8, the most allowed:
  `effect_apply()` at 4, `gen_creature_random()`, `effect_apply()` again for the new
  creature's innate effects, then `nge_effect_create()` and its calls. No hit hook
  summons; the rune's chain is 1 shallower.
- Tests: in `test_level`, not `test_effects`, as teleport reads a generated level.

### Step 3: `ATTR_TO_HIT`, the table's rows, mana drain

- `stats_to_hit[]` and `stats_to_hit_base[]` cost 150 bytes each, not 100:
  `NGE_ENTITY_MAX` is 150.
- The attribute table's `is_mask` became `combine`: `ATTRS_ADD`, `ATTRS_MASK` or
  `ATTRS_SIGNED`. A signed base keeps an `int8_t`'s bits in its `uint8_t` table, and
  `effects_raise_base()` keeps it within -128 to 127. `effects_base()` returns the bits.
- There is no `mana_drain()`: `OP_MANA` with a negative amount clamps it to the MP the
  target has and calls `mana_spend()`, which posts `EV_MANA_SPENT`. Page 110 has 154
  bytes left. A drain of nothing (no MP left) fizzles.
- `STATUS_BLINDED` is read by no rule until step 4; its -4 is `EF_BLIND`'s modifier.
- Timings the spec left open: Bane 5 turns × 4, like Weakness. The rows' names, for the
  PC screen: "Grace", "Endurance", "Insight", "Intuition", "Poison ward", "Acid ward",
  "Magic ward", "Aid", "Shield", "Hex", "Str drained" to "Cha drained", "Burning",
  "Bleeding", "Corrosion", "Blessed", "Bane", "Heroism".
- Placement: `EF_CURE_BLIND` follows `EF_CURE_ALL`, `EF_WORN_CHA` follows `EF_WORN_WIS`,
  and the worn immunities follow `EF_WORN_RES_MAGIC`. `EF_TIMED_COUNT` is 40 and
  `EF_COUNT` 84.
- The PC screen's effect list counted instances in two arrays of `EF_TIMED_COUNT`
  entries on the stack: 120 bytes at 40 kinds, over T-STACK-1's 64. It now looks for
  each kind in turn.
- Tests: `test_act`'s blinded attacks apply `EF_BLIND` instead of setting the status
  bit.

### Step 4: statuses, lost turns, waking, the tags

- Waking is `effects_on_event()` in the effects module, which `game_on_event()` calls
  for `EV_DAMAGED` only, not code in `game.c`: the effects module owns cures, the PC
  tests can reach it, and common memory keeps the smaller call.
- A lost turn: the player's timer fires but `player_turn` isn't set, so time runs on
  with no key read; an AI's turn returns before it senses anything. `STATUS_HELPLESS`
  names the two statuses.
- The confused stray picks one of the 8 directions with `rand()`, so a stray east of an
  actor going east moves it as normal. Blocked, the move fails with `ACT_R_STUMBLED`
  (attempted, the turn used), whatever blocks it: a stray into a creature is no attack.
  An AI strays too, silently.
- `act_status()` (in `act_shared.c`) tests any status for an actor with stats;
  `act_rooted()` uses it.
- Fleeing scores a tile by its larger axis from the goal, then the sum, in a byte. A
  frightened hunter only flees: no spell, potion or shot, and cornered it waits. The
  directions are `ai_dir_x()` and `ai_dir_y()`, shared from `ai_act.c`.
- Blinded: only `ai_sense()`'s look fails. `ai_sees()`, which the messages use for the
  player's view, is unchanged (D5).
- Tags: rows 17 to 19 of the stats window, 3 a row, in red.
- The help screen's abilities say what Con, Wis and Cha now resist.
- Dev aid: Z puts the next of confused, frightened, paralysed and asleep on the player.

### Step 5: flying

- The items are the "levitation potion" ("potion of levitation" is longer than 19
  characters), green, depth 3 to the bottom, rarity 2; and "boots of levitation", gold,
  feet, no AC, depth 6 to the bottom, rarity 1. Numbers 164 and 165, at the end.
- `feature_on_event()` already passed over fliers by kind (a block pass with
  `NGE_BLOCK_NO_FLOOR`, the bat); it now also passes over `STATUS_FLYING`. The trapdoor's
  drop in `act_spring()` skips a flier too, for a failed disarm, which sets the trap off
  without `feature_on_event()`.
- `STATUS_FLYING` isn't in `STATUS_CONDITIONS`, so a cure-all leaves it, but
  `effect_cure()` with `STATUS_FLYING` would end `EF_LEVITATE`: no item or op does.
- No tag on the main screen: the tags are conditions (D6).
- Dev aid: G also drops a levitation potion.
- Tests: in `test_act` (`test_traps()` and `test_trapdoor()`), where the trap tests are,
  not `test_feature`.

