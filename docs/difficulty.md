# Difficulty scaling: requirements and design

Status: spec agreed, and the check script written (2026-11-09). Questions
Q1-Q5 were answered and decisions D1-D10 agreed the same day (section 8).

Covers to-do objective **D1** (D1.1, D1.2) in [todo.md](todo.md). Builds
on [player_character.md](player_character.md) (XP, levels, the level
table: D6 there) and the generator tables in `game/gen`. The check is
`scripts/balance.py` (`make balance`).

## 1. Goal

- A short list of **anchors**: the fixed numbers the game's difficulty
  hangs from (deepest depth, highest level, and so on).
- A few **principles**: how the player's power should grow against the
  dungeon's, written so a script can test them.
- A **check** that reads the game's real tables and says whether they
  still meet the principles. It keeps working as rooms, monsters, items
  and monster kinds are added, with no edits to the script.
- A **roadmap rule**: each new mechanic that scales (spell slots, item
  tiers, new monsters) says which curve it follows, and is checked
  against it.

## 2. Terms

| Term | Meaning |
|---|---|
| Depth | A dungeon level, 1 to `LEVEL_DEPTH_BOTTOM` |
| Level | The player's experience level, `PC_LEVEL_MIN` to `PC_LEVEL_MAX` |
| Anchor | A number the design fixes. Everything else follows from the anchors |
| Curve | A value by depth, or by level: XP available, XP needed, monster HP... |
| Pool | The kinds that can appear at a depth: those whose band (`.where`) holds it, weighted by rarity |
| Clear | The share of a depth's monsters (and treasure) the player takes |
| Level on arrival | The player's level, as a fraction, when they first reach a depth: 4.5 is halfway from 4 to 5 |

## 3. Requirements

### Anchors (D1.1)

- **R1** The anchors are in section 5.1, each with the constant it comes
  from. The C constant is the only copy: the script reads it.
- **R2** Design targets that aren't in the C (the pace, the tolerances)
  are in one place: `TARGETS` at the top of `scripts/balance.py`, each
  marked with its decision here.

### Principles (D1.2)

- **R3** The player who clears each depth is at level *d* on arrival at
  depth *d* (D2).
- **R4** The share of the dungeon's XP that comes after the player
  reaches the highest level is reported (D6).
- **R5** Every depth has a few monster kinds to meet (D7), and some
  items.
- **R6** The average monster's threat, against the player at the target
  level, stays in a band from depth to depth: 5 to 9 hits to kill the
  player (C5, D9).

### The check

- **R7** `scripts/balance.py` reads the tables from the C sources: the
  depths, the level XP table, the per-level counts in the generator, and
  every random creature, item and feature kind. Adding a kind, changing a
  band, or changing a count changes the result with no edit to the
  script.
- **R8** It prints one table by depth (pacing), the results of checks
  C1-C4, the power report C5, and with `--sim N` the spread C6.
- **R9** It exits 1 if a check fails, 2 if it can't read the tables, and
  0 otherwise. Warnings don't fail.
- **R10** It counts only what the generator places at random. Kinds with
  rarity 0 (placed by hand: the quest's, the dev kit's) are left out.
  A hand-placed monster worth XP must go into the model (M5) when one is
  added.
- **R11** `make balance` runs it. It is not part of `make` or
  `make test` yet (Q4).
- **R12** If the C changes shape so that a value can't be found (a
  constant renamed, a table moved), the script stops and names it. It
  never guesses or falls back to an old number.

### The roadmap rule

- **R13** A spec that adds or changes a scaling mechanic names the curve
  it scales against (section 6), and says what `make balance` showed
  before and after.

## 4. Decisions

- **D1 One script, reading the C; no copied tables.** The tables change
  often (more rooms, monsters, items and kinds are planned). A copy in a
  spreadsheet or a doc would drift. Python is already used by the build
  (`scripts/*.py`). **Rejected:** a PC test in C, which could call the
  real generator. It gives exact counts, but needs the level generator to
  build on the PC, and the report is harder to read. It could be added
  later for M1-M4 (Q5: not yet).

- **D2 Level *d* on arrival at depth *d*.** One level for each depth
  cleared, as D1.2 asks. Level 1 at depth 1, and the highest level
  (10) on arrival at the bottom (10). The bottom depth is then played at
  the highest level (Q1). **Rejected:** the highest level only at the end
  of the bottom depth, a level every 1.1 depths.

- **D3 The target player clears every depth (`clear` 1.0).** It is the
  "fully cleared" in D1.2. `--clear 0.75` shows a player who skips some.

- **D4 XP for gold is left out until treasure gives gold.** R12a in
  player_character.md gives 1 XP for each gold piece, but picking up
  treasure doesn't call `pc_gain_gold()` yet. So no XP comes from gold
  in play today. `--gold` shows what it will do. `TARGETS["gold_xp"]`
  turns on when that is built.

- **D5 Tolerances: ±0.5 of a level passes, ±1 warns, more fails.**
  Based on the spread (C6): a single game already varies by about ±0.8
  of a level from depth 4 down.

- **D6 XP after the cap is reported, not warned about (Q2).** XP past
  the highest level is wasted. Under D2 the bottom depth is played at the
  cap anyway, and its large XP is accepted as it is: depth 10 is the
  last fight, not a place to level. **Rejected:** a warning over 25%,
  and lowering the deepest monsters' XP or raising the top of the level
  table.

- **D7 At least 3 monster kinds at every depth.**

- **D8 Expected values, with an optional simulation.** The table uses
  averages, which are exact for sums of counts and rarities. `--sim N`
  rolls the counts and picks kinds as `level_gen.c` does, for the spread
  (10th, 50th and 90th percentiles). It uses Python's random, not the
  Next's, so it shows the shape, not exact games.

- **D9 The power check is simple: hits for the average monster to kill
  the player, 5 to 9 at every depth (Q3).** It uses the player's average HP at the target level
  (hit die, then the average roll per level, Constitution 10). The
  monster's average melee damage is taken as hitting every time. It
  doesn't count AC, to-hit, ranged attacks, spells, effects, or more
  than one monster at a time. So it compares depths with each other; it
  doesn't predict deaths. Outside the band warns rather than fails, as
  the measure is rough.

- **D10 The counting model is in one function.** `per_level()` says
  what a level holds (M1-M4). When the generator changes how it counts,
  for example monsters per room rather than per level, that function
  changes to match, and the spec's model with it.

## 5. Design

### 5.1 The anchors

| Anchor | Now | From |
|---|---|---|
| Deepest depth | 10 | `LEVEL_DEPTH_BOTTOM` (game/level/level.h) |
| Highest level | 10 | `PC_LEVEL_MAX` (game/components/pc.h) |
| XP for each level | 0, 20, 40, 80, 160, 320, 640, 1200, 2000, 3000 | `level_xp[]` (game/components/pc.c) |
| HP die per level | d8 | `PC_HIT_DIE` |
| Rooms per level | 5-10 | `LEVEL_ROOMS_MIN/MAX` (game/level/level_internal.h) |
| Monsters per level | 2-5 | `LEVEL_MONSTERS_MIN/MAX` |
| Items per level | 9-15 (4-10 until 2026-11-10, raised so spell books are found: [spells.md](spells.md) D28), plus locked rooms and containers | `LEVEL_ITEMS_*`, `LEVEL_LOCKS_MAX`, `LEVEL_FEATURES_MAX`, `LEVEL_CONTENTS_MAX` |
| Kinds, bands, rarity, XP, HP | 20 random creature kinds, 159 item kinds | `gen_creature_pdata.c`, `gen_item_pdata.c`, `gen_feature_pdata.c` |
| Pace | level *d* at depth *d* | `TARGETS` (D2) |

The rooms per level don't change the results today, because monsters and
items are counted per level, not per room. They are listed because the
plan is to scale counts with them (D10).

### 5.2 The model

What an average level at depth *d* holds:

- **M1 Monsters:** the mean of `roll_between(LEVEL_MONSTERS_MIN,
  LEVEL_MONSTERS_MAX)`, i.e. (min + max) / 2.
- **M2 A monster's XP:** the rarity-weighted mean XP of the creature
  pool at *d*.
- **M3 Item draws:** (items min + max) / 2, plus `LEVEL_LOCKS_MAX` / 2
  in locked rooms, plus container contents. Contents are the features per
  level (0 to `LEVEL_FEATURES_MAX`), times the share of containers in the
  feature pool, times the mean of 1 to `holds`, capped at
  `LEVEL_CONTENTS_MAX`.
- **M4 Gold:** item draws × the rarity-weighted mean gold of the item
  pool (a treasure's `.count`; 0 for other items).
- **M5 Hand-placed:** none today (R10).

XP gained at *d* = clear × (M1 × M2 + M4, if gold gives XP). The level on
arrival at *d* is the XP gained on depths 1 to *d*−1, read against the
level table, as a fraction between levels.

### 5.3 The checks

| Check | What | Fails or warns |
|---|---|---|
| C1 Pacing | Level on arrival at each depth against D2 | ±0.5 ok, ±1 WARN, more FAIL (D5) |
| C2 Cap | The depth where the highest level is reached; the share of XP after it | Report only (D6) |
| C3 Variety | Monster kinds in the pool at each depth | WARN under 3 (D7) |
| C4 Coverage | A depth with no random items | FAIL |
| C5 Power | Average monster HP, damage, AC; the player's HP; hits to kill the player | WARN outside 5-9 (D9) |
| C6 Spread | Level on arrival over N simulated games | Report only, with `--sim N` |

### 5.4 Running it

```
make balance                         # the report
python3 scripts/balance.py --sim 2000
python3 scripts/balance.py --clear 0.75 --gold
```

## 6. Curves for the roadmap (R13)

Each scaling mechanic picks one of these to follow, so it moves with the
anchors:

| Curve | By | Follows | Used by |
|---|---|---|---|
| XP needed | level | `level_xp[]` | levels; spell slots (S1: [spells.md](spells.md) D27, spell level L at character level 2L - 1); MP |
| XP available | depth | monster pools, counts (M1-M2), gold (M4) | C1, C2 |
| Target level | depth | D2: level = depth | anything that should match the player at a depth: monster HP and damage, lock difficulty, trap DCs, item tiers |
| Monster power | depth | pools: HP, damage, AC (C5) | new monster kinds: choose band and stats so C5 stays smooth |
| Rewards | depth | item pools: bands and rarity; gold (M4) | item tiers, potions, gold |

For example, a new monster kind is given a band of depths and an XP
value. Running `make balance` then shows its effect on C1, C2, C3 and C5.
The spell slot table (S1, [spells.md](spells.md) D27) is given by
level, and the target level curve says at which depth the player gets
each slot: the spell books are found from those depths (D28).

## 7. Today's results (2026-11-09)

`make balance`: 0 failed, 4 warnings.

| Depth | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Level on arrival | 1.00 | 2.34 | 3.66 | 4.57 | 5.32 | 6.27 | 7.16 | 8.26 | 9.51 | 10.00 |
| Monster kinds | 5 | 8 | 12 | 15 | 15 | 14 | 11 | 7 | 5 | 3 |
| Hits to kill the player | 4.4 | 6.8 | 7.3 | 8.9 | 8.3 | 8.6 | 7.6 | 7.2 | 6.3 | 5.8 |

- **C1:** on target, a little ahead at depths 3, 4 and 9 (WARN).
- **C2:** level 10 is reached while clearing depth 9, which meets D2.
  61% of the dungeon's XP comes after it: depths 9 and 10 hold about
  5,100 of 7,600 XP, because the deep monsters' XP (500, 850, 1,750)
  grows faster than the level table. Accepted (D6).
- **C3:** 3 kinds at depth 10, the minimum.
- **C5:** depth 1 is below the band at 4.4 hits (WARN). Then it gets
  easier to depth 4 (8.9, near the top), and harder again from depth 7
  down (5.8 at depth 10, near the bottom).
- **With XP for gold (`--gold`):** depths 3 and 4 FAIL, half a level to
  a level ahead. Gold has to be weighed when D4 turns on.
- **Spread (`--sim 2000`):** about ±0.8 of a level between the 10th and
  90th percentile from depth 4 down.

These are findings, not changes. Tuning is for later to-do items.

## 8. Questions

All answered (2026-11-09):

- **Q1 (D2)** Level 10 on arrival at depth 10, or at the end of it?
  **On arrival.**
- **Q2 (C2, D6)** The XP after the cap: reduce it, or leave it?
  **Leave it as it is.**
- **Q3 (C5, D9)** The band for hits to kill the player? **5 to 9.**
- **Q4 (R11)** Should `make test` run the check? **Not yet.**
- **Q5 (D1)** A PC test running the real generator to confirm M1-M4?
  **Not yet.**
