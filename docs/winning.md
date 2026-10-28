# Winning the game: requirements and design

Status: implemented (2026-10-28; D1 to D10 agreed the same day). Section 5
describes the code as built, and section 8 lists where it differs from the
proposal. CSpect checks passed (2026-10-28); not yet tried on hardware. Covers the quest item, where it is kept and
placed, and winning by climbing out of the dungeon with it. Builds on
[level_transitions.md](level_transitions.md) (persistent, parked and kept
entities, stairs, R2, R3, R8, R10a and R18 there).

## 1. Goal

The player goes down to the bottom level (`LEVEL_DEPTH_BOTTOM`), takes the
quest item and climbs back out to depth 0. Climbing out with the item wins
the game. Climbing out without it isn't allowed.

The quest item is the one thing in the dungeon that can't be lost for good.
It is a persistent item: it can't be destroyed, and it stays on the depth
where the player lost it. Levels are generated again on each visit, so
"stays on the depth" means it is put on the new map every time the player
comes back to that depth.

## 2. Terms

| Term | Meaning |
|---|---|
| Quest item | The one item of kind `IT_QUEST`. Made once a game, never destroyed |
| Quest depth | The depth the quest item is on: the depth of the area its owners lead to, or the player's depth while the player has it |
| Carried | The quest item's owners lead to the player: stowed or equipped, or later inside a container the player carries |
| Lost | The quest item stops being carried while the player is on a level: dropped now. Later also thrown, stolen, knocked out of the pack or sent through a hole |
| Holder | The entity that owns the quest item directly: the player, a creature, a level area, or later a container |

## 3. Requirements

**Changed** and **New** mark requirements that differ from the request or
add to it. Section 4 gives the reasons.

### The quest item

- **R1** There is exactly one quest item in a game. It is made at a new game
  by the persistent table in `actors.c` (level_transitions.md R9), owned by
  `NGE_OWNER_AREA(LEVEL_DEPTH_BOTTOM)`, with no location.
- **R2** It is never made at random. Its kind has rarity 0, so
  `gen_item_random()` never picks it.
- **R3** It can't be destroyed. It is persistent from the moment it is made,
  and nothing clears the flag. **New:** it can't be used up either (it has
  no use hook, so `act_use()` refuses with `ACT_R_CANT_USE`). Anything that
  would destroy an item (later: fire, acid, a consuming effect) must check
  `NGE_ENTITY_PERSISTENT` and leave it alone. A debug check (R14) catches a
  quest item that has stopped being alive or persistent.
- **R4** It is an ordinary item otherwise: it can be picked up, dropped,
  carried in the pack and named in messages. It fits no equipment slot, so
  it can't be equipped. It takes one inventory slot. **New:** a full pack
  refuses it, as it would any item (`ACT_R_FULL`). The player has to drop
  something first.
- **R5** **New:** its ID is kept in a global, `quest_item`
  (`game/quest/quest.h`), as the player's is in `actor_player`. The ID never changes, because the item is never freed.

### Where it is

- **R6** The quest item is always on exactly one depth from 1 to
  `LEVEL_DEPTH_BOTTOM`: on the player (carried), held by a creature on the
  current level, or owned by a level area. It is never owned by
  `NGE_OWNER_WORLD` or by depth 0 after a new game is set up.
- **R7** When it is lost on the current level it stays at that depth:
  - dropped: owned by the level area, on the tile (as `act_drop()` does);
  - held by a creature that dies: dropped on its tile (level_transitions.md
    R10a);
  - held by a creature that leaves the level: a non-persistent creature
    drops it where it was (level_transitions.md R18). A persistent creature
    takes it with it to that creature's new depth, still held;
  - later, sent to another depth by itself (a hole, a teleport): parked at
    that depth, clamped to 1 to `LEVEL_DEPTH_BOTTOM` (level_transitions.md
    R2 and R18), so it can never be sent to depth 0.
- **R8** When the player leaves a level, a quest item that isn't carried
  stays with that depth: parked in its area, or held by a parked persistent
  creature (level_transitions.md R12).
- **R9** **MUST:** every time the player enters the quest depth, the quest
  item is on the new map. It is either on a floor tile, owned by the level
  area, or held by a creature that is on the map (R7, R8). This includes the
  first time the player enters the bottom level. A tile is always found
  (R10): placing it never gives up.
- **R10** **Changed:** the quest item goes on free room floor at random, as
  every parked entity does (level_transitions.md R8). The tile where it was
  dropped isn't remembered: the old map is gone. If no free tile is found at
  random, the placement searches every room tile by tile and takes the
  first free floor tile. If there is no free floor at all, it takes the
  first room floor tile with no feature on it, and shares the tile with
  what is there. Today `any_free_floor()` retries for ever; R10 makes the
  placement certain to end.

### Leaving and winning

- **R11** The up stairs on depth 1 (the only way to depth 0,
  level_transitions.md R2) refuse if the actor doesn't carry the quest item:
  `ACT_R_NO_QUEST`, "You can't leave without the <quest item>." No turn is
  used. This is level_transitions.md R3.
- **R12** Climbing the depth 1 up stairs with the quest item wins the game:
  `GAME_OVER_WON` replaces `GAME_OVER_ESCAPED` (escaping without it can't
  happen now). The game-over messages are "You escape with the <quest
  item>. You win!" and "Press any key."
- **R13** **New:** a non-player on the depth 1 up stairs is refused the same
  way (it can't carry the quest item out). So no creature can leave for
  depth 0, which R2 in level_transitions.md already implied.
- **R14** **New:** debug builds check R3 and R6 after each change of level
  (`quest_check()`): the item is alive and persistent, and its owners lead
  to the player, to a creature on the current level, or to an area from 1 to
  `LEVEL_DEPTH_BOTTOM`. On the current level, an item owned by the area
  directly has a location (R9). Off it, it has none.

### Messages

- **R15** Picking up the quest item for the first time in a game posts the
  usual pick-up message, then "Now climb back out of the dungeon." Picking
  it up again shows only the pick-up message.

### Out of scope

The quest item's look and name (placeholders here: section 5.1). A guardian
or boss on the bottom level, and a special room for the item. Ways to lose
it other than dropping it or a holder dying or leaving (stealing, throwing,
holes). Monsters picking items up. Showing the quest depth on a screen.
A score or end-of-game summary.

## 4. Decisions and challenges

| # | Decision | Alternative rejected | Why |
|---|---|---|---|
| D1 | The quest item is a row in the existing persistent table, parked at the bottom from the start (R1) | Generate it as part of the bottom level the first time it is entered, with a "taken" flag | Parking already places it on every entry (R8 in level_transitions.md) and keeps it through drops, deaths and holders leaving. The flag scheme would need a second path for "lost elsewhere". The table row costs 3 bytes and an entity ID for the whole game |
| D2 | Random free floor, every time, including the first visit to the bottom (R10) | Remember the tile where it was dropped; put it in a far room or a vault at the bottom | The map is new on each visit, so an old tile means nothing. A far room or vault is a better challenge but belongs with the bottom-level guardian (out of scope). The first visit could place it at the up stairs' feet, which makes the bottom trivial for now |
| D3 | Placement is certain to end: random tries, then a scan, then a shared tile (R10) | Keep `any_free_floor()`'s endless retry | The request says the item MUST be placed. The endless retry ends in practice (few entities, rooms of at least 6 tiles), but nothing proves it, and a hang on the bottom level would lose the game. The scan is a few lines, and the same function serves every parked entity. Sharing a tile with an item or creature is allowed, but not with a feature: a door or stairs tile would hide it |
| D4 | A creature holding it counts as "on the map" (R9) | The item must always be on the floor when the depth is entered | Today only a persistent creature can carry it off a level, and none exists. When the boss comes, "the boss has it" is the natural play, and killing the boss drops it (R10a). Taking it from a parked holder on entry would be extra code with no play value |
| D5 | Ascending without the item is refused, not allowed with a "you gave up" ending (R11) | Allow leaving without it and end the game as a loss | The request says winning needs the item. Refusing keeps the player in the game, and they can go and get it. A "give up" ending is already there: quit |
| D6 | `GAME_OVER_WON` replaces `GAME_OVER_ESCAPED` (R12) | Keep both | After R11, escaping without the item can't happen, so `ESCAPED` would be dead code |
| D7 | `quest_item` global (R5) | Search the player's inventory for an item of kind `IT_QUEST` | The check is then a walk up the item's owner chain (a few steps), not a scan of the pack. The ID is stable because the item is never freed. It costs one byte of common memory |
| D8 | A full pack refuses it (R4) | Let the quest item ignore the capacity | It is the normal rule and costs nothing. Dropping one thing to take it is a fair cost. A special case would also have to be undone by every later item rule |
| D9 | Non-players are refused at depth 1 too (R13) | Let a monster leave for depth 0 and destroy it | A monster carrying the quest item out would lose it for good (R6). No AI uses stairs yet, but the action works for any actor (the action rule) |
| D10 | Debug builds run `quest_check()` after each change of level (R14) | PC tests only | Most ways to lose it will come later, from code that may not know about it. A cheap assert in play catches a breach where it starts. Not in release builds: no cost there |

## 5. Design (as built)

### 5.1 The quest item kind

- `gen_kinds.h`: `IT_QUEST 7`, `IT_COUNT 8`, and a subclass of its own,
  `ITEM_QUEST 4`.
- `gen_item_pdata.c`: the record. Placeholder name "crown", and the ring
  sprite in `BANK_PURPLE` (the gold ring is the ring of health) until it
  has its own art. No hooks, no melee, no slot, `.where = { .depth =
  LEVEL_DEPTH_BOTTOM, .rarity = 0 }`. `gen_pick()` never returns a kind
  of rarity 0: `pick < 0` is never true.
- Messages name items with "a" ("You pick up a crown."), and the quest
  item with "the" where the message is about it alone ("You can't leave
  without the crown.").

### 5.2 Creating it (`actors.c`)

The persistent table gets `{ CLASS_ITEM, IT_QUEST, LEVEL_DEPTH_BOTTOM }`,
first, in all builds. `actors_create()` passes the ID of that row to
`quest_set()`, which keeps it in `quest_item` and clears the first-pick-up
flag. The table stays the only place persistent entities are made.

### 5.3 The quest module (`src/game/quest/`)

[quest.h](../src/game/quest/quest.h):

- `quest_item` (common, [quest.c](../src/game/quest/quest.c)): the ID,
  `NGE_ENTITY_INVALID` before the first game.
- Banked in page 128 ([quest_bank.c](../src/game/quest/quest_bank.c)):
  - `quest_set(id)`: 5.2.
  - `quest_carried(actor)`: 1 if the item's owner chain leads to the actor.
  - `quest_on_event(e)`: on the player's first `EV_PICKED_UP` of the item
    in a game, posts `EV_QUEST_FOUND` (target: the item; source: the
    player). Called by `game_on_event()` before the messages.
  - `quest_check()`, debug builds only (R14): alive, persistent, and the
    owner chain ends in an area of depth 1 to `LEVEL_DEPTH_BOTTOM`; placed
    if owned by the current level's area directly, not placed if owned by
    another.

### 5.4 Placing it (`level_gen.c`)

`any_free_floor()` becomes `level_free_floor()` (declared in
`level_internal.h`, so the tests can call it), certain to end (R10):

```
for PLACE_ATTEMPTS tries: a random room, a random tile: no entity → done
for each room, each tile inside it: no entity on it → done
for each room, each tile inside it: no feature on it → done
assert (a level always has room floor with no feature)
```

It is used only for what is placed after the random entities (parked
entities and the player arriving anywhere), so in debug builds the map and
its random entities are still the same for a depth (level_transitions.md
R7a).

### 5.5 The check and the win

- `act.h`: `ACT_R_NO_QUEST`. [act_stairs.c](../src/game/actions/act_stairs.c),
  step 2 (validate): a climb to `LEVEL_DEPTH_SURFACE` fails with
  `ACT_R_NO_QUEST` unless `PC_IS(actor)` and `quest_carried(actor)` (R11,
  R13). Check mode refuses it too.
- `game.h`: `GAME_OVER_WON` replaces `GAME_OVER_ESCAPED`. `game.c`:
  `change_level()` ends the game with `GAME_OVER_WON` for depth 0, and in
  debug builds calls `quest_check()` after `level_generate()`. `over_run()`
  calls `messages_game_over(reason)`.
- Messages ([messages_bank.c](../src/game/messages/messages_bank.c)):
  `EV_QUEST_FOUND` "Now climb back out of the dungeon.", `ACT_R_NO_QUEST`
  "You can't leave without the crown.", and `messages_game_over()`: "You
  escape with the crown. You win!" then "Press any key.", or "Game over.
  Press any key." (moved from `game.c`, G-FLOW-3).

### 5.6 Memory and budget

- Common: `quest_item` (1 byte) and the first-pick-up flag (1 byte, bss of
  `quest_bank.c`), and a few calls: 4 bytes in all, 244 bytes left below
  the stack.
- Page 128 (new): `quest_bank.c`, with its asserts. Page 109: one item
  record and its name. Page 112: the check in `act_stairs.c`. Page 102: the
  scan. Page 118: three messages and `messages_game_over()`.
- Entities: one more for the whole game.

## 6. Tests

| Test | Requirement | Where |
|---|---|---|
| 1000 random items over every depth: never `IT_QUEST` | R2 | `test_gen.c` |
| At depth 1: refused without it, in check mode and in do mode (no turn used, no request); refused for a monster carrying it; allowed for the player carrying it. It can't be used or equipped, and `nge_entity_destroy()` leaves it alive | R3, R4, R11, R13 | `test_act.c` |
| Parked at the bottom, not placed, while on depth 3. The first visit to the bottom: on room floor, owned by the area, alone on its tile | R1, R8, R9, R10 | `test_level.c` |
| Carried up to depth 6, dropped there: parked in area 6 while on depth 5, on the map of depth 6 on each visit (two different maps). `nge_entity_destroy()` leaves it alive | R3, R7, R8, R9 | `test_level.c` |
| Held by a monster that dies: on its tile. Held by a monster sent away: dropped where it was. Held by a persistent monster sent to depth 2: still held, parked with it, then on the map with it | R7, R8 | `test_level.c` |
| `level_free_floor()` in a room of two tiles: the free one; both taken, a shared one; one under a door, the other | R10 | `test_level.c` |
| `quest_check()` passes after each of the above, and halts for a quest item owned by `NGE_OWNER_WORLD` | R14 | `test_level.c` |
| No "Now climb back out" for a pick-up by someone else; with the player's first pick-up, after "You pick up a crown."; not with the second. The `ACT_R_NO_QUEST` message. Both game-over messages | R11, R12, R15 | `test_ui.c` |

Not covered by the PC tests (`game.c` and `actors.c` aren't linked into
them): the quest item made by `actors_create()`, `quest_on_event()`
called by `game_on_event()`, `quest_check()` after each change of level,
and the game ending with `GAME_OVER_WON`.

CSpect checks (for the user to run):

1. A new game: fall with `F` to depth 10 (the bottom). The purple ring
   ("crown") is somewhere on the floor. Picking it up says "You pick up a
   crown." then "Now climb back out of the dungeon.".
2. Drop it on depth 10, go up and down again: it is on the new map
   (look for it). Pick it up again: no second "Now climb" line.
3. Climb to depth 5 with it, drop it, go to depth 4 and back to depth 5:
   it is there, somewhere else.
4. Without it, on the depth 1 up stairs, `<`: "You can't leave without the
   crown." and no turn passes.
5. With it, `<` on the depth 1 up stairs: "You escape with the crown. You
   win!", "Press any key.", then the title.
6. No assert from `quest_check()` on any change of level (debug build).

## 7. Later

- A guardian or boss on the bottom level, and the item in a far room or
  vault (D2).
- Other ways to lose it (theft, throwing, holes): each must keep R6, and
  `quest_check()` will say if one doesn't.
- A screen line saying where the quest item is ("You left the crown on
  level 5").

## 8. As built: differences from the proposal

- The module is `src/game/quest/`, with the ID in `quest_item`, not
  `actor_quest` in `actors.h`: the actions and the tests need it, and
  `actors.c` isn't linked into the tests.
- Its functions are banked (page 128, a new pair), not common: in common
  memory they took about 600 bytes with their asserts, and only 248 were
  free.
- The first pick-up is noted by `quest_on_event()`, which posts
  `EV_QUEST_FOUND` for the message, not by a flag in the messages. The
  messages only react (G-ARCH-3).
- No assert in `change_level()` that the player carries it: the action
  already checks, and its literal would cost common memory.
- No compile-time check of the rarity: a `const` table's field isn't a
  constant expression. `test_gen.c` checks it instead.
- The name is "crown", not "golden amulet": "a amulet" reads badly in the
  messages that say "a", and "You can't leave without the golden amulet."
  is longer than the 40-column message line.
- The game-over text moved from `game.c` to `messages_game_over()`, and the
  win is two lines: one would not fit the message line.
- No `test_actors.c`: `actors.c` isn't linked into the tests (R1 is left
  to the CSpect checks).
