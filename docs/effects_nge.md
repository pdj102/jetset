# Effects in nge: design specification

How active effects are split between nge and the game: nge sets up an
active effect (an entity owned by its target, with a timer that carries two
bytes of data) and finds a target's active effects; the game decides
everything else.

**Status: built (second version).** It replaces the first
version, built in commits 572fb7e and 5f89404, in which nge ran the whole
lifecycle and called the game through hooks. Section 12 lists what the second
version removes. The review questions are answered (section 7).

## 1. Overview

**Why the split moves again.** The first version gave nge the lifecycle of
an active effect: starting, refreshing or stacking, firing, expiring and
ending, with four hooks into the game. That took a hook struct, a dispatch
function and a trampoline in common memory, a reserved timer kind, a
component with data in page 59, and a banked call more on every firing. The
game still made every real decision: whether to refresh or stack was a flag
it passed, and what a firing, a refresh or an ending meant was its hook's.

What nge has to offer is smaller: **setting up an owned entity with a
timer, and finding them again.** Once the timer can carry the effect's data,
an active effect is just that, and nge doesn't need to know what happens to
it.

**The new split:**

```text
 callers                  game effects module (120/121)            nge
 -------                  -----------------------------            ---
 act_use, act_melee,
 act_cast, generators --> effect_apply()
                            instant: on_start op, event
                            timed: one of the kind there
                            (NGE_EFFECT_FOR_EACH) and not
                            stacking: restart its timer --------> nge_timer_add()
                            raise its power (payload)
                            else: ------------------------------> nge_effect_create()
                                                                    entity, owner, tag,
                                                                    timer and payload
                            on_start op, recompute, event

 nge_timer_advance() ---> game_on_timer() (common)
                            TIMER_EFFECT, on the level:
                          effects_on_timer()
                            on_tick op, saving throw,
                            last cycle: ending
 effect_cure(), OP_CURE,
 leaving a level -------> walks NGE_EFFECT_FOR_EACH
 ending:                  reads kind and power (payload),
                          then -----------------------------------> nge_effect_destroy()
                          on_end op, recompute, event
```

**nge knows nothing about what an effect is for.** It knows that an active
effect is an alive entity, owned by another, with the effect tag and a
timer. What the timer's kind and payload mean, when one is created, and when
it goes, are the game's.

## 2. Terms

| Term | Meaning |
|---|---|
| Active effect | An entity with `NGE_COMPONENT_EFFECT` (a tag, no data) and a timer, owned by its target |
| Target | An active effect's owner: an entity |
| Payload | Two bytes per entity in nge_timer's tables, `nge_timer_payload[0]` and `[1]`, that nge_timer never reads: what they mean is the timer kind's |
| Timer kind | The handler number of the timer, as today. Active effects use one (or more) of the game's kinds |
| Kind (of effect) | The game's `EF_*`: in the game, payload 0 |
| Power | The game's strength of an active effect: in the game, payload 1 |

## 3. Requirements

### nge_timer

- **R1** A timer carries a payload of two bytes, stored with the rest of the
  timer's tables in slot 0, and saved with them.
- **R2** nge_timer never reads, sets or clears the payload. It is the timer
  kind's: whoever adds the timer writes it, and its handler reads it.
- **R3** The payload can be read without a banked call, as the timer's other
  tables can.
- **R4** nge_timer fires every kind through the game's dispatch function. It
  has no reserved kinds (4.2).

### nge_effect

- **R5** nge_effect includes nothing of the game. Like any nge module, it may
  include only the folders before it in the chain (src/nge/README.md).
- **R6** Creating an active effect makes an entity, owned by its target, with
  the effect tag and a timer of the kind, period and cycles given, and the
  payload given. It returns the entity, or `NGE_ENTITY_INVALID` if no entity
  is free.
- **R7** An active effect goes when its target goes (ownership, as today).
- **R8** Destroying an active effect destroys its entity. It is dying at
  once: no longer found by the iterator, and its timer no longer fires.
- **R9** An iterator visits a target's alive active effects.
- **R10** nge_effect calls nothing of the game's: no hooks, no dispatch, no
  events. Its firings are the game's timers.
- **R11** nge_effect decides nothing: not refreshing, stacking, expiring or
  ending. It holds no data of its own.

### The game's effects and attributes modules

- **R12** Callers keep today's interface: `effect_apply()`, `effect_end()`,
  `effect_cure()`, `effects_recompute()`, `effects_check()`,
  `effects_set_base()`, `effect_text()` and `effect_save_of()`. Callers that
  read an active effect's kind or power use the game's macros (5.2).
- **R13** Behaviour is unchanged (effects.md, R5 to R17; attributes.md):
  refreshing, stacking, firings, saving throws, cures, expiry, the off-level
  rule, the events and their order.
- **R14** No module adds a library routine to common memory (effects.md,
  R20).

## 4. Design: nge

### 4.1 nge_timer's payload

```c
/* Tables in slot 0 (nge_timer_cdata.c) */
#define NGE_TIMER_PAYLOAD  2
extern uint8_t nge_timer_payload[NGE_TIMER_PAYLOAD][NGE_ENTITY_MAX];   /* the kind's data: nge_timer never touches it */
```

- **Layout:** one row per byte, indexed by entity ID
  (`nge_timer_payload[n][id]`), so each row is a plain byte table with a
  constant base, the cheapest access on the Z80 (memory_map.md, "One table
  per field"). `[id][n]` would need the ID doubled on every access.
- **Type:** `uint8_t`, as `nge_event`'s `p1` and `p2`. A signed value (the
  game's power) is cast on the way in and out (D4).
- **Writing it:** whoever adds the timer writes the payload directly: it is
  the kind's data, not nge_timer's, so the "read-only for game code" rule of
  the other tables doesn't apply to it. `nge_timer_add()` keeps its
  signature and leaves the payload as it is, so restarting a timer
  (`nge_timer_add()` again) keeps it (D3).
- **Meaningful** from when it is written until the entity is released:
  removing the timer, or the entity dying, leaves it as it was.
- **Size:** 200 bytes of slot 0 at `NGE_ENTITY_MAX` 100, less the 4 of the
  reserved kinds' handlers: page 50 went from 6,523 to 6,719 bytes of the
  7,936 from `$0100` (`_nge_timer_payload` at `$1A73`), so about 1.2K is
  left.

### 4.2 nge_timer without reserved kinds

With nge_effect no longer firing its own timers, no reserved kind is used.
The second version removes them (Q2): `NGE_TIMER_KIND_USER`,
`NGE_TIMER_EFFECT`, `nge_timer_handler[]`, `nge_timer_set_handler()`,
`nge_timer_handler_fn`, and the branch in `nge_timer_advance()`. Every kind
goes to the game's dispatch, as before step 1 of the first version. The
game's kinds are numbered from 0 again.

### 4.3 nge_effect's interface

```c
/* Each of target's alive active effects, last to first */
#define NGE_EFFECT_FOR_EACH(i, effect, target) \
    NGE_OWNER_FOR_EACH_OWNED(i, effect, target) \
        if (!NGE_ENTITY_HAS_COMPONENT(effect, NGE_COMPONENT_EFFECT)) {} else

/* An entity owned by target, tagged, with a timer and its payload: the effect, or NGE_ENTITY_INVALID if none is free */
nge_entity_id_t nge_effect_create(nge_entity_id_t target, uint8_t kind, uint8_t period, uint8_t cycles,
                                  uint8_t p0, uint8_t p1) NGE_BANKED;

/* Destroy an alive active effect: it is dying, so the iterator no longer finds it */
void nge_effect_destroy(nge_entity_id_t effect) NGE_BANKED;
```

- **`nge_effect_create()`** asserts that the target is alive and the period
  isn't 0. It creates the entity, sets its owner, adds the tag, writes the
  payload and adds the timer (`nge_timer_add()`). Its first firing is one
  period later.
- **`nge_effect_destroy()`** asserts an alive entity with the tag, owned by
  an entity, and calls `nge_entity_destroy()`. It doesn't remove the timer: a
  dying entity's timer never fires (`nge_timer_advance()` fires alive
  entities only), and its components are cleared when it is released. Its
  owner and payload stay readable until then.
- **Only the effect's ID** is passed to destroy (D7): the target is
  `NGE_OWNER_OF(effect)`, in slot 0.
- **One iterator, alive only** (D8).
- **No accessors:** the payload is in slot 0 (R3), read with
  `nge_timer_payload[n][effect]`.

### 4.4 The component

`NGE_COMPONENT_EFFECT` stays `NGE_COMPONENT(4)`, now a tag with no data: it
marks an owned entity as an active effect, for the iterator and for the
clean-up and level code that tests it today. `NGE_COMPONENT_USER` stays 5, so
no game component moves.

### 4.5 Files and pages

| File | Page | Contents |
|---|---|---|
| `src/nge/ecs/systems/nge_effect.h` | | The interface |
| `src/nge/ecs/systems/nge_effect.c` | 58 | `nge_effect_create()`, `nge_effect_destroy()` |

Removed: `nge_effect_pdata.c` (page 59 is then unused), `nge_effect_timer.c`
(the trampoline, common), `nge_effect_internal.h`.

### 4.6 Banking

nge_effect calls `nge_entity_create()`, `nge_owner_set()`, `nge_timer_add()`
and `nge_entity_destroy()`, each a banked call one level deeper than its
caller. It reads and writes only slot 0, and calls nothing of the game's, so
nothing it holds can be mapped out under it.

## 5. Design: the game

### 5.1 What changes

| First version | Second version |
|---|---|
| `game_effect_hook()` (`game.c`), `effects_hook()` (`effects.c`), `effects_on_hook()` | Removed. `game_on_timer()` gets a `TIMER_EFFECT` case again, gated by `level_holds()` as `TIMER_AI` is, calling the banked `effects_on_timer()` |
| `nge_effect_init(game_effect_hook)` in the game and the tests | Removed: nothing to register |
| `TIMER_PLAYER`, `TIMER_AI` from `NGE_TIMER_KIND_USER` | `TIMER_EFFECT`, `TIMER_PLAYER`, `TIMER_AI` from 0 (4.2) |
| Events handled before each game firing as well as after, and after each advance (first version, D18) | After each firing, in `game_on_timer()`, as before: every firing comes through it again |
| `effect_def_t.flags & EFFECT_STACK` passed to nge | `EFFECT_STACK` a game flag again, read by `effect_apply()` |
| `nge_effect_kind()`, `nge_effect_power()` | `EFFECT_KIND_OF(e)`, `EFFECT_POWER_OF(e)` in `effects.h`: slot 0 reads, no banked call |

### 5.2 The payload

```c
/* effects.h: an active effect's data, in nge_timer's payload (slot 0) */
#define EFFECT_KIND_OF(e)   (nge_timer_payload[0][e])           /* EF_*, timed: below EF_TIMED_COUNT */
#define EFFECT_POWER_OF(e)  ((int8_t)nge_timer_payload[1][e])   /* not 0 */
```

The attributes module's `derive()`, the PC screen and the tests use these. A
recompute then reads each active effect's kind and power without the two
banked calls per effect it makes today.

### 5.3 Applying

`effect_apply(ef, power, target, source)`, timed, from its copy of the row:

1. **Not stacking:** it walks `NGE_EFFECT_FOR_EACH` for one of kind `ef`. If
   there is one, it restarts its timer with
   `nge_timer_add(e, TIMER_EFFECT, def.period, def.cycles)` (the payload is
   kept), raises its power if the new one is higher, recomputes if it rose,
   and posts `EV_EFFECT_STARTED`. Done.
2. **Otherwise:** `nge_effect_create(target, TIMER_EFFECT, def.period,
   def.cycles, ef, (uint8_t)power)`. `NGE_ENTITY_INVALID` means refused.
3. **Started:** `on_start` with the effect as its cause, recompute,
   `EV_EFFECT_STARTED`.

These are today's `REFRESHED` and `STARTED` hooks, run in line.

### 5.4 Firing

`game_on_timer()`, `TIMER_EFFECT`: if `level_holds(id)`, the banked
`effects_on_timer(id, cycles)`, which does today's `TICK` hook and then
nge's expiry:

1. If the target can be affected, `on_tick` at the stored power.
2. If the kind has a saving throw, and the effect and target are still
   alive, the roll: passed, the effect ends as saved.
3. If `cycles` is 1 and the effect is still alive, it ends as expired.

An effect off the level isn't fired, as today. Only effects that last for
ever are left off the level (effects.md, D9), so the skipped firing never
misses an expiry.

### 5.5 Ending

One static `end(effect, reason)` in `effects_bank.c`, used by expiry,
saves, cures, `effect_end()` and leaving a level:

1. Copy the target (`NGE_OWNER_OF`), kind and power.
2. `nge_effect_destroy(effect)`: dying, so the recompute no longer counts it.
3. If the target can be affected: `on_end`, recompute, `EV_EFFECT_ENDED` with
   the reason.

This is today's `ENDED` hook, run in line: the order (destroy first) and the
reasons are unchanged.

### 5.6 What the game keeps from the first version

The attributes module (pages 138/139, `game/attrs/`), `effects_defs.h`,
`effects_internal.h`, the game ops `OP_CURE` and `OP_RAISE_BASE`, and the
end reasons. `EFFECT_END_EXPIRED` is 0 by the game's choice, no longer tied
to an nge constant.

### 5.7 Banked-call depth

The deepest chain was an AI's hit that starts a timed effect: 8, the limit
(T-STACK-2). It becomes `nge_timer_advance()`, `ai_on_turn()`,
`act_melee()`, `effect_apply()`, `effects_recompute()`, `gen_hook_of()`: 6,
with `nge_effect_create()` and its own calls (also 6) returned before the
recompute. A firing that ends its effect: `nge_timer_advance()`,
`effects_on_timer()`, `nge_effect_destroy()`, `nge_entity_destroy()`: 4.

## 6. Decisions

- **D1 nge sets up and finds active effects; the game runs them.** Starting,
  refreshing, stacking, firing, expiring and ending are game rules with one
  user, and each was already decided by the game's data or hook. *Rejected:*
  the first version's lifecycle in nge, with hooks (section 12).
- **D2 The effect's data is the timer's payload.** Everything nge_timer fires
  needs its data in the handler; two bytes cover a kind and a power, and are
  general enough for other timers (a fuse's damage, a summon's kind).
  In slot 0 they are saved with the timer, and read without a banked call.
  *Rejected:* keeping kind and power as nge_effect's component data in page
  59, which needs banked accessors and is not in the saved page; the game
  keeping them in its own component, which is a third table to keep in step
  with the entity.
- **D3 The payload is written directly, and `nge_timer_add()` doesn't touch
  it.** A refresh restarts the timer and keeps the effect's kind and power
  without passing them again, and the actors' timers don't change.
  *Rejected:* payload arguments to `nge_timer_add()`, which changes every
  caller and makes a refresh pass back what is already there; a
  `nge_timer_set_payload()` function, which is a banked call for a byte
  write in slot 0.
- **D4 `uint8_t`, as `nge_event`'s payload** (Q3).
  nge doesn't know whether a byte is signed; an effect kind isn't, and a
  power is. *Rejected:* `int8_t`, which makes every unsigned use cast.
- **D5 `[2][NGE_ENTITY_MAX]`, one row per byte** (Q3). Each row is a table
  with a constant base, as every other table is (4.1).
- **D6 A tag component marks an active effect.** It keeps the iterator
  independent of timer kinds, so the game may use several kinds for
  effects, and an effect stays visible even if its timer has gone (a game
  bug, 11). *Rejected:* finding active effects by timer kind, which loses an
  effect whose timer has run out and ties the iterator to the game's kind
  numbers.
- **D7 Destroy takes only the effect.** Its target is in slot 0
  (`NGE_OWNER_OF`); passing it as well could disagree with it, and the
  assert would have to check it anyway.
- **D8 One iterator, over alive effects.** A dying effect has ended: its
  timer doesn't fire, the recompute must not count it, and the clean-up moves
  or destroys it with the ownership functions, which already see dying
  entities. No caller needs dying effects. *Rejected:* a second iterator
  over all effects in use, with no caller.
- **D9 Destroy doesn't remove the timer.** A dying entity's timer never
  fires, and its components are cleared on release; removing it would be one
  more banked call.
- **D10 nge_effect stays a module of its own** in page 58. It depends on
  timer and owner; putting it in either would make one system depend on
  another (the reason for the first version's R10d). Page 59 holds nothing.
- **D11 Expiry is the game's.** The game's handler gets `cycles` and ends
  the effect on 1, after its tick op and saving throw. *Rejected:* nge_timer
  destroying an entity whose last cycle has fired, which would end an effect
  without the game's on_end, recompute and event.
- **D12 The off-level rule is back in `game_on_timer()`,** beside the AI's,
  where it was before the first version.
- **D13 Name: `nge_effect`, prefix `nge_effect_`** (Q1). The module is about
  active effects and nothing else; keeping the name keeps
  `NGE_COMPONENT_EFFECT`, `NGE_EFFECT_FOR_EACH` and the file names. "Active
  effect" stays the term in the documents. *Rejected:* `nge_active_effect`,
  longer at every call for the same meaning.

## 7. Questions

None open. Q1 (the name) was answered by D13, Q2 (the reserved timer kinds)
by 4.2, and Q3 (the payload's type and layout) by D4 and D5.

## 8. Implementation steps

Each step builds, passes `make test` and behaves the same in the game. The
three code steps were built together, in one change:

1. **nge_timer's payload:** the table, with PC tests (kept across
   `nge_timer_add()` and `nge_timer_remove()`).
2. **nge_effect, second version:** `nge_effect_create()`,
   `nge_effect_destroy()`, the tag; its PC tests rewritten. The game switched
   in the same step, as the old interface goes: `effect_apply()` and the
   ending in line, `effects_on_timer()`, the `TIMER_EFFECT` case, the
   payload macros; the hooks, the trampoline, page 59's data and
   `nge_effect_init()` removed, in the game and in the tests.
3. **Reserved kinds:** their code and table removed, the
   game's kinds from 0, `test_nge_timer`'s reserved-kind cases removed.
4. **Docs:** section 9.

## 9. Documents this changes

| Document | Sections |
|---|---|
| effects.md | 1, 2, R13, R14, R18, 4.1, 4.3, 4.4, 4.7, 4.10, D5, D7, 7, 8 (proposed with this spec) |
| attributes.md | Where it reads an active effect's kind and power |
| memory_map.md | Page 58's contents, page 59 unused, slot 0's timer tables, page 120's contents |
| ecs.md, src/nge/README.md | The effect component as a tag, the timer's payload, no reserved kinds |
| nge_timer.h, nge_effect.h (file comments) | The payload; nge_effect's new scope |
| effects.h, timers.h, game.c (comments) | Firings come through `game_on_timer()` again |

## 10. Tests

- **`test_nge_timer`:** the payload is kept by `nge_timer_add()`,
  `nge_timer_remove()` and a firing; every kind goes to the dispatch (the
  reserved-kind cases go).
- **`test_nge_effect`** (rewritten): create sets owner, tag, timer and
  payload; no free entity; destroy makes it dying, gone from the iterator,
  its timer not fired, owner and payload still readable; the target going
  takes its effects; the asserts (dead target, period 0, destroying a
  non-effect or a dying one).
- **`test_effects`:** unchanged in what it covers (effects.md, section 7):
  refresh, stacking, firings, expiry, saves and cures are the game's again,
  and it already tests them through `effect_apply()`.

## 11. Known risks

- **Expiry is the game's.** If a handler doesn't end its effect on its last
  cycle, nge_timer removes the timer and the effect stays, counted by every
  recompute for ever. The tag keeps it visible (D6); `test_effects` covers
  running out.
- **The payload is unchecked.** nge_timer doesn't clear it, so a timer whose
  payload isn't written reads whatever the entity's last user left.
  `nge_effect_create()` always writes it; the actors' kinds don't read it.
- **Payload writes from game code.** The rows are the only nge_timer table
  the game writes. A wrong index writes another entity's payload; the
  macros (5.2) keep the indexing in one place.
- **Timer renumbering.** The game's kinds move again. They are stored
  only in `nge_timer_kind[]` at run time and nothing saves them.
- **Slot 0.** 200 more bytes, 4 fewer without the handler table.
- **zsdcc.** `EFFECT_POWER_OF()` casts a byte to `int8_t`; scaling depends
  on promotion and sign (effects.md, section 8). The PC tests can't catch a
  miscompile, so the scaling code needs reading in the asm.

## 12. What the second version removes

| First version | Why it goes |
|---|---|
| The hooks: `nge_effect_hook_t`, `NGE_EFFECT_STARTED` to `ENDED`, `nge_effect_hook_fn`, `nge_effect_init()` | The game runs the steps in line (5.3 to 5.5) |
| `game_effect_hook()`, `effects_hook()`, the trampoline `nge_effect_timer()` | Common memory: 4,203 bytes free in the debug build, against 4,097 with the first version (and 4,201 before it) |
| `nge_effect_start()`'s refresh and stack, `NGE_EFFECT_STACK` | The game's rule (5.3) |
| nge's expiry and `NGE_EFFECT_END_EXPIRED` | The game's (5.4, D11) |
| `nge_effect_kinds[]`, `nge_effect_powers[]` (page 59), `nge_effect_kind()`, `nge_effect_power()` | The timer's payload (D2) |
| The reserved timer kinds | No user (4.2) |
| Event handling before each game firing and after each advance (first version, D18) | Every firing comes through `game_on_timer()` again |
| Two levels of the deepest banked chain | 5.7 |
