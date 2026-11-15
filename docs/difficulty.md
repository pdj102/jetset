# Difficulty scaling: requirements and design

Status: spec agreed, and the check script written (2026-11-09). Questions
Q1-Q5 were answered and decisions D1-D10 agreed the same day (section 9).
Reviewed after the creature, item and feature extensions (2026-11-15): the
pace fails (section 7). Options are in section 8; Q6-Q9 were answered
the same day, giving D11-D14, built the same day (to-do D1.3): `make
balance` passes with no warnings (section 7.1). After play test 1
(section 7.1a), Q10-Q14 answered, giving D15-D19; C7 (D15-D18, section 5.5)
built the same day: to-do D1.4.

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
  C1-C4, the power report C5, the danger report C7, and with `--sim N`
  the spread C6.
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
  turns on when that is built. *Changed (2026-11-15, Q7):* gold gives XP,
  built with D11 (D12).

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
  the measure is rough. *Changed (2026-11-15, D14):* 4 to 9 at depth 1.

- **D10 The counting model is in one function.** `per_level()` says
  what a level holds (M1-M4). When the generator changes how it counts,
  for example monsters per room rather than per level, that function
  changes to match, and the spec's model with it.

- **D11 The level table is set from the XP the dungeon gives (P1, Q6).**
  `level_xp[]` becomes 0, 50, 120, 230, 430, 790, 1,650, 3,850, 6,900,
  11,900 plus gold (D12): each level's figure is the XP, monsters and
  gold, that a player who clears every depth has on arrival at that
  depth. The monsters' B/X values and the counts a level stay. When C1
  fails again (new kinds, new counts), the table is re-set the same way.
  **Rejected:** P2 scaling the monsters' XP, P3 fewer monsters, P4 a mix
  (section 8.1).

- **D12 Gold gives XP (Q7).** The player's treasure goes to their gold
  when picked up, through `pc_gain_gold()` (player_character.md R12a: 1 XP
  a gold piece; action_pickup.md R3), and `TARGETS["gold_xp"]` is on. So D11 uses the table with gold: **0, 60, 150, 270, 500, 850,
  1,750, 4,000, 7,300, 12,500**. Without gold the player would be a little
  behind (2.7 at depth 3), still within D5.

- **D13 XP scaled by depth over level (P5) is later (Q8).** With hunger
  (G2), if farming on the stairs proves to be a problem.

- **D15 C7 Danger: the HP the player loses a fight (Q10).** Play test 1
  showed C5 misses how fast the player kills (section 7.1a). C7 measures
  the HP a fight costs, against the player's kit, as a share of their HP
  (section 5.5). It comes before any change to the monsters, so changes
  are measured, not guessed.

- **D16 The kit curve: what the player has had a fair chance to find.**
  At each depth, for each slot (weapon, and each armour slot), the best
  kind that a player who clears every depth has found with a chance of at
  least 50% by arrival, from the item pools (M3's draws, by rarity), or the
  class's starting kit if better. "Best" is the most damage a turn against
  the depth's mean monster AC for a weapon (to-hit counts, so a sword +1
  can beat an axe), and the most AC for armour. One kind a slot: no
  rings, potions or wands yet. **Rejected:** the mean of the pool (counts junk the player
  wouldn't wield), the best in the pool (a rare +3 sword nobody has).

- **D17 C7 is a report until it is calibrated.** Its numbers are checked
  against play first: play test 1 felt "easy" at depths 1-5 and "a
  challenge" from 6. A band (pass, warn) is set once a second play test
  agrees with it. C7 doesn't fail or warn yet.

- **D18 C7 is for the Fighter.** The class played; its starting kit and
  abilities from `gen_creature_pdata.c` and `chargen_bank.c`. The Mage,
  whose fights are spells, is later.

- **D19 After play test 1 (Q11-Q14).** No change to the monsters for now
  (Q11), until C7 is built and calibrated. A rest command is to-do A3
  (Q12). Hell hounds and wargs opening doors (Int 6) is left as it is
  (Q13). Exchange from the wear screen is to-do U4 (Q14).

- **D14 Depth 1's power is accepted (Q9).** 4.0 hits, as its monsters are
  the weakest, every blow is counted as a hit, and the player starts in
  armour. C5's band is 4 to 9 at depth 1 (`TARGETS`), 5 to 9 below it.
  To be confirmed in play on CSpect (section 8.5).

## 5. Design

### 5.1 The anchors

| Anchor | Now | From |
|---|---|---|
| Deepest depth | 10 | `LEVEL_DEPTH_BOTTOM` (game/level/level.h) |
| Highest level | 10 | `PC_LEVEL_MAX` (game/components/pc.h) |
| XP for each level | 0, 60, 150, 270, 500, 850, 1750, 4000, 7300, 12500 (D11, D12; 0, 20, 40 ... 3,000 before) | `level_xp[]` (game/components/pc.c) |
| HP die per level | d8 | `PC_HIT_DIE` |
| Rooms per level | 7-11 | `LEVEL_ROOMS_MIN/MAX` (game/level/level_internal.h) |
| Monsters per level | 6-10 | `LEVEL_MONSTERS_MIN/MAX` |
| Items per level | 11-18 (4-10 until 2026-11-10, raised so spell books are found: [spells.md](spells.md) D28), plus locked rooms and containers | `LEVEL_ITEMS_*`, `LEVEL_LOCKS_MAX`, `LEVEL_FEATURES_MAX`, `LEVEL_CONTENTS_MAX` |
| Kinds, bands of depths, rarity, XP, HP | 54 random creature kinds (since [creature_extensions.md](creature_extensions.md), 2026-11-14), 212 item kinds | `gen_creature_pdata.c`, `gen_item_pdata.c`, `gen_feature_pdata.c` |
| Monster bands | 12, a draw a band 1 time in 4 where one is in the pool | `gen_bands[]` (`gen_creature_pdata.c`), `gen_band_pick()`'s test (`gen_creature_bank.c`) |
| Pace | level *d* at depth *d* | `TARGETS` (D2) |

The rooms per level don't change the results today, because monsters and
items are counted per level, not per room. They are listed because the
plan is to scale counts with them (D10).

### 5.2 The model

What an average level at depth *d* holds:

- **M1 Monsters:** the mean of `roll_between(LEVEL_MONSTERS_MIN,
  LEVEL_MONSTERS_MAX)`, i.e. (min + max) / 2. A band uses as many of these
  draws as it has creatures, so bands don't change the count; a band cut
  short by the last draws is left out of the model.
- **M2 A monster's XP (Changed, 2026-11-14):** per creature, not per draw.
  A draw is a band with chance *p* (1/4) where the band pool at *d* (bands
  whose depths hold *d*, by rarity) isn't empty, else one kind of the
  creature pool by rarity. A band holds its leader and each member's mean
  count, (min + max) / 2, whatever the members' own depths. The mean XP a
  creature is (*p* × a band's XP + (1 − *p*) × a kind's XP) / (*p* × a
  band's size + (1 − *p*)). C5's HP, damage and AC are means the same way.
  Bands bring weaker members below their own depths, so they lower the
  mean ([creature_extensions.md](creature_extensions.md), step 11 in "As built").
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
| C7 Danger | The Fighter with the kit curve: turns a monster lives, HP lost a fight, fights a full HP (section 5.5) | Report only until calibrated (D17) |

### 5.4 Running it

```
make balance                         # the report
python3 scripts/balance.py --sim 2000
python3 scripts/balance.py --clear 0.75 --gold
```

### 5.5 C7 Danger (D15-D18, to-do D1.4)

For each depth *d*, the player at the target level (D2) with the kit
curve (D16), against each random creature kind in the pool, one at a time:

- **The player's blow:** a hit is 1d20 + the weapon's to-hit + the
  Strength modifier at or above the monster's AC; 20 always hits, 1
  always misses (action_melee.md, `attack_hits()`). So the chance is
  (21 − (AC − to-hit)) / 20, between 0.05 and 0.95. The damage is the
  weapon's mean (dice × (sides + 1) / 2 + to-damage) + the Strength
  modifier, at least 1.
- **The monster's blow:** the same, with its melee to-hit and Strength
  modifier against the player's AC: 10 (the Fighter's `.ac`) + the kit
  curve's armour.
- **Turns it lives:** its HP ÷ the player's mean damage a turn.
- **HP lost:** (turns − 0.5) × its mean damage a turn: the player strikes
  first, so on average half a turn less. Both at speed 10.
- **The kit:** D16's curve, slot by slot. A two-handed weapon
  (`ITEMF_TWO_HANDS`) leaves no shield, so both kits are tried, the best
  one-handed weapon with the best shield and the best two-handed one
  without, and the one that loses less HP is the depth's.
- **The player's HP:** the class's own, not C5's: its `.hp_max` (12 for
  the Fighter), then the hit die's mean plus the Constitution modifier a
  level (+2 at Con 14), at least 1, as `level_up()` does.
- **The depth's figure:** the mean over creatures, as M2 (`per_creature()`,
  bands included), of the HP lost, as a share of the player's HP, and its
  inverse, **fights a full HP**.
- **Not counted:** ranged attacks, spells and breath (the hell hound's),
  more than one monster at a time, statuses, regeneration and healing,
  rings and other kit, and speed. As C5, it compares depths; it doesn't
  predict deaths.

As built (2026-11-15, `make balance`):

| Depth | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Weapon (kit curve) | short sword | short sword | short sword | sword | sword | sword | sword | sword | battle axe | battle axe |
| Player AC | 14 | 14 | 14 | 14 | 15 | 15 | 21 | 21 | 21 | 22 |
| Turns a monster lives | 0.8 | 1.1 | 1.6 | 2.0 | 2.4 | 3.4 | 4.8 | 5.2 | 5.9 | 6.4 |
| HP lost a fight | 0.1 | 0.4 | 1.5 | 2.4 | 4.3 | 10.7 | 14.8 | 19.3 | 29.4 | 29.7 |
| Player HP | 12 | 18.5 | 25 | 31.5 | 38 | 44.5 | 51 | 57.5 | 64 | 70.5 |
| Share | 1% | 2% | 6% | 8% | 11% | 24% | 29% | 34% | 46% | 42% |
| Fights a full HP | 103 | 47 | 17 | 13 | 8.8 | 4.2 | 3.4 | 3.0 | 2.2 | 2.4 |

(A scratch prototype first gave 40 at depth 1 and 1.9 at depth 9: it
used C5's HP and chose weapons by damage alone, so a hand axe over the
sword.)

It agrees with play test 1: depths 1-3 cost almost nothing, depth 6 is a
step (6 fights to 3), and from depth 7 a fight is a third of the player's
HP and more. The cause is in its rows: the monsters' HP rises ninefold
(3.4 to 28.6) while the player's damage rises by half, and **the player's
to-hit doesn't rise with level** (only HP does), while monster AC rises
from 12 to 13.5. That is a finding for a later scaling task (a to-hit by
level, as classic D&D's), not a change here (Q11).

What the build adds to `balance.py`: the weapon and armour kinds read
from `gen_item_pdata.c` (`.attack`, `.data.armour_ac`, `.fits`), the
Fighter's abilities, AC and kit (`CR_FIGHTER`, `kit_fighter[]`), the
monsters' melee to-hit and Strength; a C7 table after C5's (the kit
curve's weapon and AC, turns, HP lost, share, fights a full HP); and
section 7 updated. R12 holds: what it can't read, it names and stops.

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

## 7. Today's results

### 7.1 After D1.3 (2026-11-15)

D11, D12 and D14 built. `make balance`: **0 failed, 0 warnings**.

| Depth | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Level on arrival | 1.00 | 2.02 | 3.13 | 4.16 | 5.12 | 6.12 | 7.06 | 8.05 | 9.01 | 10.00 |
| Without gold (`--no-gold`) | 1.00 | 1.84 | 2.66 | 3.69 | 4.71 | 5.82 | 6.88 | 7.93 | 8.89 | 9.89 |
| Clearing 75% (`--clear 0.75`) | 1.00 | 1.77 | 2.72 | 3.67 | 4.60 | 5.63 | 6.62 | 7.61 | 8.46 | 9.40 |
| Spread, 10th-90th (C6) | 1.0 | 1.7-2.3 | 2.6-3.6 | 3.6-4.5 | 4.7-5.5 | 5.6-6.5 | 6.5-7.4 | 7.6-8.6 | 8.4-9.5 | 9.5-10 |

- **C1:** within 0.16 of a level at every depth. A player who clears 75%
  is about half a level behind, and a level behind at depth 9 and 10 (WARN).
- **C2:** level 10 on arrival at depth 10 (the report says "while
  clearing depth 9", as the XP before depth 10 is the figure itself); 32%
  of the XP, depth 10's, comes after it (D6).
- **C5:** unchanged, all within the band, 4.0 at depth 1 (D14).
- **The spell books** (spells.md D28) now come at the depths their slots
  do: level 5 at depth 5, level 9 at depth 9.
- **C7 (D1.4):** fights a full HP fall from 103 at depth 1 to 8.8 at
  depth 5, 4.2 at depth 6 and 2.2 at depth 9 (section 5.5). Report only
  (D17).

### 7.1a Play test 1 (2026-11-15, CSpect, after D1.3)

A Fighter, killed by a hell hound at depth 9.

| Depths | What happened |
|---|---|
| 1-5 | Easy. Walked into monsters and hit them; HP never in danger. The best kit equipped as found; a sword +2 helped a lot |
| 6 | A room of 7 monsters was a challenge: lured them into a bent corridor, one at a time. Some healing items used |
| 7 | HP down to 12 of 50 twice. No ammo. Ran from a troll at under 5 HP, escaping by speed. Found boots of speed. Fell through a trap door to depth 8 |
| 8 | Arrived at level 7 (a level behind) with under 20 HP. Ran from a dragon. Explored avoiding monsters; killed a necromancer easily. A hell hound did a lot of damage, and opened a door. Level 8 |
| 9 | Arrived next to a troll and ran. A room with a necromancer, 2 hell hounds, a zombie and a mummy: down to 5 HP, ran. A locked door with no key. Killed by a hell hound |

Overall: well balanced, but depths 1-5 needed no tactics.

What it says against the model:

- **C5 ranks the depths right but misses the start.** Its easiest depths,
  4-6 (7.3 to 8.0 hits), were easy, and the first hard one, 7 (6.7), was
  hard. But depth 1 (4.0, "the hardest") was easy: C5 counts only how hard
  monsters hit, not how fast the player kills them. A depth-1 monster has
  3.4 HP, one blow from a sword. Danger is the HP lost a fight: the
  monster's damage times the turns it lives.
- **The kit isn't in the model.** A +2 sword and boots of speed changed
  the game; C5 assumes no kit at all.
- **A player who falls behind stays behind.** A trap door and running
  from fights cost a level by depth 8, and with it the HP to fight, so
  more running: the 75% player of section 7.1 (half a level to a level
  behind) is the one who died.
- **Recovery is the limit from depth 6.** Healing items ran out, food was
  left behind (it heals), and there is no rest. Section 8.4's healing
  check matters now.
- **Not balance:** hell hounds and wargs open doors, from Int 6
  (`AI_INT_OPEN`), which beasts shouldn't; the wear screen has no
  exchange (the inventory's has); no ammo found by depth 7.

### 7.2 Before D1.3 (2026-11-15)

After the creature, item and feature extensions: 54 random creature kinds
in 12 bands, 212 item kinds, 6-10 monsters and 11-18 items a level.
`make balance`: **7 failed, 2 warnings** (exit 1).

| Depth | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Level on arrival (target: the depth) | 1.00 | 3.26 | 4.50 | 5.46 | 6.35 | 7.27 | 8.56 | 10.00 | 10.00 | 10.00 |
| C1 | ok | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL | FAIL | WARN | ok |
| XP the level table wants from the depth | 20 | 20 | 40 | 80 | 160 | 320 | 560 | 800 | 1,000 | - |
| XP the depth gives | 51 | 69 | 113 | 200 | 355 | 857 | 2,208 | 3,076 | 4,982 | 5,731 |
| Gives ÷ wants | 2.6 | 3.5 | 2.8 | 2.5 | 2.2 | 2.7 | 3.9 | 3.8 | 5.0 | - |
| Monster kinds | 10 | 16 | 22 | 21 | 21 | 18 | 16 | 16 | 13 | 13 |
| Mean XP a monster | 6 | 9 | 14 | 25 | 44 | 107 | 276 | 385 | 623 | 716 |
| Hits to kill the player (C5) | 4.0 | 6.0 | 6.8 | 7.3 | 8.0 | 7.6 | 6.7 | 7.5 | 7.8 | 8.2 |
| Spread, 10th-90th (C6) | 1.0 | 2.8-3.7 | 4.1-4.9 | 5.1-5.9 | 6.1-6.7 | 6.8-7.8 | 7.8-9.4 | 9.7-10 | 10 | 10 |

"Wants" is the level table's step, `level_xp[d] - level_xp[d-1]`: what
keeps the player at level *d* on arrival at depth *d* (D2).

#### Findings

- **C1 Pacing fails.** The player is 1.3 levels ahead at depth 2, and
  stays 1 to 1.5 ahead down to depth 7. Level 10 comes on arrival at
  depth 8, two depths early, in more than half the games (C6).
- **The cause: every depth gives 2.2 to 5 times the XP its level step
  needs.** It has two parts:
  - *Monster count (all depths, about ×2.3).* Levels went from 2-5
    monsters to 6-10 ("Denser levels", `f7f7fc7`); the mean went from 3.5
    to 8. The level table (player_character.md D6) was on target with
    2-5 monsters (section 7.3): its note there says the pace is a guess, and
    tuning it changes one table.
  - *Deep monster XP (depths 6-9, a further ×1.5 to ×2).* The phase-1
    kinds of depths 5-10 ([creature_extensions.md](creature_extensions.md)
    step 7) are worth 500 to 2,300 XP. The mean XP a monster jumps
    2.6 times between depths 6 and 7 (107 to 276), while the level table's
    steps only grow 1.75 times (320 to 560).
- **C2 Cap.** Level 10 is reached while clearing depth 7; 83% of the
  dungeon's 17,600 XP comes after it (report, D6).
- **C3 Variety.** 10 kinds or more at every depth: well clear of D7's 3.
- **C4 Coverage.** Items at every depth.
- **C5 Power.** In the 5-9 band at depths 2-10, and smooth: the
  extensions fixed section 7.3's dip at depth 10 (5.8 then, 8.2 now).
  Depth 1 is under it at 4.0 (WARN): 8 HP against a mean of 2.0 damage.
  C5 uses the *target* level, so while C1 fails the real player is
  stronger than it shows: at depth 7 they are level 8.6, with 42 HP, and
  8.1 hits rather than 6.7.
- **XP for gold (`--gold`).** Small next to monster XP (734 in a game, 4%)
  but larger at depth 2 (35 gold against 69 XP). It moves the player a
  further 0.3 to 0.5 of a level ahead at depths 2-4.
- **The spell books follow the pace.** spells.md D28 puts the adept's
  book at depths 4-8 for the level 3 slot at level 5, and the master's at
  8-10 for the level 5 slot at level 9, both assuming D2. Today the player
  is level 5 at depth 3.4 and level 9 at depth 7.2, so the slots come a
  depth before the books. Fixing C1 fixes this too.

### 7.3 Earlier results

- **2026-11-09** (before the extensions; 2-5 monsters, 4-10 items a level): 0 failed,
  4 warnings. Level on arrival 1.00, 2.34, 3.66, 4.57, 5.32, 6.27, 7.16,
  8.26, 9.51, 10.00. Hits to kill the player 4.4 at depth 1, 8.9 at depth
  4, 5.8 at depth 10. 61% of the XP after the cap.
- **2026-11-14** (after creature_extensions.md): the same as section 7.

## 8. Options and recommendations

Each option was modelled with the script's own functions against
today's tables (level on arrival, clear 1.0):

| Option | Level on arrival, depths 1-10 | C1 fails | With gold |
|---|---|---|---|
| Today | 1.0 3.3 4.5 5.5 6.4 7.3 8.6 10 10 10 | 7 | 7 |
| **P1** Level table from the XP | 1.0 2.0 3.0 4.0 5.0 6.0 7.0 8.0 9.0 10 | 0 | 0 (3.4 at depth 3) |
| **P2** Monster XP scaled down | 1.0 2.0 3.2 4.2 5.1 6.0 7.0 8.2 9.3 10 | 0 | 7 |
| **P3** Fewer monsters (3-5) | 1.0 2.3 3.5 4.5 5.4 6.2 7.3 8.9 10 10 | 1 (+1 warn) | - |
| **P4** Table ×2, deep XP ×0.5 | 1.0 2.3 3.5 4.5 5.4 6.2 7.1 8.2 9.1 10 | 0 | 3 |
| **P5** XP × depth ÷ level, with a table | 1.0 2.1 2.8 3.8 4.9 5.9 7.0 8.2 9.2 10 | 0 | 0 |

### 8.1 Pacing (C1)

- **P1 Set the level table from the XP the dungeon gives (recommended).**
  `level_xp[]` becomes the XP before each depth, rounded:

  | Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  |---|---|---|---|---|---|---|---|---|---|---|
  | Today | 0 | 20 | 40 | 80 | 160 | 320 | 640 | 1,200 | 2,000 | 3,000 |
  | P1 | 0 | 50 | 120 | 230 | 430 | 790 | 1,650 | 3,850 | 6,900 | 11,900 |
  | P1 with gold (Q7) | 0 | 60 | 150 | 270 | 500 | 850 | 1,750 | 4,000 | 7,300 | 12,500 |

  One table in `pc.c` and its PC tests (`test_pc.c`, `test_ui.c` hold
  20 and 3,000); no game data changes. It is what player_character.md D6
  planned. It fits in 16 bits (D5 there). The B/X monster values stay.
  It takes the deep XP jump as it is, so the table's steps grow faster
  at the top (860, 2,200, 3,050) than a classic table's. The XP after
  the cap falls to 33%, the bottom depth itself (D6). The spread is the
  same as today's: about ±0.5 of a level from depth 4 down. The table is
  re-set whenever kinds or counts change enough to fail C1: that is
  what `make balance` is for.

- **P2 Scale the monsters' XP down instead.** ×0.4 to depth 5, and lower
  with depth to ×0.15 at depth 10. It meets C1 today, but it rewrites 54
  kinds' values, loses the B/X values (player_character.md D6), and
  rounds badly where XP is small (a rat's 5 becomes 2). With gold on, gold
  becomes most of the XP at shallow depths and C1 fails again at 7
  depths. **Not recommended.**

- **P3 Fewer monsters a level.** Back to 3-5 meets the shallow depths,
  but depths 8-9 are still ahead, and the levels empty out again: the
  denser levels were wanted for play. **Not recommended.**

- **P4 Halve both gaps: the table ×2, the XP of kinds from depth 6 ×0.5.**
  Keeps the table's classic doubling shape (0, 40, 80 ... 6,000) and
  smooths the jump at depth 7, but changes 19 kinds' XP, sits 0.3-0.5
  ahead at depths 2-5, and fails 3 depths once gold is on. An option if
  the classic shape matters more than an exact pace.

- **P5 XP scaled by depth over level, as Angband does.** A kill gives
  `xp × kind's first depth ÷ player's level`, plus a level table set to
  match (0, 40, 110, 210, 380, 700, 1,400, 3,000, 5,200, 8,500). A player
  who gets ahead gains less, and one who falls behind more. In the
  simulation that narrows the spread little (a player clearing 60% is
  0.1-0.3 of a level closer to target). Its real use is to make farming
  shallow levels on the stairs (level_transitions.md D7) give little.
  It costs a multiply and a divide on each kill (`player_on_pc_event()`;
  2,300 × 10 fits 16 bits), and the help screen must explain it.
  **Later, with hunger (G2), if farming proves to be a problem** (Q8).

### 8.2 Power (C5) at depth 1

The mean depth-1 monster does 2.0 damage (rat 2.0, kobold 2.5), and the
player has 8 HP (the hit die in full), so 4.0 hits. Options:

- **Accept it (recommended).** Depth 1 holds the weakest kinds, C5 counts
  every blow as a hit, and the player starts in armour. Record it as
  accepted (an amendment to D9: 4 to 9 at depth 1), and confirm in play
  on CSpect.
- Raise the player's level-1 HP (a bonus on top of the die).
- Make depth 1's monsters weaker: drop the fire beetle (3.5 damage) from
  depth 1.
- Make C5 count to-hit against the player's AC. More accurate, but D9
  chose a simple measure on purpose.

### 8.3 Gold (D4)

Gold isn't XP in play yet. P1 has a table for each case. **Recommended:**
decide D4 before setting the table, so that it is set once (Q7).

### 8.4 What the model doesn't count

None of these changes a result today. They are recorded so a later spec
knows they are missing.

- **Summons.** A necromancer or lich can summon up to 3 undead, worth XP
  when killed. Only at depths 7-10, where the player is at the cap
  anyway.
- **Allies' kills give no XP** (`player_on_pc_event()` needs the player as
  the killer). A player who lets allies fight levels more slowly.
- **Farming on the stairs** (level_transitions.md D7): each round trip
  makes a new level, so XP has no limit. P5 or hunger (G2) can close it.
- **Healing and food.** Healing potions against C5's damage, and food
  against the hunger clock (G2.4), have no check yet. When hunger is
  built, G2.4 should add one (R13).
- **The spell books' depths** against the slots (spells.md D28) are
  checked by hand. A check could test that each book's first depth is at
  or before the depth where the target level gives its spell level.

### 8.5 Recommended order

Agreed (2026-11-15: Q6-Q9, D11-D14). To-do D1.3, steps 1-4 done the same
day:

1. Gold gives XP (D12): picking up treasure calls `pc_gain_gold()`, with
   a PC test; `TARGETS["gold_xp"]` on.
2. The new `level_xp[]` (D11, D12), and its PC tests (`test_pc.c`,
   `test_ui.c`).
3. C5's band of 4 to 9 at depth 1 in `TARGETS` (D14).
4. `make`, `make test`, `make balance` (0 failed), and section 7 updated.
5. On CSpect: play a Fighter and a Mage down a few depths to confirm the
   pace and depth 1's danger.
6. Later: P5 with hunger (D13), and the extra checks of section 8.4.

### 8.6 After play test 1

Options, for Q10-Q14:

- **C7 Danger: the HP the player loses a fight (a new check).** The turns
  the average monster lives against the player at the target level (its
  HP ÷ the player's mean damage, with a weapon from the depth's item
  pool), times its damage, as a share of the player's HP. It replaces
  "hits to kill the player" as the measure of difficulty, and needs a
  kit curve: the best weapon and armour a player has found by each depth
  (the item pools, by rarity). C5 stays as a report. **Recommended
  first:** it measures what the play test felt, so changes can be tested
  against it, not guessed.
- **Harder depths 1-5**, once C7 shows by how much:
  - *H1 Tougher shallow monsters:* more HP for the kinds of depths 1-5, so
    they live 2-3 blows (data only; XP stays, so the pace holds).
  - *H2 More groups early:* a band a draw 1 time in 3 rather than 4 at
    depths 1-5, or new shallow bands (goblins, kobolds, jackals exist). The
    monster count stays, so the pace barely moves.
  - *H3 Fewer good items early:* +2 weapons from a deeper depth. Changes
    the reward curve, not the danger.
  - **Recommended:** H1 and H2 together, judged by C7.
- **Recovery:**
  - *R-a A rest command:* wait until HP and MP are full, stopped by a
    monster coming into view or being hurt. Regeneration already exists,
    so it is a player command, a loop of `ACT_WAIT`. Makes the game more
    forgiving everywhere, so it goes with H1/H2. Farming is not changed
    (resting gives no XP).
  - *R-b A healing check:* healing items and food a depth against C7's HP
    lost (section 8.4), so the deep depths can be survived without luck.
- **Falling behind:** P5 (D13) is the tool if it proves a problem: a
  player behind gains more. Not yet: one game is not enough.
- **Not balance, as to-do items:** beasts don't open doors (Int 6 hell
  hounds and wargs: `CREATURE_BEAST` excluded from `AI_CAN_OPEN`, or their
  Int to 5); exchange on the wear screen; ammo supply (with R-b).

## 9. Questions

All answered (2026-11-09):

- **Q1 (D2)** Level 10 on arrival at depth 10, or at the end of it?
  **On arrival.**
- **Q2 (C2, D6)** The XP after the cap: reduce it, or leave it?
  **Leave it as it is.**
- **Q3 (C5, D9)** The band for hits to kill the player? **5 to 9.**
- **Q4 (R11)** Should `make test` run the check? **Not yet.**
- **Q5 (D1)** A PC test running the real generator to confirm M1-M4?
  **Not yet.**

Answered (2026-11-15):

- **Q6 (C1, 8.1)** Which fix for the pace: P1 the level table, P2 the
  monsters' XP, P3 fewer monsters, or P4 a mix? **P1** (D11).
- **Q7 (D4, 8.3)** Will treasure give XP for gold? **Yes** (D12).
- **Q8 (8.1)** P5, XP scaled by depth over level: later, now, or never?
  **Later** (D13).
- **Q9 (C5, 8.2)** Depth 1's 4.0 hits: accept it, or change the player's
  HP, the depth-1 kinds or the check? **Accept** (D14).

Answered (2026-11-15, after play test 1):

- **Q10 (8.6)** Add C7, the HP lost a fight, with a kit curve, before
  changing monsters? **Yes** (D15-D18).
- **Q11 (8.6)** Harder depths 1-5: H1, H2, H3 or a mix? **No change for
  now** (D19).
- **Q12 (8.6)** A rest command and a healing check? **Rest: a to-do item**
  (A3, D19).
- **Q13 (8.6)** Hell hounds and wargs opening doors? **Leave as is** (D19).
- **Q14 (8.6)** Exchange on the wear screen? **A to-do item, under UI**
  (U4, D19).
