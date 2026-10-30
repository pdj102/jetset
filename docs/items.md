# Items: design specification

How items work in Jetset: what the design requires, how it is built, and why.

The data is not repeated here. The kinds are in
[gen_item_pdata.c](../src/game/gen/gen_item_pdata.c), the effects in
[effects_pdata.c](../src/game/effects/effects_pdata.c), and memory use in
[memory_map.md](memory_map.md). The effects module is specified in
[effects.md](effects.md).

## 1. Overview

**Items are data.** Every item in the game is an instance of a **kind**, a
constant record that gives its name, look, subclass, attack, the slots it
fits, how many it holds, what it does and where it appears. Adding an item
means adding a record. Code never tests for a particular kind: it tests the
kind's subclass, flags or group.

**An item is an entity.** Creating one builds an entity from its kind, with
the item component and any attack the kind has. Because entities are few, a
stack of arrows is one entity with a count. Most of an item's data stays in
its kind and is read when needed.

**An item acts through a hook.** A kind has at most one hook, which says when
it acts, which effect it applies and at what **power**. Effects are rows in a
table shared with creatures and features, named for what they do rather than
for the item that uses them. A row's amounts are per power, so a healing
draught and a healing potion are the same effect at powers 3 and 6, and a
ring and a cursed ring can share one row at +1 and -2. An item's strength
therefore lives in the kind, and one effect serves many items.

```text
 kind records (gen_items[], in the item generator's data page)
      |  picked by depth and rarity, or asked for by kind
      v
 item entity (owner, location, slot, count, attack)
      |
      |  on the floor --pick up--> stowed --equip--> equipped
      |                              |                  |
      |                         use hook:           worn hook: its effect
      |                         effect applied,     counts while equipped
      |                         item destroyed
      |
      |  wielded weapon: hit hook, applied to what a hit leaves alive
      v
 effect_apply(effect, power, target, source)
      |
      +--> instant: done at once
      +--> timed: an effect entity that keeps its own power
```

**Some of this is data only for now.** The catalogue covers the classic item
classes, but several mechanics aren't built (section 6). Their items are
generated and carried, and everything those mechanics will need is already in
the records, so adding one should change code and leave the data alone.

## 2. Terms

| Term | Meaning |
|---|---|
| Kind | One record in `gen_items[]`, `IT_*`. Every item of a kind is the same |
| Subclass | The class of a kind, `ITEM_*`: weapon, potion, ring... |
| Hook | When a kind's effect applies, `GEN_HOOK_*` |
| Power | The number a hook gives its effect. The definition's amounts are multiplied by it |
| Band | The depths a kind appears on, `depth` to `depth_max` |
| Rarity | A kind's weight among those in band: higher is commoner, 0 is never |
| Count | An entity's quantity: missiles, charges, uses, burn time or gold |
| Group | What a kind matches: ammo with its launcher, a key with its lock |
| Pending effect | An effect defined as data whose mechanic isn't built, so it does nothing |

## 3. Requirements

### The items

- **R1** The game generates items of every subclass in section 4.2.
- **R2** Every kind is a record. Telling one kind from another needs no code: a
  rule may depend on a subclass, a flag or a group, never on an `IT_*`.
- **R3** A kind appears within a band, by rarity, so deep levels aren't filled
  with starter gear.
- **R4** Every depth has kinds to generate. Consumables stay common, magic grows
  with depth, and no subclass passes a quarter of the rarity weight at any depth.

### Power

- **R5** A hook names an effect and a power. The kind sets the strength, and the
  effect's definition stays one row.
- **R6** An effect's op amounts and number modifiers are multiplied by the power.
  Mask modifiers, statuses, durations and saving throws are not.
- **R7** A power can be negative, so a cursed item uses the same effect as a good
  one.
- **R8** An active effect keeps its power for as long as it lasts: the item that
  started it may be long gone.
- **R9** Applied again, a timed effect that doesn't stack keeps the stronger
  power, and its timer restarts.
- **R10** Creatures' and features' hooks work the same way.

### Effects

- **R11** Effects are named for what they do, so a ring, an amulet and a cloak can
  share one row.
- **R12** Every effect a kind names is defined. One whose mechanic is missing does
  nothing, safely.

### Equipment and stacks

- **R13** A humanoid has every equipment slot, and the inventory screen lists them.
- **R14** An item entity has a count, set from its kind when created.
- **R15** Kinds carry what the mechanics in section 6 will need: an aim hook, ammo
  and lock groups, and two-handed and cursed flags.

### Messages

- **R16** Messages suit the item: eat, drink or read by subclass, and "a" or "an"
  by the name's first letter.

### Constraints

- **R17** The table and its names fit their 8K page with at least 10% free.
- **R18** Items use at most 40 of the 128 sprite patterns. A recolour costs no
  pattern (it still costs a hardware sprite while on screen).
- **R19** A kind's name is 19 characters at most.
- **R20** Item generation copies no per-kind data onto the stack: the table is far
  bigger than the stack.

## 4. Design

### 4.1 The kind record

`gen_item_kind_t` ([gen_internal.h](../src/game/gen/gen_internal.h)):

| Field | Meaning |
|---|---|
| `name` | In the table's page |
| `render` | Sprite and colour bank |
| `subclass` | `ITEM_*` |
| `group` | `GROUP_*`, `KEY_*` or a lock pick's bonus, by subclass |
| `flags` | `ITEMF_*` |
| `count` | The entity's count when made |
| `fits` | The slots it can be equipped in, a mask |
| `attack` | Dice 0 for none; range 2 or more means a ranged attack |
| `hook` | When, which effect and what power; `EF_NONE` for none |
| `where` | Band and rarity |

### 4.2 Subclasses

| Subclass | What it is | How it works |
|---|---|---|
| `ITEM_WEAPON` | Melee weapons | Attack; some have a hit hook |
| `ITEM_RANGED` | Launchers | Ranged attack, and a group for its ammo |
| `ITEM_AMMO` | Missiles and thrown items | Count, group; thrown ones have their own attack |
| `ITEM_ARMOUR` | Body, head, arms, hands, legs, feet, cloak, shields | Worn hook |
| `ITEM_FOOD`, `ITEM_DRINK` | Eaten or drunk | Use hook |
| `ITEM_POTION` | Good and bad potions | Use hook |
| `ITEM_PARCHMENT` | Read on the reader, or aimed | Use or aim hook |
| `ITEM_WAND` | Wands and staves | Aim hook; count is charges |
| `ITEM_RING`, `ITEM_AMULET` | Jewellery, some cursed | Worn hook |
| `ITEM_LIGHT` | Torches, lanterns | Worn hook; count is burn time |
| `ITEM_KEY` | Keys | Group is the locks it opens |
| `ITEM_TOOL` | Lock picks | Group is its bonus; count is uses |
| `ITEM_TREASURE` | Gold and gems | Count is gold |
| `ITEM_QUEST` | The quest item | Rarity 0; see [winning.md](winning.md) |

### 4.3 Where an item is

An item's place follows from its owner, location and `item_slot`
([item.h](../src/game/components/item.h)):

| State | Owner | Location | Slot |
|---|---|---|---|
| On the floor | An area | Yes | `SLOT_NONE` |
| Stowed | Its holder | No | `SLOT_NONE` |
| Equipped | Its holder | No | A `SLOT_*` |

The slots are masks in 16 bits, and all 15 are in use, so another slot needs a
wider mask.

### 4.4 Hooks

| Hook | Used by | Applied | To |
|---|---|---|---|
| `GEN_HOOK_USE` | Items | When used up: drunk, eaten or read | The user |
| `GEN_HOOK_HIT` | Weapons, ammo, creatures | A hit that leaves its target alive | The target |
| `GEN_HOOK_WORN` | Equipment | Counts while equipped | The wearer |
| `GEN_HOOK_INNATE` | Creatures | When generated | Itself |
| `GEN_HOOK_AIM` | Wands, staves, parchments | At a chosen target; no action reads it yet | The target |

`gen_hook_of()` answers for any class, with the effect and power packed in 16
bits, read with `GEN_HOOK_EF()` and `GEN_HOOK_POWER()`. An item has one hook;
creatures and features have an array with one entry per hook.

The hook also decides what is offered: an item is usable only if it has a use
hook, and a healing potion is found by its effect, whatever its strength. That
is how the AI chooses what to drink.

### 4.5 Effects and power

A definition's amounts are per power.

- **Number modifiers** add `amount * power` to the attribute. **Mask modifiers**
  are ORed in as they are.
- **Ops** get a copy with the amount scaled, clamped to -127 to 127.
- **Worn effects** are summed from each equipped item's hook whenever the
  derived attributes are recomputed.
- **Timed effects** are entities. `effect_power[]`, beside `effect_kind[]`, holds
  each one's power for its ticks, recomputes and ending.

Effects come in three families, and timed effects are numbered first:

| Family | What it does | Examples |
|---|---|---|
| Timed | Lasts, may tick | Poison, haste, stoneskin, fire resistance |
| Instant | Happens once | Healing, cures, raising an ability, bolts |
| Worn | Counts while equipped | `EF_WORN_AC`, `EF_WORN_HP`, ability and resistance rows |

**Pending effects** are complete rows, so the data won't change when their
mechanic arrives. Their ops are defined and do nothing, or they are worn or
timed rows with nothing yet that reads what they grant.

### 4.6 Attacks

An item has one attack part, and its range says which use it is: 1 is melee,
and 2 or more is ranged. A plus on a weapon is simply larger numbers in its
attack, needing no effect.

The data is laid out for firing, which isn't built:

- A launcher holds the shot's dice, type, range and bonuses.
- Ammo with dice 0 adds only its to-hit and damage bonus to the launcher's. It
  has no attack component, and its stats are read from its kind.
- Ammo with dice is thrown (`GROUP_THROWN`), needs no launcher, and has a ranged
  attack of its own.
- A hit hook on ammo applies as on a weapon.

### 4.7 Stacks, groups and flags

- **A stack is one entity,** and `item_count[]` holds its quantity in a byte.
  What the count means depends on the subclass (section 4.2).
- **`group`** is one byte whose meaning depends on the subclass. Ammo and
  launchers must share a `GROUP_*`. A key opens locks of its `KEY_*`, and
  `KEY_ANY` opens all.
- **`flags`** hold `ITEMF_TWO_HANDS` and `ITEMF_CURSED`, stored for rules to come.

### 4.8 Generation

`gen_item_random(depth, owner)` picks a kind and `gen_item()` builds the entity.
`GEN_PICK()` sums the rarity of the kinds in band, rolls, and walks the table
again to find the one rolled. It is a macro, expanded in the code page of the
table it reads, so that page's data is mapped and nothing is copied (R20).
Creatures use the same macro, and every class has the same `gen_where_t`.

### 4.9 Names and messages

Names are one string in the record. Kinds are named so that they read well after
"You pick up a": stacks are named for their container ("sheaf of arrows"), and
parchments as "identify parchment".

`EV_USED` picks eat, drink or read from the subclass. `set_article()` corrects
" a " to " an " before a vowel in any name it prints, so individual messages
needn't handle it.

### 4.10 Sprites

Kinds of an item share a pattern, and differ by colour bank. Sprites that come in
several materials are drawn with the material in the primary ramp, because a bank
changes only slots 9-15 ([sprite_style.md](sprite_style.md) 6.6). There are too
few banks for every potion to have its own colour, so unrelated potions can share
one; their names tell them apart.

## 5. Decisions

- **D1 One kind per variation.** "ring of health +1" and "+2" are separate
  records, so each has its own band, rarity, name and colour, with no rule for
  any of it, and the name is a single string. *Rejected:* a plus rolled per
  entity, or prefix and suffix tables. Both need an extra table per entity,
  names built at run time, and a rule tying plus to depth. Either could still be
  added on top of this design.
- **D2 Power multiplies; it doesn't replace.** One rule then covers a row with
  several amounts, and a sign turns a blessing into a curse. *Rejected:* the
  kind's value replacing the amount, which is ambiguous with two amounts and
  can't express a sign. *Rejected:* dice in the hook, which cost 3 bytes a hook
  for a rule a mechanic can add later. *Rejected:* scaling durations, since this
  catalogue only needs strength.
- **D3 Every hook has a power; 0 is an error.** So there is no hidden rule that
  treats 0 as 1. A test checks this.
- **D4 Active effects store their power.** A worn effect can read its item at
  every recompute, but a potion's effect outlives the potion. *Rejected:*
  recording the source item, as an entity ID can be reused. On a refresh the
  higher power stays, so a weak poison can't dilute a strong one.
- **D5 Armour shares one effect.** All armour is `EF_WORN_AC` with the AC as its
  power, so plate +2 needs no row of its own.
- **D6 One hook per item kind.** Each kind in the catalogue needs only one, and
  it saves 7 bytes a kind. Two effects on one item can usually be one definition.
  A second hook would cost 3 bytes a kind, which would fit.
- **D7 No `health` part in the item record.** No item uses one. Destructible
  items could use a flag.
- **D8 The aim hook is separate from the hit hook.** The hook alone then says that
  an item needs a target. Everything with charges aims, even healing, because
  `GEN_HOOK_USE` destroys the item.
- **D9 Counts are one byte.** Gold stops at 255 a find, which is enough.
- **D10 No multiply or `strcmp()`.** Library routines link into common memory even
  when called from a banked page, so `times()` adds in a loop and `set_article()`
  compares by hand. Powers are at most 127, and effects are applied rarely.
- **D11 Food heals until there is hunger.** Food that did nothing would be a
  wasted turn. `EF_NOURISH` is the one row to change, and the kinds stay as they
  are.
- **D12 Timed effects come first in the numbering.** Only they can be active, so
  anything counting active effects sizes its arrays by `EF_TIMED_COUNT`, keeping
  the PC screen's stack use small. A test checks the order.
- **D13 Long names may be cut when equipped.** The list panel is 28 columns, so a
  long name can lose the end of its slot word. This is cosmetic and accepted.

## 6. Not built yet

Each item below is generated and carried. The data is in place, and only the
mechanic is missing.

| Mechanic | Waiting data |
|---|---|
| Firing and throwing | Launchers, ammo, groups; the fire command says there is nothing to fire |
| Aiming and charges | Aim hooks and counts on wands, staves and parchments; they can't be used yet |
| Merging and using up counts | `item_count[]` |
| Locks | Keys, lock picks, their groups and counts |
| Two-handed weapons | `ITEMF_TWO_HANDS` |
| Sticky curses | `ITEMF_CURSED` (a cursed item's negative power already works) |
| Hunger | `EF_NOURISH` (D11) |
| Light | `EF_WORN_LIGHT`, with the radius as its power; a light's count as burn time |
| Worn regeneration | `EF_WORN_REGEN`: a worn effect has no tick |
| The pending ops | Teleport, mapping, identify, summon, enchant, uncurse, light: used up with their message, but nothing happens |
| Blindness and sleep | The statuses are granted, but no rule reads them |
| Gold | Treasure's count: it is carried as an item, not added to the PC's gold |

Also outside this design: identification and shuffled appearances (a per-game
table from kind to look, which would sit on top of this design), new attributes
such as to-hit rings, containers, spellbooks, and more than one enchantment on
an item. The catalogue's numbers are a first pass, for play to correct.

## 7. Tests

The PC tests enforce the rules that the data could break:

- **`test_gen`:** every kind is well formed: name length, power and effect, the
  right family of effect for its hook, rarity, band, slots; each subclass has
  what its mechanic will need; the mix by depth (R4); `GEN_PICK()`; what
  `gen_item()` creates.
- **`test_effects`:** scaling for instant, worn and timed effects, negative
  powers, masks, refreshes, clamps, the numbering (D12), and that pending ops run
  safely.
- **`test_act`:** use and hit pass the kind's power, and the slots equip.
- **`test_ui`:** the verbs and "a" or "an".

## 8. Known risks

- **zsdcc.** Scaling and `GEN_HOOK_POWER()` depend on promotion and sign, and the
  PC tests can't catch a miscompile there. After changing them, read the asm
  ([toolchain.md](guidelines/toolchain.md), T-ZSDCC-3).
- **Armour can outrun monsters.** With a full set of the best gear, monsters with
  low to-hit hit only on a 20 (roll + to-hit >= AC). Their to-hit must grow with
  depth, or AC will need a cap.
- **Inert finds.** Items waiting for a mechanic are generated at their real rarity,
  so the mix can be seen in play. Until those mechanics exist, many finds do
  nothing.
- **Treasure takes pack slots** until gold is picked up as gold.
- **Row letters.** In the choosing lists, letters run past "z" after 26 rows
  (`'a' + i`), which a full list can reach.
