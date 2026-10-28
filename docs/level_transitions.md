# Level transitions: requirements and design

Status: implemented (sections 5 to 7 describe the code as built). Covers going from one dungeon level to another (stairs now;
holes, traps and teleports later), generating a level for a depth, and the up
and down stairs.

## 1. Goal

The player goes down through the dungeon to the bottom level, takes the quest
object and climbs back out to level 0. Levels are not kept: each visit
generates a new one. A few **persistent** entities (the player, the quest
object, a final boss) outlive the levels they are on.

## 2. Terms

| Term | Meaning |
|---|---|
| Depth | A level's number: 1 (top) to `LEVEL_DEPTH_BOTTOM` (bottom). 0 is the surface: leaving the dungeon, not a level |
| Level area | The owner of a level's entities: `NGE_OWNER_AREA(depth)`. Only the current level has a map |
| Current level | The level the player is on: `level_depth`, its area `LEVEL_AREA` |
| Persistent entity | Has `NGE_ENTITY_PERSISTENT`: `nge_entity_destroy()` leaves it alive |
| Kept entity | A persistent entity, or one owned (directly or through others) by a persistent entity: the player's pack, a boss's weapon |
| Parked entity | A kept entity owned by a level area that isn't the current one: in the game, not on a map |
| Transition | The player leaving the current level for another depth |
| Arrival | Where the player is put on the new level: on its up stairs, its down stairs, or anywhere |

## 3. Requirements

Numbered so tests and commits can name them. **Changed** marks a requirement
that differs from the original request; section 4 gives the reasons.

### Depths

- **R1** Depths run from 1 to `LEVEL_DEPTH_BOTTOM` (10 for now; a compile-time
  check holds it below `NGE_OWNER_AREA_MAX`). A new game starts at depth 1.
- **R2** Nothing can take the player below `LEVEL_DEPTH_BOTTOM` or above 0.
  **Changed:** only the up stairs on depth 1 lead to depth 0. Any other way up
  (a teleport, a later "recall") stops at depth 1.
- **R3** Going up to depth 0 ends the game. Later, the up stairs on depth 1
  will refuse if the player doesn't carry the quest object. That check goes in
  the ascend action's validate step, the only way to reach depth 0 (R2).

### Generating a level

- **R4** Entering a depth generates a new map and its non-persistent entities
  (doors, random items and monsters) for that depth, as `level_generate()`
  does today.
- **R5** Every level has exactly one up stairs. Every level except the bottom
  one has exactly one down stairs. **Changed:** the request was "at least
  one". A second staircase only matters once something can block or close the
  first, so the generator has one of each for now. The count is a constant
  that can be raised later.
- **R6** The up and down stairs are in different rooms. Nothing else is
  generated on a stairs tile.
- **R7** Seeding, by build:
  - Debug builds (`GAME_DEV_KIT`): `level_generate()` seeds with the depth
    (`srand(level_dev_seed + depth)`) before it builds anything.
    `level_dev_seed` is 0 unless a developer or a test changes it to see
    other dungeons. A depth's map, stairs, doors, items and monsters are then
    the same on every visit and every run, whatever happened in play before.
  - Release builds: **changed**, seeded once per new game from a varying
    source (e.g. the frames counted until the key that starts the game) and
    never per level. Each visit to a depth is different (levels aren't kept).
- **R7a** Generation that depends on play comes after the seeded part. The
  parked entities (R8) and the player's random arrival (R16) are placed after
  the map and the random entities. How many entities are parked, and how the
  player arrived, then can't change the map or its entities in a debug
  build.
- **R8** The kept entities a depth's area owns are placed on free floor when
  that depth is generated. They are never re-created.

### Persistent entities

- **R9** At a new game the game creates its persistent entities from a table
  of {class, kind, depth}: each one flagged persistent and owned by
  `NGE_OWNER_AREA(depth)`, with no location. The player comes from the same
  path, at depth 1. The quest object and the boss are content and are out of
  scope. Debug builds add a test entry, a persistent goblin on depth 2.
- **R10** **New:** a persistent creature other than the player can still die.
  At 0 hp, `health_damage()` clears its persistent flag before it destroys it.
  As things are, a persistent boss couldn't be killed. Only the dying
  entity's own flag is cleared: what it owns keeps its flag.
- **R10a** When a holder is destroyed, every persistent entity it owns
  survives, directly or through other entities (the boss killed while
  carrying the quest object, or the object in a bag the boss carries). It is
  never destroyed with its holder:
  - in play (the holder dies, or is destroyed by R18), it drops on the
    holder's tile, as `release_dying()` already drops any item;
  - when a level is left (R12), it stays on that level, parked.
- **R10b** A persistent entity can be owned by another entity only if it is
  an item. Asserted in `level_release_dying()`, when a dead holder's
  non-items are destroyed: a persistent one would refuse. Only items are
  dropped by a dead holder: anything else would pass up to the area with no
  location, so it couldn't be seen or reached.
- **R11** **New:** parked entities are frozen. They take no turns. Their timed
  effects that would run out (poison) end when they are parked; those that
  last for ever (a creature's innate regeneration) stay, and their firings
  are skipped while the entity is off the current level (`level_holds()`).
  Without this a parked boss's poison would hurt it, and later its AI would
  run, on a level with no map. Ending a lasting innate effect would lose it
  for good.

### Transitions

- **R12** Leaving a level destroys every entity its area owns except kept
  entities. Kept entities stay owned by that depth's area, parked, with no
  location. A kept entity held by a destroyed one (a persistent object carried
  by a goblin) passes up to the area.
- **R13** **New:** a destroyed entity's possessions are destroyed with it when
  a level is left. They are not dropped on the floor. Today `release_dying()`
  would drop a monster's items at its old position, on the new map.
- **R14** The player is always on the current level. A transition moves the
  player, with everything they own, to the new level's area.
- **R15** **New:** a transition is a request, carried out at a safe point (the
  end of the frame, after that frame's events are handled). It is never done
  inside the action, timer or event handler that asks for it. Whatever caused
  it can't tell when it happens. The first request in a turn wins, and later
  ones are ignored. If the player dies in the same turn, the transition doesn't
  happen.
- **R16** Arrival: going down the stairs puts the player on the new level's
  up stairs, and going up puts them on its down stairs. **Changed** (the
  request didn't say): any other transition (a fall, a teleport) puts them on
  random free floor.
- **R17** Time carries on across a transition: the turn counter, the player's
  timer and the player's active effects continue. The new level's monsters
  first act one period after they are created.
- **R18** A non-player entity sent to another depth is removed from the
  current level. A non-persistent one is destroyed, and its possessions go
  with it except kept ones, which drop where it was. A persistent one is
  parked in the target depth's area, with the depth clamped as in R2. Nothing
  in scope uses this yet (no monster AI, teleports or holes), but the ascend
  and descend actions use it for any actor that isn't the player.
- **R19** A transition posts `EV_ARRIVED` (target: the player; p1: the new
  depth; p2: how they arrived) after the new level is built. The message
  window reacts to it. The depth on the stats window is redrawn with the rest
  of the screen when play is entered again. An entity removed by R18 posts
  `EV_LEFT_LEVEL` (p1: its target depth).

### Stairs and actions

- **R20** Up stairs and down stairs are features, `FT_STAIRS_UP` and
  `FT_STAIRS_DOWN`. They are drawn as map tiles `<` and `>`, block nothing and
  can't be opened. What a feature leads to is data: new `FCAN_ASCEND` and
  `FCAN_DESCEND` bits in its kind's `can`, so a later hole or trapdoor is just
  a record.
- **R21** `act_ascend()` and `act_descend()`: the actor must be on a feature
  that can ascend (or descend). Refused with `ACT_R_NO_STAIRS` otherwise, and
  later with `ACT_R_NO_QUEST` (R3). On success the turn is used, and the actor
  travels: the player by a request (R15), anyone else by R18. Both share one
  internal function.
- **R22** Player keys: `<` to ascend, `>` to descend (the classic roguelike
  keys). The play help bar lists them.
- **R23** Ascending to depth 0 ends the game with "You leave the dungeon". The
  dead state becomes a general game-over state with a reason (died, quit or
  escaped). This is cheaper than adding a second end state.
- **R24** Debug builds: `F` drops the player to the next depth with a random
  arrival (no turn used), so falls are testable before holes exist.

### Out of scope

New items, creatures and other features: the quest object, the boss, holes,
trapdoors and teleport effects. Monsters following the player between
levels. Keeping levels. Stopping the player from going up and down the stairs
to farm new levels (section 4, D7).

## 4. Decisions and challenges

Each change of scope is set against what it costs and the play it allows.

| # | Decision | Alternative rejected | Why |
|---|---|---|---|
| D1 | A level's area is `NGE_OWNER_AREA(depth)`, and `LEVEL_AREA` becomes `NGE_OWNER_AREA(level_depth)` | A fixed current area 0, with parked entities in area d | Ownership then says which level an entity is on, with no extra state. Only 5 lines use `LEVEL_AREA`, and the actions already work from `NGE_OWNER_OF(actor)`. Render, blocking and `nge_location_find()` already filter by area, so parked entities are ignored for free |
| D2 | Persistent entities owned by a level's area (the user's model) are kept | Respawning "uniques" when a depth is generated, with a "killed" or "taken" flag | The flag scheme is cheaper but loses the state: a wounded boss would come back healed, and a boss teleported to depth 3 couldn't wait there. The area model costs one table and a parking step, and R18 needs it anyway |
| D3 | Parked entities are frozen: effects that would run out end, and the dispatcher skips the firings of effects off the level (R11) | Keep simulating them; skip all their timers; end all their effects | Skipping a firing doesn't stop `nge_timer` counting the cycle down, so a limited effect would run out without its end handler and leave its modifiers behind (a bug). Those are ended instead, which reuses `effect_end()`, and "time passed while you were away" explains it. An effect that lasts for ever has no cycles to count, so skipping it is safe, and ending it would lose an innate ability. Simulating them means messages and deaths on levels with no map. The monster turn handler will skip actors off the level the same way once there is AI, so actor timers need no change |
| D4 | Transitions are deferred to the end of the frame (R15) | Tear down and generate inside the action | The event queue can still name entities the teardown frees, and new entities could reuse those IDs, so a message would name the wrong thing. `pass_time()` could also go on running old monsters' turns in the same frame. Deferring costs two bytes of request state and one check in `pass_time()` |
| D5 | Teardown frees entities before generating the new level | Generating while the old entities are still dying, as `level_generate()` does now | The old level's ~30 entities and the new level's ~30 would be held at once, which risks running out of IDs (`NGE_ENTITY_MAX` is 100, and the player's pack and parked entities count too) |
| D6 | Stairs-to-stairs arrival (R16) | Always arrive at random | The player can retreat back up from a bad arrival. It costs two entity IDs remembered by the generator |
| D7 | Farming new levels on the stairs is accepted for now | Keeping levels, or a cost to change level | Keeping levels is outside the requirements and memory budget. The gap is recorded: each round trip gives fresh loot and monsters. A later cost (hunger, a clock, a dungeon "alert" level) can close it without touching this design |
| D8 | One stairs of each kind, in different rooms (R5, R6) | "At least one", stairs as far apart as possible | Two of a kind add no play until stairs can be blocked or closed. Different rooms is one check in the generator. Distance needs a measure, which can be tuned later |
| D9 | Stairs are feature entities | Plain map tiles | Doors set the pattern: actions find a feature with `nge_location_find()`, messages can name it, and later states (hidden, collapsed) come with it. Two IDs a level |
| D10 | Debug builds seed per level with the depth. Release builds seed once per game (R7) | One rule for both builds; a separate random stream for the generator | All game code shares one `rand()` stream, so a per-game seed lets play change later maps, which debugging can't live with. In release, reseeding per level would make every visit to a depth the same and reset combat randomness. A separate generator stream would suit both builds, but `gen_*_random()` and the generator's helpers all call `rand()`, so it means changing every caller. Rejected unless release builds need reproducible levels (e.g. a daily seed) |
| D11 | Depth 0 only by the depth 1 up stairs (R2) | Any upward travel can reach 0 | The quest check (R3) then sits in one place and can't be bypassed by a teleport |
| D12 | Off-level travel by non-players is handled now (R18) | Defer it to the teleport work | The ascend and descend actions work for any actor (the action rule), so a monster on stairs needs an answer now. It is a small function, and the parking step already exists |

## 5. Design (as built)

### 5.1 Level module (`src/game/level/`)

[level.h](../src/game/level/level.h), in common memory:

```c
#define LEVEL_DEPTH_BOTTOM  10
#define LEVEL_AREA          NGE_OWNER_AREA(level_depth)

#define LEVEL_ARRIVE_ANY          0   /* random free floor: a fall, a teleport */
#define LEVEL_ARRIVE_UP_STAIRS    1   /* on its up stairs: came down */
#define LEVEL_ARRIVE_DOWN_STAIRS  2   /* on its down stairs: came up */

extern uint8_t  level_depth;      /* the current level; 0 before a game */
extern uint16_t level_dev_seed;   /* debug builds: added to the depth for srand() */
```

`level_generate(depth, player, arrival)` (page 102) does the whole change of
level, in this order (R7a):

1. Leaves the current level (`level_leave()`, 5.3).
2. Sets `level_depth`. In debug builds, seeds with `level_dev_seed + depth`.
3. The map, the stairs (up in a random room, down in a different one, none
   at the bottom; their IDs kept in `level_stairs[]`, page 103), the doors,
   then the random items and monsters.
4. The parked entities the depth's area owns, on free floor (R8).
5. The player by `arrival`: on the stairs tile, or on free floor if arriving
   anywhere or the stairs are missing.

Steps 2 and 3 use only the depth. Steps 4 and 5 depend on play, so they
come last.

Also in common memory ([level.c](../src/game/level/level.c)):

- `level_holds(id)`: 1 if the entity's owners lead to `LEVEL_AREA`.
- The request (R15): `level_request(depth, arrival)` (the first wins; it
  clamps the depth to 1-`LEVEL_DEPTH_BOTTOM` unless it is 0 by the up
  stairs), `level_pending()`, `level_take_request()` and
  `level_clear_request()`. These live in the level module, not the game, so
  the actions can be tested without `game.c`.
- `level_release_dying()`: the per-frame clean-up, moved here from
  `game.c` because leaving a level runs it too.

In page 102 ([level_travel.c](../src/game/level/level_travel.c)):

- `level_leave(player)` (internal): 5.3.
- `level_park(id, area)`: owner set to the area, location removed, the
  effects it owns that would run out ended (`effect_end()`).
- `level_send(id, depth)`: R18 for a non-player. It clamps the depth, does
  nothing at the same depth, posts `EV_LEFT_LEVEL`, then parks a persistent
  entity or destroys the entity and its unkept possessions.

### 5.2 Carrying out a request (`game.c`)

`pass_time()` stops advancing while a request is pending. In
`post_update()`, after `nge_event_process()`:

```
if level_pending(): change_level():
    take the request
    if the game is over (the player died this turn): nothing
    else if depth == 0: game_over(GAME_OVER_ESCAPED)                (R23)
    else:
        level_generate(depth, player, arrival)
        post EV_ARRIVED
        game_set_state(GAME_PLAY)        // the new area's sprites, the map, stats, depth, help
        process the events               // before the clean-up releases what they name
level_release_dying()
```

### 5.3 Leaving a level (`level_leave()`)

```
area = LEVEL_AREA
nge_owner_set(player, NGE_OWNER_WORLD)         // out of the way: placed again on the new level
for each alive entity under area with no persistent entity on its owner chain (itself included):
    a feature loses its location (no floor to put back); destroy it
level_release_dying()                          // frees them; kept entities held by them pass up into the area
for each alive entity owned directly by area: level_park(id, area)
```

Whole possession trees are destroyed up front (R13), so
`level_release_dying()` never drops a destroyed entity's unkept items:
`inv_drop_all()` visits only alive entities. A walk up the owner chain is a
few steps, over at most 100 entities, once per change.

### 5.4 Stairs and actions

- `gen_kinds.h`: `FT_STAIRS_UP`, `FT_STAIRS_DOWN`. `gen_feature_pdata.c`:
  their records ("staircase up" and "staircase down", `<` and `>` in white for
  both looks, `block_shut` `NGE_BLOCK_NONE`, `can` `FCAN_ASCEND` or
  `FCAN_DESCEND`, never placed at random). `level_bank.c`: their tiles block
  nothing.
- `feature.h`: `FCAN_ASCEND 0x02`, `FCAN_DESCEND 0x04`.
- `kind.h`: `KIND_IS_PLAYER(id)`, the creature of kind `CR_PLAYER`, used by
  the actions and by `health_damage()`.
- `act.h`: `ACT_ASCEND`, `ACT_DESCEND`, `ACT_R_NO_STAIRS` (refused).
  [act_stairs.c](../src/game/actions/act_stairs.c) (page 112): a feature on
  the actor's tile whose kind can go that way. Applying it uses the turn,
  then for the player calls `level_request(depth ∓ 1, LEVEL_ARRIVE_DOWN_STAIRS
  or _UP_STAIRS)`, and for anyone else `level_send()`.
- `player_bank.c`: `<` and `>` (SYMBOL SHIFT R and T), and `F` in debug
  builds. Help bar: `<>:stairs` replaces `d:drop`, as the bar is 40 columns
  and can't hold both; the title's key hints now list `d drop`.

### 5.5 Persistent entities (`actors.c`)

`actors_create()` creates the player (entity 0, for timer order), then each
row of a `const` table {class, kind, depth} by its generator, owned by
`NGE_OWNER_AREA(depth)` and flagged persistent. The release table is empty.
Debug builds have a persistent goblin on depth 2.

### 5.6 Death of persistent entities (`health.c`)

Before `nge_entity_destroy()` at 0 hp: if the target isn't the player
(`KIND_IS_PLAYER()`), clear `NGE_ENTITY_PERSISTENT` on the target only
(R10). For R10a, `level_release_dying()` destroys a dead holder's non-items,
asserting that none refuses (R10b), and `inv_drop_all()` drops its alive
items on its tile, persistent ones included.

### 5.7 Messages, screen and game over

- `EV_ARRIVED`: "You go down to level 2.", "You go up to level 1.", "You
  find yourself on level 3." (any other arrival). `EV_LEFT_LEVEL`: "The
  goblin goes down and away." `ACT_R_NO_STAIRS`: "There are no stairs that
  way here."
- The stats window shows the depth (`screen_show_depth()`), drawn when play
  is entered.
- `GAME_DEAD` is now `GAME_OVER`, entered by `game_over(reason)`: died, quit
  or escaped.
- Release builds seed `rand()` once per new game from the frames counted
  since start-up, which include the time spent on the title and the name
  prompt.

### 5.8 Memory and budget

- Common: `level_depth`, `level_dev_seed`, the request (5 bytes), and the
  frame counter and game-over reason in `game.c`.
- Page 102: about 5.0K of 8K. Page 103: 138 bytes. Page 112 (actions):
  about 7.0K of 8K, which is getting full.
- Entities per level: 2 more (the stairs). Peak use during a change no
  longer doubles (D5).

## 6. Tests

| Test | Requirement | Where |
|---|---|---|
| 20 seeds × every depth: the layout rules, plus one up stairs and (above the bottom) one down stairs, in different rooms, alone on room floor, open | R5, R6 | `test_level.c` |
| Arrivals: on the up stairs, on the down stairs, anywhere; at the bottom, "on the down stairs" falls back to free floor | R16 | `test_level.c` |
| The same depth twice, with `rand()` calls and a parked entity in between: the same map and entities. The parked one is placed on floor | R7, R7a, R8 | `test_level.c` |
| Leaving a level: a monster and its potion freed, a persistent ring it held parked in the old area with no location, the player's pack kept; nothing unkept or located left under the old area; nothing left dying | R12, R13 | `test_level.c` |
| 60 changes of level: the free entity count returns to the same value | D5 | `test_level.c` |
| A parked boss: its poison ended, its regeneration kept but off the level, not poisoned; placed again on re-entry | R8, R11 | `test_level.c` |
| A persistent boss killed with a persistent ring stowed and a persistent sword equipped: both survive, unequipped, on its tile. The player at 0 hp is kept | R10, R10a | `test_level.c`, `test_health.c` |
| A persistent boss left behind with what it holds: all parked, still held, and back on re-entry | R10a, R12 | `test_level.c` |
| Requests: up from depth 1 by any other way stops at 1, the up stairs reach 0, below the bottom is clamped, the first wins, a clear drops it | R2, R15 | `test_level.c` |
| `level_send()`: the same depth does nothing; a non-persistent monster freed with its potion, its persistent ring dropped where it was, `EV_LEFT_LEVEL` posted; a persistent one parked at the bottom (clamped) and placed there later | R18 | `test_level.c` |
| `act_ascend()` and `act_descend()`: refused off stairs or the wrong way (no turn used); check mode changes nothing; the player's request; a monster leaves; up from depth 1 asks for depth 0; stairs don't open | R20, R21 | `test_act.c` |
| `<` and `>` refused off stairs; the arrival and leaving messages | R19, R22 | `test_ui.c` |
| A persistent non-player at 0 hp is destroyed, and what it owns keeps its flag | R10 | `test_health.c` |

Not covered by the PC tests, because `game.c` isn't linked into them: a
request in the turn the player dies doing nothing, `pass_time()` stopping
for a request, game over by escaping, and the depth on the stats window.
They were checked in CSpect (2026-10-18): going down and up the stairs, the
messages and the depth shown, `F` to fall, the goblin on depth 2 on each
visit, and escaping from depth 1. Not yet tried on hardware.

## 7. Later

- The quest object: done, see [winning.md](winning.md) (R3 is its R11).
- The boss: a row in the persistent table.
- Holes, trapdoors and teleports: feature records with `FCAN_DESCEND` and
  an `EV_STOOD_ON` handler, or effects calling `level_request()` and
  `level_send()`.
- Monster AI: skip `TIMER_MONSTER` firings when `!level_holds(id)`.
