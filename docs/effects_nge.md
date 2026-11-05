# Effects in nge: design specification

How the effects module is split so that the lifecycle of active effects
becomes an nge module, `nge_effect`, and everything that gives effects their
meaning stays in the game.

**Status: built.** The review questions are answered (D2, D13 to D16), and
section 12 lists where the build differs from the first plan (D17 to D20).
This spec replaces the parts of [effects.md](effects.md) and
[attributes.md](attributes.md) listed in section 9, which are updated to
match.

## 1. Overview

**Why the split moves.** Today the effects core (`effects_core.c`) does two
jobs:

1. **The lifecycle of active effects:** an entity owned by its target, with a
   kind, a power and a timer. It is created, refreshed or stacked, fired every
   period and ended for a reason.
2. **Attribute derivation:** bases plus modifiers, clamped and stored
   (`derive()`, `base_of()`, `effects_recompute()`, `effects_set_base()`,
   `OP_RAISE_BASE`, `effects_game_attrs[]`).

Only the lifecycle is generic. Derivation is game rules: what the attributes
are, which ones are masks, and where their tables live. Most of the seam exists
only because derivation sits in the core (`effects_game_has_attrs()`,
`effects_game_sources()`, `effects_game_store()`, `effects_game_attrs[]`). The
core also compiles against the game: it includes `attrs.h` (for `ATTR_COUNT`)
and `effects.h` (for `EF_COUNT`), and it uses `COMPONENT_EFFECT` and the game's
events.

**The new split:**

```text
 callers                 game effects module (120/121)          nge_effect (58/59)
 -------                 -----------------------------          ------------------
 act_use, act_melee,
 act_cast, generators -> effect_apply()
                           instant: on_start op, event
                           timed: the row's period, cycles,
                           stack flag ---------------------> nge_effect_start()
                                                                create or refresh
                         game_effect_hook() <--- STARTED ----- (one dispatch
                           on_start op, recompute, event        function, common)
 nge_timer, kind NGE_TIMER_EFFECT (nge's own) ----------------> nge_effect_on_timer()
                         game_effect_hook() <--- TICK ---------
                           on the level: on_tick op,
                           saving throw
                           (may call nge_effect_end())
                                                               last cycle: end
 effect_cure(),          walks nge's iterator, decides ------> nge_effect_end()
 leaving a level                                               destroy
                         game_effect_hook() <--- ENDED -------
                           on_end op, recompute, event

                         game attributes module (138)
                         ----------------------------
 equip, take off,
 the hooks above ------> effects_recompute()
                           bases + worn items + active effects (nge's iterator),
                           each kind's modifiers copied from page 121,
                           clamp and store
```

**nge knows nothing about what an effect does.** It holds which effects an
entity has, at what power, and how long each has left. The game is told when
something happens to one and does the rest.

## 2. Terms

| Term | Meaning |
|---|---|
| Kind | A number naming an effect. In the game, `EF_*` |
| Lifecycle | A timed kind's period, cycles and stack flag: fields of the game row, passed to `nge_effect_start()` as arguments |
| Game row | The game's data for every kind: texts, duration, lifecycle, statuses, modifiers, ops, saving throw, innate mark (`effect_def_t`) |
| Active effect | An entity with `NGE_COMPONENT_EFFECT`, owned by its target, with a timer |
| Hook | A call from nge into the game's dispatch function: `STARTED`, `REFRESHED`, `TICK`, `ENDED` |
| Reason | Why an effect ended: a number nge passes through. nge sets only `NGE_EFFECT_END_EXPIRED` itself |
| Reserved timer kind | A timer kind below `NGE_TIMER_KIND_USER`, fired by a handler an nge module registers, never by the game's dispatch |
| Trampoline | A small function in common memory that makes the banked call to a module's handler, because a function pointer can't switch banks |

## 3. Requirements

### nge_effect

- **R1** nge_effect includes nothing of the game. Like any nge module, it may
  include only the folders before it in the chain (src/nge/README.md).
- **R2** It handles timed effects only. Instant and worn effects have no
  lifecycle and stay in the game.
- **R3** An active effect is an entity owned by its target, with a timer. Its kind
  and power are kept with it. It goes when its target goes (effects.md, R9 to R12).
- **R4** When a non-stacking kind is started again, the effect already there is
  refreshed: its timer restarts and the higher power is kept. A stacking kind adds
  another instance (effects.md, R13).
- **R5** If no entity is free, starting an effect is refused and returns
  `NGE_ENTITY_INVALID`.
- **R6** All endings go through one path. The effect is destroyed before the
  `ENDED` hook, so a recompute run by the hook no longer counts it (effects.md, D7
  and D8).
- **R7** nge calls the game only through one dispatch function, in common memory,
  passed at init, as `nge_timer` and `nge_event` do. The hooks are direct calls,
  not events, so derived values are recomputed before the caller continues
  (effects.md, R17).
- **R8** nge never decides whether an effect *should* end, apart from its time
  running out. Cures, innate effects, saving throws and leaving a level are the
  game's decisions, made with `nge_effect_end()` and the iterator.
- **R9** nge doesn't scale amounts, run ops, roll saves or post events.
- **R10** nge has its own code and data pages. It reads no table of the game's:
  a kind's lifecycle comes as arguments.
- **R10a** nge_effect's timers are fired inside nge. The game's timer dispatch
  never sees them, and the game doesn't wire nge_effect into it.

### nge_timer

- **R10b** Timer kinds below `NGE_TIMER_KIND_USER` are reserved for nge's modules,
  as component and event numbers are. The game's kinds start at
  `NGE_TIMER_KIND_USER`.
- **R10c** An nge module registers one handler for each reserved kind it uses.
  `nge_timer` then fires those kinds through that handler, and every other kind
  through the game's dispatch, as it does today.
- **R10d** `nge_timer` depends on no other nge system. A program that doesn't use
  effects doesn't link nge_effect.

### The game's effects and attributes modules

- **R11** Callers keep today's interface: `effect_apply()`, `effect_cure()`,
  `effects_recompute()`, `effects_check()`, `effects_set_base()`, `effect_text()`
  and `effect_save_of()`. Their callers don't change. The one exception is
  `effect_kind_of()`, which goes (D14).
- **R12** What effects act on, what ops do, how saves are rolled, what cures and
  what counts as innate are decided by the game. Behaviour is unchanged
  (effects.md, R5 to R16; attributes.md).
- **R13** Derivation is the game's, in a game attributes module of its own. It
  walks active effects through nge's iterator and equipped items through
  ownership.
- **R14** Neither module adds a library routine to common memory (effects.md,
  R20).

## 4. Design: nge_effect

### 4.1 Files and pages

| File | Page | Contents |
|---|---|---|
| `src/nge/ecs/systems/nge_effect.h` | | The interface |
| `src/nge/ecs/systems/nge_effect.c` | 58 | The code |
| `src/nge/ecs/systems/nge_effect_pdata.c` | 59 | Each active effect's kind and power (200 bytes), and the hook |
| `src/nge/ecs/systems/nge_effect_timer.c` | Common | The timer trampoline, which calls `nge_effect_on_timer()` (section 4.8) |
| `src/nge/ecs/systems/nge_effect_internal.h` | | `nge_effect_on_timer()`, for the trampoline and the tests only |

It goes in `ecs/systems/` beside `nge_timer` because it is a system with a
private component. Pages 58/59 are the first free pair of the `ecs` range
(memory_map.md, D17).

### 4.2 The component

`NGE_COMPONENT_EFFECT` is `NGE_COMPONENT(4)`, and `NGE_COMPONENT_USER` goes from
4 to 5. The game's components are numbered from `NGE_COMPONENT_USER`, so they
move up by one without being edited. `COMPONENT_EFFECT` is removed from the
game's `components.h`.

### 4.3 The interface

```c
#define NGE_EFFECT_STACK  0x01   /* started again: another instance; else the one there is refreshed */

/* What happened: nge_effect_hook_t.what */
#define NGE_EFFECT_STARTED    0   /* a new instance; arg unused */
#define NGE_EFFECT_REFRESHED  1   /* the one there restarted; arg 1 if its power rose */
#define NGE_EFFECT_TICK       2   /* a firing; arg: cycles left, counting this one (0 for ever) */
#define NGE_EFFECT_ENDED      3   /* destroyed already; arg: the reason */

#define NGE_EFFECT_END_EXPIRED  0   /* the only reason nge sets; the game numbers its own from 1 */

typedef struct nge_effect_hook {
    uint8_t         what;     /* NGE_EFFECT_STARTED... */
    nge_entity_id_t effect;   /* dying when ENDED: only its ID is passed on */
    nge_entity_id_t target;
    uint8_t         kind;
    int8_t          power;
    uint8_t         arg;
} nge_effect_hook_t;

/* The game's dispatch function: in common memory */
typedef void (*nge_effect_hook_fn)(const nge_effect_hook_t *h);

/* Registers the hook, and its timer handler for NGE_TIMER_EFFECT; after nge_timer_init() */
void nge_effect_init(nge_effect_hook_fn hook) NGE_BANKED;

/* Start kind at power on target, or refresh it: the effect, or NGE_ENTITY_INVALID if no entity is free.
   period: ticks between firings, 1-255; cycles: firings, 0 for ever; flags: NGE_EFFECT_STACK */
nge_entity_id_t nge_effect_start(uint8_t kind, int8_t power, nge_entity_id_t target,
                                 uint8_t period, uint8_t cycles, uint8_t flags) NGE_BANKED;

/* End an active effect for reason: destroy it, then the ENDED hook */
void nge_effect_end(nge_entity_id_t effect, uint8_t reason) NGE_BANKED;

uint8_t nge_effect_kind(nge_entity_id_t effect) NGE_BANKED;
int8_t  nge_effect_power(nge_entity_id_t effect) NGE_BANKED;

/* Each of target's active effects: a macro over NGE_OWNER_FOR_EACH_OWNED and the component */
#define NGE_EFFECT_FOR_EACH(i, effect, target)  ...
```

`period`, `cycles` and `flags` are the kind's lifecycle, which the game reads
from its own row (5.3). They go by value: a pointer into the game's page 121
would point into slot 7, where nge's own page 59 is mapped while it runs.

### 4.4 Starting

`nge_effect_start()` asserts that the power isn't 0, that the period isn't 0,
that `flags` has no bit but `NGE_EFFECT_STACK`, and that the target is alive.
Then:

- **Not stacking, and one of the kind is there** (found by the iterator): it
  restarts the timer with the period and cycles given, keeps the higher power,
  and calls `REFRESHED` with `arg` 1 if the power rose.
- **Otherwise:** it creates the entity, owned by the target, records kind and
  power, adds the component and the timer, then calls `STARTED`.

nge doesn't keep a kind's lifecycle. The game passes the same row each time it
starts a kind, so a refresh restarts the timer as the first start did.

### 4.5 Firing

`nge_effect_on_timer()` calls `TICK` with the cycles left. The hook may end the
effect (a saving throw) or kill the target. Afterwards, if the effect is still
alive and this was its last cycle, nge ends it with `NGE_EFFECT_END_EXPIRED`.

### 4.6 Ending

`nge_effect_end()` asserts an alive active effect. It copies its target, kind and
power, removes the timer, destroys the entity, then calls `ENDED` with the
reason. The hook gets the values in `nge_effect_hook_t`, not by reading the
dying entity.

### 4.7 Banking

nge_effect's code is in page 58 (slot 6) and its data in page 59 (slot 7). It
maps no page of the game's. The hooks are called through the dispatch function
in common memory, which makes banked calls to the game. The game's code may
then map its own pages, so nge copies anything it needs into locals before each
hook and reads nothing from slot 6 or 7 afterwards.

### 4.8 nge_timer's reserved kinds

Today `nge_timer_advance(dispatch)` hands every firing to the game's dispatch
function, and the game's `switch` calls `effects_on_timer()` for
`TIMER_EFFECT` ([game.c](../src/game/game.c), `game_on_timer()`). Under this
design, nge fires its own kinds:

```c
/* Timer kinds 0 to NGE_TIMER_KIND_USER-1 are nge's; the game's start here */
#define NGE_TIMER_KIND_USER  2
#define NGE_TIMER_EFFECT     0   /* nge_effect's active effects */

/* A reserved kind's handler: in common memory, like the game's dispatch */
typedef void (*nge_timer_handler_fn)(nge_entity_id_t id, uint8_t cycles);

/* Fire reserved kind with handler from now on; asserts kind < NGE_TIMER_KIND_USER */
void nge_timer_set_handler(uint8_t kind, nge_timer_handler_fn handler) NGE_BANKED;
```

- **The table:** `nge_timer_handler[NGE_TIMER_KIND_USER]`, 4 bytes, in slot 0
  with the rest of `nge_timer`'s data. `nge_timer_init()` clears it.
- **Firing:** for a kind below `NGE_TIMER_KIND_USER`, `nge_timer_advance()`
  calls its handler (asserting one is registered). Any other kind goes to the
  game's dispatch, as today. Everything else about firing is unchanged: ID
  order, the period after the handler, and cycle counting.
- **The trampoline:** a function pointer can't switch banks, because
  `banked_call` takes the page from inline call data. The registered handler
  is therefore `nge_effect_timer()`, a few bytes of common code in
  `nge_effect_timer.c`, which makes the banked call to
  `nge_effect_on_timer()`. `nge_effect_init()` registers it.
- **The game's kinds** are numbered from `NGE_TIMER_KIND_USER`:
  `TIMER_PLAYER` and `TIMER_AI` move up, and `TIMER_EFFECT` goes. The case in
  `game_on_timer()` goes too.
- **Two reserved kinds** leave room for one more nge system that uses timers,
  for 2 more bytes of slot 0 (D16).

## 5. Design: the game's effects and attributes modules

### 5.1 What stays and what goes

| Today | After |
|---|---|
| `effects_core.c` | Removed. Its lifecycle part moves to `nge_effect.c`; its derivation part moves to the game attributes module (5.6) |
| `effects_core.h`'s seam | Removed: `has_attrs`, `affectable`, `save`, `op`, `sources`, `store`, `effects_game_attrs[]` and `timer_kind` become plain game code with no seam |
| `effect_def_t` | The game row, as today: it keeps period, cycles and the stack flag, which `effect_apply()` passes to nge. `EFFECT_STACK` is defined as `NGE_EFFECT_STACK`; `EFFECT_INNATE` stays a game flag |
| `effect_kind[]`, `effect_power[]` | Removed: nge's, read through `nge_effect_kind()` and `nge_effect_power()` |
| `effect_kind_of()` | Removed. Its callers (the PC screen, `screen_pc_bank.c`, and `test_effects`, `test_ui`, `test_gen`) call `nge_effect_kind()` and test `NGE_COMPONENT_EFFECT` (D14) |
| `OP_CURE`, `OP_RAISE_BASE` | Game ops, like the others |
| `check-effects-core` | Removed: the nge folder chain enforces R1 |

### 5.2 One table

`effect_defs[EF_COUNT]` (page 121) stays the only table of kinds: name and
texts, duration, lifecycle, innate mark, statuses, modifiers, the three ops and
the saving throw. nge has no table of its own (D2).

### 5.3 Applying

`effect_apply(ef, power, target, source)` keeps its contract:

1. It refuses a target that can't be affected, as today (effects.md, R6).
2. **Instant:** it runs `on_start` scaled by the power, with `source` as the
   cause, and posts `EV_EFFECT_STARTED`.
3. **Timed:** from its copy of the row it calls
   `nge_effect_start(ef, power, target, def.period, def.cycles, def.flags & EFFECT_STACK)`.
   `NGE_ENTITY_INVALID` from nge means refused.

### 5.4 The hooks

`game_effect_hook()` is in common memory (`game.c`, next to `game_on_timer()`).
It skips a `TICK` off the level (`level_holds(effect)`, R16), and passes the rest
to `effects_hook()` (`effects.c`, common), which banked-calls `effects_on_hook()`
in page 120 (D19):

| Hook | The game does |
|---|---|
| `STARTED` | `on_start` with the effect as its cause, then recompute, then `EV_EFFECT_STARTED` |
| `REFRESHED` | Recompute if the power rose, then `EV_EFFECT_STARTED` |
| `TICK` | If the target can be affected: `on_tick`. Then, if the kind has a saving throw and the effect and target are still alive, the roll: passed, `nge_effect_end(effect, EFFECT_END_SAVED)` |
| `ENDED` | If the target can be affected: `on_end`, then recompute, then `EV_EFFECT_ENDED` with the reason |

These are the steps `effect_apply()`, `effects_on_timer()` and `end()` do today,
in the same order. Recompute is now a banked call into the attributes module
(5.6).

**The off-level test moves into the hook.** `game_on_timer()` used to skip the
firing of an effect that is off the level. nge now fires it, and the hook does
nothing. The behaviour is the same, because only effects that last for ever
are left off the level (5.5, effects.md D9). They have no cycles, so nge never
expires them, and the hook skips their op and their saving throw as the skipped
firing does today.

### 5.5 Decisions nge leaves to the game

- **Cure** (`effect_cure()`, `OP_CURE`) walks `NGE_EFFECT_FOR_EACH`, reads each
  kind's statuses and innate mark from the game row, and calls
  `nge_effect_end(e, EFFECT_END_CURED)`.
- **Leaving a level** (`level_travel.c`) ends each effect with cycles left the
  same way, with `EFFECT_END_EXPIRED` (effects.md, D9).
- **Innate** is the game row's mark, read only by cure.
- **End reasons** `EFFECT_END_CURED` and `EFFECT_END_SAVED` are numbered from 1.

### 5.6 Derivation: the game attributes module

A game module of its own (D13), in `src/game/attrs/`:

| File | Page | Contents |
|---|---|---|
| `attrs.h` | | The attribute numbers (moved from `components/`), and the interface: `effects_recompute()`, `effects_check()`, `effects_set_base()`, `effects_raise_base()` (for `OP_RAISE_BASE`) |
| `attrs_internal.h` | | The attribute table's type and `attrs_table[]`, for the tests |
| `attrs_bank.c` | 138 | `derive()`, `base_of()`, adding a kind's modifiers (today's `effects_add()`), the worn sources (today's `effects_game_sources()`), `store()`, the entity test (today's `effects_game_has_attrs()`), and the attribute table (today's `effects_game_attrs[]`, `const`) |

Page 139 holds nothing: while the module reads modifiers, page 121 is mapped
into slot 7. 138/139 is the first free game pair (memory_map.md). The
functions keep their names, so their callers don't change (R11).

The module works as the core does today (attributes.md, section 4.3), with two
changes:

- **Active effects** are walked with `NGE_EFFECT_FOR_EACH`, with kind and power
  read through nge.
- **Modifiers are copied from page 121.** For each source (a worn item or an
  active effect), after reading its kind and power, `derive()` maps page 121
  into slot 7 with `nge_mmu_map(NGE_MMU_DATA_SLOT, 121)`, copies the kind's
  statuses and modifiers (5 bytes, next to each other in `effect_def_t`) to the
  stack, and maps the old page back. Nothing stays mapped across a call to nge
  or to another module.

The effects module calls into this one for every recompute and for
`OP_RAISE_BASE` (`effects_set_base()`). This module calls the effects module
not at all: it reads only the game rows, which are data.

## 6. Decisions

- **D1 nge gets the lifecycle only.** Derivation is the game's rules. A generic
  modifiers module can follow if a second user appears. *Rejected:* a second
  nge module for derivation now, with one user.
- **D2 The lifecycle is passed as arguments** (Q1). The game keeps one table
  and reads its own row; nge registers no table and maps no page of the
  game's. The cost is that a wrong period could be passed, which the game avoids
  by always passing its row's. *Rejected:* a lifecycle table in the game's page
  that nge maps to read (the first draft), which adds a page mapping to nge and
  a second table to keep in step with the first; and one game table embedding
  nge's row, which ties nge to the game's row size.
- **D3 Cure is the game's.** nge offers ending and the iterator. *Rejected:* an
  opaque cure mask in nge's row.
- **D4 The hooks are direct calls** at the points where today's code recomputes,
  which keeps R17.
- **D5 Own pages, 58/59** (D17). nge maps no page of the game's (D2).
- **D6 nge handles timed effects only.** An instant effect has no state for nge to
  manage, so routing it through nge would add a banked call for nothing.
- **D7 One dispatch function with a `what` code,** as `nge_timer` and `nge_event`
  do. *Rejected:* one function pointer per hook, which means four pointers to keep
  in common memory.
- **D8 The `ENDED` hook gets kind and power by value.** The entity is already
  dying (R6), so it isn't read again.
- **D9 nge posts no events.** The game posts its own, after its recompute, so the
  messages, screens and their event numbers don't change. *Rejected:* nge events
  in its reserved range, which messages would have to handle before the recompute
  had happened.
- **D10 No affectable hook.** nge only asserts that the target is alive. Whether
  an op or a save applies is decided inside the game's hooks, which already test
  it today. *Rejected:* an `AFFECTABLE` hook, which is one more call per firing.
- **D11 nge fires its own timers through reserved kinds** (4.8). The game shouldn't
  have to route an nge module's internal handler. Reserved kinds follow the
  pattern of components (`NGE_COMPONENT_USER`) and events (`NGE_EVENT_USER`), and
  registration keeps `nge_timer` free of any other system (R10d). *Rejected:*
  routing through the game's dispatch (the first draft), which makes the game
  wire nge's plumbing; `nge_timer` calling `nge_effect_on_timer()` directly,
  which makes one system depend on another and links effects into every
  program with timers; and a far-pointer call mechanism (address and page) in
  `nge_timer`, which is a new piece of banking assembly to save one trampoline.
- **D12 The off-level rule is in the game's `TICK` hook, not in a timer gate.**
  It is a game rule (R16), and D9 means it never has to stop an expiry. *Rejected:*
  a "may fire" hook from nge, which is one more call per firing for a rule nge
  doesn't need to know about.
- **D13 Derivation is a game attributes module of its own** (Q2), pages 138/139
  (5.6). Attributes are a game system apart from effects: worn items, bases and
  the store have nothing to do with the lifecycle. Its price is a page mapping
  per source while deriving. *Rejected:* derivation in page 120 with the effects
  module, which reads the rows without mapping but keeps two systems in one
  module, and leaves page 120 fuller.
- **D14 No `effect_kind_of()`** (Q3). Its callers call `nge_effect_kind()`
  directly. *Rejected:* a game wrapper, which is one more banked call that adds
  nothing.
- **D15 The common memory is accepted** (Q4): estimated at about 45 bytes, it
  came to 104 in the debug build (4,201 free before, 4,097 after):
  `game_effect_hook()` (43), `effects_hook()` (26), the trampoline, and the two
  calls that handle events between firings (D18), less `game_on_timer()`'s
  effect case. *Rejected:* routing the
  hook through an existing common function.
- **D16 Two reserved timer kinds** (Q5). One more nge system with timers can come
  without renumbering the game's kinds again, for 2 bytes of slot 0.
  *Rejected:* one kind.

- **D17 Pages 58/59, not 92/93.** nge_effect is an `ecs` system, in
  `ecs/systems/`, and the `ecs` range (48-87) has free pairs. *Rejected:* 92/93
  in the "further nge modules" range, the first draft, which would put an `ecs`
  system outside its range.
- **D18 Events are handled between firings by `game.c`.** `game_on_timer()` used to
  handle the events of each firing before the next. Effects no longer fire
  through it, so it handles waiting events before its handler as well as after,
  and `pass_time()` handles them after each advance. Effects due at the same
  tick are handled together, after the last of them. *Rejected:* handling events
  inside nge's firing (a hook at its end), which runs event handlers a banked
  call deeper than today, with chains already near the limit of 8 (T-STACK-2).
- **D19 The off-level gate is in `game.c`'s hook, the rest in `effects_hook()`.**
  `level_holds()` is common code in `level.c`, which needs the level module's
  banked code: the PC tests that link the effects without the level couldn't
  link it. The gate stays beside `game_on_timer()`'s AI gate, where the rule
  was, and the tests register `effects_hook()` as they used to call
  `effects_on_timer()` directly. This refines D12: still not in nge.
- **D20 Steps 3 and 4 were built together,** as the state between them (the old
  core's `derive()` walking nge's iterator) would have been thrown away at once.

## 7. Questions

None open. Q1 to Q5 were answered by D2 and D13 to D16.

## 8. Implementation steps

Each step builds, passes `make test` and behaves the same in the game:

1. **nge_timer's reserved kinds:** `NGE_TIMER_KIND_USER`, the handler table and
   `nge_timer_set_handler()`, with PC tests. The game's `TIMER_*` kinds are
   renumbered from `NGE_TIMER_KIND_USER`. No reserved kind is used yet.
2. **nge_effect alone:** the module, its component number, its trampoline, and PC
   tests with a fake hook (starting, refreshing, stacking, firing through
   `nge_timer`, ending in a hook, expiry, no free entity, the target going).
3. **Switch the lifecycle** (built with step 4, D20): add `game_effect_hook()`
   and `effects_on_hook()`,
   with the off-level test in `TICK`. `effect_apply()` passes the row's
   lifecycle to nge; cure and level travel use nge; the core's `derive()` walks
   nge's iterator. Remove `effect_kind[]`, `effect_power[]`, `effect_kind_of()`
   (its callers call `nge_effect_kind()`), `COMPONENT_EFFECT`, `TIMER_EFFECT` and
   its case in `game_on_timer()`.
4. **Move derivation:** the attributes module in pages 138/139, copying
   modifiers from page 121. `OP_CURE` and `OP_RAISE_BASE` become game ops, the
   seam and `check-effects-core` go, and `effects_core.[ch]` are removed.
5. **Docs:** update effects.md, attributes.md, memory_map.md, ecs.md and
   src/nge/README.md (section 9).

## 9. Documents this changes

| Document | Sections |
|---|---|
| effects.md | 1 (the split), 2 (seam), R18, 4.1 (the definition), 4.2 (core ops), 4.4, 4.7 (the seam), 4.10 (banking), D5, section 6 ("An nge module"), section 7 (`check-effects-core`) |
| attributes.md | 3 ("The effects core"), 4.3, 4.4 (the seam), D4: derivation is the attributes module's |
| memory_map.md | Pages 58/59, page 120's and 121's contents, pages 138/139 |
| ecs.md, src/nge/README.md | The component number, `ecs/systems/`, the reserved timer kinds |
| nge_timer.h (its file comment) | Reserved kinds and their handlers, beside the game's dispatch |
| effects.h, timers.h (comments naming `TIMER_EFFECT`) | Effects' firings are nge's |

## 10. Tests

- **`test_nge_timer`:** a reserved kind fires through its handler and not the
  dispatch, a game kind through the dispatch, an unregistered reserved kind
  asserts, and the order and cycles are as before.
- **`test_nge_effect`** (new): the cases in step 2, with a hook that records its
  calls, and the asserts (power 0, period 0, an unknown flag, a dead target).
- **`test_effects`:** unchanged in what it covers (effects.md, section 7),
  derivation included.

## 11. Known risks

- **Re-entry.** A hook may end its own effect, other effects or the target.
  After every hook, nge reads only its own data, through the effect's ID and
  whether it is alive.
- **Page mapping.** A hook runs game code that maps slots 6 and 7. nge must not
  hold pointers into page 59 across a hook (4.7). The attributes module must
  not keep page 121 mapped across a call to nge (5.6). The PC tests can't catch
  either, so they need reading the code and checking in CSpect.
- **Component renumbering.** Moving `NGE_COMPONENT_USER` changes every game
  component's number. Anything that stores a number (saves, test data) would
  break. Nothing does today.
- **Cost.** Each firing goes nge_timer, then the trampoline, then nge_effect,
  then the game's hook: one more banked call than today. A recompute adds a
  page mapping per source and a banked call into page 138. Effects fire and
  recompute at turn rate, so this is accepted.
- **Timer renumbering.** The game's `TIMER_*` kinds move up by
  `NGE_TIMER_KIND_USER`. They are stored only in `nge_timer_kind[]` at run time,
  and nothing saves them, so this is safe today.
- **Slot 0.** The handler table adds 4 bytes to the ECS data page
  (`_nge_timer_handler` at `$1A73`, well inside `$0100`-`$1FFF`).
- **Banked-call depth.** An AI's hit that starts a timed effect nests 8 banked
  calls (`nge_timer_advance()`, `ai_on_turn()`, `act_melee()`, `effect_apply()`,
  `nge_effect_start()`, `effects_on_hook()`, `effects_recompute()`,
  `gen_hook_of()`), the most allowed (T-STACK-2), against 5 before. nge_effect
  and the attributes module call their own functions plainly so as not to add
  more, but anything new on that path breaks the limit.

## 12. How the build differs from the plan

| Plan | Built | Why |
|---|---|---|
| Pages 92/93 | 58/59 | D17 |
| `game_effect_hook()` banked-calls `effects_on_hook()`, which tests the level | `game_effect_hook()` tests the level and calls `effects_hook()` | D19 |
| — | Events handled before and after each game firing, and after each advance | D18 |
| `effects_core.h`, `effects_game.h` | `effects_defs.h` (the rows, shared with the attributes module), `effects_internal.h` | The row is read by two modules |
| `attrs.h` in `components/` | `game/attrs/attrs.h`, the module's header | G-MOD-1: the interface is in the module's folder |
| `OP_RAISE_BASE` reads the base through `base_of()` | `effects_raise_base()` | The base table is the attributes module's |
| 45 bytes of common memory | 104 | D15 |
