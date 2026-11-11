# Effects: design specification

How Jetset's effects work: what potions, poison, rings and the like do to an
entity, and for how long.

The definitions are in
[effects_pdata.c](../src/game/effects/effects_pdata.c) and the public interface
is in [effects.h](../src/game/effects/effects.h). What an effect changes is
specified in [attributes.md](attributes.md), and how items use effects in
[items.md](items.md).

## 1. Overview

**An effect is data.** Each is a row in a constant table, `effect_defs[]`, that
bundles what it changes: modifiers to attributes, statuses it grants, and ops
that act at its start, at each tick and at its end. Adding an effect means
adding a row. Only a new kind of op needs code.

**An effect has one of three durations:**

- **Instant:** its start op runs once and nothing is left behind. A healing
  potion is instant, and raising a base is how an instant effect lasts for good.
- **Timed:** it becomes an **active effect**, an entity owned by its target, with
  a timer that fires its tick op until it ends.
- **Worn:** it is never applied. Its modifiers and statuses simply count while
  the item naming it is equipped.

A timed effect with 0 cycles lasts for ever, and is one of two things. A
**permanent** effect is one a creature has gained, and a cure can end it. An
**innate** effect is part of what the creature is (a plant's roots, a troll's
regeneration), and ends only with the creature.

**Strength comes from outside.** A definition's amounts are per **power**. The
hook that names the effect gives the power, so one row serves every strength of
an item, and an active effect keeps its power after what started it has gone.

**Three parts share the work** ([effects_nge.md](effects_nge.md), second
version). nge's effect system
(`ecs/systems/nge_effect.h`) sets up an active effect, an entity owned by
its target with a timer whose payload holds the effect's kind and power, and
finds a target's active effects; it decides
nothing. The game's effects module decides everything else: when an effect
starts, is refreshed or stacks, what it does at each firing, when it ends,
what cures it, and its events. The game's attributes module (`game/attrs`)
derives values from bases, equipment and active effects.

```text
 callers                  effects module (120/121)               nge
 -------                  ------------------------               ---
 use, hit, aim hooks,
 a spell, innate hook --> effect_apply()
                            instant: on_start op, event
                            timed, one there, not stacking:
                              restart its timer -------------> nge_timer_add()
                              raise its power (payload)
                            timed, otherwise: ---------------> nge_effect_create()
                              on_start op, recompute, event      entity, owner, tag,
                                                                 timer, payload
 nge_timer_advance() ---> game_on_timer(), TIMER_EFFECT,
                          on the level:
                          effects_on_timer()
                            tick op, saving throw,
                            last cycle: ending
 effect_cure(), OP_CURE,
 leaving a level -------> walks NGE_EFFECT_FOR_EACH
 an ending:               copies target, kind, power ------> nge_effect_destroy()
                            on_end op, recompute, event

                          attributes module (138)
 equip, take off,         -----------------------
 the steps above -------> effects_recompute()
                            bases + worn items + active effects,
                            modifiers copied from page 121; clamp and store
```

**Effects are called, and may be triggered.** Actions, generators and ops apply
their effects directly. Handlers may apply effects as reactions. Derived values
are never recomputed by a handler (R17).

## 2. Terms

| Term | Meaning |
|---|---|
| Effect | A row of `effect_defs[]`, `EF_*`. `EF_NONE` is an empty hook |
| Duration | Instant, timed or worn (`EFFECT_*`) |
| Active effect | A timed effect in progress: an entity with `NGE_COMPONENT_EFFECT` (a tag) and a `TIMER_EFFECT` timer, owned by its target |
| Payload | The two bytes an nge timer carries: an active effect's kind (`EFFECT_KIND_OF()`) and power (`EFFECT_POWER_OF()`) |
| Modifier | An amount per power added to a number attribute, or bits ORed into a mask |
| Op | An action in a definition: what, an argument, and an amount per power |
| Power | What a use of the effect multiplies its amounts by, from the hook. Not 0, and may be negative |
| Firing | One run of an active effect's timer, every period |
| Period, cycles | The ticks between firings, and how many firings; 0 cycles lasts for ever |
| Permanent | A timed effect with 0 cycles: a cure ends it |
| Innate | A timed effect with 0 cycles marked `EFFECT_INNATE`: nothing ends it but its target going |
| Refresh | Applying a timed effect that is already there, restarting its timer |
| Stack | Applying a timed effect marked `EFFECT_STACK`, which adds another instance |
| Pending effect | A row whose mechanic isn't built, so it does nothing |

## 3. Requirements

### Definitions

- **R1** An effect is a row of data. A new effect needs code only if it needs a new
  kind of op.
- **R2** Effects are named for what they do, not for what uses them.
- **R3** A definition's op amounts and number modifiers are per power. Masks,
  statuses, durations and saving throws don't scale.
- **R4** Each definition uses only what its duration allows. A saving throw needs a
  timed effect, and only a timed effect can be active.

### Applying

- **R5** An effect may be applied by a direct call or by an event handler
  (G-ARCH-3). A direct call is needed only if the caller needs the result, or if the
  rest of the action must see the change (events.md, R27). An effect applied by a
  handler takes hold after the action that caused it, before any other actor acts.
- **R6** Effects act on an entity that is alive, has health and isn't at 0 hit
  points. Anything else refuses the effect.
- **R7** A worn effect can't be applied.
- **R8** Every applied effect posts `EV_EFFECT_STARTED`.

### Active effects

- **R9** An active effect is an entity owned by its target, with a timer. It isn't
  a counter in a component.
- **R10** It goes when its target goes.
- **R11** It records no source entity, as an ID can be reused. Its ops name the
  effect itself as the cause.
- **R12** It keeps its own power for as long as it lasts.
- **R13** Applied again, it is refreshed: its timer restarts and the stronger power
  stays. A definition marked `EFFECT_STACK` adds an instance instead.
- **R14** A timed effect is refused if no entity is free. An instant one never needs
  an entity.
- **R15** An ending runs its end op, recomputes its target and posts
  `EV_EFFECT_ENDED` with the reason. A dead target gets none of these.
- **R15a** A cure ends a permanent effect, not an innate one. An innate effect is
  timed, lasts for ever and has no saving throw.
- **R16** An active effect doesn't fire while its target is off the level. One with
  a limited time ends when its target leaves, and one that lasts for ever waits.

### Derived values

- **R17** Derived values are always worked out from scratch, and whatever changes
  their inputs calls `effects_recompute()` directly.

### The split

- **R18** nge sets up an active effect (entity, owner, tag, timer and payload),
  destroys it, and finds a target's active effects, knowing nothing of the game
  ([effects_nge.md](effects_nge.md), R5 to R11). Everything else about an active
  effect, its lifecycle included, is the effects module's, and derived values
  are the attributes module's.

### Constraints

- **R19** The module is banked: its code in one page and its data in the next. Its
  callers don't map either.
- **R20** The module adds no library routine to common memory, such as a multiply
  (items.md, D10).

## 4. Design

### 4.1 The definition

`effect_def_t` ([effects_defs.h](../src/game/effects/effects_defs.h)):

| Field | Meaning | Applies to |
|---|---|---|
| `name` | Shown while it lasts, 14 characters at most | Timed |
| `start_text`, `end_text` | What the player is told, or none | Instant, timed |
| `duration` | `EFFECT_INSTANT`, `EFFECT_TIMED` or `EFFECT_WORN` | All |
| `flags` | `EFFECT_STACK`, `EFFECT_INNATE` | Timed |
| `period`, `cycles` | Firings: ticks between them, how many; given to the timer when it is created or restarted | Timed |
| `status` | Status bits granted while it lasts | Timed, worn |
| `mod[]` | Two modifiers; an amount of 0 changes nothing | Timed, worn |
| `on_start` | Op run once, when applied | Instant, timed |
| `on_tick` | Op run at each firing, a period after the last | Timed |
| `on_end` | Op run once, when it ends for any reason | Timed |
| `save` | A saving throw: an ability and a DC; DC 0 for none | Timed |

Timed effects are numbered first, so anything counting active effects by kind needs
only `EF_TIMED_COUNT` entries (items.md, D12).

### 4.2 Ops

An op is a number, dispatched by a `switch` in `run_op()` (`effects_bank.c`):

| Op | What it does |
|---|---|
| `OP_NONE` | Nothing |
| `OP_CURE` | Ends the target's effects granting statuses `arg`, except innate ones |
| `OP_RAISE_BASE` | Adds the amount to attribute `arg`'s base, for good, kept within 0-255 (`effects_raise_base()`) |
| `OP_DAMAGE` | Damage of type `arg`, through `health_damage()` |
| `OP_HEAL` | Healing, through `health_heal()` |
| `OP_MANA` | MP back, through `mana_restore()`; nothing without mana |
| `OP_UNCURSE` | Lifts the curse of everything the target has equipped (`ITEM_UNCURSED`) |
| `OP_TELEPORT` | To random free floor on the level (`level_free_floor()`), with `EV_MOVED` and no step ([effects_extensions.md](effects_extensions.md)) |
| `OP_SUMMON` | 1 random creature for the depth per power, at most 3, on free tiles around the target |
| `OP_LEVEL_TELEPORT` | Up, down or a random way (`LT_*` in `arg`) by the amount in levels: the player by a request, a monster by `level_send()` |
| The pending ops | Nothing yet (section 4.8) |

An op that can do nothing (a summon with no room, a level teleport at the limit) posts
`EV_FIZZLED`, and an instant effect whose op fizzled posts no `EV_EFFECT_STARTED`.

An op's amount is scaled by the power and clamped to -127 to 127. Ops go through
the components' own rules, so a healing effect can't bypass what `health_heal()`
enforces.

### 4.3 Applying an effect

`effect_apply(ef, power, target, source)` copies the definition, then refuses a
target that can't be affected (R6).

- **Instant:** runs `on_start`, with `source` as its cause, and posts
  `EV_EFFECT_STARTED`.
- **Timed, already there, not stacking:** the first of the kind found by
  `NGE_EFFECT_FOR_EACH` is refreshed. Its timer restarts with the row's period
  and cycles (`nge_timer_add()`, which keeps the payload), it keeps the higher
  power (compared as plain numbers, written to the payload), the target is
  recomputed if the power rose, and the event is posted.
- **Timed, otherwise:** `nge_effect_create()` makes the entity, owned by the
  target, with a `TIMER_EFFECT` timer of the row's period and cycles, and the
  kind and power as its payload. `NGE_ENTITY_INVALID` (no entity free) means
  refused. Then `on_start` runs with the effect as its cause, the target is
  recomputed and the event is posted. Its first `on_tick` comes a period later.

The callers are `act_use()` (the use hook, before destroying the item),
`act_melee()` (the hit hook, refused if the hit killed the target) and the creature
generator (the innate hook, which names only innate effects).

### 4.4 Active effects

An active effect's data is its entity: its owner is the target, the tag
`NGE_COMPONENT_EFFECT` marks it, and its timer holds how long is left and,
in its payload, what it is (`EFFECT_KIND_OF()`, `EFFECT_POWER_OF()`, slot 0
reads). The timer is one of the game's kinds, `TIMER_EFFECT`: `game_on_timer()`
calls the banked `effects_on_timer()` unless the effect is off the level, and
handles the events the firing posted before the next, as for any timer.

At each firing (`effects_on_timer()`):

1. If the target can be affected, `on_tick` runs at the stored power, with the
   effect as its cause.
2. If the effect has a saving throw and it and its target are still there, the
   target rolls. A pass ends the effect.
3. On the last cycle (`cycles` 1), if it hasn't ended, it ends as expired.

`effect_cure()` walks the target's effects (`NGE_EFFECT_FOR_EACH`) and ends those granting any of the
given statuses, unless they are innate. See [attributes.md](attributes.md), sections 4.5 to 4.7, for
statuses, saving throws and end reasons.

**Ending** is one function in `effects_bank.c`, whatever the reason: it copies
the target, kind and power, destroys the effect first (`nge_effect_destroy()`),
so a recompute no longer counts it, then, if the target can still be affected,
runs `on_end`, recomputes and posts the event.

**Leaving a level:** when a creature is parked off the level, effects with cycles
left end at once, because skipping their firings would still count their cycles.
Effects that last for ever stay, and their timers are ignored while the creature
isn't on the level (R16).

### 4.5 Worn effects

A worn effect has no instance. A recompute (the attributes module's `derive()`)
walks its entity's equipped items, reads each worn hook, and adds that effect's
modifiers and statuses at the hook's power.

### 4.6 Recomputing

The attributes module (`game/attrs/attrs_bank.c`, page 138) does it: `derive()`
starts from the bases, adds the worn effects and the active effects, copying each
definition's statuses and modifiers from page 121, and `store()` clamps and stores
the results. [attributes.md](attributes.md), section 4.3, has the detail.

### 4.7 The split

| Part | Files | What it holds |
|---|---|---|
| nge_effect | `ecs/systems/nge_effect.*` (58) | Creating an active effect (entity, owner, tag, timer, payload), destroying it, the iterator |
| nge_timer | `ecs/systems/nge_timer.*` (54, tables in slot 0) | Firings, and the payload that holds each active effect's kind and power |
| The effects module | `effects_bank.c` (120), `effects_pdata.c` (121), `effects_defs.h`, `effects_internal.h` | The definitions, applying, refreshing and stacking, firings, expiry, the ops, saving throws, cures, ending, events, texts |
| The attributes module | `game/attrs/attrs_bank.c` (138), `attrs.h`, `attrs_internal.h` | The attribute table, deriving, clamping and storing, setting bases |
| `game.c` | Common | `game_on_timer()`'s `TIMER_EFFECT` case: firings off the level skipped |

### 4.8 Pending effects

Pending effects are complete rows, so the data won't change when a mechanic
arrives. Instant ones name an op that does nothing yet, grouped in `run_op()`. Worn and timed ones have no modifiers, or grant a status that
no rule reads yet. See [items.md](items.md), "Not built yet".

### 4.9 Events and text

| Event | Target | Source | Payload |
|---|---|---|---|
| `EV_EFFECT_STARTED` | The entity affected | The active effect, or none if instant | p1: `EF_*` |
| `EV_EFFECT_ENDED` | The entity affected | The effect (dying) | p1: `EF_*`, p2: `EFFECT_END_*` |
| `EV_ATTRS_CHANGED` | The entity | None | Posted by a recompute that changed something |
| `EV_FIZZLED` | The entity affected | None | p1: the op (`OP_*`) that could do nothing |

`effect_text()` copies a definition's start text, end text or name into the caller's
buffer, and `effect_save_of()` gives its saving throw. The messages module and the
PC screen use these, so they never read the table.

### 4.10 Banking

Everything public is banked in page 120, with the definitions in page 121, in slot
7 while page 120's code runs. Tables are
in the odd page because addresses in page 120 carry the page in their high bits.
Code that calls further banked functions copies what it needs first:
`effect_apply()` copies the definition, and the firing and the ending copy an op
or a field into locals. The attributes module maps page 121 by hand only to copy
a row's statuses and modifiers.

The deepest chain of banked calls is an AI's hit that starts a timed effect:
`nge_timer_advance()`, `ai_on_turn()`, `act_melee()`, `effect_apply()`, then
`nge_effect_create()` and its calls into nge, or `effects_recompute()` and
`gen_hook_of()`. That is 6 of the 8 allowed (T-STACK-2).

## 5. Decisions

- **D1 Three durations, all in one table.** Items, creatures and features name any
  effect by number, and a hook needs no idea how an effect works.
- **D2 An active effect is an entity.** It reuses ownership (it goes with its target)
  and timers (firings and cycles), so the module needs no list of its own.
  *Rejected:* counters in a component, which would need a slot per effect per
  creature.
- **D3 No source entity.** An ID can be reused while an effect runs. *Rejected:*
  recording the source and checking it, which needs every effect cleared when its
  source goes.
- **D4 Power is kept with the active effect** (items.md, D2 and D4).
- **D5 nge sets up and finds active effects; the game runs them**
  ([effects_nge.md](effects_nge.md), D1). What nge offers is an owned entity with
  a timer that carries the effect's data. Refreshing, stacking, firing, expiring
  and ending are the game's rules. *Replaced:* one game module split by a seam
  into a core and the game's side; then the lifecycle in nge, calling the game
  through hooks (effects_nge.md, section 12).
- **D6 Ops are numbers.** Data can't hold pointers to banked functions, and a `switch`
  keeps each op with the rules it calls (G-ARCH-7).
- **D7 One ending path.** Time, cure, save and leaving a level all go through one
  function in `effects_bank.c`, with only the reason differing.
- **D8 Destroy the effect before running its end op.** The recompute then no longer
  counts it.
- **D9 Effects with a limited time end when their target leaves the level.** The
  dispatcher can skip a firing, but `nge_timer` still counts the cycle, so a skipped
  effect would run out without ending and leave its modifiers behind. An effect that
  lasts for ever has no cycles to count, so it is safe to skip, and ending it would
  lose an innate ability ([level_transitions.md](level_transitions.md), D3).
- **D10 Innate is a mark on the definition, `EFFECT_INNATE`** (creatures.md, D5).
  An effect that lasts for ever may be something gained, which a cure should end, or
  what a creature is, which it shouldn't. So rooted that can be cured is a row of its
  own, with the same status and no mark. *Rejected:* 0 cycles alone deciding, which
  can't tell the two apart; marking each active effect by how it was applied, which
  needs a byte per entity and another argument to `effect_apply()`, to save a row.

## 6. Not built yet

| Mechanic | What's there, what's missing |
|---|---|
| Start ops on timed effects | The code runs them, but no definition has one (`EF_DESCENT` and `EF_RECALL` use `on_end`) |
| Aimed effects | An effect can be applied to any target, but no action reads the aim hook |
| Worn effects that tick | A worn effect has no firings: `EF_WORN_REGEN` waits on this |
| Hit and use effects as reactions | R5 allows it, and today they are direct calls (events.md, section 6) |
| Pending ops | Mapping, identify, enchant, light |
| Messages by end reason | See [attributes.md](attributes.md), "Not built yet" |

## 7. Tests

`test_effects` covers:

- the definitions: well formed, each duration with only what it uses, timed first;
- instant effects, and refused targets;
- timed effects: firings, refresh, stacking, cures, saving throws, and running out;
- innate effects: well formed, and not ended by a cure;
- power: instant, worn and timed, including after the source has gone;
- worn effects, and every attribute's clamps;
- a target that dies, an entity without stats, and no free entity;
- pending effects applying safely, the texts, and the asserts.

`test_nge_effect` covers nge's side: creating (owner, tag, timer, payload),
destroying, the iterator, no free entity and the target going. `test_nge_timer`
covers the payload.

## 8. Known risks

- **Negative power on an op.** `run_op()` casts the scaled amount to
  `uint8_t`, so a negative power on `OP_DAMAGE`, `OP_HEAL` or `OP_MANA` becomes a
  large positive amount. Only `OP_RAISE_BASE` handles a negative amount. Negative
  powers are only used on worn hooks today, which go through modifiers, so nothing
  reaches this yet. Nothing checks it either.
- **`source` on a timed effect.** `effect_apply()`'s `source` is used only by instant
  effects. A timed effect's `on_start` gets the effect as its cause, whatever was
  passed. No timed effect has an `on_start` yet.
- **`on_end` is untested,** as no definition has one.
- **Entities.** Each active effect takes one of the 100 entities, so a timed effect can
  be refused when the game is busy, and the caller may not say so. For an innate
  effect that shows: the generator ignores the result, so a lichen made with no entity
  free would walk. Regeneration lost that way never mattered. Levels stay far below
  100 entities, so this is accepted.
- **zsdcc.** `run_op()` and `effects_on_timer()` read struct fields in code that
  branches, so they copy them into locals first (T-ZSDCC-1). Scaling depends on
  promotion and sign, and the power is a payload byte cast to `int8_t`; the PC
  tests can't catch a miscompile there.
- **Expiry is the game's.** If `effects_on_timer()` didn't end an effect on its
  last cycle, nge_timer would remove the timer and the effect would stay,
  counted by every recompute ([effects_nge.md](effects_nge.md), section 11).
