# Actions: design specification

How Jetset's actions work: what an entity does on its turn, the same for the
player and for monsters.

The public interface is in [act.h](../src/game/actions/act.h), which lists each
action's inputs, outputs and failure reasons. Memory use is in
[memory_map.md](memory_map.md). What drives actions is in
[game_loop.md](game_loop.md) and [ai.md](ai.md).

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
 3. resolve    roll dice and decide the outcome, changing nothing
 4. apply      change the game, post events
                  attempted (a miss) --> reason set, turn used
                  success --> turn used
```

**Check mode asks without doing.** `ACT_CHECK` runs only the first two steps, so it
changes nothing, posts nothing and draws no random numbers. The AI weighs up its
options with it, and the inventory screen uses it to decide what to offer.

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
 inventory screen: ACT_CHECK only
```

## 2. Terms

| Term | Meaning |
|---|---|
| Actor | The entity acting |
| Context | An `act_t`: inputs set by the caller, outputs set by the action |
| Mode | `ACT_DO` does the action, `ACT_CHECK` only checks it |
| Refused | A precondition failed: nothing changed and the turn isn't used |
| Attempted | The action was tried and didn't succeed: the turn is used and events say so |
| Reason | `ACT_R_*`: why an action failed, refused or attempted |
| Turn | One action of an actor; how long it takes depends on the actor's speed |
| Consequence | What an action causes, such as damage or an effect. It uses no turn of its own |

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
- **R9** Validation fails only with refused reasons.

### Time

- **R10** Every action takes one turn of the actor, whatever the action.
- **R11** Consequences use no turn.

### Rules

- **R12** Rules live in components. An action finds and checks its target, then
  calls them.
- **R13** An action never calls another action. Callers combine them.
- **R14** An action says who may act only where a rule needs it, as with leaving the
  dungeon. Otherwise it treats every actor alike.
- **R15** An action posts events only in its apply step. A refused action posts
  nothing.
- **R16** Reactions are event handlers, not code in the action. What the player is
  told about a refused action comes from `messages_act_failed()`.

### Constraints

- **R17** The actions are banked in one page. Only the dispatcher and the reason table
  are in common memory.
- **R18** Actions draw random numbers only in the resolve step.

## 4. Design

### 4.1 The context

`act_t` ([act.h](../src/game/actions/act.h)). Each action reads only what it lists,
so callers zero the context and set those fields.

| Field | In | Out |
|---|---|---|
| `actor` | Who acts | |
| `target` | An entity acted on, or `NGE_ENTITY_INVALID` to find one | The entity found, or what blocked the action |
| `item` | The item used | The weapon used |
| `dx`, `dy` | A direction | |
| `kind` | A slot to equip in | The slot, damage type or effect used |
| `amount` | | The damage taken |
| `reason` | | `ACT_R_*` |
| `used` | | 1 if the turn was used |

A context can be passed straight to the next action. After a move is refused,
`target` already holds the blocker, ready for a melee or an open.

### 4.2 Results

| Ending | Function | `reason` | `used` | Returns |
|---|---|---|---|---|
| Success | `act_succeed()` | `ACT_R_NONE` | 1 | `ACT_SUCCESS` |
| Check passed | `act_checked()` | `ACT_R_NONE` | 0 | `ACT_SUCCESS` |
| Refused | `act_fail()` | Why | 0 | `ACT_FAIL` |
| Attempted | `act_fail()` | Why | 1 | `ACT_FAIL` |

`act_fail()` reads the kind from `act_reason_flags[]`, so an action can't set it
wrongly. `ACT_R_MISSED` is the only attempted reason today.

### 4.3 The actions

| Action | What it does | Events | Random |
|---|---|---|---|
| `ACT_MOVE` | Moves one tile, if the tile can be entered | `EV_MOVED`, `EV_STOOD_ON` for what is there | No |
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

**Melee** rolls 1d20 in the resolve step, and the attack component decides whether
it hits. Damage goes through `health_damage()`, and the hit hook of the weapon, or of
the actor if it has none, applies to a target left alive.

**Equip** takes the first free slot the item fits, or the slot asked for. If every
slot is in use, it is refused, and `target` is the item in the last one.

**Stairs** treat the player differently. The player's move is a request that the
game carries out at the end of the frame, because the whole level changes. Anyone
else leaves at once. Only the player, carrying the quest item, can leave the dungeon
([winning.md](winning.md)).

### 4.4 Event order

Events are posted in the order the messages should read, so a hit's event comes
before the damage it causes:

- **Melee:** `EV_HIT`, then the damage's events, then the effect's.
- **Use:** `EV_USED`, then the item is destroyed, then the effect's. The item is
  dying when handlers see the event, and stays readable until it is released.

Everywhere else, the change comes first and its event follows.

### 4.5 Callers

| Caller | How it calls | On failure |
|---|---|---|
| Player commands | `player_act()`: `ACT_DO` | `messages_act_failed()`; time passes only if `used` is set |
| AI | `ai_try()`: `ACT_CHECK`, then `ACT_DO` if it passed | Tries its next option |
| Inventory screen | `ACT_CHECK` only | Doesn't offer it, or says why |

**Bumping** is the player's move command combining actions: it checks the move, and
if the blocker has health it melees, and if it is a closed door it opens it. The AI
does the same with its own choices.

### 4.6 Banking

Each action is `NGE_BANKED`, in the even page of the actions' pair. `act_do()` and
`act_attempted()` are in common memory, with `act_reason_flags[]`.

- **`act_do()` is a `switch`,** because a pointer to a banked function doesn't switch
  banks (N-BANK-6).
- **`act_shared.c`** holds the endings and `act_distance()`. They aren't banked, so
  only code in the actions' page can call them.
- **The context is on the caller's stack,** in common memory, so it stays readable
  across banked calls.

### 4.7 Adding an action

P-ACT-1 in [patterns.md](guidelines/patterns.md) is the checklist: a function with
the four steps, an `ACT_*` number and a case in `act_do()`, and for each new reason a
row in `act_reason_flags[]` and a text in `messages_act_failed()`.

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

## 6. Not built yet

| Mechanic | What's there, what's missing |
|---|---|
| Firing and throwing | Items and `COMPONENT_RANGED` are ready; there is no action, and the fire command says there is nothing to fire |
| Aiming | Aim hooks are ready ([items.md](items.md), "Not built yet"); there is no action |
| Locks | `ACT_OPEN`'s resolve step has nothing to roll yet |
| Two-handed weapons, sticky curses | `ACT_EQUIP` and `ACT_UNEQUIP` don't read the flags |
| Merging stacks, using up counts | `ACT_PICKUP` and `ACT_USE` treat each item as one |
| Delayed actions | A timer's handler could fill a context and call `act_do()`, but none does |
| Tile targets | `act_t` has `x` and `y`, but no action reads them |

## 7. Tests

`test_act` covers each action's successes, every refusal and its events; check mode
giving the same answers while changing nothing, including `rand()`; `act_do()`
matching the direct calls; hits, misses and hit effects; that every reason is refused
except a miss; and the asserts.

## 8. Known risks

- **`act_use()` can succeed with no effect.** It destroys the item and ignores
  `effect_apply()`'s result, so if no entity is free for a timed effect the item is
  lost ([effects.md](effects.md), "Known risks"). This breaks the rule that apply
  can't fail.
- **`act_attempted()` has no callers outside the tests.** It costs common memory, and
  callers use `used` instead.
- **Check-heavy AI.** Each check is banked calls into the actions' page, so an AI that
  checks many options has a real cost per turn ([ai.md](ai.md)).
- **zsdcc.** Don't return comma expressions from the endings (T-ZSDCC-2), and copy
  event and struct fields into locals before branching on them (T-ZSDCC-1).
