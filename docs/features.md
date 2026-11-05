# Features: design specification

How features work in Jetset: what the design requires, how it is built, and
why, and the set of features proposed for the dungeon.

Status: implemented (2026-11-04), steps 1 to 7 of section 7, each with its
CSpect checks passed the same day. The questions in section 10 were answered
before the build. Not yet tried on hardware. Section 12 lists where the build
differs from the design.

The data is not repeated here. The kinds are in
[gen_feature_pdata.c](../src/game/gen/gen_feature_pdata.c), the component in
[feature.h](../src/game/components/feature.h), and the effects in
[effects_pdata.c](../src/game/effects/effects_pdata.c). An entity's lifecycle,
ownership and clean-up are specified in [ecs.md](ecs.md). The actions'
pattern is in [actions.md](actions.md), and its section 6.9 points here.

## 1. Overview

**A feature is a fixed part of a level that does something:** a door, stairs,
a chest, a shrine, a boulder, a trap. Like items, **features are data.** Each
one is an instance of a **kind**, a constant record giving its name, its
looks, what it can do, what it blocks, and the parts it needs: hit points if
it can be broken, an attack if it strikes, an effect if it acts. Adding a
feature means adding a record. Code tests what a kind can do (`FCAN_*`),
never which kind it is.

**A feature is an entity on the map, drawn as a tile.** It is owned by its
area and has a location (ecs.md, 4.6). It is drawn into the tilemap, not as a
sprite, so it costs no hardware sprite. It never moves or changes owner. Its
per-entity data is one state byte (`FEAT_*`). Everything else stays in its
kind.

**What a feature does comes from a few abilities that combine.** A chest is
something that opens and holds items. A cracked statue holds items and can be
broken. A trapdoor is a trap that leads down. Each ability is one bit, with
one rule, in one place:

```text
 kind record (gen_features[], page 123)
      |  placed by the level generator: doors, stairs (rarity 0)
      |  or picked by depth and rarity (gen_feature_random())
      v
 feature entity: owned by the area, located, drawn as a tile
      |
      |  FCAN_OPEN       act_open / act_close / act_unlock   (built)
      |  FCAN_ASCEND,
      |  FCAN_DESCEND    act_ascend / act_descend           (built)
      |  FCAN_OPERATE    act_operate: use hook -> the operator
      |  FCAN_CONTAINER  opened or broken: drops what it owns on its tile
      |  FCAN_TRAP       stood on: attack, then hit hook -> the stander
      |  FCAN_DISARM     act_disarm: d20 + Dex vs its DC
      |  FCAN_ONCE       after one use: spent
      |  health part     struck like a creature; at 0 hp destroyed
      |  FEAT_HIDDEN     drawn as the floor until found (act_search, or it fires)
      v
 effect_apply(effect, power, target, feature)    (effects.md)
```

**Levels aren't kept** (level_transitions.md R4), so a spent shrine or a
disarmed trap needs no saving. A revisited depth is a new level.

## 2. Terms

| Term | Meaning |
|---|---|
| Kind | One record in `gen_features[]`, `FT_*`. Every feature of a kind is the same |
| Ability | One `FCAN_*` bit in a kind's `can`: what the feature can do |
| State | `feature_state[]`, the `FEAT_*` bits that are per entity: open, locked, hidden, spent, the lock's key |
| Look | A metatile and its palette bank. A kind has two: ready and used |
| Ready, used | Look 0: closed, full, unused, armed. Look 1: open or spent |
| Furniture | A feature that blocks walking: a chest, a shrine, a boulder |
| Trap | A feature with `FCAN_TRAP`: it fires at what stands on it |
| Hidden | `FEAT_HIDDEN` set: the player hasn't found it, and it is drawn as its cell's own look |
| Spent | `FEAT_SPENT` set: a feature with `FCAN_ONCE` that has been used |
| Fire | A trap acting on the creature on its tile |
| Contents | The items a container owns, held off the map |
| DC | A kind's difficulty: what d20 plus a modifier must reach to find or disarm it |
| Breakable | A kind with a health part: hit points, AC and damage masks |

## 3. Requirements

### The features

- **R1** Every feature is a kind's record. Telling one from another needs no
  code: a rule may depend on an ability, a state bit or a part of the record,
  never on an `FT_*`.
- **R2** Abilities combine. A rule for one ability works whatever else the
  kind can do.
- **R3** The door and the staircases keep working as they do today, from the
  same records with the new fields left at none.
- **R4** The game generates the kinds in section 5, at depths and rarities, as
  creatures and items are (items.md R3).

### Using features

- **R5** An actor can **operate** a feature next to it that has
  `FCAN_OPERATE`, for example praying at a shrine or drinking from a fountain.
  Its use hook is applied to the actor at the kind's power.
- **R6** A feature with `FCAN_ONCE` becomes spent after one use and refuses
  the next, with its used look.
- **R7** A **container** holds items that are generated with it. They drop on
  its tile when it is opened, or when it is broken.
- **R8** A container that opens can be locked, and is unlocked as a door is,
  by the same rule and action (actions.md 6.5).
- **R9** A **breakable** feature has hit points, AC and damage masks, is
  attacked as a creature is, and is destroyed at 0 hit points. It gives no XP
  and takes no effects.

### Traps

- **R10** A **trap** fires at a creature that moves onto its tile. If the kind
  has an attack, it is rolled against the creature's AC, and its hit hook
  applies to a hit that leaves the creature alive. If it has no attack, it
  always hits.
- **R11** A trap that leads down (`FCAN_DESCEND`) drops what it fires at to
  the next depth, arriving at random.
- **R12** A trap can start **hidden**. A hidden trap is drawn as the floor
  under it, and the player can't tell it from that floor.
- **R13** A hidden feature becomes known when it fires, or when a **search**
  finds it: d20 plus the searcher's Wisdom modifier against the kind's DC, for
  each hidden feature within one tile. A search always uses the turn.
- **R14** A known trap with `FCAN_DISARM` can be **disarmed** from its tile or
  next to it: d20 plus the disarmer's Dexterity modifier against its DC. On a
  success it is destroyed. Failing by 5 or more fires it at the disarmer.
  Otherwise the attempt fails and uses the turn.
- **R15 (New)** A creature that flies doesn't set off traps. A flier is one
  whose pass flags include `NGE_BLOCK_NO_FLOOR`, the flag for passing holes,
  so no new flag is needed. *(As built: no kind had it; the bat and the ghost
  now do, section 12.)*
- **R16 (New)** Every walking creature sets off traps, not only the player.
  Reason: one rule for every actor (creatures.md D4), and a monster lured
  onto a trap is a tactic. Monsters don't avoid traps (section 8).

### Generation

- **R17** A level has up to `LEVEL_FEATURES_MAX` random features (Q1),
  picked by depth and rarity.
- **R18 (Changed when built)** Furniture is placed only where it can't cut a
  path: on room floor whose eight neighbours are all room floor. Traps go on
  a free corridor tile 3 times in 4, else on free room floor, never on a
  doorway or a stairs tile. Reason: section 12.
- **R19** Nothing else is generated on a feature's tile, and no feature goes
  in a locked room (actions.md R35).
- **R20** A feature never cuts off the stairs or the quest item. Locks and
  hidden features may cut off only what is optional.

### Messages and looks

- **R21** The player is told what happens in words that suit the feature: the
  verb comes from the kind ("You pray at the shrine.", "You drink from the
  fountain."), and the effect's own text follows.
- **R22** A feature looks different when it is spent or open, and a known
  trap is shown on the map.

### Constraints

- **R23** New features use at most 16 of the 36 free metatiles (64 in all,
  28 used).
- **R24** Features take entities from the 100, as items do. The random
  features and their contents together are at most
  `LEVEL_FEATURES_MAX` + `LEVEL_CONTENTS_MAX` entities a level (Q1).
- **R25** The kind table and its names fit page 123 with at least 10% free.
- **R26** Generation copies only the record it needs onto the stack (items.md
  R20).

## 4. Design

### 4.1 The kind record

`gen_feature_kind_t` ([gen_internal.h](../src/game/gen/gen_internal.h)), with
the new fields marked:

| Field | Meaning |
|---|---|
| `name` | In the table's page |
| `info.look[2]` | Ready and used looks |
| `info.can` | `FCAN_*` abilities |
| `info.block_shut` | What it blocks while not open: `NGE_BLOCK_SOLID` for furniture and doors, `NGE_BLOCK_NONE` for stairs and traps |
| `state` | `FEAT_*` it starts in: `FEAT_HIDDEN` for most traps |
| `verb` *(new)* | `FVERB_*`, what the messages say when it is operated (4.11) |
| `dc` *(new)* | To find it when hidden, and to disarm it |
| `holds` *(new)* | A container's most random items, 1 or more; 0 if not a container |
| `health` *(new)* | `health_kind_t`. `hp_max` 0 for none: not breakable |
| `attack` *(new)* | `attack_kind_t`. Dice 0 for none: a trap that always hits |
| `hooks[]` | By `GEN_HOOK_*`. Features use `USE` (operated) and `HIT` (a trap's) |
| `where` | Band and rarity. 0 for kinds the generator places itself |

The record grows from 22 bytes to 36. With about 20 kinds that is about
0.7K, plus names (R25).

`gen_feature()` adds the health component when `hp_max` isn't 0, and a melee
attack when `dice` isn't 0, as `gen_creature()` and `gen_item()` do. A
container's contents are made by the caller (4.9), because it needs the depth.

### 4.2 Abilities

`info.can`, a byte, with all eight bits used:

| Bit | Ability | What reads it |
|---|---|---|
| `0x01` | `FCAN_OPEN` | `act_open()`, `act_close()`, `act_unlock()` (built) |
| `0x02` | `FCAN_ASCEND` | `act_ascend()` (built) |
| `0x04` | `FCAN_DESCEND` | `act_descend()` (built); a trap that leads down (4.8) |
| `0x08` | `FCAN_OPERATE` | `act_operate()` |
| `0x10` | `FCAN_TRAP` | The stood-on reaction (4.8) |
| `0x20` | `FCAN_DISARM` | `act_disarm()` |
| `0x40` | `FCAN_CONTAINER` | The opened reaction, and clean-up (4.9) |
| `0x80` | `FCAN_ONCE` | `act_operate()`, which marks it spent |

Being breakable isn't a bit. It is the health part, as it is for creatures.

### 4.3 State

`feature_state[]` keeps its layout. The two reserved bits come into use:

| Bits | Meaning |
|---|---|
| `0x01` | `FEAT_OPEN` |
| `0x02` | `FEAT_LOCKED` |
| `0x04` | `FEAT_HIDDEN` *(new)*: not yet found by the player |
| `0x08` | `FEAT_SPENT` *(new)*: used, with `FCAN_ONCE` |
| `0x70` | The lock's `KEY_*`, while locked |
| `0x80` | Free |

`feature_find()` and `feature_spend()` join `feature_open()` and the others:
they change the state without checking the rules, and keep the look and the
blocking in step. The actions check the rules first, as for doors.

### 4.4 Looks and blocking

`update()` in [feature.c](../src/game/components/feature.c) decides both:

| State | Drawn as | Blocks |
|---|---|---|
| Hidden | The cell's own look, through `level_redraw()` | Nothing |
| Open or spent | `look[1]` | Open: nothing. Spent: `block_shut` |
| Otherwise | `look[0]` | `block_shut` |

A spent shrine is still furniture, so it still blocks. Kinds share looks
wherever they can (R23). Two kinds with one look and one name can't be told
apart until they are used (D9).

### 4.5 Hooks

A feature has the creature's hook array, `hooks[GEN_HOOK_COUNT]`, and
`gen_hook_of()` already answers for it. Two hooks have a meaning for
features:

| Hook | Applied | To |
|---|---|---|
| `GEN_HOOK_USE` | When operated | The operator |
| `GEN_HOOK_HIT` | When a trap hits and leaves its target alive | The creature it fired at |

The effect is applied with the feature as its source. Effects never act on a
feature: it has no attributes (`affectable()` in `effects_bank.c`), so a fire bolt at
a boulder does nothing (Q6).

### 4.6 Actions

Three actions are added, with the numbers actions.md 6.1 reserved, each given
when it is built:

| Action | Context | Refused | Attempted | Success |
|---|---|---|---|---|
| `ACT_OPERATE` | A feature next to the actor or on its tile, by `target` or direction | `ACT_R_NOTHING_THERE`, `ACT_R_CANT_OPERATE`, `ACT_R_SPENT`, `ACT_R_OUT_OF_RANGE` | None | Use hook applied; spent if `FCAN_ONCE`; `EV_OPERATED` |
| `ACT_SEARCH` | The actor's tile and its eight neighbours | None | None | Each hidden feature there rolls; found ones get `EV_FOUND`. Finding nothing is still a success |
| `ACT_DISARM` | A known feature with `FCAN_DISARM`, next to the actor or on its tile | `ACT_R_NOTHING_THERE`, `ACT_R_CANT_DISARM` | `ACT_R_DISARM_FAILED`, `ACT_R_SET_OFF` | `EV_DISARMED`, then destroyed |

`find_openable()` in `act_open.c` becomes a shared `find_feature(a, can)`
that all six feature actions use. A rooted actor can operate, search and
disarm, as none of them changes its tile.

The existing actions need these changes:

| Action | Change |
|---|---|
| `ACT_OPEN` | None. A container empties in reaction to `EV_OPENED` (4.9) |
| `ACT_CLOSE` | A container that is open refuses with `ACT_R_CANT_OPEN`. Once it has spilled there is nothing to close it on |
| `ACT_DESCEND` | On a trap that leads down: arrives at random (`LEVEL_ARRIVE_ANY`), not at the up stairs |
| `ACT_MELEE`, `ACT_FIRE` | None: they already strike anything with the health component |
| `ACT_MOVE` | None: it already posts `EV_STOOD_ON` for each entity on the new tile |

### 4.7 Events and reactions

New events:

| Event | Target | Source | Params |
|---|---|---|---|
| `EV_OPERATED` | The feature | Operator | p1: its effect |
| `EV_TRIGGERED` | The trap | The creature it fired at | - |
| `EV_FOUND` | The feature | Finder (`NGE_EVENT_NONE` when it fired) | - |
| `EV_DISARMED` | The trap | Disarmer | - |

`feature_on_event()`, a new handler in page 132, is a reaction (G-ARCH-3):

- **`EV_STOOD_ON`** whose target is an alive trap and whose source is an
  alive walker: the trap fires (4.8).
- **`EV_OPENED`** whose target is a container: `inv_drop_all()` puts its
  contents on its tile. It works for any holder with a location.

A broken container needs no handler: clean-up already drops what a dying
entity owns (ecs.md 4.4, step 2).

### 4.8 Traps

A trap fires in this order:

1. **`EV_TRIGGERED`**, so the message comes first ("You step on a dart
   trap!").
2. **If it is hidden,** it becomes found (`EV_FOUND`) and is drawn.
3. **If it has an attack,** `act_strike()` rolls it with the trap as the
   attacker, as for a monster's bite. A trap has no stats, so it adds only its
   own to-hit. A hit leaving the target alive gets the hit hook.
4. **If it has no attack,** the hit hook is applied at once. Any saving throw
   is the effect's.
5. **If it leads down,** and the target is alive and not rooted: the player
   goes through `level_request(depth + 1, LEVEL_ARRIVE_ANY)` and anyone else
   through `level_send()`.

A trap stays armed after it fires. It goes only when it is disarmed, or
broken if it is breakable (a web). Firing a disarm attempt that fails by 5
runs the same steps, with the disarmer as the target, which may be next to
the trap and not on it.

**The DC.** One number serves for finding and disarming: d20 plus the
modifier must reach it. A thief's bonus (player_character.md R16) adds to the
roll later.

**The player's moves.** A step onto a known trap with `FCAN_DISARM` becomes a
disarm attempt, as a bump onto a locked door becomes an unlock (D12). A
known trap without `FCAN_DISARM` (a web) can be walked into.

### 4.9 Containers

- **Contents are generated with it.** After placing a container, the level
  generator makes 1 to `holds` items with `gen_item_random(depth, chest)`,
  owned by the chest, with no location. They are held as a creature's pack is
  (items.md 4.3).
- **Opening drops them** on its tile (4.7). An open chest blocks nothing, so
  the player steps onto it and picks them up as from the floor.
- **Breaking drops them** through clean-up.
- **Leaving the level** destroys them with the chest: features lose their
  location first, so nothing is dropped (level_transitions.md R13).
- **A locked container** uses `feature_lock()`, and its key is placed as a
  door's is (actions.md 6.12, R34): on free floor outside every locked room,
  never inside the chest it opens (Q4, D17). The lock's kind is rolled by
  depth as a door's. Chests are locked after the side rooms, from what is left
  of `LEVEL_LOCKS_MAX`, one chest in three at most.

### 4.10 Breakable features

A breakable feature has the health component from its record, and an
`ac` of 1 or 2, as a door's comment in `health.h` foresees.

- **The bump rule already attacks it:** `move()` in
  [player_bank.c](../src/game/player/player_bank.c) tests the health
  component before the door rule, so walking into a boulder strikes it.
- **No XP:** `player_on_pc_event()` gives XP only for `CLASS_CREATURE`.
- **No effects:** it has no attributes (4.5).
- **"The boulder is destroyed."** is already the `EV_DIED` message for
  anything that isn't a creature, and clean-up redraws the cell.
- **The AI never attacks one:** its targets are creatures. A monster treats a
  boulder as a wall.

### 4.11 Messages

`verb` picks the words, as `EV_USED` picks eat, drink or read by subclass:

| `FVERB_*` | Operated | Fired |
|---|---|---|
| `FVERB_NONE` | - | - |
| `FVERB_PRAY` | "You pray at the shrine." | - |
| `FVERB_DRINK` | "You drink from the fountain." | - |
| `FVERB_TOUCH` | "You touch the altar." | - |
| `FVERB_STEP` | - | "You step on a dart trap!" |
| `FVERB_FALL` | - | "You fall into a pit!", "You fall through a trapdoor!" |

Other messages:

| When | Player | Monster in sight |
|---|---|---|
| Spent | "The shrine is silent." (`ACT_R_SPENT`) | - |
| Found | "You find a dart trap!" | - |
| Disarmed | "You disarm the dart trap." | - |
| Failed | "You fail to disarm the dart trap." | - |
| Opened container | "The chest opens." "You see a dagger here." | "The orc opens the chest." |
| Fired at a monster | - | "The orc steps on a dart trap!" |

`set_article()` handles "a" and "an" (items.md 4.9).

### 4.12 Generation and placement

`gen_feature_random(depth, owner)` picks a kind with `GEN_PICK()`, in page
122 where the table is mapped (items.md 4.8).

`place_features(depth)` in `level_gen.c` runs after the locks and before the
items, so items keep out of furniture tiles through `scan_floor()`:

1. Roll the number of features, from 0 to `LEVEL_FEATURES_MAX`.
2. For each one, pick a kind. A kind that blocks needs an interior tile (R18):
   if none is found, it is dropped. A trap needs free floor on a room or
   corridor, not on a doorway.
3. Place it with `feature_place()`, which draws it. A hidden trap draws the
   floor.
4. Make a container's contents.

Where a kind goes follows from what it blocks, so the record needs no
placement field (D11).

### 4.13 Banking and memory

| Page | What is added | Estimate |
|---|---|---|
| 122 | `gen_feature_random()`, the health and attack parts | 0.2K |
| 123 | About 17 more kinds and their names | 0.7K |
| 132 | Finding, spending, firing, the reaction handler | 0.8K |
| `BANK_56` | The three actions and the shared finder | 1.0K |
| 102 | `place_features()` | 0.3K |
| 118 | The messages | 0.5K |
| 114 | The search and disarm keys | 0.1K |
| Common | A `case` per action in `act_do()`, a byte per reason | 0.05K |

These are estimates. The debug build must fit too.

## 5. The catalogue

Proposed kinds, with numbers to be corrected in play. Hit points and attacks
follow the creatures' scale (creatures.md section 1). "Pending" means the
effect's mechanic isn't built (items.md section 6). Those kinds have rarity 0
until it is (D15).

### 5.1 Built

| Kind | Abilities | Blocks | State | Depth | Rarity |
|---|---|---|---|---|---|
| door | `OPEN` | Solid when shut | Closed, maybe opened or locked by the generator | Doorways | 0 |
| staircase up | `ASCEND` | Nothing | - | One a level | 0 |
| staircase down | `DESCEND` | Nothing | - | One a level, not the bottom | 0 |

### 5.2 Furniture

| Kind | Abilities | Parts | Hook | Depth | Rarity | What it adds |
|---|---|---|---|---|---|---|
| chest | `OPEN`, `CONTAINER` | `holds` 2 | - | 1-10 | 6 | Loot behind a bump, and a use for keys away from doors |
| healing shrine | `OPERATE`, `ONCE` | `FVERB_PRAY` | Use: `EF_HEALING` 15 | 1-10 | 3 | A heal to save for later |
| cleansing shrine | `OPERATE`, `ONCE` | `FVERB_PRAY` | Use: `EF_CURE_ALL` 1 | 2-10 | 2 | Ends poison and holds |
| altar of might | `OPERATE`, `ONCE` | `FVERB_TOUCH` | Use: `EF_MIGHT` 2 | 3-10 | 2 | A boost to use before a fight |
| ancient altar | `OPERATE`, `ONCE` | `FVERB_TOUCH` | Use: `EF_VITALITY` 2 | 5-10 | 1 | A lasting reward, rare |
| fountain (clear) | `OPERATE`, `ONCE` | `FVERB_DRINK` | Use: `EF_RENEW` 1 | 1-10 | 2 | A gamble: one of three (D9) |
| fountain (clear) | `OPERATE`, `ONCE` | `FVERB_DRINK` | Use: `EF_MANA` 4 | 1-10 | 2 | |
| fountain (foul) | `OPERATE`, `ONCE` | `FVERB_DRINK` | Use: `EF_POISON` 1 | 1-10 | 2 | |
| boulder | - | 12 hp, AC 1, resists slash and pierce, immune to poison | - | 1-10 | 4 | Furniture to break or go round: blunt weapons pay |
| statue | - | - | - | 2-10 | 2 | Cover: blocks walking and sight |
| cracked statue | `CONTAINER` | 8 hp, AC 1, resists slash and pierce; `holds` 1 | - | 3-10 | 1 | Hidden loot for those who bump everything |

The three fountains are three kinds with one look and the name "fountain"
(D9). The bracketed words are for this table only.

### 5.3 Traps

All start hidden, block nothing, and can be disarmed unless noted.

| Kind | Abilities | Attack | Hook | DC | Depth | Rarity | What it adds |
|---|---|---|---|---|---|---|---|
| dart trap | `TRAP`, `DISARM` | +4, 1d4 pierce | Hit: `EF_POISON` 1 | 12 | 1-6 | 4 | A first trap: small, and teaches searching |
| pit | `TRAP`, `DISARM` | +8, 1d6 blunt | Hit: `EF_HOLD` 1 | 10 | 1-10 | 4 | Stuck in the pit: a hold while monsters come |
| fire rune | `TRAP`, `DISARM` | None | Hit: `EF_BOLT_FIRE` 6 | 14 | 4-10 | 3 | Fire resistance matters |
| trapdoor | `TRAP`, `DISARM`, `DESCEND` | None | - | 12 | 2-9 | 2 | Down a level, at random, without the stairs |
| spider web | `TRAP` (visible, can't be disarmed) | None | Hit: `EF_HOLD` 1 | - | 3-7 | 3 | 3 hp, weak to fire: cut it or burn it. Starts known |
| sleeping gas | `TRAP`, `DISARM` | None | Hit: `EF_SLEEP` 1 | 13 | 3-10 | 0 | Pending: no rule reads sleep |
| teleport rune | `TRAP`, `DISARM` | None | Hit: `EF_TELEPORT` 1 | 13 | 3-10 | 0 | Pending: `EF_TELEPORT` |
| summoning rune | `TRAP`, `DISARM` | None | Hit: `EF_SUMMON` 1 | 15 | 5-10 | 0 | Pending: `EF_SUMMON` |

### 5.4 Looks

| Metatile | Used by |
|---|---|
| chest shut, chest open | chest |
| shrine, shrine spent | both shrines |
| altar, altar spent | both altars |
| fountain, fountain dry | the three fountains |
| boulder | boulder |
| statue | statue, cracked statue (named apart) |
| trap | every known trap but the trapdoor and the web |
| trapdoor | trapdoor |
| web | spider web |

That is 13 metatiles, within R23's 16, drawn in tile_style.md's style.
Spent and open looks share the ready look where a kind can't be used up.

## 6. Decisions

- **D1 Abilities, not types.** A kind is a set of `FCAN_*` bits and optional
  parts, as an item's rules read flags and groups (items.md R2). A chest that
  can also be broken, or a trap that leads down, is a new record, not new
  code. *Rejected:* a subclass per feature (`FEATURE_CHEST`, `FEATURE_TRAP`),
  which makes a breakable chest a third subclass and puts a `switch` in every
  rule.
- **D2 The kind record grows; the entity doesn't.** Per-entity data stays one
  state byte, in slot 0. *Rejected:* a count per feature (a fountain's
  draughts), 100 bytes of slot 0 for what `FCAN_ONCE` covers. A count can be
  added with the first kind that needs it.
- **D3 A chest is a door that holds items.** It opens and unlocks with the
  existing actions (actions.md 6.9), its lock is a door's, and the bump rule
  already opens it. *Rejected:* `ACT_LOOT`, or opening by operating, which
  repeats what opening does.
- **D4 Contents are entities owned by the container.** That is how a pack
  works, and clean-up already drops a dead holder's items. *Rejected:*
  generating the contents when it is opened, which needs the depth and a seed
  at that moment, and makes a broken chest empty.
- **D5 An open container spills.** Its items go on its tile, which then blocks
  nothing, so taking them is picking up from the floor. *Rejected:* a
  container screen, a new UI for something the floor already does.
- **D6 Breakable means a health part.** Boulders and statues are struck,
  missed, damaged and destroyed by the code that does it to creatures, and
  the damage masks say what breaks them. *Rejected:* a "break" action with its
  own roll, which repeats melee.
- **D7 A trap fires as a reaction to `EV_STOOD_ON`** (actions.md 6.9), so
  every way onto a tile that posts it sets traps off, and the move action
  stays as it is. *Rejected:* a check in `act_move()`, which would make the
  move's result depend on the trap.
- **D8 A trap's effect is its hit hook, and its attack is optional.** The
  creature's rule (hit hook on a hit that leaves the target alive) covers a
  dart trap, and a rune with no attack simply always hits. *Rejected:* a new
  `GEN_HOOK_TRIGGER`, which adds 2 bytes to every creature and feature record
  for what `HIT` already says.
- **D9 An unknown kind is two kinds with one look and one name.** The fountain
  gamble needs no random table or identification. The effect's text tells the
  player what it was. *Rejected:* a random effect from a list per kind, which
  needs a table and a rule.
- **D10 Hidden is the player's knowledge, in one bit.** Monsters don't need to
  know, as they don't avoid traps (R16). *Rejected:* knowledge per creature.
- **D11 Placement follows from blocking.** Furniture needs an interior tile so
  that it can't cut a path (R18), and anything that blocks nothing can go on
  any free floor. *Rejected:* a placement field per kind, a rule the data
  could get wrong.
- **D12 A step onto a known trap is a disarm attempt.** It is the bump rule
  that turns a step into a door into opening it, so the player never walks
  onto a trap they can see by accident. *Rejected:* stepping on and firing it,
  which punishes a slip of the key. *Rejected:* asking for confirmation, which
  needs a prompt in the move command. Agreed (Q3).
- **D13 A disarmed trap is destroyed.** Its cell is redrawn as floor by
  clean-up, and its entity is freed. *Rejected:* a disarmed state with its own
  look, which costs a look and an entity for nothing.
- **D14 Fliers pass traps.** The test is the pass flags creatures already
  have (R15). *Rejected:* a flag per trap for whether it catches fliers, which
  only a web would want.
- **D15 Pending traps aren't generated.** Unlike items (items.md, "Inert
  finds"), a trap that fires and does nothing teaches the player that traps
  are harmless. Their records are complete, and their rarity is raised when
  their mechanic arrives.
- **D16 Effects don't act on features.** A feature has no stats, so it has no
  attributes or statuses (attributes.md D7). Damage reaches a breakable
  feature through attacks only. *Rejected:* attributes for features, so that
  a fire bolt burns a web: a cost in slot 0 for one kind (Q6).
- **D17 A locked chest's key is placed as a door's** (Q4). The rule of
  actions.md 6.12 then covers both, and a chest's lock is never part of the
  way to the stairs. The cap is shared and side rooms come first, so chests
  never take a lock from a door. *Rejected:* making the locked side room's
  item a chest, a lock behind a lock. *Rejected:* a cap of their own, which
  adds keys, and entities, a level.

## 7. Scope and order of work

Each step builds and passes the tests on its own:

1. **The record and the state:** the new fields, `FEAT_HIDDEN` and
   `FEAT_SPENT` in `update()`, the new abilities, and the existing kinds
   unchanged (R3).
2. **Breakable furniture:** the health part in `gen_feature()`, the boulder
   and the statues, `gen_feature_random()` and `place_features()`.
3. **Operate:** `ACT_OPERATE`, the shrines, altars and fountains, its key and
   messages.
4. **Containers:** the chest, its contents, its lock and key, the reaction
   to `EV_OPENED`, and the cracked statue's contents.
5. **Traps:** the reaction to `EV_STOOD_ON`, the dart trap, pit, fire rune and
   web.
6. **Search and disarm:** the two actions, hidden traps, the bump rule (D12).
7. **The trapdoor:** firing down, and `ACT_DESCEND` on a trapdoor.
8. **Closing docs:** the "as built" section here, and the documents in 7.1.

### 7.1 Documents changed with the build

- **actions.md:** R30 and 6.9 point here. 6.1's reasons and events take
  their final names.
- **events.md:** the four new events in the table of events and reactions.
- **feature.h, gen.h, gen_internal.h:** the comments on what features are and
  do.
- **memory_map.md:** pages 122, 123 and 132.
- **level_transitions.md:** the trapdoor as a way down.

## 8. Not built yet

| Mechanic | What it needs |
|---|---|
| Monsters avoiding traps | AI knowledge of a trap, D10 |
| Trapped chests | A container that fires its hit hook when opened |
| A chest mimic | A creature that looks like a chest (the mimic, creatures.md) |
| Secret doors | A door that starts hidden. Its hidden look must be a wall, so it needs a third look or a terrain change |
| Breaking doors | A door with a health part. The bump rule would then attack it before it tried to open it, so the rule must change first |
| Levers and portcullises | One feature acting on another: an ID kept in data (G-ENT-3), safe only if neither can go |
| Pushing boulders | A move that moves the boulder |
| Passive searching | A roll on each step, for a thief (player_character.md R16) |
| Bookshelves and the like | A container whose contents are of one subclass: `gen_item_random()` by subclass |
| XP for disarming | A rule in `player_on_pc_event()` |

## 9. Tests

PC tests:

- **`test_gen`:** every feature kind is well formed: name length, a power for
  each hook that names an effect, `FCAN_TRAP` kinds have a hit hook or an
  attack or lead down, `FCAN_OPERATE` kinds have a use hook and a verb,
  `FCAN_CONTAINER` kinds have `holds`, `FCAN_DISARM` kinds have a DC, blocking
  kinds that are traps don't exist, pending kinds have rarity 0. What
  `gen_feature()` creates: health and attack components from the parts.
  `GEN_PICK()` over the feature table.
- **`test_feature`** *(new, or in `test_act`)*: the looks and blocking of each
  state; found and spent; the lock on a chest.
- **`test_act`:** operate applies the hook and spends a once kind, and refuses
  a spent one; search finds by the roll and always uses the turn; disarm's
  three outcomes; a rooted actor can operate; descend on a trapdoor arrives
  at random.
- **`test_effects`** or **`test_act`:** a trap fires at a walker, not at a
  flier; a hit hook after a hit, not after a miss; a trap with no attack
  always applies its hook; a found trap is drawn.
- **`test_level`:** furniture only on interior tiles; no feature in a locked
  room, on a doorway or on stairs; the stairs are always reachable; contents
  are owned by their container; the entity peak at every depth over many
  seeds stays under 100; a chest's contents go with it when the level is left.

CSpect checks:

1. Bump a chest: it opens, its items lie on its tile, and they can be picked
   up. A locked chest unlocks with its key.
2. Pray at a shrine when hurt: healed, "The shrine is silent." the second
   time, and drawn spent.
3. Walk into a boulder until it breaks: "The boulder is destroyed.", the
   floor is drawn, and no XP is gained.
4. Search next to a hidden dart trap until it is found; step into it and
   disarm it; a failure by 5 fires it.
5. Fall through a trapdoor: the next depth, at a random place.
6. A monster walking over a pit is held, with a message if it is in sight.

## 10. Questions (answered 2026-11-04)

- **Q1 How many? Answered: as proposed.** `LEVEL_FEATURES_MAX` 4 and
  `LEVEL_CONTENTS_MAX` 4, so at most 8 more entities a level, and one more
  for each locked chest's key, within `LEVEL_LOCKS_MAX`. `test_level`
  measures the peak.
- **Q2 Should monsters set off traps? Answered: yes** (R16), with no
  avoidance, so luring them is a tactic.
- **Q3 A step onto a known trap? Answered: a disarm attempt** (D12). A known
  trapdoor can't be stepped onto on purpose, only disarmed.
- **Q4 Locked chests? Answered: locked, with the key elsewhere** (4.9, D17).
  The locked side room's item stays an item.
- **Q5 The catalogue? Answered: the kinds in section 5, with the fountain
  gamble** (D9).
- **Q6 Fire and webs? Answered: acceptable for now.** A web is cut or
  struck, not burnt (D16).
- **Q7 Keys? Answered:** `o` operate, `s` search, `D` disarm.

## 11. Known risks

- **Entities.** Every feature, and every item in a container, takes one of the
  100 (ecs.md 8). Doors already take up to 12.
- **The event queue.** A trap firing posts `EV_TRIGGERED`, `EV_FOUND`,
  `EV_HIT` and `EV_DAMAGED` at least, and its effect's events, within the
  move's frame. Several monsters on traps in one frame could come close to
  the 16-event queue.
- **A trap that kills during a move.** The stander may be dying when later
  handlers run, so every reaction tests `NGE_ENTITY_IS_ALIVE()` (ecs.md 4.4).
- **Falling with the quest item.** A trapdoor takes the player down with it.
  That is allowed (winning.md), but test that the quest item stays carried.
- **Blocked monsters.** A boulder in a room makes a monster's straight path
  fail, as a wall does. The AI copes with walls, but test a hunter next to
  one.
- **Inert finds.** Until sleep, teleport and summoning are built, their traps
  aren't generated (D15).
- **zsdcc.** `FCAN_ONCE` is `0x80` and must be written as a constant, not
  `1 << 7` (the signed-char shift trap). Read the asm of `update()` and the
  firing after changing them ([toolchain.md](guidelines/toolchain.md)).

## 12. As built: differences from the design

### Behaviour

- **R15, flying (changed).** No creature kind passed `NGE_BLOCK_NO_FLOOR`, so
  the rule had nothing to act on. The bat and the ghost now pass it
  (creatures.md, answered 2026-11-04): they fly over traps, and would over
  holes, which the generator doesn't make yet.
- **R18, where traps go (changed).** A trap goes on a free corridor tile 3
  times in 4 (`free_corridor()`), else on room floor. Reason: with "room or
  corridor" alike, CSpect check 3 met no hidden trap in 8 levels. About 0.6
  traps a level are made, and most now lie where walkers go.
- **D12, a step into a known web strikes it** (answered 2026-11-04), as a
  bump into a boulder does: it is never walked into once seen. A web can't
  be disarmed, and effects don't reach it (D16), so it is cut, struck or
  shot.
- **`D` with a known trap underfoot** disarms that one without asking. The
  direction prompt has the four directions only, and `.` is the wait key.
- **A failed disarm that sets the trap off** says "You set off a dart trap!",
  not the kind's step or fall verb: `EV_TRIGGERED`'s message checks whether
  the victim stands on the trap.
- **Spent:** "The healing shrine is spent.", not "The shrine is silent.": the
  longer wording wrapped onto two lines.
- **`EV_SEARCHED` (new),** posted by every search with the count found, so
  the player is told "You find nothing.". Events are numbered: `EV_OPERATED`
  31, `EV_TRIGGERED` 32, `EV_FOUND` 33, `EV_DISARMED` 34, `EV_SEARCHED` 35
  (above `NGE_EVENT_USER`).
- **`FVERB_DROP` (new),** for the trapdoor: "You fall through a trapdoor!".
- **Monsters open chests** when one blocks their step, as they open doors:
  `ai_step()` opens any closed feature. Its contents spill as for the player.
- **Numbers.** Actions `ACT_OPERATE` 17, `ACT_SEARCH` 18, `ACT_DISARM` 19.
  Reasons `ACT_R_CANT_OPERATE` 40, `ACT_R_SPENT` 41, `ACT_R_CANT_DISARM` 42,
  and the attempted `ACT_R_DISARM_FAILED` 43 and `ACT_R_SET_OFF` 44. Kinds
  `FT_BOULDER` 3 to `FT_TRAPDOOR` 21, 22 in all.

### Code

- **`act_find_feature()`** (`act_shared.c`) is the one finder for open, close,
  unlock, operate and disarm. A hidden feature is "nothing there" to all of
  them.
- **The reactions are in `feature_event.c`,** page 132 with the component,
  not in `feature.c`: they call `act_spring()`, and the PC tests that link
  the component without the actions would not link. `game_on_event()` calls
  `feature_on_event()` only for `EV_OPENED` and `EV_STOOD_ON`.
- **`act_spring()`** is public (`act.h`) and banked in `BANK_56` with
  `act_strike()`. It is not an action: it has no entry in `act_do()`.
- **A hidden feature has no third look.** It blocks nothing and leaves its
  tile alone: it starts hidden, is placed on drawn terrain, and only being
  found changes that.
- **`level_redraw()` draws the features around a cell again.** It redraws a
  3 x 3 of terrain, so a feature destroyed next to a door painted over the
  door. Nothing was destroyed in play before traps.
- **The search and disarm rolls** are rules of the component, as the lock
  pick's is: `feature_found_by()`, `feature_disarm_margin()`,
  `FEAT_DISARM_SET_OFF`.
- **Locks** share `lock_kind()` and `place_key()` in `level_gen.c`, and
  `place_locks()` returns its count for `place_features()`.

### Development aid

- **The `T` key** (debug builds, dev.h): a locked chest with two items and
  its key at the player's feet, and a cracked statue with one, on the nearest
  interior tiles tried (`level_dev_features()`, page 102); and a dart trap, a
  pit, a fire rune and a web, found, on the free tiles around the player, with
  a trapdoor hidden on the next (`dev_traps()`, page 114).

### Memory

- **Page 102 filled up.** `place_features()` and its helpers left 63 bytes, so
  `level_travel.c` moved unchanged to a pair of its own, 136/137 (`banks.inc`):
  it reads none of page 103's data. Page 102 now has 266 bytes free in debug
  builds, with the `T` key's code.
- **Common memory:** 4,309 bytes free before step 1, 4,203 after: about 25
  bytes for each `act_do()` case calling a banked action, a byte a reason,
  and the event filter. No library routine was linked (the map was compared).
- **Pages:** 122 has 6,893 bytes free and 123 (the kinds) 7,170, well within
  R25; 132 (the component and reactions) 14,437 in its pair; `BANK_56` 2,758;
  136/137 15,294.
- **Tiles:** 13 metatiles, as section 5.4: 41 of 64, one bank of 13 colours.
  Three colours were added: glow and two of water (tile_style.md 6.5.2).

### Checks

- **The asm was read** for `update()`, `feature_on_event()`, `act_operate()`
  and `act_spring()`: the state bits, the abilities (`FCAN_ONCE` as bit 7,
  tested with `rlca`), the pass flag, and the offsets of `look`, `can` and
  `block_shut` in the copied info are right.
- **Tests:** `test_act` 577 checks (513 before), `test_feature` 55,
  `test_gen` 84 (the record's rules in its loop), `test_level` 86, `test_ui`
  502 (490 before). The probability of meeting a trap in play has no test:
  CSpect check 3 covered it.
