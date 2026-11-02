# Events: design specification

How Jetset's events work: how code says that something has happened, when the rest
of the game hears about it, and what it may do then.

The queue is in [nge_event.c](../src/nge/event/nge_event.c) and its interface in
[nge_event.h](../src/nge/event/nge_event.h). The game's events are listed in
[events.h](../src/game/events.h) and dispatched by `game_on_event()` in
[game.c](../src/game/game.c). When events are handled is part of the turn engine
([game_loop.md](game_loop.md)), and why IDs in events stay valid is in
[ecs.md](ecs.md).

## 1. Overview

**An event is a fact.** It says that something has already happened: "the rat was
damaged by the player, slashing, 4 hit points". Handlers react to it. They can't
cancel it or change it, and nothing asks permission with one. An action that is
refused says so with a reason code and posts nothing.

**Events wait in a queue.** Posting only records the event, so the code that posts
it carries on and finishes what it was doing. Handlers run later, at fixed points in
the frame, oldest event first. They never run in the middle of an action.

**The event is small and the module is smaller.** An event is 5 bytes: a target, an
ID, a source and two payload bytes. The module is a ring buffer of 16 and a loop, in
common memory, so banked code can post without thinking about pages.

**nge has the queue, and the game has the meaning.** nge doesn't know what any event
means. The game passes it one dispatch function, which calls each system's handler
in a fixed order.

```text
 posters                       queue                     handlers
 -------                       -----                     --------
 actions (act_melee...)  ---+
 components (health...)  ---+
 effects                 ---+-> nge_event_post()
 level, quest, AI        ---+       |
                                    v
                            16 events, in common memory
                                    |
 game_on_timer(), each firing       |                    game_on_event(), each event
 change_level(), after arriving ----+-> nge_event_process() --> anim_on_event()
 play_step(), each frame            ^                       +-> player_on_event()
                                    |                       +-> quest_on_event()
                                    |                       +-> ai_on_event()
                                    |                       +-> messages_on_event()
                                    |                       +-> death_on_event()
                                    |                               |
                                    +---- handlers may post --------+
```

**Handlers do more than draw.** Messages, the screen and animations only show what
happened, but XP for a kill, a monster waking, the quest being found and game over
also come from handlers (section 4.7).

**Every actor starts with the queue empty.** Events are handled after each timer
firing, so an actor's reactions have all happened before the next actor does anything.

## 2. Terms

| Term | Meaning |
|---|---|
| Event | A record that something happened: `nge_event_t`, 5 bytes |
| Post | Add an event to the back of the queue, with `nge_event_post()` |
| Process | Hand each waiting event to the dispatch function, with `nge_event_process()` |
| Dispatch function | `game_on_event()`: the game's one function that calls every handler |
| Handler | A system's `*_on_event()` function, called for every event |
| Target | The entity it happened to, or `NGE_EVENT_NONE` |
| Source | The entity that caused it, or `NGE_EVENT_NONE` |
| Payload | `p1` and `p2`, 8 bits each, 0 if unused. Their meaning depends on the event |
| Cascade | Events posted by handlers, handled in the same `nge_event_process()` call |
| Reaction | What a handler does about an event |
| Advance | One `nge_timer_advance()`: every timer due at the next tick fires |

## 3. Requirements

### What an event is

- **R1** An event records something that has happened. It is named in the past
  tense, `EV_<WHAT_HAPPENED>`.
- **R2** An event can't be cancelled or changed by a handler, and nothing posts one
  to ask whether something may happen.
- **R3** A refused action posts nothing (actions.md, R15).
- **R4** Every event has a row in the table in `events.h`, giving its target, source
  and payloads.
- **R5** Event IDs below `NGE_EVENT_USER` are nge's. The game's start there.

### Posting

- **R6** Posting never blocks and never runs a handler.
- **R7** An event is posted after the change it reports, or in the same piece of code
  as that change, so that the fact is true by the time any handler sees it (section
  4.3).
- **R8** Any code can post at any time, from any page.

### Handling

- **R9** Events are handled in the order they were posted, each exactly once.
- **R10** Each event goes to every handler, in the fixed order of `game_on_event()`.
- **R11** The dispatch function is in common memory. The handlers it calls may be
  banked.
- **R12** A handler gets a const pointer to its own copy of the event, and copies the
  fields it needs into locals first (T-ZSDCC-1).
- **R13** A handler may post events. They go to the back of the queue and are handled
  in the same call.
- **R14** Events are handled after each timer firing, after the player's command, and
  after an arrival on a new level (game_loop.md, R5).
- **R15** A handler that needs a living entity checks. An ID in an event is valid, but
  the entity may be dying.
- **R24** Every timer fires with the queue empty. The events of one firing, and their
  cascades, are handled before the next timer fires.

### Entity IDs

- **R16** Every event is handled before the entities it names are released, so no
  event names a freed ID (ecs.md, R14).

### Limits

- **R17** A full queue is a programming error: posting asserts, and with `NDEBUG` the
  new event is dropped.
- **R18** `nge_event_process()` handles every waiting event, including those posted by
  handlers, until the queue is empty. Debug builds assert after `NGE_EVENT_PROCESS_MAX`
  events, which catches handlers that keep posting.
- **R25** The queue is sized for the largest single action. Debug builds record the
  highest count reached, and a test holds the largest actions known to half the queue.
- **R26** One move posts `EV_STOOD_ON` for at most `STOOD_ON_ITEMS_MAX` items on the
  tile (4), and for every other entity there.

### What events can't be relied on for

- **R19** Derived values are never kept up to date through events (G-ARCH-6,
  effects.md, R17). Events can be dropped, and they come after the action that posted
  them.
- **R27** A consequence is a direct call only if the caller needs its result, or if the
  rest of the action, or another handler of the same event, must see the change. Any
  other consequence may be a reaction.

### Constraints

- **R20** The module is in common memory, all of it (memory_map.md).
- **R21** `event/` touches no hardware, so it runs entirely in the PC tests
  (N-LAYER-3).
- **R22** nge includes nothing of the game. It reaches the game only through the
  dispatch function (N-LAYER-2).
- **R23** `nge_event_init()` empties the queue, whatever was in it, and can be called
  again (N-INIT-1).

## 4. Design

### 4.1 The event

`nge_event_t` ([nge_event.h](../src/nge/event/nge_event.h)):

| Field | Meaning |
|---|---|
| `target` | The entity it happened to, or `NGE_EVENT_NONE` |
| `id` | What happened, `EV_*` |
| `source` | The entity that caused it, or `NGE_EVENT_NONE` |
| `p1`, `p2` | Payloads, 0 if unused |

`NGE_EVENT_NONE` is `0xFF`, the same value as `NGE_ENTITY_INVALID`, so an ID that
means "none" can be passed straight in. `nge_event_post0()` posts an event with only a
target.

Two bytes is little, so larger payloads are split or packed. `EV_XP_GAINED` carries 16
bits in `p1` and `p2`, and `EV_DAMAGED` packs the outcome and damage type into `p2`
with `EV_DMG_PACK()`.

An event may also say less and leave the handler to read the current value:
`EV_GOLD_CHANGED` carries nothing, as `pc_gold` is the new amount.

### 4.2 The queue

```c
static nge_event_t queue[NGE_EVENT_QUEUE];   /* 16: a power of 2, checked at compile time */
static uint8_t head;                         /* oldest event */
static uint8_t count;                        /* events waiting */
```

`nge_event_post()` writes to `(head + count) & (NGE_EVENT_QUEUE - 1)`, so wrapping
costs a mask and no test. `nge_event_process()` runs:

```text
 while events are waiting:
     assert fewer than NGE_EVENT_PROCESS_MAX handled
     copy the oldest event into a local
     take it off the queue
     dispatch(&copy)
```

The copy comes first, so a handler that posts into a full queue reuses the freed slot
and can't overwrite the event it is reading. `nge_event_process()` returns how many
events it handled.

`nge_event_flush()` throws away the waiting events. Only the tests use it, and the
game has no need to (D10).

### 4.3 Posting order

The queue separates when an event is posted from when it is handled. Handlers run
only after the poster has finished, so a poster can put events in the order they
should be told, and a change that follows the post in the same function has still
happened by the time a handler sees it:

| Poster | Order | Why |
|---|---|---|
| `act_strike()` | `EV_HIT`, the damage's events, the effect's | Reads as "hits you", then the damage (actions.md, 4.4) |
| `act_fire()` | `EV_FIRED`, `act_strike()`'s, then the shot used up | The shot comes first, and a stack that runs out is dying when handlers see it |
| `act_aim()` | `EV_AIMED`, the charge used or the parchment destroyed, the effect's | Reads as "you aim", then the effect |
| `act_use()` | `EV_USED`, destroys the item, the effect's | Reads as "you drink", then the effect |
| `health_damage()` | Takes the hit points, `EV_DAMAGED`, `EV_DIED`, destroys the entity | The target is dying when handlers see it |
| `level_send()` | `EV_LEFT_LEVEL`, parks or destroys the entity | The target may be dying when handlers see it |

Everywhere else, the change comes first and its event follows.

**A handler sees the game as it is, not as it was.** Only the event's own fields are
a record of the moment it was posted. When three monsters hit the player in one
advance, each `EV_DAMAGED` has its own `p1`, but `player_on_event()` reads the hit
points from `health_hp[]`, and shows the final value three times.

### 4.4 When events are handled

`nge_event_process(game_on_event)` is called in three places in
[game.c](../src/game/game.c):

| Where | When | Why |
|---|---|---|
| `game_on_timer()` | After each timer firing | Consequences come in order: each actor sees the last one's reactions, and once the player dies, no further advance runs. Animations started here stop the next advance |
| `play_step()` | Once a frame, after the turn | The player's command |
| `change_level()` | After a new level is generated and `EV_ARRIVED` posted | The arrival, and effects ended by parking, before the clean-up releases what they name |

A frame of play, as handled:

```text
 play_turn()
   pass_time(), up to 32 times, while the world can act
     nge_timer_advance()        every timer due at this tick, each in turn:
       the timer's handler        events wait
       nge_event_process()        this firing's events
   player_command()             events wait
 play_step()
   nge_event_process()          the command's events
   change_level(), if asked
     level_generate(), post EV_ARRIVED
     nge_event_process()
   level_release_dying()        IDs are free only from here
   nge_ecs_check_guard()
```

This gives each poster a known delay:

- **The player's command:** handled before any time passes, so no other actor can act
  before its reactions.
- **A timer's handler:** handled as soon as it returns, so the reactions have happened
  before the next timer fires, even in the same tick.
- **Generating a level:** `EV_EFFECT_STARTED` for each innate effect waits for
  `change_level()`'s call. On the first level they wait for the first frame's.

`nge_event_process()` now runs inside `nge_timer_advance()`, which is banked, so
handlers are one banked level deeper than from common code. An animation started by a
firing doesn't stop the tick: `world_can_act()` is asked only between advances, so
everything due at a tick still starts its animation together (animations.md, R8).

No events are handled while a chooser runs, and none are posted, as no time passes
(game_loop.md, R7). A chooser's command posts after the player answers.

### 4.5 The dispatch function

```c
static void game_on_event(const nge_event_t *e)
{
    anim_on_event(e);
    player_on_event(e);
    quest_on_event(e);
    ai_on_event(e);
    messages_on_event(e);
    death_on_event(e);
}
```

Every handler gets every event and chooses what it wants. The order is fixed by these
lines, and two of them matter: `anim_on_event()` is first, so animations start in the
frame the action happened, and `death_on_event()` is last, so "You die..." comes
before "Game over".

| Handler | Memory | Notes |
|---|---|---|
| `anim_on_event()` | Banked, page 126 | |
| `player_on_event()` | Common | Calls banked `player_on_pc_event()` for every event, which keeps common memory free |
| `quest_on_event()` | Banked | |
| `ai_on_event()` | Common | A filter: only events about a living AI entity reach the banked code |
| `messages_on_event()` | Banked, page 118 | |
| `death_on_event()` | Common | In `game.c` |

So each event costs four banked calls, whoever wants it. `ai_on_event()` shows how to
avoid that for a handler that wants few events.

`game_over()` from a handler stops the clock, which takes effect after the tick
(`nge_timer.h`). The rest of that tick still fires, and the game's rules cope with a
dead player: the player stays alive at 0 hit points, attacks on them are refused, and
effects don't act on them (D16). The events still waiting are handled in the same
call, and those of the tick's later firings as they come, so their messages appear
(game_loop.md, R6).

### 4.6 Cascades

A handler's posts go to the back of the queue, behind everything waiting. Today's
cascades:

| Event | Handler | Posts |
|---|---|---|
| `EV_DIED`, killed by the player | `player_on_pc_event()` through `pc_gain_xp()` | `EV_XP_GAINED`, and for each level gained `EV_ATTRS_CHANGED`, `EV_HEALED`, `EV_LEVEL_GAINED` |
| `EV_PICKED_UP`, the quest item, first time | `quest_on_event()` | `EV_QUEST_FOUND` |
| `EV_HIT`, `EV_MISSED`, `EV_DAMAGED` | `ai_on_event()` through `ai_stimulus()` | `EV_AI_CHANGED`, if the state changed |

`NGE_EVENT_PROCESS_MAX` (64) catches handlers that keep posting. It is a guard, not a
budget: the game is turn-based, so a long cascade only delays the next frame (D15).

### 4.7 The events and their reactions

Reactions marked **rule** change the game's state. The rest only show something.

| Event | Posted by | Reactions |
|---|---|---|
| `EV_MOVED` | `act_move()` | anim: a slide |
| `EV_STOOD_ON` | `act_move()`, for each entity on the tile | messages: "You see a..." |
| `EV_USED` | `act_use()` | messages |
| `EV_PICKED_UP` | `act_pickup()` | **rule** quest: found; messages |
| `EV_DROPPED` | `act_drop()` | messages |
| `EV_EQUIPPED`, `EV_UNEQUIPPED` | `act_equip()`, `act_unequip()` | messages |
| `EV_OPENED`, `EV_CLOSED` | `act_open()`, `act_close()` | messages |
| `EV_UNLOCKED` | `act_unlock()` | messages |
| `EV_HIT`, `EV_MISSED` | `act_strike()` | anim: a lunge; **rule** AI: hurt; messages |
| `EV_FIRED` | `act_fire()`, for the player and monsters | messages; the projectile animation when it comes |
| `EV_AIMED` | `act_aim()` | messages |
| `EV_DAMAGED` | `health_damage()` | player: hit points; **rule** AI: hurt; messages |
| `EV_HEALED` | `health_heal()` | player: hit points |
| `EV_DIED` | `health_damage()` | **rule** player: XP, maybe a level; messages; **rule** death: game over |
| `EV_EFFECT_STARTED`, `EV_EFFECT_ENDED` | `effects_core.c` | messages |
| `EV_ATTRS_CHANGED` | `effects_recompute()` | player: the stats |
| `EV_MANA_SPENT`, `EV_MANA_RESTORED` | `mana.c` | player: MP |
| `EV_XP_GAINED` | `pc_gain_xp()` | player: XP |
| `EV_LEVEL_GAINED` | `level_up()` | player: level; messages |
| `EV_GOLD_CHANGED` | `pc_gain_gold()`, `pc_spend_gold()` | player: gold |
| `EV_ARRIVED` | `change_level()` | **rule** player: deepest depth; messages |
| `EV_LEFT_LEVEL` | `level_send()` | messages |
| `EV_QUEST_FOUND` | `quest_on_event()` | messages |
| `EV_AI_CHANGED` | `ai_stimulus()` | messages |

### 4.8 Calls and events

The same kind of consequence is sometimes a call and sometimes a reaction. As built:

| Consequence | How it happens |
|---|---|
| Damage from a hit, and the hit's effect | Called by `act_strike()` |
| A used item's effect | Called by `act_use()` |
| A creature's innate effect | Called by the generator |
| Recompute after equipping, or an effect starting or ending | Called by whatever changed it |
| Destroying what reached 0 hit points | Called by `health_damage()` |
| XP for gold | Called by `pc_gain_gold()` |
| XP for a kill | `EV_DIED` handler |
| Game over | `EV_DIED` handler |
| The quest being found | `EV_PICKED_UP` handler |
| A monster reacting to being hurt | `EV_HIT`, `EV_MISSED` and `EV_DAMAGED` handler |
| The deepest depth reached | `EV_ARRIVED` handler |

No handler applies an effect today, though effects.md, R5, allows it.

What separates the two ways, on this design:

- **The poster's knowledge.** A call needs the caller to know about what it calls. A
  reaction needs only a new handler.
- **Timing.** A call has finished when it returns, and the caller can use its result.
  A reaction comes after the action has finished, but before any other actor acts
  (section 4.4).
- **Order.** A call's events come where the call is. A reaction's events come after
  everything already waiting.
- **Certainty.** A call happens. A reaction depends on its event reaching the queue
  (section 8).

**The rule (R27).** Make it a call if the caller needs the result, or if the rest of
the action, or another handler of the same event, must see the change. Otherwise it may
be a reaction. Against the table above:

| Consequence | Result | Why |
|---|---|---|
| A hit's effect | May be a reaction | `EV_HIT` gives the attacker and weapon. The target is refused if the hit killed it, and the messages keep their order |
| A used item's effect | May be a reaction | `EV_USED` gives the user, and the dying item is still readable |
| Damage from a hit | Stays a call | `a->amount` is the action's result, though only the tests read it today. Damage is what melee does, so moving it separates little |
| Destroying at 0 hit points | Stays a call | Everything else must see it as dying straight away |
| Recompute | Stays a call | Another handler of `EV_EQUIPPED` could read stale values (R19) |

A reaction isn't free. The handler has to know the event's payloads, its place in
`game_on_event()` becomes part of the rules, and a banked handler adds a banked call to
every event unless a filter in common memory stops it (section 4.5).

### 4.9 Limits and costs

| Item | Size |
|---|---|
| The queue | 80 bytes, plus 2 for `head` and `count`, in common memory |
| Code | About 240 bytes in common memory, in a debug build |
| The copy of each event | 5 bytes of stack |
| Pages 46/47 | Reserved for the module, and unused |

How many events one firing, command or level change can leave waiting:

| Case | Events | Result |
|---|---|---|
| A melee miss | 1 | |
| A melee hit | 2 | |
| A hit that kills | 3 | |
| A hit with a timed effect | 3, and 1 more if it changes the target's attributes | |
| A timed effect firing | 3, or 2 if it kills | |
| Using an item | 3 | |
| A move onto a tile | 1 + up to 4 items + each other entity there | R26 |
| Arriving on a level | 1 + one for each innate effect, and each effect ended by parking | Bounded by the creatures on the level |

Handlers drain the queue as they post, so a cascade adds to the peak much less than
it adds to the count. The exception is `pc_gain_xp()`, which posts 3 events for each
level gained in one handler call.

`nge_event_peak()`, in debug builds only, gives the highest count since
`nge_event_init()`.

## 5. Decisions

- **D1 An event is a fact, not a request.** Nothing can veto one, so handlers can run
  in any order without one undoing what another has seen. Permission is asked with a
  check (`ACT_CHECK`) and refused with a reason code.
- **D2 A queue, not a call at the moment of posting.** A handler never runs in the
  middle of an action, with its target half changed, and a poster can order its events
  as they should be told (section 4.3). *Rejected:* calling handlers from
  `nge_event_post()`, where they could change what the poster is working on and a long
  chain could run deep into the banked call limit.
- **D3 One fixed 5-byte event.** The queue is a plain array, and copying an event is
  cheap. *Rejected:* variable payloads, which would need sizes and a byte buffer for
  the two events that want more than two bytes.
- **D4 One dispatch function, with handlers in a fixed order.** The order can be read
  in one place, and is the order things reach the screen. A pointer to a banked
  function doesn't switch banks, so handlers can't be registered in a table (G-ARCH-7).
  The cost is that every handler sees every event (section 4.5). *Rejected:*
  subscribing to event IDs.
- **D5 Common memory.** Nearly every module posts, and most are banked. The module
  needs no mapping, and the banked handlers can be reached from the dispatch function.
  *Rejected:* its own page, as reserved in 46/47, which would add a banked call to each
  post.
- **D6 Handlers get a copy, off the queue.** A handler can post into the slot it came
  from, and can't change what other handlers see. The copy lives on the stack of
  `nge_event_process()`.
- **D7 Handled after each timer firing, not after each advance.** Actions change the
  world at once, so handling events later gives no tick a shared view of the world: it
  only delays reactions. Handling them after each firing means every actor starts
  settled (R24), keeps the queue's peak to one action, and puts a reaction before the
  next actor acts. Animations still start together, as they are held only between
  advances. The cost is one more banked level, and a monster reacting within its tick.
  *Rejected:* after each advance, as first built, where five rats biting at one tick
  fill the queue.
- **D8 Handled before entities are released.** Dying entities stay readable until the
  end of the frame, so handlers can name what died (ecs.md).
- **D9 A full queue is a programming error.** A size is chosen for the worst case and
  an assert says when that was wrong. *Rejected:* overwriting the oldest event, which
  loses it silently in debug builds as well. With D7, the worst case is one action
  (R25), and a queue of 16 fits today's. Actions that touch many entities will need 32
  (section 6).
- **D10 Events aren't flushed on a level change.** A change of level is deferred until
  the frame's events are handled (level_transitions.md, D4), so nothing is left to
  throw away and no message is lost.
- **D11 Handlers may post.** A handler's reaction is itself something that happened,
  and other systems can react to it. The guard catches loops.
- **D12 A call only when a call is needed (R27).** This replaces "part of an action's
  own result" as the test, which said nothing about why. A reaction is safe to use more
  widely because of D7: it lands before any other actor acts. It still isn't free
  (section 4.8), so nothing moves just because it could.
- **D13 Limit the items, not the events.** A pile of items produces one line a step
  with nothing to react to (R26). Other entities are always posted, so a future trap
  under a pile still hears about it. *Rejected:* one `EV_STOOD_ON` a move with a count,
  which would hide a trap, and a larger queue, as a pile has no limit but the inventory.
- **D14 Check the peak, don't trust the sum (R25).** Section 4.9's sizes will go out of
  date as content is added, and a test will not. Half the queue leaves room for the
  cascades.
- **D15 Process until the queue is empty (R18).** No rule has a frame deadline, so
  nothing needs a limit on the work done, and R16 and R24 both need the queue empty
  afterwards. The assert stays in debug builds, where a halt with a message beats a
  silent hang. With `NDEBUG`, handlers that keep posting hang, like any other endless
  loop. *Rejected:* stopping at the limit and leaving the rest for the next call, as
  first built, which breaks R16 and R24 to keep a broken game running.
- **D16 The rest of the tick runs after game over.** The rules already cope with a dead
  player, so no firing needs stopping, and no advance follows. The only cost is that a
  monster later in that tick can act, and its message can follow "You die...".
  *Rejected:* `game_on_timer()` testing `ended` first, which adds a rule and common
  memory for a cosmetic gain.

## 6. Not built yet

| Item | What's there, what's missing |
|---|---|
| Hit and use effects as reactions | Allowed by R27 and still calls. Moving them needs a handler in the game's side of effects, with a filter in common memory |
| A queue of 32 | Needed by the first action that touches many entities, such as an area spell: about 3 events a target. It costs 80 bytes of common memory |
| nge's own events | IDs 0 to 15 are reserved, and nothing uses them |
| Events with no target | The queue allows them, and no game event has one |
| Reactions to `EV_STOOD_ON` | Only a message. Traps and the like would react here (level_transitions.md) |
| An event's handlers filtering early | Only `ai_on_event()` does it (section 4.5) |

## 7. Tests

`test_event` covers:

- the size of an event, and `NGE_EVENT_NONE` matching `NGE_ENTITY_INVALID`;
- posting: every field, in order, and each event once;
- a full queue asserting, and order kept across the wrap;
- flushing, and init emptying the queue;
- cascades: events from handlers coming after those waiting, in the same call;
- handlers getting a copy, and a handler posting without overwriting its event;
- the guard on runaway handlers, and a `NULL` dispatch function.

The tests of each poster (`test_act`, `test_health`, `test_effects`, `test_pc`,
`test_mana`, `test_level`, `test_ai`, `test_ui`) record the events it posts and check
them.

`game_on_event()` is static and `play_run()` waits for real frames, so the order of
the handlers and the three calls to `nge_event_process()` have no PC test. The CSpect
checks in game_loop.md, section 8, cover part of this, such as a death's message
coming before "Game over".

`test_event` also covers the peak and init clearing it. `test_act` also covers a move
onto a pile of 20 items, which posts `EV_MOVED` and 4 `EV_STOOD_ON`, with a feature
under the pile still posted, and holds the largest actions to half the queue.

The CSpect checks of events handled after each firing passed on 2026-10-31: monsters at
one tick animating together, death, message order, a level-up in view, an item pile and
stairs with an effect running. Not yet run on hardware.

## 8. Known risks

- **The queue is safe only while R25 holds.** Overflow now needs a single action larger
  than the queue. An area spell, or a kill worth several levels, can be one, and an
  action can't drain the queue part way through (D2). Before R24 and R26, walking onto 16
  dropped items, or five rats all biting in one advance on a turn regeneration fired,
  overflowed it (both measured).
- **A dropped event can be a rule.** R19 protects derived values, but section 4.7's
  rules have no such protection, and R27 adds to them. If `EV_DIED` were dropped for the
  player, by reading the code: the player is left at 0 hit points,
  `health_damage()` calls them dead already, nothing calls `game_over()`, and play goes
  on. A dropped `EV_DIED` for a
  monster loses its XP, and a dropped `EV_PICKED_UP` means the quest isn't marked found.
- **Handlers recompute.** `EV_DIED` → `player_on_pc_event()` → `pc_gain_xp()` →
  `level_up()` → `effects_set_base()` → `effects_recompute()` is a derived value
  recomputed from a handler, which G-ARCH-6 and effects.md forbid ("Derived values are
  never recomputed by a handler"). The value stays right, as the change and its
  recompute stay together: if the event were dropped, neither would happen.
- **The documents have moved apart from the code.** Rule 6 in
  [src/game/README.md](../src/game/README.md) says events "carry reactions only:
  messages, the screen", which section 4.7 contradicts. P-EVENT-1 says
  handlers are in common code, and three of them are banked. `nge_event.h` names a
  `stats_on_event()` that doesn't exist, and suggests a flush on a level change, which
  the game doesn't do (D10).
- **Late readers.** A handler reading a table sees the current value (section 4.3).
  This is harmless on the screen, but a new handler that needs the value at the moment
  of the event has to carry it in a payload.
- **A dead player's effects.** An effect ending on a dead target skips its recompute
  (effects.md, R15), so `effects_check()` in `player_on_turn()` would fail if the
  player's timer fired afterwards. It can't today: the player is ID 0 and fires first in
  a tick, and no advance follows a death. Changing either would expose it.
- **Banked depth.** Handlers run inside `nge_timer_advance()` (section 4.4). The deepest
  chain from a handler is 6 of the 8 banked levels allowed: the advance, then
  `player_on_pc_event()`, `pc_gain_xp()`, `effects_set_base()`, `effects_recompute()` and
  `nge_timer_set_period()`. This comes from a count of the source, and the deepest
  message chain is shorter. A new handler that applies an effect should be counted
  again (T-STACK-2).
- **zsdcc.** Handlers read struct fields in code that branches, so they copy them into
  locals first (T-ZSDCC-1). `messages_on_event()` has a comment on what happened
  without this, and the PC tests can't catch it.
