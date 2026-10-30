# Attributes, statuses and conditions: design specification

How Jetset stores a creature's numbers and flags, how equipment and effects
change them, and how conditions end.

The list of attributes is in [attrs.h](../src/game/components/attrs.h), the
statuses in [stats.h](../src/game/components/stats.h), and the effect
definitions in [effects_pdata.c](../src/game/effects/effects_pdata.c). What items
do with effects is in [items.md](items.md), and the effects module is specified in
[effects.md](effects.md).

## 1. Overview

**An attribute is a number or mask that equipment and effects can change:**
max hit points, AC, speed, the abilities and the damage masks. Each has three
stages, and the rules read only the last:

```text
 kind value           base                   value (derived)
 in the kind record   the entity's           what rules read
 (constant)           permanent number

 hp_max  ---------->  health_hp_max_base --+--> health_hp_max
       when created                        |
                   effects_set_base() ...  |  + modifiers of worn effects
                   changes it for good     |  + modifiers of active effects
                                           |  = clamped, stored
                                           effects_recompute()
```

**Values are never adjusted, only recomputed.** Whenever a source changes (an
item is equipped, an effect starts or ends, a base changes),
`effects_recompute()` works everything out again from the bases. A modifier
therefore can't be left behind, and removing a source needs no undo.

**Attributes belong to their components.** Each attribute's tables sit in the
component they describe, and `attrs.h` only numbers them, so that effect data can
name them. The effects module finds them through one table, which lets its core
stay free of game knowledge.

**Statuses work the same way.** A status is a flag that an effect grants, and
the status mask is worked out from scratch with the values. A **condition** is a
status that impairs a creature. It ends when every effect granting it has ended,
because time ran out, a saving throw was passed or it was cured.

**Pools are separate.** A pool's maximum is an attribute, but its current amount
is a resource that damage and healing change. Hit points are a pool.

## 2. Terms

| Term | Meaning | Hit points | AC |
|---|---|---|---|
| Attribute | A number or mask that equipment and effects can change, with an id (`ATTR_*`) so effect data can name it | `ATTR_HP_MAX` | `ATTR_AC` |
| Kind record | A kind's constant record in its generator's page | `gen_creature_kind_t` | |
| Kind value | An attribute's value in the kind record: it sets the base when the entity is created | `health_kind_t.hp_max` | `health_kind_t.ac` |
| Base | The entity's permanent value, changed only by `effects_set_base()` | `health_hp_max_base[]` | `health_ac_base[]` |
| Modifier | An amount added, or bits ORed, by a worn or active effect while it lasts | | |
| Value (derived) | Base plus modifiers, clamped: the one rules read | `health_hp_max[]` | `health_ac[]` |
| Pool | An attribute that is the maximum of a resource | hit points | |
| Current | A pool's amount now, kept within the value. Not an attribute | `health_hp[]` | |
| Status | A flag (a bit of the status mask) that an effect grants while it lasts. Not an attribute | | |
| Condition | A status that impairs a creature | | |
| Saving throw | A roll that ends an effect: 1d20 + an ability's modifier against a DC | | |
| DC | Difficulty class: what a saving throw must reach | | |

## 3. Requirements

### Attributes

- **R1** Code and docs use the terms in section 2. "Base" means only the entity's
  permanent value.
- **R2** An attribute's tables follow one naming rule: the plain name is the value
  rules read, the base has a `_base` suffix, and a pool's current amount has the
  pool's plain name.
- **R3** A kind record's field holding a pool's maximum is named for the maximum:
  `hp_max`, `mp_max`.
- **R4** An attribute's base is a table of the component it belongs to, next to its
  value.
- **R5** The attribute ids are one numbered list, in `attrs.h`, and each names its
  component and tables.
- **R6** Only `effects_recompute()` writes a value, and only `effects_set_base()`
  changes a base.
- **R7** Setting the base of an attribute whose component the entity lacks is
  asserted.
- **R8** `attrs.h` explains attributes once. Other headers point to it, and every
  derived table says that it is derived.

### The effects core

- **R9** The core reads and writes bases only through the seam
  (`effects_core.h`). It includes nothing of the components.
- **R10** The core knows nothing of what a status means, of abilities or of dice.
  It ORs the bits of the effects granting them, ends effects by the bits they
  grant, and asks the game's side whether a saving throw is passed.

### Statuses and conditions

- **R11** The game says which statuses are conditions (`STATUS_CONDITIONS`).
- **R12** Only an entity with the stats component has statuses. Effects still act on
  any entity with health (poison damages a chest), but it gets no statuses.
- **R13** A condition lasts while an effect grants it, and ends when every effect
  granting it has ended.
- **R14** An effect ends when its time runs out, its saving throw is passed, or it
  is cured.
- **R15** An effect's definition may name a saving throw: an ability and a DC. The
  target rolls it each time the effect fires, after its tick.
- **R16** `EV_EFFECT_ENDED` says why the effect ended.

## 4. Design

### 4.1 Attributes

| Attribute | Component | Combines | Range |
|---|---|---|---|
| `ATTR_HP_MAX` | health | Added | 1-255 |
| `ATTR_AC` | health | Added | 0-255 |
| `ATTR_SPEED` | stats | Added | 1-255 |
| `ATTR_IMMUNE`, `ATTR_RESIST`, `ATTR_VULN` | health | ORed | Damage type masks |
| `ATTR_STR` to `ATTR_CHA` | stats | Added | 1-30 |

Each is only meaningful for an entity with its component. Modifiers can add bits
to a mask but not remove them.

`ATTR_ABILITY(a)` gives an ability's attribute, as both lists are in the same
order.

### 4.2 Table naming

| Stage | Name | Example |
|---|---|---|
| Value | Plain | `health_ac`, `health_hp_max`, `stats_ability[a]` |
| Base | `_base` | `health_ac_base`, `health_hp_max_base`, `stats_ability_base[a]` |
| Current | The pool's name | `health_hp` |

Bases are read-only outside their component and the effects module.

### 4.3 Setting and recomputing

- **At creation,** `health_add()` and `stats_add()` set base and value from the kind
  values. There are no modifiers yet, so no recompute is needed.
- **`effects_recompute()`** calls `derive()`, which starts from each base (0 for an
  attribute the entity hasn't) and adds the worn effects of equipped items and the
  entity's active effects. `effects_game_store()` then clamps and stores the
  results, and `EV_ATTRS_CHANGED` is posted if anything changed.
- **`effects_set_base()`** and `OP_RAISE_BASE` change a base for good, then
  recompute: potions that raise an ability, and gaining a level.
- **Pools:** a lower maximum takes the current amount down with it, and a higher one
  doesn't heal.
- **Speed** also sets the entity's timer period, so its turns follow it.

`effects_check()` compares what a recompute would make with what is stored, for
asserts and tests.

### 4.4 The seam

The effects core could become an nge module, so it doesn't include the components.
Three things connect it to the game:

- **`effects_game_attrs[]`** has one row per attribute: its base table, the
  component that has it, and whether modifiers are ORed. `derive()` reads it
  directly, and `base_of()` adds the assert for R7.
- **`effects_game_store()`** keeps the special cases: the clamps, hit points within
  the maximum, and the timer period.
- **`effects_game_save()`** rolls a saving throw.

`check-effects-core`, part of `make test`, fails if the core includes game content.

### 4.5 Statuses and conditions

The status mask is `stats_status[]` in slot 0, with `STATUS_*`, `STATUS_CONDITIONS`
and `STATUS_HAS()` in `stats.h`. It is derived like a value: the OR of the statuses
of every active effect and every equipped item's worn effect.

| Status | Condition | Granted by |
|---|---|---|
| `STATUS_POISONED` | Yes | Poison |
| `STATUS_BLINDED` | Yes | Blindness |
| `STATUS_UNCONSCIOUS` | Yes | Sleep |

A status that isn't a condition would be a beneficial one. The difference matters
only where a rule acts on conditions as a group: a cure-all
(`OP_CURE` with `STATUS_CONDITIONS`), and the PC screen's colours.

### 4.6 Saving throws

`effect_def_t.save` holds an ability and a DC, with DC 0 for no saving throw. It
works on any timed effect, so a non-condition such as slowness can have one.

At each firing of a timed effect, `effects_on_timer()` runs the tick op and then, if
the effect and its target are still there, calls `effects_game_save()`. That rolls
1d20 + the ability's modifier against the DC, with modifier 0 for an entity without
stats. A pass ends the effect.

### 4.7 How an effect ends

The core ends an effect through one internal function, with a reason in
`EV_EFFECT_ENDED`'s p2:

| Reason | When |
|---|---|
| `EFFECT_END_EXPIRED` | Its last firing, or `effect_end()` |
| `EFFECT_END_CURED` | `effect_cure()` or `OP_CURE`, which end effects by the statuses they grant |
| `EFFECT_END_SAVED` | Its saving throw was passed |

## 5. Decisions

- **D1 "Kind record", not "base record".** A kind's constant data and an entity's
  permanent value are different things, so only the second is "base". *Rejected:*
  "template" or "prototype", which are new words for what the code already calls a
  kind.
- **D2 The plain name is the value rules read.** Those names are used most, so they
  stay short, and a suffix keeps one attribute's tables together. *Rejected:*
  prefixes (`base_hp_max`, `cur_hp`), which split them apart.
- **D3 "Derived" for base plus modifiers,** with "value" as its short form in prose.
  *Rejected:* "effective", a second word for the same thing.
- **D4 The core finds bases through a table.** It is data, and sits beside the
  effect definitions. *Rejected:* get and set functions on the seam, which need a
  switch over every attribute.
- **D5 `attrs.h` stays with the components.** Attributes are the components'; effects
  only change them.
- **D6 A condition is a status the game marks as one.** *Rejected:* a separate
  condition mask, which means two masks, two definition fields and two cures for
  flags that work the same way.
- **D7 The status mask is in the stats component,** because conditions are about
  creatures. *Rejected:* health, which would give a door statuses, and the effects
  module, as the mask describes the creature.
- **D8 Statuses are derived; the core never sets or clears a bit.** *Rejected:*
  setting a bit when an effect starts and clearing it when one ends. With two
  sources of one bit, the first to end would clear it while the other still grants
  it, and a cure that cleared the bit would leave its effect running. Avoiding that
  needs a count per bit per entity.
- **D9 A saving throw is a field of the definition, rolled by the game's side.**
  *Rejected:* a game op in `on_tick`, because an effect has one tick op and poison
  uses it for damage. *Rejected:* the core rolling the d20, because abilities and
  dice are the game's rules.

## 6. Not built yet

| Mechanic | What's there, what's missing |
|---|---|
| Maximum MP as an attribute | It is a kind value today ([mana.md](mana.md)). It would follow this design: `ATTR_MP_MAX`, `mana_mp_max_base[]` and one row of `effects_game_attrs[]` |
| Blinded and unconscious | Effects grant them and the PC screen shows them, but no rule reads them and the main screen shows only poisoned |
| Other ways to avoid or end a condition | A saving throw when it is applied, waking when damaged, condition immunities (which could be a mask attribute) |
| Messages by reason | The reason is in the event, but every ending uses the effect's one text |
| More than 8 statuses | The mask and the definitions' `status` field would become `uint16_t` |
| A table-driven `effects_game_store()` | It keeps its special cases as code |

## 7. Tests

- **`test_health`, `test_stats`, `test_gen`:** bases and values are set from the kind
  values.
- **`test_effects`:** the table's rows; the assert on a missing component;
  recomputes, including an entity without stats; saving throws, with the ability
  modifier and with DC 0; and each reason for ending.
- **`check-effects-core`:** the core stays free of game content.

## 8. Known risks

- **zsdcc.** `derive()` and `effects_on_timer()` read struct fields in code that
  branches, so they copy the fields into locals first (T-ZSDCC-1). A row of a 2D
  array isn't a constant initialiser, so `effects_game_attrs[]` writes
  `&stats_ability_base[a][0]`.
- **Saves shorten effects.** Tuning an effect's duration or damage has to allow for
  its save: poison's DC 10 Constitution save usually ends it within two of its four
  firings for an average creature.
