# Actions: design specification

How Jetset's actions work: what an action is, how it succeeds or fails, who calls
it, and which actions are still to be built.

The public interface is in [act.h](../src/game/actions/act.h), which lists each
action's inputs, outputs and failure reasons, and the pattern every action follows
is P-ACT-1 in [patterns.md](guidelines/patterns.md). What drives actions is in
[game_loop.md](game_loop.md) and [ai.md](ai.md). What they cause is in
[effects.md](effects.md) and [events.md](events.md). Memory use is in
[memory_map.md](memory_map.md).

This spec covers the module in `src/game/actions/`. The rules actions call, such as
whether an attack hits, belong to the components, and are in their headers. Section 6
is the list of actions to add, and none of it is built. Sections 1 to 5 describe the
code as it stands, except the decisions under "Agreed, not built".

Status: D10 to D23 agreed 2026-10-31, not implemented. The rest of section 6 is
proposed.

## 1. Overview

**An action is anything an actor chooses to do.** Moving, attacking, picking up,
using, opening and taking stairs are all actions. The player and monsters use the
same ones, so they play by the same rules.

**Every action has the same shape.** It takes a context (`act_t`), which the caller
fills in and keeps on its stack, and a mode. It returns success or failure, and the
context says why it failed and whether the turn was used.

**Inside, an action takes four steps:**

```text
 act_<verb>(context, mode)

 1. context    find what it needs: components, the target, the tile
 2. validate   check preconditions, with no randomness
                  refused --> reason set, nothing changed, turn not used
                  ACT_CHECK stops here
 3. resolve    roll dice and decide the outcome
                  attempted (a miss, a rooted actor) --> reason set, turn used
 4. apply      change the game, post events, success: turn used
```

**Check mode asks without doing.** `ACT_CHECK` runs only the first two steps, so it
changes nothing, posts nothing and draws no random numbers. The AI weighs up its
options with it, and the inventory screen uses it to decide what to offer.

**Failing comes in two kinds.** A refused action didn't happen, and the actor can try
something else. An attempted action did happen and didn't work, so the turn is gone.
Each reason is one kind or the other, defined once in a table.

**Actions orchestrate; they don't own rules.** An action finds and checks its
target, then calls component functions, which hold the rules. It posts events for
what happened, and messages, animations and the screen react to those.

**Actions don't call each other.** A caller combines them: moving into a monster is
a move that is refused, followed by a melee.

```text
 player command --> player_act() --+
                                   +--> act_do() or act_<verb>() --> components
 AI state ----------> ai_try() ----+                             --> events
                                                                 --> effects
 inventory screen: ACT_CHECK only, then player_act()
```

## 2. Terms

| Term | Meaning |
|---|---|
| Actor | The entity acting |
| Context | An `act_t`: inputs set by the caller, outputs set by the action |
| Mode | `ACT_DO` does the action, `ACT_CHECK` only checks it |
| Step | One of context, validate, resolve and apply, always in that order |
| Refused | A precondition failed: nothing changed and the turn isn't used |
| Attempted | The action was tried and didn't succeed: the turn is used |
| Reason | `ACT_R_*`: why an action failed, refused or attempted |
| Ending | `act_succeed()`, `act_checked()` or `act_fail()`: how every action returns |
| Turn | One action of an actor; how long it takes depends on the actor's speed |
| Consequence | What an action causes, such as damage or an effect. It uses no turn of its own |
| Bump | The caller's habit of turning a refused move into a melee or an open |
| Stowed, equipped, carried | In the pack, in a slot, either ([items.md](items.md), section 4.3) |
| Rooted | Has `STATUS_ROOTED`: can't leave its tile ([creatures.md](creatures.md)) |
| Line of fire | The tiles a shot or bolt crosses (section 6.2) |

## 3. Requirements

### The interface

- **R1** Anything an actor chooses to do is an action, and the same actions serve
  the player and monsters.
- **R2** Every action takes a context and a mode, and returns `ACT_SUCCESS` or
  `ACT_FAIL`.
- **R3** Every action follows the four steps, in order, and its apply step can't
  fail.
- **R4** Any action can be called by number, through `act_do()`.

### Check mode

- **R5** Check mode changes no game state, posts no events and draws no random
  numbers. It may set the context's outputs.
- **R6** A check that passes means the action won't be refused. It may still be
  attempted and fail.

### Results

- **R7** A failure always gives a reason, and every reason is either refused or
  attempted, defined once.
- **R8** The action sets `used`: 1 on success or an attempted failure, 0 if refused
  or checked. Callers decide whether the turn is over from `used` alone.
- **R9** Validation fails only with refused reasons, and attempted reasons come only
  after the check has stopped.

### Time

- **R10** Every action takes one turn of the actor, whatever the action.
- **R11** Consequences use no turn.

### Rules

- **R12** Rules live in components. An action finds and checks its target, then
  calls them.
- **R13** An action never calls another action. Callers combine them.
- **R14** An action says who may act only where a rule needs it, as with leaving the
  dungeon. Otherwise it treats every actor alike.
- **R15** An action posts events only once its check has passed. A refused action
  posts nothing.
- **R16** Reactions are event handlers, not code in the action. What the player is
  told about a failed action comes from `messages_act_failed()`.

### Constraints

- **R17** The actions are banked in one page. Only the dispatcher and the reason table
  are in common memory.
- **R18** Actions draw random numbers only in the resolve step.
- **R19** A context lives on the caller's stack, and an action keeps nothing between
  calls, so the module has no variables and no init.
- **R20** Actions are tested on the PC with the components they call, so everything
  they do must work without the Next's hardware (G-TEST-1).

### Conditions

- **R21** A rooted actor that tries to leave its tile fails with `ACT_R_ROOTED`,
  attempted, so time passes for a hold to end. Its check passes, as for an attack that
  may miss.
- **R22** A blocker comes before being rooted, so a rooted actor can still bump to
  attack. An actor without stats has no statuses and is never rooted.

### Ranges

- **R23** Every range or distance test in the game uses `nge_map_within()`, so a range
  means the same tiles everywhere ([nge_map.h](../src/nge/map/nge_map.h)). No other code
  works out a distance. Two calls to `act_distance()` break this today, harmlessly, as
  they test only range 1 (section 4.10).

### Actions to add

- **R24 (later)** Launchers and thrown items can be fired at a chosen tile
  (`ACT_FIRE`).
- **R25 (later)** Wands, staves and aimed parchments can be aimed at a chosen tile
  (`ACT_AIM`).
- **R26 (later)** Doors can be locked, and keys and lock picks open them
  (`ACT_UNLOCK`).
- **R27 (later)** Spells and creature abilities are cast (`ACT_CAST`, mana.md R13,
  ai.md R22).
- **R28 (later)** An actor can steal from a neighbour (`ACT_STEAL`, the bandit), and
  no theft can take the quest item (winning.md R6).
- **R29 (later)** Stacks merge and counts are used up, and two-handed weapons and
  sticky curses are enforced, in the existing actions (section 6.8).
- **R30 (later)** Features that do something when operated, hidden features and traps
  have their actions (`ACT_OPERATE`, `ACT_SEARCH`, `ACT_DISARM`), once such features
  exist.

## 4. Design

### 4.1 Files

| File | Where it runs | What it has | Bytes |
|---|---|---|---|
| `act.h` | | The public interface | |
| `act_internal.h` | | The endings and helpers, for the actions only | |
| `act.c` | Common | `act_do()`, `act_attempted()`, `act_reason_flags[]` | |
| `act_shared.c` | Page 112 | The endings, `act_rooted()`, `act_distance()` | 317 |
| `act_move.c` | Page 112 | Move | 946 |
| `act_pickup.c` | Page 112 | Pick up | 728 |
| `act_drop.c` | Page 112 | Drop | 468 |
| `act_equip.c` | Page 112 | Equip and unequip | 1,155 |
| `act_melee.c` | Page 112 | Melee | 1,173 |
| `act_use.c` | Page 112 | Use | 396 |
| `act_open.c` | Page 112 | Open and close | 1,175 |
| `act_stairs.c` | Page 112 | Ascend and descend | 781 |
| `act_wait.c` | Page 112 | Wait | 161 |

The sizes are from the debug build's map, with asserts, and total 7,300 bytes of the
page's 8,192. That leaves 892 (section 6.11).

### 4.2 The context

`act_t` ([act.h](../src/game/actions/act.h)). Each action reads only what it lists,
so callers zero the context and set those fields. Zero is not "none" for `target`
and `item`: entity 0 is a real ID, so an action that accepts no target says so with
`NGE_ENTITY_INVALID`.

| Field | In | Out |
|---|---|---|
| `actor` | Who acts | |
| `target` | An entity acted on, or `NGE_ENTITY_INVALID` to find one | The entity found, or what blocked the action |
| `item` | The item used | The weapon used |
| `dx`, `dy` | A direction | |
| `x`, `y` | A tile: nothing reads these yet | |
| `kind` | A slot to equip in | The slot, damage type or effect used |
| `amount` | | The damage taken |
| `reason` | | `ACT_R_*` |
| `used` | | 1 if the turn was used |

Who reads what:

| Action | `target` | `item` | `dx`, `dy` | `kind` |
|---|---|---|---|---|
| Move | Out: the blocker | | In | |
| Pick up | In or find; out | | | |
| Drop | | In | | |
| Equip | Out: the item in the slot | In | | In and out: the slot |
| Unequip | | In | | |
| Melee | In or find; out | Out: the weapon | If `target` is invalid | Out: damage type |
| Use | | In | | Out: the effect |
| Open, close | In or find; out | | If `target` is invalid | |
| Ascend, descend | Out: the stairs | | | |
| Wait | | | | |

A context can be passed straight to the next action. After a move is refused,
`target` already holds the blocker, ready for a melee or an open. It can also be
passed to the same action twice, which is what `ai_try()` does: what a check writes
to `kind` and `item` is what the action would choose anyway.

### 4.3 Results

| Ending | Function | `reason` | `used` | Returns |
|---|---|---|---|---|
| Success | `act_succeed()` | `ACT_R_NONE` | 1 | `ACT_SUCCESS` |
| Check passed | `act_checked()` | `ACT_R_NONE` | 0 | `ACT_SUCCESS` |
| Refused | `act_fail()` | Why | 0 | `ACT_FAIL` |
| Attempted | `act_fail()` | Why | 1 | `ACT_FAIL` |

`act_fail()` reads the kind from `act_reason_flags[]`, so an action can't set it
wrongly. A debug build asserts that a reason isn't `ACT_R_NONE` and is in range.

### 4.4 Event order

Events are posted in the order the messages should read, so a hit's event comes
before the damage it causes:

- **Melee:** `EV_HIT`, then the damage's events, then the effect's.
- **Use:** `EV_USED`, then the item is destroyed, then the effect's. The item is
  dying when handlers see the event, and stays readable until it is released.

Everywhere else, the change comes first and its event follows.

Consequences are direct calls, because the rest of the action reads their result
(events.md, R27): melee reads the damage taken for `amount`.

### 4.5 The actions

| Action | What it does | Events | Random |
|---|---|---|---|
| `ACT_MOVE` | Moves one tile, if the tile can be entered | `EV_MOVED`, `EV_STOOD_ON` for what is there, at most 4 items (events.md, R26) | No |
| `ACT_PICKUP` | Stows an item from the actor's tile | `EV_PICKED_UP` | No |
| `ACT_DROP` | Puts a stowed item on the actor's tile | `EV_DROPPED` | No |
| `ACT_EQUIP` | Puts a stowed item in a slot, and recomputes | `EV_EQUIPPED` | No |
| `ACT_UNEQUIP` | Stows an equipped item, and recomputes | `EV_UNEQUIPPED` | No |
| `ACT_MELEE` | Attacks an adjacent entity with the wielded weapon or its own attack | `EV_HIT` or `EV_MISSED`, then the damage's and hit effect's | Yes |
| `ACT_USE` | Uses up a stowed item with a use hook | `EV_USED`, then the effect's | No |
| `ACT_OPEN` | Opens a feature that opens | `EV_OPENED` | No |
| `ACT_CLOSE` | Closes one with nothing else on its tile | `EV_CLOSED` | No |
| `ACT_ASCEND`, `ACT_DESCEND` | Takes stairs on the actor's tile | `EV_LEFT_LEVEL` for anyone but the player | No |
| `ACT_WAIT` | Nothing; it always succeeds | None | No |

**Move** takes the tile from the actor's location and `dx`, `dy`, each -1, 0 or 1,
and asks `nge_block_can_enter()` with the actor's pass flags, so a ghost's walls are
the block component's business. Diagonals are allowed.

**Melee** rolls 1d20 in the resolve step, and `attack_hits()` decides whether it
hits. Damage goes through `health_damage()`, and the hit hook of the weapon, or of
the actor if it has none, applies to a target left alive. The roll is posted in the
event, so a message can show it.

**Equip** takes the first free slot the item fits, or the slot asked for. If every
slot is in use, it is refused, and `target` is the item in the last one. The change
goes through `effects_recompute()`, so the worn effect counts before the event.

**Use** destroys the item before applying its effect, and the item is passed as the
effect's source, dying but still readable.

**Open and close** share `find_openable()`. Distance 0 is allowed, so an actor can
try to close the door it stands in: it is then refused with `ACT_R_OBSTRUCTED`, by
itself.

**Stairs** treat the player differently. The player's move is a request that the
game carries out at the end of the frame, because the whole level changes. Anyone
else leaves at once. Only the player, carrying the quest item, can leave the dungeon
([winning.md](winning.md)).

### 4.6 Reasons

There are 28 reasons, with `ACT_R_NONE`. Two are attempted:

| Attempted reason | Where | Event | What the player is told |
|---|---|---|---|
| `ACT_R_MISSED` | Melee | `EV_MISSED` | The event's message |
| `ACT_R_ROOTED` | Move, stairs | None | "You can't move!" from `messages_act_failed()` |

Rooted is an attempted reason that posts nothing, unlike the rule `act.h` gives for
attempted failures. Nothing needs an event today, as only the player struggles: the
AI tests `AI_CAN_MOVE` first and doesn't try.

**A failed action's text.** `messages_act_failed()` covers every reason a player can
reach. `ACT_R_MISSED` has no text because its event has one. `ACT_R_NO_INVENTORY`,
`ACT_R_NOT_ITEM` and `ACT_R_NOT_CARRIED` have none because the player's commands
can't produce them: the player has an inventory, and items are chosen from their own
lists. P-ACT-1 asks for a text for each new reason, and these three are the exception.

### 4.7 Callers

| Caller | How it calls | On failure |
|---|---|---|
| Player commands | `player_act()`: `ACT_DO` | `messages_act_failed()`; time passes only if `used` is set |
| AI | `ai_try()`: `ACT_CHECK`, then `ACT_DO` if it passed | Tries its next option |
| Inventory screen | `ACT_CHECK` only, then returns the action to `item_command()` | Doesn't offer it, or says why |

| Action | Player command | AI |
|---|---|---|
| `ACT_MOVE` | Movement keys | `ai_step()` |
| `ACT_PICKUP` | `g`, `,` | `opt_pickup()` |
| `ACT_DROP` | `d` | No |
| `ACT_EQUIP` | `w` | `opt_wield()` |
| `ACT_UNEQUIP` | `r` | No |
| `ACT_MELEE` | Bump | `opt_attack()` |
| `ACT_USE` | `u` | `opt_drink()` |
| `ACT_OPEN` | Bump | `ai_step()` |
| `ACT_CLOSE` | `c`, then a direction | No |
| `ACT_ASCEND`, `ACT_DESCEND` | `<`, `>` | No |
| `ACT_WAIT` | `.` | The end of an AI turn |

**Bumping** is the player's move command combining actions: it checks the move, and
if the blocker has health it melees, and if it is a closed door it opens it. The AI
does the same with its own choices.

**Diagonals.** Actions and the AI use all 8 directions, and the player's movement keys
give only 4. So a monster can attack the player diagonally, and the player can't
attack back without moving. That is the command's limit, not the action's.

**Timers and event handlers** may call actions too, but none does (section 6.10).

### 4.8 Banking

Each action is `NGE_BANKED`, in the even page of the actions' pair. `act_do()` and
`act_attempted()` are in common memory, with `act_reason_flags[]`, which the endings
read.

- **`act_do()` is a `switch`,** because a pointer to a banked function doesn't switch
  banks (N-BANK-6).
- **`act_shared.c`** holds the endings and helpers. They aren't banked, so only code
  in the actions' page can call them.
- **The context is on the caller's stack,** in common memory, so it stays readable
  across banked calls.
- **Page 113 is empty.** The actions keep no data of their own (R19): what they read
  is the components', in slot 0 or the components' pages.
- **Actions are called from other banked modules** (114 player, 116 screens, 130 AI).
  The caller's page comes back when the action returns.

### 4.9 Adding an action

P-ACT-1 in [patterns.md](guidelines/patterns.md) is the checklist:

1. A function `act_<verb>()` with the four steps, in `act_<verb>.c`, with the page's
   pragmas.
2. An `ACT_<VERB>` number and a case in `act_do()`.
3. For each new reason, a row in `act_reason_flags[]` and a text in
   `messages_act_failed()`.
4. Its inputs, outputs, events and reasons in its header comment.
5. Its row in the tables in sections 4.2, 4.5 and 4.7 here.
6. Tests in `test_act`, including that check mode changes nothing and that the reason
   table's kinds are as intended.

A new `ACT_*` adds 7 bytes to `modes[]` in the inventory screen (page 116), used or
not, because that table is sized by `ACT_COUNT`.

### 4.10 Ranges

Every distance test in `src/`, from a search of the code on 2026-10-31:

| Where | What it tests | Range | How, today | After D23 |
|---|---|---|---|---|
| `target_bank.c` | The cursor's tile is in range | The caller's | `nge_map_within()` | The same |
| `ai_sense.c`, `hears()` | A sleeper hears the player | `AI_HEAR`, 4 | `nge_map_within()` | The same |
| `ai_sense.c`, `ai_in_sight()` | A creature sees a tile | `AI_SIGHT`, 8 | `nge_map_within()` | The same |
| `act_melee.c` | The target is in reach | 1 | `act_distance()` | `nge_map_within()`, and not the actor's own tile |
| `act_open.c` | The feature is in reach | 1 | `act_distance()` | `nge_map_within()` |

So the two rules differ in only those two calls, and level generation and decoration
measure no distances. `nge_map_within()` is a circle and `act_distance()` a square, and
both give the same 8 neighbours at range 1, so D23 changes no result.

**The actor's own tile.** `nge_map_within()` at range 1 includes the tile itself, and
`act_distance()` gave melee a 0 to refuse. Melee must keep refusing that case with a
test that the tiles are different. Open allows it (section 4.5), so it needs nothing.

**Two things that look like distances and aren't.** `nge_block_line()` and
`ai_step_toward()` follow a line, tile by tile, and test no range. Moving is the other:
a diagonal step takes one turn like any step, so moves aren't a measure of range. A bow
of range 6 reaches 6 tiles straight along a row, but (5, 5) is out of range although it
is 5 moves away. Nothing in the code counts moves, so nothing disagrees, but the player
will see this in the cursor.

## 5. Decisions

- **D1 One context for every action.** Callers and the AI can treat actions alike, and
  a new action needs no new signature. The cost is that a field's meaning depends on
  the action, so `act.h` lists what each one reads.
- **D2 One function, two modes.** Validation can't drift from the action, because a
  check is the same code stopped early.
- **D3 No randomness before resolve.** A check is free of side effects, so the AI can
  weigh its options without changing the random sequence.
- **D4 Reasons are classified in a table.** The kind is defined once, and tested, so
  actions can't disagree about whether a reason uses the turn.
- **D5 Every action costs one turn.** An actor's timer fires every `speed` ticks, so
  time depends on who acts, not what they do. Callers read `used` and need no costs.
- **D6 Callers combine actions.** Bumping is the player's choice, not a rule of
  movement, and the AI decides for itself.
- **D7 Endings are functions.** zsdcc dropped the assignments in a comma expression
  (T-ZSDCC-2), so the endings can't be macros.
- **D8 The player's stairs are a request.** A level change replaces everything the
  action's caller is working with, so it waits for the end of the frame
  ([level_transitions.md](level_transitions.md)).
- **D9 Rooted is tested after check mode.** It is attempted, not refused, so a check
  passes and the actor can spend its turn struggling (creatures.md, D4). *Rejected:* a
  refused reason, as a held player could never pass the time by trying to move.

### Agreed, not built

All agreed 2026-10-31, for the actions in section 6.

- **D10 Fire and aim take a tile, not a direction.** They use `act_t`'s `x` and `y`
  and `target_choose()`, as targeting.md planned, and the `f` key's direction prompt
  goes. `act_t` needs no new field. *Rejected:* keeping the direction prompt, which
  can only reach 8 lines.
- **D11 A shot stops at the first creature on its line.** The target is found by
  walking the line, so a creature in the way is hit in place of the one aimed at. That
  is what the player can see and it needs no rule for shooting past. *Rejected:*
  hitting the chosen tile only, which lets a shot pass through whatever is on the way.
- **D12 Shooting at nothing is refused.** A line with no creature fails with
  `ACT_R_NO_TARGET`, costing no ammo or charge, and a wall in the way with
  `ACT_R_NO_LINE`. Both are found in validation, so the check and the AI get the same
  answer. *Rejected:* letting the shot land, which needs a ranged miss to place an item
  and gains little.
- **D13 Ammo is used up, hit or miss, and isn't recovered.** Landing ammo would need
  a new entity for each shot, out of 100, or a stack per tile. *Rejected:* splitting
  off a landed stack; arrows lying about would be the commonest item in the game. A
  chance of breaking, with the rest landing, can be added later and needs no change to
  this design's shape.
- **D14 Fire is one action for launchers, thrown ammo and natural attacks.** All three
  are "the ranged attack of whoever or whatever is shooting", and differ only in which
  entity that is and what is used up. A dart in `SLOT_AMMO` is fired, so there is no
  `ACT_THROW`. *Rejected:* a general throw of any item, which needs rules for what
  every item does on landing, and is a way to lose the quest item.
- **D15 Aim is separate from use.** The aim hook alone says that an item needs a tile
  (items.md, D8), so the two share no path that matters. *Rejected:* `ACT_USE` with a
  tile: it would give that action two shapes and its callers two prompts.
- **D16 A wand keeps its count at 0; a parchment is destroyed.** An empty wand can be
  recharged later, and it tells the player it is empty. A parchment has no count, so
  aiming one uses it up as `ACT_USE` would. This is a rule by subclass, which items.md
  R2 allows.
- **D17 Aim's and cast's effects name the actor as their source.** `EV_DIED` gives XP
  to its source, so a bolt must name the actor, where `act_use()` names the item.
  Without this a kill by wand earns nothing.
- **D18 Unlocking is its own action, and doesn't open.** One action is one turn (D5),
  and `ACT_OPEN` needs no item input. Bump combines them: an open refused with
  `ACT_R_LOCKED` looks for a key. *Rejected:* opening with the key in one action, which
  would make a locked door cost nothing once the key is found.
- **D19 A lock's kind goes in spare bits of `feature_state[]`.** `KEY_COUNT` is 6, so 3
  bits hold it, with `FEAT_LOCKED` reserved already. *Rejected:* a new table of 100
  bytes in slot 0, which has little left.
- **D20 Abilities are spells at no cost.** A gaze or a shaman's spell is one action,
  `ACT_CAST`, with a row saying what it does, so a creature's ability is data
  (G-ARCH-7) and not an action per monster. Spitting is `ACT_FIRE` with a natural
  ranged attack (D14). *Rejected:* `ACT_GAZE`, `ACT_BREATHE` and so on, which spend
  the page on verbs that differ only in their data.
- **D21 A shared strike and a shared line.** Melee's resolve and apply become
  `act_strike()` and the walk becomes `act_line()`, both in `act_shared.c`, so fire
  and aim don't repeat them (G-ARCH-1). *Rejected:* an `act_melee()` that takes a range,
  which would leave one function deciding between two kinds of attacker and target.
- **D22 The actions move to a 16K section, `BANK_56`.** Pages 112 and 113 are already
  theirs and 113 is empty, so this gives them 16K for a pragma in each file and an
  origin in `banks.inc`. They map nothing by hand, so they meet the rule for code that
  extends into slot 7 (memory_map.md). *Rejected:* a second pair, as 130/131 is the
  last in the range. *Rejected:* moving actions to other pages, as `act_shared.c`
  isn't banked and its callers must share a page. Nothing uses a `BANK_n` section yet,
  so this is the first (section 8).
- **D23 Every distance in the game is `nge_map_within()`.** `act_distance()` goes, and
  melee and open test range 1 with the function that the cursor, sight and hearing use.
  That leaves one rule across the game (R23, section 4.10). The tiles at range 1 are the
  same, so nothing changes today. Melee gains a test that the target isn't on its own
  tile, which uses up part of the 60 bytes saved. *Rejected:* changing the game to
  `act_distance()`'s square, which would change sight, hearing and the cursor.

## 6. Not built yet

None of this is built. Its decisions are agreed (D10 to D23), and the details are
still proposals. Eight new actions are listed, and three of
them are needed by items the game already generates:

| Action | Needed for | Priority | Blocked by |
|---|---|---|---|
| `ACT_FIRE` | 15 item kinds: launchers and ammo. The `f` key says "You have nothing to fire." | Needed | Line walk, `TARGET_FIRE`, reading ammo's data |
| `ACT_AIM` | 13 item kinds: wands, staves, aimed parchments | Needed | As fire; `TARGET_AIM` |
| `ACT_UNLOCK` | 7 item kinds: keys and lock picks | Needed | No door is ever locked: level generation and a lock on the door |
| `ACT_CAST` | Mana exists and nothing spends it; the goblin shaman | Wanted | Its own spec: what spells exist, who knows them |
| `ACT_STEAL` | The bandit's "steals and runs" | Wanted | An AI that flees (ai.md R18) |
| `ACT_OPERATE` | Fountains and the like | If added | No such feature kind: there are only doors and stairs |
| `ACT_SEARCH` | Hidden features, `FEAT_HIDDEN` | If added | As above |
| `ACT_DISARM` | Traps | If added | As above |

Those 35 kinds are of 160, generated at their real rarity, and do nothing today
(items.md, "Inert finds"). The reasons, events and keys below are suggestions in the
form `act.h` will need.

### 6.1 Numbers to reserve

| Addition | Names |
|---|---|
| Actions | `ACT_FIRE` 12, `ACT_AIM` 13, `ACT_UNLOCK` 14, `ACT_CAST` 15, `ACT_STEAL` 16, `ACT_OPERATE` 17, `ACT_SEARCH` 18, `ACT_DISARM` 19 |
| Refused reasons | `ACT_R_NO_AMMO`, `ACT_R_WRONG_AMMO`, `ACT_R_NO_LINE`, `ACT_R_CANT_AIM`, `ACT_R_NO_CHARGES`, `ACT_R_LOCKED`, `ACT_R_NOT_LOCKED`, `ACT_R_WRONG_KEY`, `ACT_R_NO_MANA`, `ACT_R_NO_SPELL`, `ACT_R_NOTHING_TO_STEAL`, `ACT_R_CURSED`, `ACT_R_CANT_OPERATE`, `ACT_R_SPENT` |
| Attempted reasons | `ACT_R_PICK_FAILED`, `ACT_R_CAUGHT` |
| Events | `EV_FIRED`, `EV_AIMED`, `EV_UNLOCKED`, `EV_CAST`, `EV_STOLEN` |

Numbers are given only for ordering: each gets its number when it is built, as
`ACT_COUNT` sizes `modes[]`.

### 6.2 The line of fire

Fire, aim and cast find their target the same way, in `act_line()` (D11, D21). It runs
in validation, so it has no randomness and a check gets the same answer.

```text
 act_line(a, range): from the actor, towards (a->x, a->y)

 chosen tile out of range, or the actor's own tile --> ACT_R_OUT_OF_RANGE or
                                                       ACT_R_NO_TARGET
 for each tile along the line, nearest first:
     solid (a wall, a closed door) ------------------> ACT_R_NO_LINE
     an alive entity with health ---------------------> a->target, done
 reached the chosen tile with nothing ----------------> ACT_R_NO_TARGET
```

`nge_block_line()` answers only whether a line is clear, and aim needs the first tile
that stops it. So page 56 gets a walk that gives each tile in turn, with
`nge_block_line()` built on it. The line must stay Bresenham's from the shooter, the
same as the AI's sight, so what a monster sees it can shoot. Passing over a pit doesn't
stop a shot: only `NGE_BLOCK_SOLID` does.

Range uses `nge_map_within()`, so the green cursor and the action agree (R23). The
cursor can still be green on a tile the action refuses for another reason, such as a
wall in the way, and `messages_act_failed()` then says why.

### 6.3 `ACT_FIRE`

**Fire a shot at a tile.** In: actor (alive, placed in an area), x, y. Out: target;
item, the launcher or thrown item; on a hit amount and kind.

What shoots, in order, from `inv_equipped()`:

| In `SLOT_AMMO` | In `SLOT_RANGED` | Who attacks | Used up |
|---|---|---|---|
| `GROUP_THROWN` | Anything | The ammo's own ranged attack | One of the ammo |
| Ammo of the launcher's group | A launcher | The launcher, with the ammo's to-hit and damage bonus | One of the ammo |
| Ammo of another group | A launcher | Refused: `ACT_R_WRONG_AMMO` | |
| Nothing | A launcher | Refused: `ACT_R_NO_AMMO` | |
| Nothing | Nothing | The actor's own `COMPONENT_RANGED`, or `ACT_R_NO_ATTACK` | Nothing |

Steps:

1. **Context:** the shooter above, its range.
2. **Validate:** the attack, then `act_line()`. `ACT_CHECK` stops here.
3. **Resolve:** the d20 and `attack_hits()`. A miss posts `EV_MISSED`, uses up the shot
   and fails with `ACT_R_MISSED`.
4. **Apply:** `act_strike()`: `EV_HIT`, the damage, the hit effect. Then the shot is
   used up: `item_count[]` goes down and at 0 the item is destroyed.

`EV_FIRED` comes first in both cases, for the projectile animation (animations.md
R14): target the entity aimed at, source the actor, p1 the launcher or thrown item.

**The hit hook is the ammo's,** not the launcher's (items.md, section 4.6), so the
flask of oil's fire applies on a hit. It is read before the ammo can go.

**What the data still needs:**

- `attack_hits()` and `attack_damage()` read one entity's row. Ammo with dice 0 has no
  attack component, so its bonus is in its kind. They need to accept a bonus, keeping
  the rule in the component (R12), and `gen_item_attack()` must read it, banked with the
  item table.
- A group accessor, `gen_item_group()`, in the same place. `gen_item_subclass()` and
  `gen_item_fits()` are its model.

**Callers.** The `f` key uses `target_choose()` with a new `TARGET_FIRE` and the
shooter's range, and `PLAYER_PROMPT_FIRE` goes. The AI gets `AI_CAN_RANGED` from
`ai_can()` and an option such as `opt_shoot()`, on `ai_try()`, so a check that fails
costs it nothing. The PC screen gets its ranged row (ranged_ai_components.md R5).

Largest case: `EV_FIRED`, `EV_HIT`, `EV_DAMAGED`, `EV_DIED`, `EV_EFFECT_STARTED` and
`EV_ATTRS_CHANGED`, 6 events, within the half queue events.md R25 allows.

### 6.4 `ACT_AIM`

**Send an item's effect at a tile.** In: actor (alive, placed in an area), item, x, y.
Out: target; kind, the effect.

- **Validate:** the item is carried and stowed (`ACT_R_NOT_CARRIED`, `ACT_R_EQUIPPED`),
  has a `GEN_HOOK_AIM` (`ACT_R_CANT_AIM`) and, if a wand, a charge
  (`ACT_R_NO_CHARGES`), then `act_line()`.
- **Range:** `ACT_AIM_RANGE`, 8, the same as `AI_SIGHT`. A kind has no aim range, and
  one number keeps wands simple.
- **Own tile:** aiming at the actor's own tile picks the actor, so a staff of healing
  can heal its holder. Only aim does this: firing there is refused.
- **Resolve:** nothing to roll. An effect's own saving throw decides what it does.
- **Apply:** `EV_AIMED`, the charge used or the parchment destroyed (D16), then
  `effect_apply()` with the actor as source (D17).

13 kinds use it: 8 wands and staves, and 5 parchments. A row with a pending op, such as
the wand of teleport, uses its charge and does nothing yet (items.md R12).

### 6.5 `ACT_UNLOCK`

**Unlock a feature next to the actor.** In: actor, target or dx, dy, item (a key or a
lock pick). Out: target. It reuses `find_openable()`.

| Item | Rule | Used up |
|---|---|---|
| Key whose group is the lock's, or `KEY_ANY` | Always works | Kept |
| Other key | Refused: `ACT_R_WRONG_KEY` | |
| Lock pick | A roll: d20 + its bonus + Dex modifier against the lock's difficulty. Fails with `ACT_R_PICK_FAILED`, attempted | One use each try |

Other refusals: `ACT_R_NOT_CARRIED`, `ACT_R_CANT_OPEN`, `ACT_R_OUT_OF_RANGE`,
`ACT_R_NOT_LOCKED`. On success the lock clears and `EV_UNLOCKED` is posted. The door
stays closed (D18).

**`ACT_OPEN` changes too:** a closed locked door is refused with `ACT_R_LOCKED`, and
`target` is the door. This replaces the "later: locks" note in its resolve step. It is
a validation, so the AI's check sees it, as ai.md says it should.

**What has to come first:** `feature_state[]` gets the lock (D19), `feature_unlock()`
and the pick rule go in the feature component, and the level generator locks some doors
and places their keys where the player can reach them. That last rule is a level design
question and needs its own requirements, since a locked door with an unreachable key
could block the stairs.

**Callers.** Bump: an open refused with `ACT_R_LOCKED` looks for a fitting key in the
pack and calls `ACT_UNLOCK`. A pick is chosen on the inventory screen, with a list of
tools. The AI opens no locks at first: `ai_step()` already goes round what it can't
open.

### 6.6 `ACT_CAST`

**Cast a spell or ability.** In: actor, kind (a spell number), x, y if it is aimed.
Out: target.

- **Data:** one table of spells: effect, power, MP cost, aimed or not, range. It is
  read by number (G-ARCH-7), with what a creature or class knows kept per kind.
- **Validate:** the actor knows it (`ACT_R_NO_SPELL`), has the MP with `MANA_HAS()`
  (`ACT_R_NO_MANA`), and if aimed, `act_line()`.
- **Apply:** `EV_CAST`, `mana_spend()`, then `effect_apply()`, with the actor as source.
- **Abilities:** a row with cost 0 (D20), for the floating eye's gaze and the like.

It shares its end with `ACT_AIM`. It needs a spec of its own first, for what spells
there are and how they are learned (player_character.md R16, mana.md R13). `ai_can()`
has `AI_CAN_CAST` reserved.

Two "Later" abilities in the creature table don't fit a single target. A dragon's breath
hits an area and a shriek wakes the level, which is a stimulus, not an effect on a
target. Those need `ACT_CAST` to grow an area, or a new op: this spec doesn't settle it.

### 6.7 `ACT_STEAL`

**Take something from a neighbour.** In: actor (an inventory with room), target
(adjacent). Out: item.

- **Validate:** the target has an inventory, with something stealable stowed
  (`ACT_R_NOTHING_TO_STEAL`).
- **Resolve:** a roll against the target, then a random stowed item. Failing is
  attempted, `ACT_R_CAUGHT`.
- **Apply:** `inv_stow()` into the actor, `EV_STOLEN`.

Persistent items are never stealable, so the quest item stays safe (winning.md R6) and
`quest_check()` holds. Equipped items are out of reach. What the thief does next is its
brain's, not the action's, so the action sets no flee state: the AI reacts, as it would
for any other stimulus.

### 6.8 Changes to existing actions

These add no action, and need only the accessors and some reasons.

| Action | Change | What it reads |
|---|---|---|
| `ACT_OPEN` | Refused with `ACT_R_LOCKED` (section 6.5) | `FEAT_LOCKED` |
| `ACT_EQUIP` | A two-handed item needs `SLOT_SHIELD` free as well, and a shield needs no two-handed item wielded: `ACT_R_SLOT_TAKEN`, with the item in the way as `target` | `ITEMF_TWO_HANDS`, 4 kinds |
| `ACT_UNEQUIP` | A cursed item stays on: `ACT_R_CURSED` | `ITEMF_CURSED`, 5 kinds |
| `ACT_PICKUP` | Merges into a stowed stack of the same kind, which also needs no room. Treasure goes to the player's gold (player_character.md R20) | `item_count[]`, kind, subclass |
| `ACT_USE` | Uses one of a count, where a subclass has one | `item_count[]` |
| `ACT_MELEE` | Its end moves to `act_strike()` (D21). No change in behaviour | |
| `ACT_MOVE`, `ACT_CLOSE`, `ACT_WAIT`, stairs, `ACT_DROP` | None needed | |

The flags need a `gen_item_flags()`. Who may equip what is a rule, so it belongs in the
inventory component, called from the action (R12).

A blinded reader and an unconscious actor are other rules to add, as no rule reads
those statuses (attributes.md, "Not built yet"). Unconscious belongs in whatever gives
the actor its turn, not in each action (ai.md R19).

### 6.9 Features: operate, search, disarm

`feature.h` promises chests, fountains and traps, and reserves `FEAT_HIDDEN` and
`FEAT_SPENT`, but `FT_COUNT` is 3. These wait until those kinds are specified.

| Action | What it does | Notes |
|---|---|---|
| `ACT_OPERATE` | Uses a feature on or next to the actor: applies its use hook to the actor, and may mark it spent (`ACT_R_SPENT`) | A new `FCAN_OPERATE`. `gen_hook_of()` already answers for features |
| `ACT_SEARCH` | Clears `FEAT_HIDDEN` on features near the actor, with a roll. Always succeeds, uses the turn | Finding nothing isn't a failure |
| `ACT_DISARM` | Removes a known trap with a roll, or sets it off | Attempted failure; a class ability (player_character.md R16) |

A chest should be `ACT_OPEN` with `FCAN_OPEN`, with something dropping its contents, not
a new action. A trap going off is a reaction to `EV_STOOD_ON` (G-ARCH-3), not an action.

### 6.10 What needs no action

| Mechanic | Why not |
|---|---|
| Throwing | `ACT_FIRE` with thrown ammo in `SLOT_AMMO` (D14) |
| Reading a parchment, eating, drinking | `ACT_USE`, or `ACT_AIM` for aimed parchments |
| Hunger | `ACT_USE`, with `EF_NOURISH` changed (items.md, D11) |
| Rest, running, travelling | A player command that repeats `ACT_WAIT` or `ACT_MOVE`, stopping when something happens |
| Looking, the PC screen | They change nothing, so they use no turn (targeting.md) |
| Bump to unlock | A caller combining actions (R13) |
| Swapping weapons | `ACT_UNEQUIP`, then `ACT_EQUIP`: two turns (R10) |
| Delayed actions | A timer's handler fills a context and calls `act_do()`. None does yet. It must test its actor is alive first |
| Teleport, falling through a hole | Effects and level requests, not actor choices |
| Corroding gear, splitting, webs | Hit effects and reactions |
| Containers, shops | Outside the items design. If they come, each is a spec first |

### 6.11 Memory, and the order of work

| Work | Estimate, with asserts | Source of the estimate |
|---|---|---|
| `act_line()` | 0.2K | |
| `ACT_FIRE`, less what moves to `act_strike()` | 0.9K | Melee is 1,173 bytes |
| `ACT_AIM` | 0.5K | Use is 396 bytes |
| `ACT_UNLOCK` | 0.5K | Open and close share 758 bytes |
| Equip's, unequip's and pickup's rules | 0.4K | |
| **Total for what is needed** | **2.5K** | **892 bytes are free** |

So the needed actions don't fit in page 112, even before `ACT_CAST` and the rest. These
are estimates, and a release build is smaller, but the debug build must fit too (D22).

Outside the page:

| Where | What it needs |
|---|---|
| Common | A `case` in `act_do()` per action, and a byte per reason. 4,711 bytes are free in debug, so check after each |
| Page 56 | The line walk |
| Pages 108/109 | `gen_item_group()`, `gen_item_flags()`, `gen_item_attack()` |
| Page 110 | The bonus in the attack functions and the lock rule. It has 460 bytes left |
| Page 114, 116, 118, 124 | Commands, inventory modes, texts, target purposes |

The order of work is in [actions_plan.md](actions_plan.md).

## 7. Tests

`test_act` covers:

- each action's successes, every refusal and its events;
- check mode giving the same answers while changing nothing, including `rand()`;
- `act_do()` matching the direct calls;
- hits, misses, and hit effects with the kind's power;
- the rooted cases: a move and the stairs fail attempted, the check passes, a blocker
  still comes first;
- that every reason is refused except `ACT_R_MISSED` and `ACT_R_ROOTED`;
- the asserts.

`test_ai` covers `ai_try()` and the AI's use of actions, and `test_ui` covers
`messages_act_failed()` and the inventory screen's checks.

To add with section 6:

- **`test_act`, for D23:** melee and open reach all 8 neighbours and nothing further,
  as they do now, and melee refuses a target on the actor's own tile. No test covers
  that last case today, so add it before the change.
- **`test_act`:** each row of the shooter table in section 6.3; a line stopped by a wall,
  a door and a creature; a shot at the last tile in range, and one past it, the same as
  `target_in_range()`; ammo running out and its slot emptying; a wand at 0; a parchment
  destroyed; the actor as source, so a kill by wand gives XP; `ACT_R_LOCKED`, and each
  key and pick case.
- **`test_block`:** the line walk, and `nge_block_line()` still the same.
- **`test_event`:** the largest action is now fire, against R25.
- **CSpect:** firing, aiming and the projectile, by the user, as animations can't be
  seen in the PC tests.

## 8. Known risks

- **`act_use()` can succeed with no effect.** It destroys the item and ignores
  `effect_apply()`'s result, so if no entity is free for a timed effect the item is
  lost ([effects.md](effects.md), "Known risks"). This breaks R3, and `ACT_AIM` would
  share it, losing a charge.
- **Two distance tests are out of step until D23 is built.** `act_melee.c` and
  `act_open.c` use `act_distance()`'s square, and everything else uses the circle (section
  4.10). They agree at range 1, so the game is right today, and only a range above 1
  would show it. Removing the function closes the risk, so `ACT_FIRE` must not be built
  before it goes, and a new distance test must call `nge_map_within()` (R23).
- **The page is nearly full.** 892 bytes are left, not the 1.1K in memory_map.md, and
  the needed actions want about 2.5K. `check-banks.sh` stops the build if the page
  passes 8K, so this fails safely, but only `BANK_56` or cuts will fix it.
- **`BANK_56` hasn't run.** Nothing in the project uses a `BANK_n` section. A trial on a
  scratch copy built, passed `check-banks.sh` and put code in page 113 of the NEX file,
  but no code past `$E000` has run yet. Step 1 of the plan is alone so that any problem
  shows there.
- **`act_attempted()` has no callers outside the tests.** It costs common memory, and
  callers use `used` instead.
- **Check-heavy AI.** Each check is banked calls into the actions' page, so an AI that
  checks many options has a real cost per turn ([ai.md](ai.md)). `ACT_FIRE`'s check walks
  a line, and `ai_try()` runs it twice for each shot, so a shooting monster's option should
  test sight first.
- **Comments behind the code.** `act.h` says an attempted action's events say so, and
  rooted posts none (section 4.6). `act_internal.h` still mentions `a->cost`, which is gone.
- **A dying target.** A target found earlier may be dead by the time an action uses it, and
  melee tests for that. Delayed actions must test their actor too (section 6.10).
- **zsdcc.** Don't return comma expressions from the endings (T-ZSDCC-2), and copy
  event and struct fields into locals before branching on them (T-ZSDCC-1).
