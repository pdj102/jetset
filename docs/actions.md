# Actions: design specification

How Jetset's actions work: what an action is, how it succeeds or fails, who calls it, and
what all actions share. Each action has its own file, listed in section 4.6.

The public interface is in [act.h](../src/game/actions/act.h), which lists each action's
inputs, outputs and failure reasons. The pattern every action follows is P-ACT-1 in
[patterns.md](guidelines/patterns.md). What drives actions is in [game_loop.md](game_loop.md)
and [ai.md](ai.md). What they cause is in [effects.md](effects.md) and [events.md](events.md).
Memory use is in [memory_map.md](memory_map.md).

This spec covers the module in `src/game/actions/`. The rules that actions call, such as
whether an attack hits, belong to the components and are in their headers.

## 1. Overview

**An action is anything an actor chooses to do.** Moving, attacking, picking up, using,
opening and taking stairs are all actions. The player and monsters use the same ones, so
they play by the same rules.

**Every action has the same shape.** It takes a context (`act_t`), which the caller fills
in and keeps on its stack, and a mode. It returns success or failure, and the context says
why it failed and whether the turn was used.

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

**Check mode asks without doing.** `ACT_CHECK` runs only the first two steps, so it changes
nothing, posts nothing and draws no random numbers. The AI weighs up its options with it,
and the inventory screen and the spell list use it to decide what to offer.

**Failing comes in two kinds.** A refused action didn't happen, and the actor can try
something else. An attempted action did happen and didn't work, so the turn is gone. Each
reason is one kind or the other, defined once in a table.

**Actions orchestrate; they don't own rules.** An action finds and checks its target, then
calls component functions, which hold the rules. It posts events for what happened, and
messages, animations and the screen react to those.

**Actions don't call each other.** A caller combines them: moving into a monster is a move
that is refused, followed by a melee.

```text
 player command --> player_act() --+
                                   +--> act_do() or act_<verb>() --> components
 AI option -------> ai_try() ------+                             --> events
                                                                 --> effects
 inventory screen, spell list: ACT_CHECK only, then player_act()
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
| Bump | The caller's habit of turning a refused move into a melee, an open or an unlock |
| Stowed, equipped, carried | In the pack, in a slot, either ([items.md](items.md), section 4.3) |
| Rooted | Has `STATUS_ROOTED`: can't leave its tile ([creatures.md](creatures.md)) |
| Line of fire | The tiles a shot or bolt crosses (section 4.11) |
| Line mode | How a line picks its target: `ACT_LINE_BOLT` or `ACT_LINE_DIRECT` (section 4.11) |
| Bolt | A line that stops at the first creature on it, whoever was aimed at |
| Direct | A line that reaches the creature on the chosen tile, past any on the way |

## 3. Requirements

### The interface

- **R1** Anything an actor chooses to do is an action, and the same actions serve the
  player and monsters.
- **R2** Every action takes a context and a mode, and returns `ACT_SUCCESS` or `ACT_FAIL`.
- **R3** Every action follows the four steps, in order, and its apply step can't fail.
- **R4** Any action can be called by number, through `act_do()`.

### Check mode

- **R5** Check mode changes no game state, posts no events and draws no random numbers. It
  may set the context's outputs.
- **R6** A check that passes means the action won't be refused. It may still be attempted
  and fail.

### Results

- **R7** A failure always gives a reason, and every reason is either refused or attempted,
  defined once.
- **R8** The action sets `used`: 1 on success or an attempted failure, 0 if refused or
  checked, and 0 on the success of an action that takes no time (R10). Callers decide whether
  the turn is over from `used` alone.
- **R9** Validation fails only with refused reasons, and attempted reasons come only after
  the check has stopped.

### Time

- **R10** Every action takes one turn of the actor, whatever the action, except look, which
  takes none (D18, [action_look.md](action_look.md)).
- **R11** Consequences use no turn.

### Rules

- **R12** Rules live in components. An action finds and checks its target, then calls them.
- **R13** An action never calls another action. Callers combine them.
- **R14** An action says who may act only where a rule needs it, as with leaving the
  dungeon or learning a spell. Otherwise it treats every actor alike.
- **R15** An action posts events only once its check has passed. A refused action posts
  nothing.
- **R16** Reactions are event handlers, not code in the action. What the player is told
  about a failed action comes from `messages_act_failed()`.

### Constraints

- **R17** The actions are banked in one section. Only the dispatcher and the reason table
  are in common memory.
- **R18** Actions draw random numbers only in the resolve step.
- **R19** A context lives on the caller's stack, and an action keeps nothing between calls,
  so the module has no variables and no init.
- **R20** Actions are tested on the PC with the components they call, so everything they do
  must work without the Next's hardware (G-TEST-1).

### Conditions

- **R21** A rooted actor that tries to leave its tile fails with `ACT_R_ROOTED`, attempted,
  so time passes for a hold to end. Its check passes, as for an attack that may miss.
- **R22** A blocker comes before being rooted, so a rooted actor can still bump to attack.
  An actor without stats has no statuses and is never rooted. Actions that don't change the
  actor's tile (operate, search, disarm, and the rest) ignore being rooted.

### Ranges

- **R23** Every range or distance test in the game uses `nge_map_within()`, so a range means
  the same tiles everywhere ([nge_map.h](../src/nge/map/nge_map.h)). No other code works out
  a distance (section 4.10).

### Lines

- **R24** `act_line()` takes a line mode, `ACT_LINE_BOLT` or `ACT_LINE_DIRECT`. Every action
  that finds a target along a line passes one, and nothing else changes how a line picks its
  target.
- **R25** A direct line reaches the creature on the chosen tile, past any creature on the
  way. A wall or closed door on the way still stops it with `ACT_R_NO_LINE`, and a chosen
  tile with no creature is `ACT_R_NO_TARGET`, as for a bolt (D12).
- **R26** The mode comes from what is used, not from the action: `ACT_FIRE` is always a
  bolt, `ACT_AIM` reads the item kind's `ITEMF_DIRECT`, and `ACT_CAST` reads the spell's
  shape.
  Since I3 (built, [items_aimed.md](items_aimed.md)), `ACT_AIM` reads the item's shape, a
  field in its flags of which `ITEMF_DIRECT` is one value, and aim and cast share one helper,
  `act_shape_find()`, which also handles tiles and areas.
- **R27** The actor's own tile is unchanged by the mode: aim picks the actor, and fire and
  cast refuse it.

## 4. Design

### 4.1 Files

| File | Where it runs | What it has | Bytes |
|---|---|---|---|
| `act.h` | | The public interface | |
| `act_internal.h` | | The endings, helpers and line modes, for the actions only | |
| `act.c` | Common | `act_do()`, `act_attempted()`, `act_reason_flags[]` | |
| `act_shared.c` | `BANK_56` | The endings, `act_rooted()`, `act_status()`, `act_strike()`, `act_line()`, `act_shape_find()`, `act_find_feature()` | 3,196 |
| `act_move.c` | `BANK_56` | Move ([action_move.md](action_move.md)) | 1,123 |
| `act_pickup.c` | `BANK_56` | Pick up ([action_pickup.md](action_pickup.md)) | 1,054 |
| `act_drop.c` | `BANK_56` | Drop ([action_drop.md](action_drop.md)) | 468 |
| `act_equip.c` | `BANK_56` | Equip, exchange and unequip | 2,060 |
| `act_melee.c` | `BANK_56` | Melee, as far as validation | 1,145 |
| `act_use.c` | `BANK_56` | Use | 396 |
| `act_open.c` | `BANK_56` | Open, close and unlock | 996 |
| `act_stairs.c` | `BANK_56` | Ascend and descend | 782 |
| `act_wait.c` | `BANK_56` | Wait | 161 |
| `act_fire.c` | `BANK_56` | Fire | 893 |
| `act_aim.c` | `BANK_56` | Aim | 813 |
| `act_cast.c` | `BANK_56` | Cast | 602 |
| `act_learn.c` | `BANK_56` | Learn | 690 |
| `act_operate.c` | `BANK_56` | Operate | 232 |
| `act_disarm.c` | `BANK_56` | Search and disarm | 718 |
| `act_spring.c` | `BANK_56` | `act_spring()`: a trap going off, a reaction, not an action ([features.md](features.md), 4.8) | 660 |
| `act_look.c` | `BANK_56` | Look ([action_look.md](action_look.md)) | 291 |

The sizes are from the debug build's map of 2026-11-13, with asserts. Built, they total
16,280 bytes of `BANK_56`'s 16,384 (pages 112 and 113, D15), which leaves 104. Look has no
asserts, as their text didn't fit ([action_look.md](action_look.md), 4.6).

### 4.2 The context

`act_t` ([act.h](../src/game/actions/act.h)). Each action reads only what it lists, so
callers zero the context and set those fields. Zero is not "none" for `target` and `item`:
entity 0 is a real ID, so an action that accepts no target says so with
`NGE_ENTITY_INVALID`.

| Field | In | Out |
|---|---|---|
| `actor` | Who acts | |
| `target` | An entity acted on, or `NGE_ENTITY_INVALID` to find one | The entity found, or what blocked the action |
| `item` | The item used | The weapon or launcher used |
| `dx`, `dy` | A direction, each -1, 0 or 1 | |
| `x`, `y` | A tile: fire, aim and cast read them | |
| `kind` | One byte, named by its use: `slot` to equip in, `spell` to cast or learn | `slot`, `damage` or `effect` used |
| `amount` | | The damage taken; how many were found; in fire's check, its range |
| `reason` | | `ACT_R_*` |
| `used` | | 1 if the turn was used |

Who reads what:

| Action | `target` | `item` | `dx`, `dy` | `x`, `y` | `kind` | `amount` |
|---|---|---|---|---|---|---|
| Move | Out: the blocker | | In | | | |
| Pick up | In or find; out | | | | | |
| Drop | | In | | | | |
| Equip | Out: the item in the way | In | | | In and out: `slot` | |
| Exchange | Out: the first item taken off, or invalid | In | | | In and out: `slot` | |
| Unequip | | In | | | | |
| Melee | In or find; out | Out: the weapon | If `target` is invalid | | Out: `damage` | Out: damage |
| Use | | In | | | Out: `effect` | |
| Open, close | In or find; out | | If `target` is invalid | | | |
| Unlock | In or find; out | In: the key or pick | If `target` is invalid | | | |
| Ascend, descend | Out: the stairs | | | | | |
| Wait | | | | | | |
| Fire | Out: the first in line | Out: the launcher or thrown item | | In | Out: `damage` | Out: damage; range in a check |
| Aim | Out: the target | In | | In | Out: `effect` | |
| Cast | Out: the target | | | In, unless the spell is on the actor | In: `spell` | |
| Learn | | In: the book | | | In: `spell` | |
| Operate | In or find; out | | If `target` is invalid | | Out: `effect` | |
| Search | | | | | | Out: how many found |
| Disarm | In or find; out | | If `target` is invalid | | | |
| Look | | | | In | | |

A context can be passed straight to the next action. After a move is refused, `target`
already holds the blocker, ready for a melee or an open. It can also be passed to the same
action twice, which is what `ai_try()` does: what a check writes to `kind` and `item` is
what the action would choose anyway.

`kind` is an anonymous union: `slot`, `damage`, `effect` and `spell` are names for the same
byte, and each action reads or writes only its own. Two actions in a row still share it, so
an output of one is the next one's input if that reads it.

### 4.3 Results

| Ending | Function | `reason` | `used` | Returns |
|---|---|---|---|---|
| Success | `act_succeed()` | `ACT_R_NONE` | 1 | `ACT_SUCCESS` |
| Success taking no time | `act_succeed_free()` | `ACT_R_NONE` | 0 | `ACT_SUCCESS` |
| Check passed | `act_checked()` | `ACT_R_NONE` | 0 | `ACT_SUCCESS` |
| Refused | `act_fail()` | Why | 0 | `ACT_FAIL` |
| Attempted | `act_fail()` | Why | 1 | `ACT_FAIL` |

`act_fail()` reads the kind from `act_reason_flags[]`, so an action can't set it wrongly. A
debug build asserts that a reason isn't `ACT_R_NONE` and is in range.

`act_succeed_free()` is look's ending (D18). It is a `#define` to `act_checked()`, which sets
the same fields, so it costs nothing; the name says what the action means.

### 4.4 Event order

Events are posted in the order the messages should read, so the event of what the actor did
comes before what it caused:

- **A strike** (melee, fire, a trap): `EV_HIT`, then the damage's events, then the effect's.
- **Fire, aim, cast:** `EV_FIRED`, `EV_AIMED` or `EV_CAST` first, for the projectile
  animation and the message, then the rest.
- **Use, operate:** `EV_USED` or `EV_OPERATED`, then the effect's. A used item is destroyed
  between the two: it is dying when handlers see the event, and stays readable until it is
  released.
- **Exchange:** `EV_UNEQUIPPED` for each item taken off, then `EV_EQUIPPED`.
- **Look:** `EV_LOOKED` alone (p1, p2 the tile); the messages read the tile when they handle
  it.

Everywhere else, the change comes first and its event follows.

Consequences are direct calls, because the rest of the action reads their result
(events.md, R27): a strike reads the damage taken for `amount`.

### 4.5 Reasons

There are 54 reasons with `ACT_R_NONE` (`ACT_R_COUNT`), the last `ACT_R_BLIND` (refused, by
look: "You can't see!"). Each action's file lists the ones it gives, with
the player's text. Six are attempted:

| Attempted reason | Where | Event | What the player is told |
|---|---|---|---|
| `ACT_R_MISSED` | Melee, fire | `EV_MISSED` | The event's message |
| `ACT_R_ROOTED` | Move, ascend, descend | None | "You can't move!" |
| `ACT_R_PICK_FAILED` | Unlock | None | "The lock holds." |
| `ACT_R_DISARM_FAILED` | Disarm | None | "You fail to disarm the (trap)." |
| `ACT_R_SET_OFF` | Disarm | `EV_TRIGGERED` and the trap's | The trap's messages |
| `ACT_R_STUMBLED` | Move, confused | None | `messages_act_failed()`'s text |

Rooted, stumbling, a failed pick and a failed disarm post no event: their text comes from
`messages_act_failed()`. Nothing needs an event for a rooted monster, as the AI tests
`AI_CAN_MOVE` first and doesn't try.

**A failed action's text.** `messages_act_failed()` covers every reason a player can reach.
`ACT_R_MISSED` and `ACT_R_SET_OFF` have no text because their events have one.
`ACT_R_NO_INVENTORY`, `ACT_R_NOT_ITEM` and `ACT_R_NOT_CARRIED` have none because the
player's commands can't produce them: the player has an inventory, and items are chosen
from their own lists. P-ACT-1 asks for a text for each new reason, and these are the
exceptions.

### 4.6 The actions

| Number | Action | File | Spec | Player | AI |
|---|---|---|---|---|---|
| 0 | `ACT_MOVE` | `act_move.c` | [action_move.md](action_move.md) | Movement keys | `ai_step()` |
| 1 | `ACT_PICKUP` | `act_pickup.c` | [action_pickup.md](action_pickup.md) | `g`, `,` | `opt_pickup()` |
| 2 | `ACT_DROP` | `act_drop.c` | [action_drop.md](action_drop.md) | `d`; `d` on the inventory screen | No |
| 3 | `ACT_EQUIP` | `act_equip.c` | [action_equip.md](action_equip.md) | `w`; `w` on the inventory screen | `opt_wield()` |
| 4 | `ACT_UNEQUIP` | `act_equip.c` | [action_unequip.md](action_unequip.md) | `r`; `r` on the inventory screen | No |
| 5 | `ACT_MELEE` | `act_melee.c` | [action_melee.md](action_melee.md) | Bump | `opt_attack()` |
| 6 | `ACT_USE` | `act_use.c` | [action_use.md](action_use.md) | `u`; `u` on the inventory screen | `opt_drink()` |
| 7 | `ACT_OPEN` | `act_open.c` | [action_open.md](action_open.md) | Bump | `ai_step()` |
| 8 | `ACT_CLOSE` | `act_open.c` | [action_open.md](action_open.md) | `c`, then a direction | No |
| 9 | `ACT_ASCEND` | `act_stairs.c` | [action_stairs.md](action_stairs.md) | `<` | No |
| 10 | `ACT_DESCEND` | `act_stairs.c` | [action_stairs.md](action_stairs.md) | `>` | No |
| 11 | `ACT_WAIT` | `act_wait.c` | [action_wait.md](action_wait.md) | `.` | The end of an AI turn |
| 12 | `ACT_FIRE` | `act_fire.c` | [action_fire.md](action_fire.md) | `f`, then a tile | `opt_shoot()` |
| 13 | `ACT_AIM` | `act_aim.c` | [action_aim.md](action_aim.md) | `a`; `a` on the inventory screen; then a tile | No |
| 14 | `ACT_UNLOCK` | `act_open.c` | [action_unlock.md](action_unlock.md) | Bump with a key that fits; `p`, a key or pick, then a direction | No |
| 15 | `ACT_CAST` | `act_cast.c` | [action_cast.md](action_cast.md) | `m`, a spell, then a tile if it needs one | `opt_cast()` |
| 16 | `ACT_EXCHANGE` | `act_equip.c` | [action_exchange.md](action_exchange.md) | `e`; `x` on the inventory screen | No |
| 17 | `ACT_OPERATE` | `act_operate.c` | [action_operate.md](action_operate.md) | `o`, then a direction | No |
| 18 | `ACT_SEARCH` | `act_disarm.c` | [action_search.md](action_search.md) | `s` | No |
| 19 | `ACT_DISARM` | `act_disarm.c` | [action_disarm.md](action_disarm.md) | `D` | No |
| 20 | `ACT_LEARN` | `act_learn.c` | [action_learn.md](action_learn.md) | `b`; `b` on the inventory screen; then the book screen | No |
| 21 | `ACT_LOOK` | `act_look.c` | [action_look.md](action_look.md) | `x`, then a tile; uses no turn | No |
| | `ACT_STEAL`, not built | | [action_steal.md](action_steal.md) | | The bandit |

### 4.7 Callers

| Caller | How it calls | On failure |
|---|---|---|
| Player commands | `player_act()`: `ACT_DO` | `messages_act_failed()`; time passes only if `used` is set |
| AI | `ai_try()`: `ACT_CHECK`, then `ACT_DO` if it passed | Tries its next option |
| AI, aimed | `opt_shoot()` and `opt_cast()`: `ACT_CHECK`, then `ACT_DO` only if the check's `target` is its own | Holds, and tries its next option |
| Inventory screen | `ACT_CHECK` to list and offer; does drop, equip, unequip, use and exchange itself; returns aim, unlock and learn to `item_command()` | Doesn't offer it, or says why on its status line |
| Spell list | `ACT_CHECK` for each spell | Says why it can't be cast |
| Targeting | `ACT_CHECK` at the cursor's tile, for fire, aim, cast and look ([targeting.md](targeting.md) R19) | Colours the cursor; on ENTER, says why and goes on |

**Bumping** is the player's move command combining actions. A step into a known web strikes
it; a step onto any other known trap walks onto it (only `D` disarms). Otherwise it checks the
move, and if the blocker has health it melees, and if it is a closed door it opens it, or
unlocks it if it is locked and a key in the pack fits. The AI makes the same choices for
itself.

**Directions.** Actions, the AI and the player's keys use all 8 directions
([diagonals.md](diagonals.md)), and so do the direction prompts.

**Timers and event handlers** may call actions, but none does (section 6).

### 4.8 Banking

Each action is `NGE_BANKED`, in the 16K section `BANK_56` (pages 112 and 113). `act_do()`
and `act_attempted()` are in common memory, with `act_reason_flags[]`, which the endings
read.

- **`act_do()` is a `switch`,** because a pointer to a banked function doesn't switch banks
  (N-BANK-6).
- **`act_shared.c`** holds the endings and helpers. They aren't banked, so only code in
  `BANK_56` can call them.
- **The context is on the caller's stack,** in common memory, so it stays readable across
  banked calls.
- **The actions keep no data of their own** (R19): what they read is the components', in
  slot 0 or the components' pages.
- **Actions are called from other banked modules** (114 player, 116 screens, 104 spell
  screens, 130 AI, 132 features). The caller's page comes back when the action returns.
- **`check-banks.sh`** stops the build if a `BANK_n` section passes 16K, as the linker
  doesn't check it.

### 4.9 Adding an action

P-ACT-1 in [patterns.md](guidelines/patterns.md) is the checklist:

1. A function `act_<verb>()` with the four steps, in `act_<verb>.c`, with the section's
   pragmas.
2. An `ACT_<VERB>` number and a case in `act_do()`.
3. For each new reason, a row in `act_reason_flags[]` and a text in
   `messages_act_failed()`.
4. Its inputs, outputs, events and reasons in its header comment.
5. An `action_<verb>.md`, and its rows in the tables of sections 4.1, 4.2 and 4.6 here.
6. Tests in `test_act`, including that check mode changes nothing and that the reason
   table's kinds are as intended.

A new `ACT_*` adds 7 bytes to `modes[]` in the inventory screen (page 116), used or not,
because that table is sized by `ACT_COUNT`. Its `case` in `act_do()` costs common memory:
check the map after each.

### 4.10 Ranges

Every distance test in `src/`:

| Where | What it tests | Range | How |
|---|---|---|---|
| `target_bank.c` | The cursor's tile is in range | The caller's | `nge_map_within()` |
| `ai_sense.c`, `hears()` | A sleeper hears the player | `AI_HEAR`, 4 | `nge_map_within()` |
| `ai_sense.c`, `ai_in_sight()` | A creature sees a tile | `AI_SIGHT`, 8 | `nge_map_within()` |
| `act_melee.c` | The target is in reach | The attack's, 1 today | `nge_map_within()`, and not the actor's own tile |
| `act_find_feature()` | The feature is in reach | 1 | `nge_map_within()` |
| `act_line()` | The chosen tile is in range | The caller's | `nge_map_within()` |

**The actor's own tile.** `nge_map_within()` at range 1 includes the tile itself. Melee
refuses that case with a test that the tiles are different. The feature actions allow it,
so an actor can act on the door or trap it stands on.

**Two things that look like distances and aren't.** `nge_block_line()` and
`ai_step_toward()` follow a line, tile by tile, and test no range. Moving is the other: a
diagonal step takes one turn like any step, so moves aren't a measure of range. A bow of
range 6 reaches 6 tiles straight along a row, but (5, 5) is out of range although it is 5
moves away. Nothing in the code counts moves, so nothing disagrees, but the player sees
this in the cursor.

### 4.11 The line of fire

Fire, aim and cast find their target the same way, in `act_line()` (D11, D14). It runs in
validation, so it has no randomness and a check gets the same answer.

```text
 act_line(a, range, how): from the actor, towards (a->x, a->y)

 chosen tile out of range ----------------------------> ACT_R_OUT_OF_RANGE
 the actor's own tile --------------------------------> ACT_R_NO_TARGET
 for each tile along the line, nearest first:
     solid (a wall, a closed door) or the map's edge -> ACT_R_NO_LINE
     ACT_LINE_DIRECT, before the chosen tile --------> passed over
     an alive entity with health above 0 ------------> a->target, done
 reached the chosen tile with nothing ----------------> ACT_R_NO_TARGET
```

The wall test comes first on every tile, so a creature standing in an open doorway doesn't
let the line through a wall. A pit or an open door doesn't stop a line, and a corpse at 0
hit points isn't a target.

**The two modes** (`act_internal.h`):

| Mode | Target | Creatures on the way | Walls on the way | Used by |
|---|---|---|---|---|
| `ACT_LINE_BOLT` 0 | The first creature on the line | Stop it: the first is the target | `ACT_R_NO_LINE` | Every shot; wands, staves and parchments of bolts; `SPELL_BOLT` |
| `ACT_LINE_DIRECT` 1 | The creature on the chosen tile | Passed over | `ACT_R_NO_LINE` | `ITEMF_DIRECT` items; `SPELL_DIRECT` |

`ACT_LINE_COUNT` is 2, and `act_line()` asserts that `how` is below it. Both modes use the
same range, walk and reasons, so the cursor, the check and the AI agree for either.

**The walk.** `nge_block_line()` answers only whether a line is clear, and an action needs
the first tile that stops it. So page 56 has a walk that gives each tile in turn,
`nge_block_walk_start()` and `nge_block_walk_next()`, with `nge_block_line()` built on it.
The line is Bresenham's from the shooter, the same as the AI's sight, so what a monster sees
it can shoot.

**The cursor** uses `nge_map_within()` too, so its green tiles and the action agree (R23).
It can still be green on a tile the action refuses for another reason, such as a wall in the
way, and `messages_act_failed()` then says why. It marks range only and doesn't show who a
bolt would hit.

**The projectile** (animations.md R14). `EV_FIRED` and `EV_AIMED` name the target, so the
animation runs to the right tile in either mode. For a direct line it passes over the
creatures on the way, which matches what happens.

### 4.12 The strike

`act_strike(a, attacker, use, hooks, to_hit, to_damage)` is steps 3 and 4 of every attack, once
validated: melee, fire, and a trap going off. `attacker` is whose attack component is used
(a weapon, a launcher, a thrown item, the actor, a trap), `use` is `ATTACK_MELEE` or
`ATTACK_RANGED`, and `hooks` is the entity whose `GEN_HOOK_HIT` applies.

1. **Resolve:** 1d20, and `attack_hits()` with `to_hit` added. A miss posts `EV_MISSED`
   (p1 the roll, p2 `a->item`) and fails with `ACT_R_MISSED`.
2. **Apply:** `EV_HIT` (p1 the roll, p2 `a->item`), `health_damage()` with
   `attack_damage()` plus `to_damage` (sets `a->damage` and `a->amount`), then the hit hook's
   effect on the target, which `effect_apply()` refuses if the hit killed it.

The roll is in the events, so a message can show it.

### 4.13 The feature finder

`act_find_feature(a, can, cant)` is steps 1 and 2 of open, close, unlock, operate and
disarm. It takes `a->target`, or with `NGE_ENTITY_INVALID` the feature in direction
`a->dx`, `a->dy`, and puts it in `a->target`. In order:

| Test | Reason |
|---|---|
| A feature on that tile | `ACT_R_NOTHING_THERE` |
| Alive and a feature | `cant` |
| Not hidden: unknown to the actor is not there | `ACT_R_NOTHING_THERE` |
| Its kind can do `can` (`FCAN_*`) | `cant` |
| In the actor's area, on its tile or next to it | `ACT_R_OUT_OF_RANGE` |

`act_rooted(id)` says whether an actor can't leave its tile: it has stats and
`STATUS_ROOTED`.

## 5. Decisions

- **D1 One context for every action.** Callers and the AI can treat actions alike, and a
  new action needs no new signature. The cost is that a field's meaning depends on the
  action, so `act.h` lists what each one reads.
- **D2 One function, two modes.** Validation can't drift from the action, because a check
  is the same code stopped early.
- **D3 No randomness before resolve.** A check is free of side effects, so the AI can weigh
  its options without changing the random sequence.
- **D4 Reasons are classified in a table.** The kind is defined once, and tested, so
  actions can't disagree about whether a reason uses the turn.
- **D5 Every action costs one turn.** An actor's timer fires every `speed` ticks, so time
  depends on who acts, not what they do. Callers read `used` and need no costs. Look is the
  one exception, and costs nothing (D18).
- **D6 Callers combine actions.** Bumping is the player's choice, not a rule of movement,
  and the AI decides for itself.
- **D7 Endings are functions.** zsdcc dropped the assignments in a comma expression
  (T-ZSDCC-2), so the endings can't be macros.
- **D8 The player's level change is a request.** A level change replaces everything the
  action's caller is working with, so it waits for the end of the frame
  ([level_transitions.md](level_transitions.md)).
- **D9 Rooted is tested after check mode.** It is attempted, not refused, so a check passes
  and the actor can spend its turn struggling (creatures.md, D4). *Rejected:* a refused
  reason, as a held player could never pass the time by trying to move.
- **D10 Lines take a tile, not a direction.** Fire, aim and cast use `act_t`'s `x` and `y`,
  chosen with `target_choose()`. *Rejected:* a direction prompt, which can only reach 8
  lines.
- **D11 A bolt stops at the first creature on its line.** The target is found by walking
  the line, so a creature in the way is hit in place of the one aimed at. That is what the
  player can see, and it needs no rule for shooting past. *Rejected:* hitting the chosen
  tile only, which lets a shot pass through whatever is on the way. Effects that don't fly
  use a direct line instead (D17).
- **D12 Shooting at nothing is refused.** A line with no creature fails with
  `ACT_R_NO_TARGET`, costing no ammo, charge or mana, and a wall in the way with
  `ACT_R_NO_LINE`. Both are found in validation, so the check and the AI get the same
  answer. *Rejected:* letting the shot land, which needs a ranged miss to place an item and
  gains little.
- **D13 Aim's and cast's effects name the actor as their source.** `EV_DIED` gives XP to its
  source, so a bolt must name the actor, where `act_use()` names the item. Without this a
  kill by wand or spell earns nothing.
- **D14 A shared strike and a shared line.** The resolve and apply of an attack are
  `act_strike()`, and the walk is `act_line()`, both in `act_shared.c`, so fire, aim, cast
  and traps don't repeat them (G-ARCH-1). *Rejected:* an `act_melee()` that takes a range,
  which would leave one function deciding between two kinds of attacker and target.
- **D15 The actions are in a 16K section, `BANK_56`.** Pages 112 and 113 are theirs, which
  gives them 16K for a pragma in each file and an origin in `banks.inc`. They map nothing by
  hand, so they meet the rule for code that extends into slot 7 (memory_map.md).
  *Rejected:* a second pair of pages, as 130/131 is the last in the range. *Rejected:*
  spreading the actions over other pages, as `act_shared.c` isn't banked and its callers
  must share a page.
- **D16 Every distance in the game is `nge_map_within()`.** Melee, the feature actions and
  lines test range with the function that the cursor, sight and hearing use, which leaves
  one rule across the game (R23). *Rejected:* a square distance of the actions' own, which
  would disagree with sight, hearing and the cursor.
- **D17 Two line modes, one walk.** The mode is a parameter of `act_line()`. A direct line
  is the same walk that doesn't stop at creatures before the chosen tile. *Rejected:* a
  second function, `act_line_direct()`, which repeats the range, own tile and wall tests
  (G-ARCH-1). *Rejected:* letting a direct line ignore walls too, as that reaches creatures
  the actor can't see, and the cursor would be green where sight says no.
- **D18 Look is an action that takes no time.** It changes nothing in the world, so it costs no
  turn, but as an action it gets a check, so blindness and sight refuse it and colour the
  targeting cursor with no special case (targeting.md D7). The action ends with
  `act_succeed_free()`, which sets `used` to 0, so no caller changes: they already read `used`
  alone (R8). *Rejected:* a table of free actions, costing common memory for one entry.
  *Rejected:* the caller ignoring `used` for look, a rule of the action in its caller.
  Built 2026-11-13 ([action_look.md](action_look.md) D1, D2).
- **D19 Sight is not the line of fire.** Look tests sight as a monster does, `AI_SIGHT` and
  `nge_block_line()` with `NGE_BLOCK_SOLID`, not `act_line()`: the line of fire refuses the
  actor's own tile and an empty one, and stops at a wall on the chosen tile, which sight sees
  ([action_look.md](action_look.md) D3, D4).

## 6. Not built yet

What applies to every action:

- **Blinded and unconscious.** Being unconscious is in whatever gives the actor its turn, not
  in each action (ai.md R19), and loses the turn (E4.6). Blindness is read by a monster's sight
  (`ai_sense()`) and by the to-hit modifier, and look will refuse it (`ACT_R_BLIND`,
  [action_look.md](action_look.md)). A blinded reader of parchments and books belongs in the
  actions that read, and isn't built.
- **Delayed actions.** A timer's handler would fill a context and call `act_do()`. None does
  yet. It must test that its actor is alive first.
- **Monsters' use of actions.** The AI drops, unequips, closes, takes stairs, aims, unlocks,
  exchanges, operates, searches, disarms and learns not at all.

What needs no new action:

| Mechanic | How it is done |
|---|---|
| Throwing | `ACT_FIRE` with thrown ammo in `SLOT_AMMO` ([action_fire.md](action_fire.md)) |
| Reading a parchment, eating, drinking | `ACT_USE`, or `ACT_AIM` for aimed parchments |
| Rest, running, travelling | A player command that repeats `ACT_WAIT` or `ACT_MOVE`, stopping when something happens |
| The PC screen | It changes nothing, so it uses no turn. Looking was here too, until it became an action that uses no turn (D18) |
| Bump to unlock, bump to strike a web | A caller combining actions (R13) |
| Swapping weapons | `ACT_EXCHANGE`, one turn |
| A chest | `ACT_OPEN`, and the chest spills in reaction to `EV_OPENED` ([features.md](features.md), 4.9) |
| A trap going off | A reaction to `EV_STOOD_ON`, `act_spring()` ([features.md](features.md), 4.8) |
| Teleport, falling through a hole | Effects and level requests, not actor choices |
| Corroding gear, splitting, webs | Hit effects and reactions |
| Containers you look into, shops | Outside the items design. If they come, each is a spec first |

What is still to build for each action is in its own file, under "Not built yet": pick up,
use, exchange, fire, aim, unlock, cast, search, disarm, and the whole of
[action_steal.md](action_steal.md).

## 7. Tests

`test_act` covers, for every action:

- its successes, every refusal and its events;
- check mode giving the same answers while changing nothing, including `rand()`;
- `act_do()` matching the direct call;
- the reason table: exactly the six reasons of section 4.5 are attempted;
- look: a success leaves `used` at 0 in both modes (R8, R10);
- the asserts.

And for what the actions share:

- `act_line()`, called directly, in both modes: range the same as `nge_map_within()` on
  every tile; stopped by a wall, a closed door and the map's edge; not by an open door or a
  pit; the nearest creature first for a bolt, the one on the chosen tile for a direct line;
  an empty chosen tile is `ACT_R_NO_TARGET`; no random numbers;
- the rooted cases: a move and the stairs fail attempted, the check passes, a blocker still
  comes first;
- reach: melee and the feature actions reach all 8 neighbours and nothing further; melee
  refuses a target on the actor's own tile, and the feature actions allow it.

`test_ai` covers `ai_try()` and the AI's use of actions, `test_ui` covers
`messages_act_failed()` and the inventory screen's checks, and `test_target` covers the
cursor's range.

The projectile animation and the messages in play can't be seen in the PC tests: those are
CSpect checks, by the user.

## 8. Known risks

- **`act_use()` can succeed with no effect.** It destroys the item and ignores
  `effect_apply()`'s result, so if no entity is free for a timed effect the item is lost
  ([effects.md](effects.md), "Known risks"). This breaks R3. `ACT_AIM` shares it, losing a
  charge, and so does `ACT_CAST`, losing the MP.
- **`act_attempted()` has no callers outside the tests.** It costs common memory, and
  callers use `used` instead.
- **Check-heavy AI.** Each check is a banked call into `BANK_56`, so an AI that checks many
  options has a real cost per turn ([ai.md](ai.md)). Fire's and cast's checks walk a line,
  and run twice for each shot, so `opt_shoot()` and `opt_cast()` test sight first.
- **A dying target.** A target found earlier may be dead by the time an action uses it, and
  melee tests for that. Delayed actions must test their actor too.
- **`BANK_56` has 104 bytes left** in the debug build of 2026-11-13, with look built. `ACT_STEAL` and the gaps in the action files won't fit in that: the next action
  of any size needs another section, and an action there can't call the shared helpers in
  `act_shared.c`, which aren't banked (D15).
- **zsdcc.** Don't return comma expressions from the endings (T-ZSDCC-2), and copy event and
  struct fields into locals before branching on them (T-ZSDCC-1).
