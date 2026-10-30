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

**Strength comes from outside.** A definition's amounts are per **power**. The
hook that names the effect gives the power, so one row serves every strength of
an item, and an active effect keeps its power after what started it has gone.

**The module is split in two.** The core is a generic mechanism: it applies, ends
and cures effects, runs active effects, and recomputes derived values. The game's
side says what attributes and ops mean. The core reaches it only through a narrow
seam, so the mechanism could become an nge module.

```text
 callers                       effects core                   game's side
 --------                      ------------                   -----------
 use hook (act_use)      ---+
 hit hook (act_melee)    ---+-> effect_apply()
 innate hook (generator) ---+       |
                                    +-- instant: run start op ---> damage, heal,
                                    |                              mana...
                                    +-- timed: create or refresh
                                         active effect
                                             |
 TIMER_EFFECT firing ----------> effects_on_timer()
                                    tick op, saving throw, last firing
                                             |
 effect_cure(), OP_CURE -------> end(): on_end op
                                             |
 equip, take off ---------------> effects_recompute()  <--- worn effects of
                                    bases + modifiers       equipped items
                                             |
                                             +-----------> clamp and store
```

**Effects are called, not triggered.** Actions, generators and ops apply effects
with direct calls. Events only report what happened, for messages and screens.

## 2. Terms

| Term | Meaning |
|---|---|
| Effect | A row of `effect_defs[]`, `EF_*`. `EF_NONE` is an empty hook |
| Duration | Instant, timed or worn (`EFFECT_*`) |
| Active effect | A timed effect in progress: an entity with `COMPONENT_EFFECT`, owned by its target |
| Modifier | An amount per power added to a number attribute, or bits ORed into a mask |
| Op | An action in a definition: what, an argument, and an amount per power |
| Power | What a use of the effect multiplies its amounts by, from the hook. Not 0, and may be negative |
| Firing | One run of an active effect's timer, every period |
| Period, cycles | The ticks between firings, and how many firings; 0 cycles lasts for ever |
| Refresh | Applying a timed effect that is already there, restarting its timer |
| Stack | Applying a timed effect marked `EFFECT_STACK`, which adds another instance |
| Seam | The functions and data in `effects_core.h` through which the core reaches the game |
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

- **R5** Effects are applied by direct calls, never by event handlers.
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
- **R16** An active effect doesn't fire while its target is off the level. One with
  a limited time ends when its target leaves, and one that lasts for ever waits.

### Derived values

- **R17** Derived values are always worked out from scratch, and whatever changes
  their inputs calls `effects_recompute()` directly.

### The core

- **R18** The core includes nothing of the game's content, and everything it needs
  from the game goes through the seam.

### Constraints

- **R19** The module is banked: its code in one page and its data in the next. Its
  callers don't map either.
- **R20** The module adds no library routine to common memory, such as a multiply
  (items.md, D10).

## 4. Design

### 4.1 The definition

`effect_def_t` ([effects_core.h](../src/game/effects/effects_core.h)):

| Field | Meaning | Applies to |
|---|---|---|
| `name` | Shown while it lasts, 14 characters at most | Timed |
| `start_text`, `end_text` | What the player is told, or none | Instant, timed |
| `duration` | `EFFECT_INSTANT`, `EFFECT_TIMED` or `EFFECT_WORN` | All |
| `flags` | `EFFECT_STACK` | Timed |
| `period`, `cycles` | Firings: ticks between them, how many | Timed |
| `status` | Status bits granted while it lasts; the core doesn't know their meanings | Timed, worn |
| `mod[]` | Two modifiers; an amount of 0 changes nothing | Timed, worn |
| `on_start` | Op run once, when applied | Instant, timed |
| `on_tick` | Op run at each firing, a period after the last | Timed |
| `on_end` | Op run once, when it ends for any reason | Timed |
| `save` | A saving throw: an ability and a DC; DC 0 for none | Timed |

Timed effects are numbered first, so anything counting active effects by kind needs
only `EF_TIMED_COUNT` entries (items.md, D12).

### 4.2 Ops

An op is a number, dispatched by a `switch` in the code that owns it:

| Op | Owner | What it does |
|---|---|---|
| `OP_NONE` | Core | Nothing |
| `OP_CURE` | Core | Ends the target's effects granting statuses `arg` |
| `OP_RAISE_BASE` | Core | Adds the amount to attribute `arg`'s base, for good, kept within 0-255 |
| `OP_DAMAGE` | Game | Damage of type `arg`, through `health_damage()` |
| `OP_HEAL` | Game | Healing, through `health_heal()` |
| `OP_MANA` | Game | MP back, through `mana_restore()`; nothing without mana |
| The pending ops | Game | Nothing yet (section 4.8) |

The core scales an op's amount by the power, clamped to -127 to 127, and passes the
game's side a copy. Ops go through the components' own rules, so a healing effect
can't bypass what `health_heal()` enforces.

### 4.3 Applying an effect

`effect_apply(ef, power, target, source)` copies the definition, then refuses a
target that can't be affected (R6).

- **Instant:** runs `on_start`, with `source` as its cause, and posts
  `EV_EFFECT_STARTED`.
- **Timed, already there and not stacking:** restarts the timer, keeps the higher
  power (compared as plain numbers), recomputes if the power rose, and posts the
  event.
- **Timed, otherwise:** creates the entity, owned by the target, records its kind
  and power, adds its timer, runs `on_start`, recomputes the target and posts the
  event. Its first `on_tick` comes a period later.

The callers are `act_use()` (the use hook, before destroying the item),
`act_melee()` (the hit hook, refused if the hit killed the target) and the creature
generator (the innate hook).

### 4.4 Active effects

An active effect's data is its entity: its owner is the target, `effect_kind[]` and
`effect_power[]` hold what it is, and its timer holds how long is left. The timer's
handler is common code, `game_on_timer()`, which calls `effects_on_timer()`.

At each firing:

1. If the target can be affected, `on_tick` runs at the stored power, with the
   effect as its cause.
2. If the effect has a saving throw and it and its target are still there, the
   target rolls. A pass ends the effect.
3. On the last cycle, if it hasn't ended, it ends as expired.

`effect_cure()` walks the target's effects and ends those granting any of the
given statuses. See [attributes.md](attributes.md), sections 4.5 to 4.7, for
statuses, saving throws and end reasons.

**Ending** (`end()`) destroys the effect first, so a recompute no longer counts it.
Then, if the target can still be affected, it runs `on_end`, recomputes and posts
the event.

**Leaving a level:** when a creature is parked off the level, effects with cycles
left end at once, because skipping their firings would still count their cycles.
Effects that last for ever stay, and their timers are ignored while the creature
isn't on the level (R16).

### 4.5 Worn effects

A worn effect has no instance. `effects_game_sources()` walks a recompute's
entity's equipped items, reads each worn hook, and adds that effect's modifiers and
statuses at the hook's power.

### 4.6 Recomputing

`derive()` starts from the bases, adds the game's sources (worn effects) and the
active effects through `effects_add()`, and `effects_game_store()` clamps and stores
the results. [attributes.md](attributes.md), section 4.3, has the detail.

### 4.7 The seam

| Seam | What the core gets from it |
|---|---|
| `effects_game_attrs[]` | Each attribute's base table, component and whether it is a mask |
| `effects_game_timer_kind` | The timer kind of active effects |
| `effects_game_has_attrs()` | Whether an entity has attributes |
| `effects_game_affectable()` | Whether effects can act on an entity |
| `effects_game_save()` | The result of a saving throw |
| `effects_game_op()` | Runs a game op, already scaled |
| `effects_game_sources()` | Adds the sources that aren't active effects |
| `effects_game_store()` | Clamps and stores derived values, or compares them |

These are plain calls in one page today, and would be an nge API's callbacks.

### 4.8 Pending effects

Pending effects are complete rows, so the data won't change when a mechanic
arrives. Instant ones name an op that does nothing yet, grouped in
`effects_game_op()`. Worn and timed ones have no modifiers, or grant a status that
no rule reads yet. See [items.md](items.md), "Not built yet".

### 4.9 Events and text

| Event | Target | Source | Payload |
|---|---|---|---|
| `EV_EFFECT_STARTED` | The entity affected | The active effect, or none if instant | p1: `EF_*` |
| `EV_EFFECT_ENDED` | The entity affected | The effect (dying) | p1: `EF_*`, p2: `EFFECT_END_*` |
| `EV_ATTRS_CHANGED` | The entity | None | Posted by a recompute that changed something |

`effect_text()` copies a definition's start text, end text or name into the caller's
buffer, and `effect_save_of()` gives its saving throw. The messages module and the
PC screen use these, so they never read the table.

### 4.10 Banking

Everything public is banked in page 120, with the definitions and each active
effect's kind and power in page 121, in slot 7 only while page 120's code runs.
Tables are in the odd page because addresses in page 120 carry the page in their high
bits. Code that calls further banked functions copies what it needs first:
`effect_apply()` copies the definition, and the others copy an op or a field into
locals.

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
- **D5 The core and the game's side are split by a seam.** The mechanism has no reason
  to know about hit points. It is enforced by a test, not just by convention.
- **D6 Ops are numbers.** Data can't hold pointers to banked functions, and a `switch`
  keeps each op with the rules it calls (G-ARCH-7).
- **D7 One ending path.** Time, cure and save all go through `end()`, with only the
  reason differing.
- **D8 Destroy the effect before running its end op.** The recompute then no longer
  counts it.
- **D9 Effects with a limited time end when their target leaves the level.** The
  dispatcher can skip a firing, but `nge_timer` still counts the cycle, so a skipped
  effect would run out without ending and leave its modifiers behind. An effect that
  lasts for ever has no cycles to count, so it is safe to skip, and ending it would
  lose an innate ability ([level_transitions.md](level_transitions.md), D3).

## 6. Not built yet

| Mechanic | What's there, what's missing |
|---|---|
| Start and end ops on timed effects | The code runs them, but no definition has one |
| Aimed effects | An effect can be applied to any target, but no action reads the aim hook |
| Worn effects that tick | A worn effect has no firings: `EF_WORN_REGEN` waits on this |
| Pending ops | Teleport, mapping, identify, summon, enchant, uncurse, light |
| An nge module | The seam's plain calls would become callbacks |
| Messages by end reason | See [attributes.md](attributes.md), "Not built yet" |

## 7. Tests

`test_effects` covers:

- the definitions: well formed, each duration with only what it uses, timed first;
- instant effects, and refused targets;
- timed effects: firings, refresh, stacking, cures, saving throws, and running out;
- power: instant, worn and timed, including after the source has gone;
- worn effects, and every attribute's clamps;
- a target that dies, an entity without stats, and no free entity;
- pending effects applying safely, the texts, and the asserts.

`check-effects-core`, part of `make test`, fails if the core includes game content.

## 8. Known risks

- **Negative power on an op.** `effects_game_op()` casts the scaled amount to
  `uint8_t`, so a negative power on `OP_DAMAGE`, `OP_HEAL` or `OP_MANA` becomes a
  large positive amount. Only `OP_RAISE_BASE` handles a negative amount. Negative
  powers are only used on worn hooks today, which go through modifiers, so nothing
  reaches this yet. Nothing checks it either.
- **`source` on a timed effect.** `effect_apply()`'s `source` is used only by instant
  effects. A timed effect's `on_start` gets the effect as its cause, whatever was
  passed. No timed effect has an `on_start` yet.
- **`on_end` is untested,** as no definition has one.
- **Entities.** Each active effect takes one of the 100 entities, so a timed effect can
  be refused when the game is busy, and the caller may not say so.
- **zsdcc.** `run_op()` and `effects_on_timer()` read struct fields in code that
  branches, so they copy them into locals first (T-ZSDCC-1). Scaling depends on
  promotion and sign, and the PC tests can't catch a miscompile there.
