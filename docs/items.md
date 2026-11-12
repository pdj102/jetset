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

**Armour's AC is in its record, not its hook** (section 4.11). Armour's hook
is then free for a property, so a mail can give AC and resist fire, as a
weapon's bonus is in its attack and its effect in its hook.

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
| Data | A byte of the kind record whose meaning depends on the subclass, a named union (4.7): the ammo group, a key's lock group, a pick's bonus, a book's spells or armour's AC |
| Group | What a kind matches: ammo with its launcher (`GROUP_*`), a key with its lock (`KEY_*`) |
| Armour's AC | The AC an armour kind gives while equipped, from its record (section 4.11) |
| Property | An armour kind's worn effect beside its AC: a resistance, an ability... |
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

### Armour (I2)

- **R21** An armour kind's AC is a number in its record, not its hook. It
  counts while the armour is equipped, recomputed with the worn effects.
- **R22** Armour's hook is free for one property, a worn effect that counts
  beside its AC.
- **R23** A plus on armour is a larger AC in the record, as a plus on a weapon
  is larger numbers in its attack (4.6).
- **R24** Armour with a property has the AC of its plain kind: gauntlets of
  might give an iron gauntlet's AC as well as their Str.
- **R25** Jewellery's AC stays a worn effect: the ring of armour and the amulet
  of guarding keep `EF_WORN_AC` on their hook.
- **R26** The inventory shows an armour's AC from its record, and an
  `EF_WORN_AC` item's from its hook, as "AC n" (ui_inventory.md).
- **R27** Ego armour, from existing worn effects: armour of fire ward, dragon
  scale mail (AC and fire immunity), an elven cloak and a shield of warding
  (4.11).
- **R28** The record gains no bytes: page 109 has 589 bytes free (R17).
- **R29** The record's subclass-dependent byte is a named union, `data`, with a
  member for each meaning: `ammo_group`, `key_lock_group`, `pick_bonus`,
  `book_spells`, `armour_ac`. The data and the code name the meaning they mean,
  never a bare `group`.
- **R30** Each meaning has its own reader, which checks the subclass, so a
  reader never returns another meaning's number (a key's lock group as AC).

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
| `render` | Sprite pattern and colour bank, 2 bytes: the generator adds the rest (items_extensions.md D1) |
| `subclass` | `ITEM_*` |
| `data` | One byte, a named union by subclass (4.7): `ammo_group`, `key_lock_group`, `pick_bonus`, `book_spells` or `armour_ac` |
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
| `ITEM_RANGED` | Launchers | Ranged attack, and `ammo_group` for its ammo |
| `ITEM_AMMO` | Missiles and thrown items | Count, `ammo_group`; thrown ones have their own attack |
| `ITEM_ARMOUR` | Body, head, arms, hands, legs, feet, cloak, shields | `armour_ac`; a worn hook for a property, or none (4.11) |
| `ITEM_FOOD`, `ITEM_DRINK` | Eaten or drunk | Use hook |
| `ITEM_POTION` | Good and bad potions | Use hook |
| `ITEM_PARCHMENT` | Read on the reader, or aimed | Use or aim hook |
| `ITEM_WAND` | Wands and staves | Aim hook; count is charges |
| `ITEM_RING`, `ITEM_AMULET` | Jewellery, some cursed | Worn hook |
| `ITEM_LIGHT` | Torches, lanterns | Worn hook; count is burn time |
| `ITEM_KEY` | Keys | `key_lock_group` is the locks it opens; rarity 0: made only with a lock (action_unlock.md R11) |
| `ITEM_TOOL` | Lock picks | `pick_bonus`; count is uses |
| `ITEM_TREASURE` | Gold and gems | Count is gold |
| `ITEM_QUEST` | The quest item | Rarity 0; see [winning.md](winning.md) |
| `ITEM_BOOK` | Spell books: basic, adept's, master's (`IT_BOOK_*`, after `IT_QUEST`) | `book_spells` is its list of spells (`SPELL_BOOK_*`); no hook, can't be used, worn or aimed; read with `b`. See [spells.md](spells.md) section 10 |

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
| `GEN_HOOK_WORN` | Jewellery, lights, armour's property | Counts while equipped | The wearer |
| `GEN_HOOK_INNATE` | Creatures | When generated: only `EFFECT_INNATE` effects | Itself |
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
- **Timed effects** are entities. Each one's kind and power are its timer's
  payload (`EFFECT_POWER_OF()`), for its ticks, recomputes and ending.

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
- **`data`** is one byte whose meaning depends on the subclass: a named union,
  `gen_item_data_t` (D19), one member per meaning, each with its reader in page
  108 (R30):

  | Member | Subclasses | Holds | Reader | Another subclass |
  |---|---|---|---|---|
  | `ammo_group` | `ITEM_RANGED`, `ITEM_AMMO` | `GROUP_*`: ammo and launchers must share one | `gen_item_ammo_group()` | `GROUP_NONE` |
  | `key_lock_group` | `ITEM_KEY` | `KEY_*`: the locks it opens; `KEY_ANY` opens all | `gen_item_key_lock_group()` | 0, no lock |
  | `pick_bonus` | `ITEM_TOOL` | the lock pick's bonus to its roll | `gen_item_pick_bonus()` | 0 |
  | `book_spells` | `ITEM_BOOK` | `SPELL_BOOK_*`: its list of spells | `gen_item_book_spells()` | an assert: 0 is a list |
  | `armour_ac` | `ITEM_ARMOUR` | its AC (4.11) | `gen_item_armour_ac()` | 0 |

  The table sets a member by name, `.data.armour_ac = 5`; a subclass with no
  meaning leaves `data` 0. `gen_item_group()` goes: its callers (firing, the
  shot's look, keys, picks, learning, the spell screen) each call their
  meaning's reader.
- **`flags`** hold `ITEMF_TWO_HANDS`, `ITEMF_CURSED` and `ITEMF_DIRECT`. The first two's
  rules are in the inventory component, and an item's lifted curse is an entity flag,
  `ITEM_UNCURSED`. `ITEMF_DIRECT` makes an aimed item pass creatures on the way to the
  chosen tile ([actions.md](actions.md), section 4.11, and [action_aim.md](action_aim.md)).

### 4.8 Generation

`gen_item_random(depth, owner)` picks a kind and `gen_item()` builds the entity.
`GEN_PICK()` sums the rarity of the kinds in band, rolls, and walks the table
again to find the one rolled. It is a macro, expanded in the code page of the
table it reads, so that page's data is mapped and nothing is copied (R20).
Creatures use the same macro, and every class has the same `gen_where_t`.

Kinds added after the catalogue are numbered at the end, so no number moves: the spell
books (after `IT_QUEST`), then the stairway parchment (`EF_LEVEL_TELEPORT`), the
levitation potion (`EF_LEVITATE`) and boots of levitation (`EF_WORN_FLY`), from
[effects_extensions.md](effects_extensions.md), then I1's 42 kinds (166 to 207,
[items_extensions.md](items_extensions.md)), then I2's four armour kinds (208 to
211, section 4.11). `IT_COUNT` is 212.

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

### 4.11 Armour

**AC.** An armour kind's AC is `data.armour_ac` (D14), unsigned, 0 for none.
`gen_item_armour_ac(kind)`, banked with the table in page 108, returns it for
`ITEM_ARMOUR` and 0 for any other subclass, so a key's lock group never counts
as AC (D16). In the table: `.data.armour_ac = 5`.

**Counting.** `derive()` in `attrs_bank.c` already walks the equipped items
for their worn hooks. For each one it also adds `gen_item_armour_ac()` to
`ATTR_AC`, as a number modifier would, so the AC is clamped and stored with
the rest and the PC screen shows it against the base as before. A property
on the hook is added as today. No effect row is needed for the AC.

**The data.** Every armour kind whose hook is `EF_WORN_AC` moves the power to
its AC and drops the hook (24 kinds). Armour with a property gains the AC of
its plain kind (R24):

| Kind | AC | Hook |
|---|---|---|
| gauntlets of might | 2, as iron gauntlets | `EF_WORN_STR` 2, as now |
| boots of speed | 1, as leather boots | `EF_WORN_SPEED` 2, as now |
| boots of levitation | 1, as leather boots | `EF_WORN_FLY` 1, as now |
| fireproof cloak | 1, as a cloak | `EF_WORN_RES_FIRE` 1, as now |
| cloak of protection | 3, its old power: a cloak +2 | none |

The best full set still totals AC 25 (plate +2, iron head, arms, hands, legs
and feet, cloak of protection, tower shield): gauntlets of might now match
iron gauntlets instead of giving up their AC.

**Ego armour** (I2.2), numbered at the end, data only:

| ID | Name | Look | AC | Hook | Band, rarity |
|---|---|---|---|---|---|
| `IT_MAIL_FIRE_WARD` 208 | mail of fire ward | mail, `BANK_ORANGE` | 5, as chain mail | worn `EF_WORN_RES_FIRE` 1 | 5-B, 1 |
| `IT_DRAGON_MAIL` 209 | dragon scale mail | mail, `BANK_RED` | 6, as scale mail +2 | worn `EF_WORN_IMMUNE_FIRE` 1 | 8-B, 1 |
| `IT_ELVEN_CLOAK` 210 | elven cloak | cloak, `BANK_GREEN` | 1, as a cloak | worn `EF_WORN_DEX` 1 | 4-B, 1 |
| `IT_SHIELD_WARDING` 211 | shield of warding | shield, `BANK_PURPLE` | 2, as an iron shield | worn `EF_WORN_RES_MAGIC` 1 | 5-B, 1 |

B is `LEVEL_DEPTH_BOTTOM`. Each look is a pattern and bank no other kind uses.
"Mail of fire ward" follows "amulet of acid ward" (a resistance); "fireproof"
names immunity on the I1 amulets, and the fireproof cloak, older, keeps its
name. Each adds 22 bytes and its name: 154 bytes of page 109 for the four.

**The inventory** (R26). `screen_inv_bank.c`'s last case prints "AC n" from
`gen_item_armour_ac()` when it isn't 0, else from an `EF_WORN_AC` hook (the ring of
armour, the amulet of guarding). A property isn't shown: the name says it.

**As built.**

- *Step 1 (the data union, 2026-11-12).* Page 109 came out byte for byte the
  same. 30 rows set a member (the plan said 32). Page 108 grew by 516 bytes in
  the debug build, not the 60 or so the plan expected. Each reader is about 110
  bytes, about 45 of them for the assert's file and expression strings: zsdcc
  keeps one copy per function. Page 108 still has 6.2K free. The callers and
  common memory stayed the same size. A key reader's "another subclass" value
  is named `KEY_NONE` (0) in `gen_kinds.h`. `feature_key_fits()` keeps its own
  subclass check, because an unlocked door's key bits are also 0.
- *Step 2 (AC in the record, 2026-11-12).* Page 109 stayed the same size.
  Page 138 grew by 56 bytes, not about 15: the banked call and the 16-bit add
  into `d->value[ATTR_AC]`. Page 116 grew by 16 bytes. effects.md never named
  `EF_WORN_AC`, so only its comment in `effects.h` changed.
- *Step 3 (ego armour, 2026-11-12).* Page 109 grew by 154 bytes to 7,757,
  as predicted (435 free). The dev G key's first kind is now a constant,
  `DEV_ITEM_FIRST` in `player_bank.c`, set to the newest kinds
  (`IT_MAIL_FIRE_WARD`), so G cycles through just the four. Page 114 stayed
  the same size.
- *Step 4 (docs, 2026-11-12).* As well as the docs the plan named,
  action_learn.md and action_unlock.md now name the readers in place of the
  kind's "group". spells_plan.md is a finished plan and keeps `.group`.

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
- **D5 Armour shares one effect.** *(Superseded by D14: armour's AC is in its
  record. `EF_WORN_AC` stays, for jewellery.)* All armour is `EF_WORN_AC` with
  the AC as its power, so plate +2 needs no row of its own.
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
- **D13 Long names may be cut when equipped.** *(Superseded: the inventory's rows
  have no slot words now, and every name fits; [ui_inventory.md](ui_inventory.md).)*
  The list panel is 28 columns, so a
  long name can lose the end of its slot word. This is cosmetic and accepted.
- **D14 Armour's AC shares the subclass byte** (answered 2026-11-12). Armour
  has no ammo, lock, bonus or spells, so the byte that holds them is free,
  and it costs nothing in page 109. It is the `armour_ac` member of the
  union `data` (D19). *Rejected:* an `ac` field of its own, the to-do's first
  proposal: 1 byte for each of 212 kinds, 212 of the 589 bytes free, paid by
  every potion and key. *Rejected:* a second hook (3 bytes a kind, 636), which
  I2.3 keeps for a kind needing two properties.
- **D15 AC is unsigned.** 0 to 255, as `health_ac[]`. Cursed armour, when it
  is wanted, puts a negative `EF_WORN_AC` on its hook.
- **D16 One reader per meaning, checking the subclass** (R30). With five
  meanings in one byte, a read with the wrong meaning returns a plausible
  number, not an error: a key's `KEY_GOLD` would be AC 4. Each reader returns
  its member only for its subclasses, so the rule is in one place, and
  `derive()` can ask any equipped item for its AC without knowing subclasses.
  About 15 bytes a reader in page 108, which has 6.7K free. *Rejected:* one
  `gen_item_data()` returning the byte, as `gen_item_group()` did, leaving
  every caller to know the subclass.
- **D17 Armour with a property keeps its plain kind's AC** (R24). A pair of
  gauntlets of might that gave up their AC was the cost of the one hook, not a
  design. The best set's AC doesn't change (4.11).
- **D19 A named union, not an anonymous one** (asked for 2026-11-12, as an
  anonymous union). zsdcc (z88dk's, `-clib=sdcc_iy`) gets designated
  initialisers of an anonymous union's members wrong, with only a warning
  ("excess elements in struct initializer"): only the first member can be set,
  any other comes out 0, and the fields after it in the record are lost or
  shifted (a pick's bonus landed in `fits`). gcc compiles it correctly, so the
  PC tests would pass. A named union, `.data.armour_ac = 5` or
  `.data = { .armour_ac = 5 }`, compiled right in the same test (checked
  2026-11-12 by reading the asm), and reads the same in code:
  `gen_items[kind].data.armour_ac`.
- **D18 Not now: I2.3 and I2.4** (low). A second hook only when a kind needs
  two properties; enchantment rolled per entity would need a byte per entity
  and names built at run time (D1).

## 6. Not built yet

Each item below is generated and carried. The data is in place, and only the
mechanic is missing.

| Mechanic | Waiting data |
|---|---|
| Using up counts | `item_count[]`, on what isn't ammo. Ammo merges and is used up |
| Hunger | `EF_NOURISH` (D11) |
| Light | `EF_WORN_LIGHT`, with the radius as its power; a light's count as burn time |
| Worn regeneration | `EF_WORN_REGEN`: a worn effect has no tick |
| The pending ops | Mapping, identify, enchant, light: used up with their message, but nothing happens. Teleport and summon work (effects_extensions.md) |
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
  `gen_item()` creates; armour has an AC or a worn hook, never `EF_WORN_AC`
  on its hook (D14); each `data` reader returns its member for its subclasses
  and its "another subclass" value for every other kind (R30); the table's
  bytes match the members set (a guard against D19's miscompile, for the
  copy the Next runs: read the asm of `gen_item_pdata.c` once).
- **`test_effects`:** scaling for instant, worn and timed effects, negative
  powers, masks, refreshes, clamps, the numbering (D12), and that pending ops run
  safely.
- **`test_act`:** use and hit pass the kind's power, and the slots equip.
- **`test_effects`** (armour): worn armour adds its AC and its property
  together (dragon scale mail: AC and fire immunity), and taking it off
  removes both.
- **`test_ui`:** the verbs and "a" or "an"; "AC n" for armour and for a ring of
  armour.

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
- **Page 109 is below R17's 10%.** 589 bytes free (7.2%) after I1, 435 (5.3%)
  after I2's four kinds (7,757 bytes used). A later kind needs room made first:
  the names could move to page 108, the generator's, which has about 6.1K free.
- **Treasure takes pack slots** until gold is picked up as gold.
- **Row letters.** In the choosing lists, letters run past "z" after 26 rows
  (`'a' + i`), which a full list can reach.
