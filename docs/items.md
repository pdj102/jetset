# Items: requirements and design

Status: implemented (2026-10-30); D1 to D20 and Q1 to Q6 agreed as proposed
(section 13). `make` and `make test` pass, and the CSpect checks in section 11
passed (2026-10-30). Not yet tried on hardware. Section 14 lists where the build
differs from the design.

This spec gives the game a classic roguelike's items: 160 kinds in 16
subclasses, spread over depths 1 to 10 by rarity. It changes how effects get
their amounts, so one effect can serve every strength of an item, and extends
the kind record, the hooks and the equipment slots to cover the new classes.

Mechanics the new items will need (firing, zapping, locks, hunger,
identifying) and new art are out of scope. Section 9 says what each subclass
does once this is built, and what it only carries as data.

## 1. Goal

- The game can generate every classic class of item, across a range of depths
  and rarities.
- Items are data: a new item is a record, not code.
- A stronger version of an item needs a new kind, not a new effect.
- It all fits: the table in its 8K page, the art in its share of the 128
  patterns, and the code in what is left of common memory and the stack.

## 2. How it works today

- **A kind** is a 27-byte record in `gen_items[]` (page 109), with its name:
  8 kinds, 302 bytes.
- **A hook** is an effect number in the record, `hooks[GEN_HOOK_*]`: use, hit,
  worn, innate. The kind says which effect, and nothing about how strong.
- **An effect's amounts are fixed in its definition** (`effect_defs[]`, page
  121). `EF_HEALING` heals 6, and `EF_RING_HEALTH` adds 2. A second strength
  is a second effect.
- **`gen_item_random()`** copies every kind's `where` onto the stack, and
  `gen_pick()` (page 110) chooses from the copy, because page 109 is mapped
  out while page 110 runs.
- **A weapon's numbers are in the kind:** the `melee` part (to-hit, dice,
  bonus, type, range). They use no effect.

What limits the design, measured on the current build:

| Limit | Today | Why it matters here |
|---|---|---|
| Page 109: 8,192 bytes | 302 used | Room for about 200 kinds, so this is not the tightest limit |
| Stack: 480 bytes (`$BE00`-`$BFE0`) | | `where[160]` at 3 bytes a kind is 480 bytes: the whole stack |
| Common memory: **83 bytes free** | program ends at `$BDAD` | New common code has almost no room |
| 128 sprite patterns | 14 used, 30 budgeted for items (sprite_style.md 6.8) | Item art must share patterns |
| Names: 19 characters | `NAME_MAX` 20 | Longer names are silently cut |
| Equipment slots: 15 at most | 10 used | The `uint16_t` masks end at bit 15 |
| Entities: 100 | | 20 arrows can't be 20 entities |

The 128 that limits art is **patterns**, the 4-bit shapes in sprite memory,
rather than sprites on screen (`NGE_RENDER_SPRITES`, 64). A recolour costs no
pattern. It still costs a hardware sprite while on screen, like any item.

## 3. Terms

| Term | Meaning |
|---|---|
| Kind | One record in `gen_items[]`, `IT_*`: "ring of health +2". Every item of a kind is the same |
| Subclass | The class of a kind, `ITEM_*`: weapon, potion, ring... |
| Hook | Which game event applies a kind's effect, `GEN_HOOK_*` |
| Power | **New.** The number a hook gives its effect. The definition's amounts are multiplied by it |
| Band | The depths a kind appears on, `depth` to `depth_max` |
| Rarity | A kind's weight among those in band: higher is commoner, 0 is never |
| Count | **New.** An entity's quantity: missiles, charges, uses, burn time or gold |
| Group | **New.** What a kind matches: ammo with its launcher, a key with its lock |
| Placeholder sprite | A sprite name that means an existing sprite until its art is drawn |
| Pending effect | An effect defined as data whose op does nothing until its mechanic is built |

## 4. Requirements

**Changed** and **New** mark requirements that differ from the request or add
to it.

### The items

- **R1** The game generates items of these subclasses:

  | Subclass | Kinds | Examples |
  |---|---|---|
  | `ITEM_WEAPON` | 23 | dagger, sword, axe, mace, spear, sword +3, flame sword |
  | `ITEM_RANGED` | 6 | sling, short bow, long bow, crossbow |
  | `ITEM_AMMO` | 9 | arrows, bolts, stones, and **New** thrown darts and javelins |
  | `ITEM_ARMOUR` | 27 | body, head, arms, hands, legs, feet, **New** cloak; shields |
  | `ITEM_FOOD` | 6 | apple, bread, pie |
  | `ITEM_DRINK` **New** | 2 | waterskin, ale |
  | `ITEM_POTION` | 26 | healing, abilities, haste, resistance, bad potions |
  | `ITEM_PARCHMENT` | 12 | identify, teleport, fire |
  | `ITEM_WAND` | 8 | wands, and **New** staves |
  | `ITEM_RING` | 18 | health, armour, abilities, wards, cursed rings |
  | `ITEM_AMULET` **New** | 5 | health, warding |
  | `ITEM_LIGHT` **New** | 3 | torch, lantern |
  | `ITEM_KEY` | 5 | iron key to gold key, skeleton key |
  | `ITEM_TOOL` | 2 | lock pick, fine lock pick |
  | `ITEM_TREASURE` **New** | 7 | gold, gems |
  | `ITEM_QUEST` | 1 | the crown, unchanged (winning.md) |

  *Reason for the additions: amulets, lights and treasure are classic classes.
  `SLOT_NECK` and `SLOT_LIGHT` exist with nothing to fill them, and gold items
  are already planned (player_character.md R20). Drinks are separate so that
  the message can say "drink" (R23).*
- **R2** Every kind is a record in `gen_item_pdata.c`. Telling one kind from
  another needs no code. A rule may depend on a subclass, a flag or a group,
  never on an `IT_*`.
- **R3 (Changed)** A kind appears within a band, `depth` to `depth_max`, by
  rarity. *Reason: "from depth n" keeps every kind for ever. With 160 kinds, a
  dagger would still be picked on depth 10, and the deep levels would be
  mostly starter gear.*
- **R4** Each depth offers a mix like section 8.2: consumables stay common,
  magic grows with depth, and the commonest subclass never passes about 20%.

### The constraints

- **R5** The table and its names fit page 109 with at least 10% free.
- **R6** Items use at most 40 patterns. The catalogue uses 39.
- **R7** A kind's name is 19 characters at most (a PC test checks this).
- **R8 (New)** Item generation copies no per-kind data onto the stack.
- **R9 (New)** The change adds no net code to common memory. The build's
  "bytes free below the stack" stays at 60 or more.

### Power

- **R10** A hook names an effect and a power. The kind sets the strength, and
  the effect's definition stays one row.
- **R11** An effect's op amounts and number modifiers are multiplied by the
  power. Mask modifiers, statuses, durations and saving throws are not.
- **R12** A power can be negative, so a cursed ring of strength uses the same
  effect as a good one.
- **R13 (New)** An active effect keeps its power for as long as it lasts. The
  item that started it may be long gone.
- **R14 (New)** Applied again, a timed effect that doesn't stack keeps the
  stronger power, and its timer restarts.
- **R15** Creatures' hooks work the same way: a viper can bite harder than a
  rat with the same `EF_POISON`.
- **R16** The existing items keep their amounts: a healing potion still heals
  6, and the ring of health that adds 2 is now "ring of health +2".

### Effects

- **R17** Effects are named for what they do, not for the item that uses them,
  so a ring, an amulet and a cloak can share one row.
- **R18** Every effect the catalogue names is defined (section 7).
- **R19** Effects that need a missing mechanic are defined now and do nothing
  yet: applying one is safe, and its op doesn't abort.

### The framework

- **R20** `SLOT_RANGED`, `SLOT_AMMO`, `SLOT_ARMS`, `SLOT_LEGS` and `SLOT_CLOAK`
  are added, and a humanoid has them all. The inventory screen lists them.
- **R21** An item entity has a count, set from its kind when created. Using a
  count up is a later mechanic.
- **R22** Kinds can carry what later mechanics need, stored and tested now but
  with no rules yet: an aim hook, ammo and lock groups, two-handed and cursed
  flags.
- **R23 (New)** Messages suit the new items: eat, drink or read by subclass, and
  "a" or "an" by the name's first letter. *Reason: "You drink the fire
  parchment", and 21 kinds start with a vowel ("You pick up a oil lantern").*

### Out of scope, for later

- **R24** Sprites: the art itself. Placeholders stand in (D15).
- **R25** The mechanics: firing, throwing and zapping; merging and using up
  counts; locks, keys and lock picks; the two-handed rule; sticky curses;
  hunger; light radius; teleport, mapping, identify, summon, enchant, uncurse;
  the blinded and unconscious rules; picking gold up into the PC's gold.
- **R26** Identification: unknown items and shuffled appearances. A shuffle
  would need a per-game table from kind to look, which sits on top of this
  design without changing it.
- **R27** New attributes: to-hit and damage rings, light radius.
- **R28** Containers, spellbooks and more than one enchantment on an item.
- **R29** Balance. The numbers are a first pass, for play to correct (Q6).

## 5. Decisions

- **D1 Keep one kind per variation, as proposed (R2).** "ring of health +1"
  and "+2" are separate records. Each has its own band, rarity, name and
  colour, so a +3 can be deep and rare with no rule for it, and the name is one
  string to print. 160 kinds fit in 76% of the page, so the page doesn't force
  a more compact scheme.
  **Rejected:** a plus rolled per entity, as Rogue and NetHack do. It means
  fewer kinds, but needs a 100-byte table, names built at run time in 19
  characters, a rule tying plus to depth, and it splits an item's definition
  between data and code. It can still be added later on top of this design.
  **Rejected:** prefix and suffix tables ("flaming" + "sword"): the same costs,
  more so.

- **D2 Power is a multiplier held by the hook, not a replacement (R10, R11).**
  A definition's amounts become amounts per power: `EF_HEALING` heals 1 per
  power, and the healing potion's hook says 6.

  ```c
  .hook = { GEN_HOOK_USE, EF_HEALING, 6 },
  ```

  One rule then covers an effect with several amounts. Stoneskin is AC +1 and
  speed +1 per power, so power 3 scales both, and power -2 on a worn effect
  turns a blessing into a curse without another row.
  **Rejected:** the kind's value replaces the definition's amount. It is
  ambiguous for a definition with two amounts, and can't express a sign.
  **Rejected:** dice in the hook (`1d8+2`). Healing is flat today, and dice
  take 3 bytes a hook for a rule that a mechanic can add later.
  **Rejected:** scaling durations too: most effects want either more strength
  or more time, and this catalogue only needs strength. A flag can add it if
  an item wants it.

- **D3 Every hook needs a power; 0 is an error (R10).** A test checks that a
  hook naming an effect has a non-zero power. So `{ EF_CURE, 1 }` is written
  out, and no hidden rule treats 0 as 1.

- **D4 An active effect stores its power: `effect_power[]`, page 121 (R13,
  R14).** This is the trap in the proposal. A worn effect can read its item's
  kind at every recompute, because the item is still there. A potion's effect
  outlives the potion, and its later ticks and recomputes have nothing to read.
  So `effect_apply()` takes the power, and an active effect keeps it beside
  `effect_kind[]`, 100 bytes in a page with plenty of room. On a refresh the
  higher power stays, so a weak poison can't dilute a strong one. Ties and
  negative powers compare as plain numbers.
  **Rejected:** recording the source item (effects.h already rules that out:
  an ID can be reused).

- **D5 "+N" weapons and armour need no effect of their own.** A weapon's plus
  is already data: "sword +2" is `to_hit` and `bonus` in its attack part.
  All armour shares `EF_WORN_AC`, with the AC as its power, so leather is 2,
  plate is 7, and plate +2 is 9. This removes most of the effects the request
  feared.

- **D6 Effects are named by what they do (R17).** `EF_RING_HEALTH` becomes
  `EF_WORN_HP`. Rings, amulets and armour share the `EF_WORN_*` rows, 51
  effects serve 160 kinds, and `carried_use()` in the AI keeps working: every
  healing potion is `EF_HEALING`, whatever its strength.

- **D7 An item kind has one hook, not an array (record 25 bytes).** No kind in
  the catalogue needs two. A single `{ hook, ef, power }` is 3 bytes against 10
  for an array of five pairs, and that saves 1,120 bytes over 160 kinds. Two
  effects on one item can usually be one definition (`EFFECT_MODS` is 2).
  Creatures and features keep their arrays, now of pairs.
  **Option:** a second hook later costs 3 bytes a kind, 480 in all, which fits.

- **D8 Remove the item record's `health` part.** No item kind uses it, and it
  costs 5 bytes a kind, 800 bytes in all. Destructible items would bring it
  back or, better, use a flag.

- **D9 `GEN_HOOK_AIM`: an effect sent at a chosen target (R22).** Wands, staves
  and attack parchments use it. Keeping it apart from `GEN_HOOK_HIT` means the
  hook alone says that an item needs a target. Every wand and staff aims, even
  healing, because `GEN_HOOK_USE` destroys the item (`act_use()`) and a wand has
  charges. With no action reading the hook yet, they are simply carried, and
  the inventory doesn't offer to use them.

- **D10 A stack is one entity with a count (R21).** With 100 entities, 15
  arrows can't be 15 entities. `item_count[]` in slot 0 holds a stack's
  missiles, or a wand's charges, a lock pick's uses, a light's burn time or a
  treasure's gold. A kind's `count` sets it. Gold in one byte stops at 255 a
  find, which is enough here.

- **D11 One attack part: range says melee or ranged.** `.melee` becomes
  `.attack`, and `create()` adds `ATTACK_RANGED` when its range is 2 or more.
  How a shot will work, so the data is right now:
  - A launcher holds the shot's dice, type, range and bonuses.
  - Ammo with dice 0 (arrows) adds only its `to_hit` and `bonus` to the
    launcher's. It has no attack component, and its stats are read from its
    kind.
  - Ammo with dice is thrown, needing no launcher (`GROUP_THROWN`: darts,
    javelins, oil), and has a ranged attack of its own.
  - A hit hook on ammo applies as on a weapon, so a flask of oil burns.

- **D12 `group`: one byte, meaning set by subclass (R22).** Ammo and launchers
  must have the same `GROUP_*` to work together. A key opens locks of its group
  (`KEY_ANY` opens all), and a lock pick's group is its bonus. One byte serves
  all three, and only these subclasses read it.

- **D13 Bands are in `gen_where_t` for every class (R3).** It becomes `{ depth,
  depth_max, rarity }`, and creatures and features get a `depth_max` too, so
  there is one picker.

- **D14 The pick runs in the table's own page (R8).** `GEN_PICK()`, a macro
  like `GEN_COPY_NAME()`, walks the table directly from page 108 or 106, and
  `rand()` is in common memory. It replaces `gen_pick()` in page 110 and the
  stack copies. The alternative, keeping the function and a `where` copy, would
  use the whole stack at 160 kinds.

- **D15 Placeholder sprites are names, not frames (R6, R24).** One header
  falls back to an existing sprite until the real tag exists:

  ```c
  #ifndef SPRITE_DAGGER
  #define SPRITE_DAGGER  SPRITE_SWORD
  #endif
  ```

  `sprite_data.h` is generated, so the names can't go there. When the artist
  tags `dagger`, the importer defines `SPRITE_DAGGER`, the fallback goes quiet,
  and no record changes. No pattern is used until there is art.
  **Rejected:** blank frames in the Aseprite file: they spend 36 patterns on
  nothing and every item would look blank.
  Banks are already final in the records, so the placeholders show the right
  colours on the wrong shapes.

- **D16 Materials need the primary ramp (R6).** Slots 1-8 are the same in every
  bank, so a sprite drawn in steel greys never recolours. Sprites that come in
  several materials (armour, helmets, shields, weapons with colourways) must be
  drawn with the material in the primary ramp (sprite_style.md 6.6). The rule
  for the art to come: a bank changes only what is drawn in slots 9-15. Today's
  sword shows it only in its gem. With 13 usable colourways, some unrelated
  potions share a colour; their names tell them apart until identification
  (R26) makes it matter.

- **D17 Pending effects are complete rows with ops that do nothing yet (R19).**
  `OP_TELEPORT`, `OP_MAP` and the rest are defined, and `effects_game_op()`
  groups them under one comment and a `break`. The data then won't change when
  a mechanic arrives. Only one `case` will. Worn and timed pending effects need
  no op: they have no modifiers yet, or grant a status no rule reads yet.

- **D18 Food heals for now.** `EF_NOURISH` is `OP_HEAL` per power, because
  there is no hunger, and food that did nothing would be a wasted turn.
  Eating already works. When hunger comes, this row changes, and the kinds stay
  as they are.

- **D19 Timed effects come first in the numbering.** `draw_effects()` (the PC
  screen) keeps 3 bytes per `EF_COUNT` on the stack, 153 bytes at 51 effects.
  Numbering the 12 timed effects first, and sizing those arrays by
  `EF_TIMED_COUNT` (13), makes that 39 bytes with no new logic. Only timed
  effects can be active, and a test checks the order.

- **D20 Names fit 19 characters, and stacks are named as one thing (R7).**
  "parchment of identify" is too long, so parchments are "identify parchment".
  Stacks are named for their container ("sheaf of arrows"), which reads right
  after "You pick up a". Three strengths of healing are "healing draught",
  "healing potion" and "healing philtre". The list panel is 28 columns, so a
  long name loses the end of its slot word when equipped ("gauntlets of might
  (on han"). That is cosmetic and accepted.

## 6. Design

### 6.1 The kind record

```c
/* gen_internal.h */
typedef struct gen_where {
    uint8_t depth;       /* the shallowest it appears */
    uint8_t depth_max;   /* the deepest */
    uint8_t rarity;      /* weight among those in band, 0: never */
} gen_where_t;

typedef struct gen_hook {   /* creatures and features: hooks[GEN_HOOK_COUNT] */
    uint8_t ef;      /* EF_*, EF_NONE for none */
    int8_t  power;   /* the definition's amounts are multiplied by this; not 0 if ef is set */
} gen_hook_t;

typedef struct gen_item_hook {
    uint8_t hook;    /* GEN_HOOK_*: when */
    uint8_t ef;
    int8_t  power;
} gen_item_hook_t;

typedef struct gen_item_kind {     /* bytes */
    const char     *name;          /* 2: 19 characters at most, in the table's page */
    gen_render_t    render;        /* 5 */
    uint8_t         subclass;      /* 1: ITEM_* */
    uint8_t         group;         /* 1: GROUP_* or KEY_*, or a lock pick's bonus */
    uint8_t         flags;         /* 1: ITEMF_* */
    uint8_t         count;         /* 1: item_count[] when made */
    uint16_t        fits;          /* 2 */
    attack_kind_t   attack;        /* 6: dice 0 for none; range 2 or more: ranged */
    gen_item_hook_t hook;          /* 3: ef EF_NONE for none */
    gen_where_t     where;         /* 3 */
} gen_item_kind_t;                 /* 25 (27 today) */
```

A record, as the table will have it:

```c
[IT_RING_HEALTH_2] = {
    .name = "ring of health +2",
    .render = ITEM_RENDER(SPRITE_RING, BANK_RED),
    .subclass = ITEM_RING,
    .fits = SLOT_BIT(SLOT_RING_L) | SLOT_BIT(SLOT_RING_R),
    .hook = { GEN_HOOK_WORN, EF_WORN_HP, 2 },
    .where = { .depth = 3, .depth_max = 10, .rarity = 2 },
},
```

```c
/* gen_kinds.h: the subclasses, the existing numbers kept */
#define ITEM_POTION     0
#define ITEM_WEAPON     1
#define ITEM_RING       2
#define ITEM_FOOD       3
#define ITEM_QUEST      4
#define ITEM_RANGED     5
#define ITEM_AMMO       6
#define ITEM_ARMOUR     7
#define ITEM_DRINK      8
#define ITEM_PARCHMENT  9
#define ITEM_WAND       10
#define ITEM_AMULET     11
#define ITEM_LIGHT      12
#define ITEM_KEY        13
#define ITEM_TOOL       14
#define ITEM_TREASURE   15

/* Flags: stored now, their rules later (R25) */
#define ITEMF_TWO_HANDS  0x01   /* fills SLOT_SHIELD too */
#define ITEMF_CURSED     0x02   /* can't be taken off */

/* Groups: an ammo and a launcher must match; a key opens its own */
#define GROUP_NONE    0
#define GROUP_ARROW   1
#define GROUP_BOLT    2
#define GROUP_STONE   3
#define GROUP_THROWN  4         /* needs no launcher */
#define KEY_IRON      1
#define KEY_BRASS     2
#define KEY_SILVER    3
#define KEY_GOLD      4
#define KEY_ANY       5
```

The `IT_*` numbers run in the order of section 8.3, and `IT_COUNT` is 160.
`kind_index[]` holds them in a byte, up to 254.

### 6.2 Hooks

| Hook | Who | Applied | To |
|---|---|---|---|
| `GEN_HOOK_USE` | items | used up: drunk, eaten, read | the user |
| `GEN_HOOK_HIT` | weapons, ammo, creatures | a hit that leaves its target alive | the target |
| `GEN_HOOK_WORN` | equipment | counts while equipped | the wearer |
| `GEN_HOOK_INNATE` | creatures | when generated | itself |
| `GEN_HOOK_AIM` **New** | wands, staves, parchments | later, at a chosen target (D9) | the target |

Common memory can't take a second dispatch function (R9), so the existing one
returns both values:

```c
/* gen.h: effect in the low byte, power in the high byte */
uint16_t gen_hook_of(nge_entity_id_t id, uint8_t hook);

#define GEN_HOOK_EF(h)     ((uint8_t)(h))
#define GEN_HOOK_POWER(h)  ((int8_t)(uint8_t)((h) >> 8))
```

The rename is deliberate. `gen_hook(...) == ef` in the AI would still compile
and would never be true, so a missed caller should fail to build. The callers
to change are `ai_think.c:55`, `act_use.c:36`, `act_melee.c:88`,
`effects_game.c:81` and `screen_inv_bank.c:107` and `:317`, plus the tests.

### 6.3 Effects

```c
/* effects.h */
uint8_t effect_apply(uint8_t ef, int8_t power, nge_entity_id_t target,
                     nge_entity_id_t source) NGE_BANKED;

/* effects_core.h */
void effects_add(effects_derived_t *d, uint8_t ef, int8_t power);

/* page 121: each active effect's power */
extern int8_t effect_power[NGE_ENTITY_MAX];
```

- **`effects_add()`** adds `amount * power` to a number attribute, and ORs a
  mask attribute's amount as it is. The sum is `int16_t`, as `value[]` is.
- **`run_op()`** takes the power and passes the game's side a copy of the op
  with its amount scaled, clamped to -127 to 127. The seam doesn't change.
- **`derive()`** reads `effect_power[]` with `effect_kind[]`.
- **`effects_game_sources()`** passes the worn hook's power.
- **`effect_apply()`** stores the power in a new instance, and keeps the larger
  on a refresh (D4).
- **`end()` and `effects_on_timer()`** read the power before the effect goes.

Callers: `act_use()` reads the hook before destroying the item, as it does now.
`act_melee()` and `gen_creature()` pass the hook's power.

### 6.4 Items and the generator

```c
/* item.h */
#define SLOT_RANGED  11   /* new: a launcher */
#define SLOT_AMMO    12   /* new: what it shoots, or what is thrown */
#define SLOT_ARMS    13   /* new */
#define SLOT_LEGS    14   /* new */
#define SLOT_CLOAK   15   /* new */
#define SLOT_COUNT   16   /* the limit: the masks are full */

extern uint8_t item_count[NGE_ENTITY_MAX];   /* slot 0 */
void item_add(nge_entity_id_t id, uint8_t count) NGE_BANKED;
```

- `SLOTS_HUMANOID` gains the five.
- `create()` no longer reads `health`. It adds the attack by its range, and
  passes the count.
- `gen_item_hook()` returns the packed pair if the kind's hook is the one asked
  for, and 0 if not.
- `gen_item_random()` uses `GEN_PICK()`, as does `gen_creature_random()`.
- Screens: `slot_words[]` gains " (ready)", " (quiver)", " (on arms)",
  " (on legs)" and " (on back)". `SCREEN_INV_MAX` goes from 30 to 35: a pack of
  20 and 15 in use.
- Messages: `EV_USED` picks eat, drink or read from the subclass. A helper in
  page 118 picks "a" or "an" for the places that say "a" before a name.

### 6.5 Placeholder sprites

`src/game/gen/gen_item_sprites.h` holds the fallbacks (D15). Budget: 39 of 40
patterns.

| Have (3) | New (36), with the placeholder shown until drawn |
|---|---|
| potion, sword, ring | **as sword:** dagger, greatsword, axe, mace, spear, staff, bow, crossbow, arrow, dart, wand, torch |
| | **as ring:** sling, shot, armour.soft, armour.mail, armour.plate, helmet, vambraces, gauntlets, greaves, boots, cloak, shield, parchment, amulet, key, lock.pick, coins, gem, crown |
| | **as potion:** bread, meat, fruit, flask, lantern |

Shared patterns: war hammer and club with the mace; halberd and javelin with the
spear; bolts with the arrow; quarterstaff with the magic staff. The dead rat
keeps the rat's own `rat.dead`.

sprite_style.md 6.8 budgets 30 patterns for items, so this needs 10 from
somewhere else (Q2).

### 6.6 Memory

| Where | Now | After | Note |
|---|---|---|---|
| Page 109 | 302 | **6,248**, 76% | 4,000 of records, 2,248 of names; 1,944 free, about 50 more kinds |
| Page 121 | 486 | about 2.6K | 51 rows of 26 bytes, their texts, `effect_power[]` |
| Page 120 | 4,816 | about 5.1K | the scaling |
| Page 108 | 886 | about 1K | the pick; less from `health` |
| Page 110 | | a little less | `gen_pick()` goes |
| Slot 0 | ends `$19AF` | ends `$1A13` | `item_count[]`, 100 bytes; the limit is `$1FFF` |
| Common | 83 free | 60 or more free (R9) | the same functions, one return type wider |
| Stack | | 16 bytes less in generation; 15 more on the PC screen | D14, D19 |

## 7. The effects

51 rows, counting `EF_NONE`. Amounts are per power. "Today" marks rows that exist, and **Pending**
marks rows with no working mechanic yet. Rows are in `EF_*` order (D19).

### Timed (12)

| Effect | Per power | Lasts | Status, saving throw |
|---|---|---|---|
| `EF_POISON` | 1 poison damage a firing | 3 turns x 4 | poisoned; Con 10. Today |
| `EF_REGEN` | heal 1 a firing | 10 turns, for ever, stacks | Today, the player's innate |
| `EF_RENEW` | heal 1 a firing | 1 turn x 10 | |
| `EF_HASTE` | speed -1 | 20 turns | |
| `EF_SLOW` | speed +1 | 20 turns | Wis 12 |
| `EF_MIGHT` | Str +1 | 20 turns | |
| `EF_WEAKNESS` | Str -1 | 20 turns | Con 12 |
| `EF_STONE_SKIN` | AC +1, speed +1 | 20 turns | |
| `EF_RESIST_FIRE` | resist fire (mask) | 30 turns | |
| `EF_RESIST_COLD` | resist cold (mask) | 30 turns | |
| `EF_BLIND` | | 10 turns | blinded; Con 12. **Pending:** no rule reads it |
| `EF_SLEEP` | | 8 turns | unconscious; Wis 12. **Pending:** no rule reads it |

### Instant (23)

| Effect | Per power | Note |
|---|---|---|
| `EF_HEALING` | heal 1 | Today, was 6 |
| `EF_CURE` | ends poisoning | Today |
| `EF_CURE_ALL` | ends every condition | `OP_CURE` with `STATUS_CONDITIONS` |
| `EF_MANA` | restore 1 MP | **New** `OP_MANA`, `mana_restore()` |
| `EF_VITALITY` | base max HP +1 | Today |
| `EF_STRENGTH` | base Str +1 | Today |
| `EF_RAISE_DEX`, `_CON`, `_INT`, `_WIS`, `_CHA` | base ability +1 | 5 rows, `OP_RAISE_BASE` |
| `EF_NOURISH` | heal 1 | D18 |
| `EF_BOLT_MAGIC`, `_FIRE`, `_COLD`, `_ACID` | 1 damage of its type | 4 rows, `OP_DAMAGE`. Aimed, or as a weapon's hit |
| `EF_LIGHT` | | **Pending** `OP_LIGHT` |
| `EF_IDENTIFY` | | **Pending** `OP_IDENTIFY` |
| `EF_MAP` | | **Pending** `OP_MAP` |
| `EF_TELEPORT` | | **Pending** `OP_TELEPORT` |
| `EF_UNCURSE` | | **Pending** `OP_UNCURSE` |
| `EF_ENCHANT` | | **Pending** `OP_ENCHANT` |
| `EF_SUMMON` | | **Pending** `OP_SUMMON` |

### Worn (15)

| Effect | Per power | Note |
|---|---|---|
| `EF_WORN_HP` | max HP +1 | Today as `EF_RING_HEALTH`, +2 |
| `EF_WORN_AC` | AC +1 | all armour (D5) |
| `EF_WORN_SPEED` | speed -1 | speed is ticks a turn: lower is faster |
| `EF_WORN_STR`, `_DEX`, `_CON`, `_INT`, `_WIS` | ability +1 | 5 rows, negative when cursed |
| `EF_WORN_RES_FIRE`, `_COLD`, `_POISON`, `_ACID`, `_MAGIC` | resist it (mask) | 5 rows |
| `EF_WORN_REGEN` | | **Pending:** a worn effect has no tick |
| `EF_WORN_LIGHT` | | **Pending:** no light radius (R27). Power is the radius |

## 8. The catalogue

### 8.1 Existing kinds

| Now | Becomes | Changed |
|---|---|---|
| `IT_POTION` | `IT_HEAL_POTION` | depth 1-8 |
| `IT_SWORD` | `IT_SWORD` | depth 1-8 |
| `IT_RING` | `IT_RING_HEALTH_2` | "ring of health +2", red, depth 3-10, rarity 2 (was 3) |
| `IT_ANTIDOTE` | `IT_ANTIDOTE` | rarity 4, as today |
| `IT_FOOD` | `IT_DEAD_RAT` | depth 1-6, rarity 3 (was 4) |
| `IT_ELIXIR` | `IT_ELIXIR` | from depth 3 |
| `IT_STRENGTH` | `IT_POTION_STR` | from depth 3 |
| `IT_QUEST` | `IT_QUEST` | its own sprite when drawn |

### 8.2 The mix by depth

Each subclass's share of the rarity weight in band, in percent, computed from
the tables below. Rows are subclasses, and columns are depths.

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|
| Melee | 18 | 15 | 16 | 14 | 14 | 13 | 11 | 11 | 9 | 6 |
| Ranged | 5 | 5 | 5 | 4 | 5 | 4 | 5 | 4 | 3 | 3 |
| Ammo | 7 | 9 | 8 | 7 | 8 | 8 | 7 | 8 | 8 | 8 |
| Armour | 16 | 13 | 13 | 15 | 14 | 15 | 11 | 10 | 10 | 10 |
| Food, drink | 12 | 10 | 9 | 8 | 9 | 9 | 7 | 9 | 9 | 10 |
| Potion | 16 | 16 | 16 | 17 | 17 | 15 | 18 | 19 | 18 | 19 |
| Parchment | 11 | 12 | 12 | 12 | 12 | 12 | 11 | 13 | 15 | 16 |
| Wand | 0 | 1 | 2 | 3 | 3 | 3 | 4 | 4 | 4 | 5 |
| Ring | 1 | 3 | 4 | 4 | 5 | 6 | 6 | 6 | 6 | 6 |
| Amulet | 0 | 1 | 1 | 1 | 1 | 1 | 2 | 2 | 2 | 2 |
| Light | 3 | 3 | 2 | 2 | 2 | 3 | 3 | 3 | 4 | 4 |
| Key, tool | 4 | 3 | 3 | 3 | 3 | 4 | 4 | 4 | 3 | 2 |
| Treasure | 8 | 11 | 9 | 9 | 8 | 8 | 10 | 8 | 9 | 10 |
| **Kinds in band** | 41 | 71 | 100 | 122 | 134 | 139 | 128 | 120 | 107 | 103 |

The level generator places 4 to 10 items a level, so a game sees about 70 items,
and less than half the kinds.

### 8.3 The kinds

`IT_` is left off the first column. Stats are to-hit, dice and type. For ammo
without dice they are its to-hit and damage bonuses, added to the launcher's.
Hooks read `when, effect, power`. Fits names a `SLOT_*`, and RING means both.

#### Melee weapons (23)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `CLUB` | club | MACE | EARTH | 1-3 | 5 | +0 1d4 blunt; fits WEAPON |
| `DAGGER` | dagger | DAGGER | IRON | 1-5 | 6 | +1 1d4 pierce; fits WEAPON |
| `QUARTERSTAFF` | quarterstaff | STAFF | EARTH | 1-4 | 5 | +0 1d6 blunt; fits WEAPON; two hands |
| `SHORT_SWORD` | short sword | SWORD | IRON | 1-6 | 5 | +1 1d6 slash; fits WEAPON |
| `HAND_AXE` | hand axe | AXE | IRON | 1-6 | 5 | +0 1d6+1 slash; fits WEAPON |
| `SPEAR` | spear | SPEAR | IRON | 1-6 | 5 | +1 1d6 pierce; fits WEAPON |
| `MACE` | mace | MACE | IRON | 2-7 | 5 | +0 1d6+1 blunt; fits WEAPON |
| `SWORD` | sword | SWORD | BLUE | 1-8 | 5 | +1 1d8 slash; fits WEAPON |
| `BATTLE_AXE` | battle axe | AXE | BLUE | 3-9 | 4 | +0 1d8+1 slash; fits WEAPON |
| `WAR_HAMMER` | war hammer | MACE | BLUE | 3-9 | 4 | +0 1d8+1 blunt; fits WEAPON |
| `GREATSWORD` | greatsword | GREATSWORD | IRON | 4-10 | 3 | +0 1d10+1 slash; fits WEAPON; two hands |
| `HALBERD` | halberd | SPEAR | BLUE | 5-10 | 3 | +1 1d10 pierce; fits WEAPON; two hands |
| `DAGGER_1` | dagger +1 | DAGGER | GREEN | 2-6 | 2 | +2 1d4+1 pierce; fits WEAPON |
| `SWORD_1` | sword +1 | SWORD | GREEN | 3-8 | 2 | +2 1d8+1 slash; fits WEAPON |
| `SWORD_2` | sword +2 | SWORD | PURPLE | 5-10 | 2 | +3 1d8+2 slash; fits WEAPON |
| `SWORD_3` | sword +3 | SWORD | GOLD | 8-10 | 1 | +4 1d8+3 slash; fits WEAPON |
| `MACE_1` | mace +1 | MACE | GREEN | 3-8 | 2 | +1 1d6+2 blunt; fits WEAPON |
| `BATTLE_AXE_1` | battle axe +1 | AXE | GREEN | 5-10 | 2 | +1 1d8+2 slash; fits WEAPON |
| `GREATSWORD_2` | greatsword +2 | GREATSWORD | PURPLE | 7-10 | 1 | +2 1d10+3 slash; fits WEAPON; two hands |
| `RUSTY_SWORD` | rusty sword | SWORD | EARTH | 1-5 | 2 | -1 1d8-1 slash; fits WEAPON; cursed |
| `VENOM_DAGGER` | venom dagger | DAGGER | LIME | 4-10 | 1 | +1 1d4 pierce; fits WEAPON; hit `EF_POISON` +2 |
| `FLAME_SWORD` | flame sword | SWORD | ORANGE | 6-10 | 1 | +2 1d8 slash; fits WEAPON; hit `EF_BOLT_FIRE` +3 |
| `FROST_AXE` | frost axe | AXE | ICE | 6-10 | 1 | +1 1d8+1 slash; fits WEAPON; hit `EF_BOLT_COLD` +3 |

#### Ranged weapons (6)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `SLING` | sling | SLING | EARTH | 1-5 | 5 | +0 1d4 blunt, range 5; fits RANGED; group STONE |
| `SHORT_BOW` | short bow | BOW | EARTH | 1-7 | 5 | +0 1d6 pierce, range 6; fits RANGED; group ARROW |
| `LONG_BOW` | long bow | BOW | GREEN | 3-10 | 4 | +1 1d8 pierce, range 8; fits RANGED; group ARROW |
| `CROSSBOW` | crossbow | CROSSBOW | EARTH | 2-8 | 4 | +1 1d8 pierce, range 7; fits RANGED; group BOLT |
| `HEAVY_CROSSBOW` | heavy crossbow | CROSSBOW | IRON | 5-10 | 2 | +1 1d10+1 pierce, range 7; fits RANGED; group BOLT |
| `LONG_BOW_2` | long bow +2 | BOW | PURPLE | 7-10 | 1 | +3 1d8+2 pierce, range 9; fits RANGED; group ARROW |

#### Ammo (9)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `STONES` | pouch of stones | SHOT | IRON | 1-6 | 5 | +0 +0; fits AMMO; count 15; group STONE |
| `IRON_SHOT` | pouch of iron shot | SHOT | BLUE | 3-10 | 2 | +0 +1; fits AMMO; count 12; group STONE |
| `ARROWS` | sheaf of arrows | ARROW | EARTH | 1-10 | 6 | +0 +0; fits AMMO; count 15; group ARROW |
| `ARROWS_1` | sheaf of arrows +1 | ARROW | GREEN | 4-10 | 2 | +1 +1; fits AMMO; count 10; group ARROW |
| `BOLTS` | case of bolts | ARROW | IRON | 2-10 | 5 | +0 +0; fits AMMO; count 12; group BOLT |
| `BOLTS_1` | case of bolts +1 | ARROW | GREEN | 5-10 | 2 | +1 +1; fits AMMO; count 8; group BOLT |
| `DARTS` | handful of darts | DART | IRON | 1-6 | 4 | +0 1d3 pierce, range 4; fits AMMO; count 8; group THROWN |
| `JAVELINS` | bundle of javelins | SPEAR | EARTH | 2-8 | 3 | +0 1d6 pierce, range 5; fits AMMO; count 5; group THROWN |
| `OIL` | flask of oil | FLASK | ORANGE | 2-10 | 3 | +0 1d2 blunt, range 4; fits AMMO; hit `EF_BOLT_FIRE` +4; count 1; group THROWN |

#### Armour (27)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `ROBE` | cloth robe | ARMOUR_SOFT | BLUE | 1-4 | 5 | fits BODY; worn `EF_WORN_AC` +1 |
| `LEATHER_ARMOUR` | leather armour | ARMOUR_SOFT | EARTH | 1-6 | 6 | fits BODY; worn `EF_WORN_AC` +2 |
| `RING_MAIL` | ring mail | ARMOUR_MAIL | EARTH | 2-7 | 4 | fits BODY; worn `EF_WORN_AC` +3 |
| `SCALE_MAIL` | scale mail | ARMOUR_MAIL | GREEN | 3-8 | 3 | fits BODY; worn `EF_WORN_AC` +4 |
| `CHAIN_MAIL` | chain mail | ARMOUR_MAIL | IRON | 4-9 | 3 | fits BODY; worn `EF_WORN_AC` +5 |
| `PLATE_ARMOUR` | plate armour | ARMOUR_PLATE | IRON | 6-10 | 2 | fits BODY; worn `EF_WORN_AC` +7 |
| `LEATHER_ARMOUR_1` | leather armour +1 | ARMOUR_SOFT | GREEN | 2-7 | 2 | fits BODY; worn `EF_WORN_AC` +3 |
| `CHAIN_MAIL_1` | chain mail +1 | ARMOUR_MAIL | PURPLE | 5-10 | 1 | fits BODY; worn `EF_WORN_AC` +6 |
| `PLATE_ARMOUR_2` | plate armour +2 | ARMOUR_PLATE | GOLD | 8-10 | 1 | fits BODY; worn `EF_WORN_AC` +9 |
| `CAP` | leather cap | HELMET | EARTH | 1-5 | 4 | fits HEAD; worn `EF_WORN_AC` +1 |
| `HELMET` | iron helmet | HELMET | IRON | 3-10 | 3 | fits HEAD; worn `EF_WORN_AC` +2 |
| `BRACERS` | leather bracers | VAMBRACES | EARTH | 1-6 | 3 | fits ARMS; worn `EF_WORN_AC` +1 |
| `VAMBRACES` | iron vambraces | VAMBRACES | IRON | 4-10 | 2 | fits ARMS; worn `EF_WORN_AC` +2 |
| `GLOVES` | leather gloves | GAUNTLETS | EARTH | 1-6 | 3 | fits HANDS; worn `EF_WORN_AC` +1 |
| `GAUNTLETS` | iron gauntlets | GAUNTLETS | IRON | 4-10 | 2 | fits HANDS; worn `EF_WORN_AC` +2 |
| `GAUNTLETS_MIGHT` | gauntlets of might | GAUNTLETS | ORANGE | 6-10 | 1 | fits HANDS; worn `EF_WORN_STR` +2 |
| `LEGGINGS` | leather leggings | GREAVES | EARTH | 1-6 | 3 | fits LEGS; worn `EF_WORN_AC` +1 |
| `GREAVES` | iron greaves | GREAVES | IRON | 4-10 | 2 | fits LEGS; worn `EF_WORN_AC` +2 |
| `BOOTS` | leather boots | BOOTS | EARTH | 1-6 | 3 | fits FEET; worn `EF_WORN_AC` +1 |
| `IRON_BOOTS` | iron boots | BOOTS | IRON | 4-10 | 2 | fits FEET; worn `EF_WORN_AC` +2 |
| `BOOTS_SPEED` | boots of speed | BOOTS | ICE | 7-10 | 1 | fits FEET; worn `EF_WORN_SPEED` +2 |
| `CLOAK` | cloak | CLOAK | EARTH | 1-7 | 3 | fits CLOAK; worn `EF_WORN_AC` +1 |
| `CLOAK_PROTECTION` | cloak of protection | CLOAK | PURPLE | 5-10 | 1 | fits CLOAK; worn `EF_WORN_AC` +3 |
| `CLOAK_FIRE` | fireproof cloak | CLOAK | ORANGE | 4-10 | 1 | fits CLOAK; worn `EF_WORN_RES_FIRE` +1 |
| `BUCKLER` | wooden buckler | SHIELD | EARTH | 1-6 | 4 | fits SHIELD; worn `EF_WORN_AC` +1 |
| `SHIELD` | iron shield | SHIELD | IRON | 3-10 | 3 | fits SHIELD; worn `EF_WORN_AC` +2 |
| `TOWER_SHIELD` | tower shield | SHIELD | BLUE | 6-10 | 1 | fits SHIELD; worn `EF_WORN_AC` +3 |

#### Food (6)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `APPLE` | apple | FRUIT | RED | 1-6 | 7 | use `EF_NOURISH` +1 |
| `BREAD` | loaf of bread | BREAD | EARTH | 1-10 | 8 | use `EF_NOURISH` +2 |
| `PIE` | meat pie | BREAD | GOLD | 2-10 | 4 | use `EF_NOURISH` +3 |
| `RATION` | travel ration | MEAT | EARTH | 3-10 | 4 | use `EF_NOURISH` +5 |
| `ROAST` | roast joint | MEAT | RED | 5-10 | 2 | use `EF_NOURISH` +8 |
| `DEAD_RAT` | dead rat | RAT_DEAD | EARTH | 1-6 | 3 | use `EF_POISON` +1 |

#### Drink (2)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `WATERSKIN` | waterskin | FLASK | EARTH | 1-10 | 5 | use `EF_NOURISH` +1 |
| `ALE` | mug of ale | FLASK | GOLD | 1-8 | 3 | use `EF_NOURISH` +2 |

#### Potion (26)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `HEAL_DRAUGHT` | healing draught | POTION | RED | 1-5 | 12 | use `EF_HEALING` +3 |
| `HEAL_POTION` | healing potion | POTION | RED | 1-8 | 10 | use `EF_HEALING` +6 |
| `HEAL_PHILTRE` | healing philtre | POTION | RED | 4-10 | 5 | use `EF_HEALING` +12 |
| `POTION_LIFE` | potion of life | POTION | GOLD | 8-10 | 1 | use `EF_HEALING` +100 |
| `ANTIDOTE` | antidote | POTION | GREEN | 1-10 | 4 | use `EF_CURE` +1 |
| `PANACEA` | panacea | POTION | FLESH | 4-10 | 2 | use `EF_CURE_ALL` +1 |
| `MANA_DRAUGHT` | mana draught | POTION | BLUE | 1-7 | 4 | use `EF_MANA` +3 |
| `MANA_POTION` | mana potion | POTION | BLUE | 4-10 | 3 | use `EF_MANA` +8 |
| `ELIXIR` | elixir | POTION | GOLD | 3-10 | 1 | use `EF_VITALITY` +1 |
| `POTION_STR` | potion of strength | POTION | ORANGE | 3-10 | 1 | use `EF_STRENGTH` +1 |
| `POTION_DEX` | potion of dexterity | POTION | LIME | 3-10 | 1 | use `EF_RAISE_DEX` +1 |
| `POTION_CON` | potion of vigour | POTION | FLESH | 3-10 | 1 | use `EF_RAISE_CON` +1 |
| `POTION_INT` | potion of intellect | POTION | ICE | 5-10 | 1 | use `EF_RAISE_INT` +1 |
| `POTION_WIS` | potion of wisdom | POTION | PURPLE | 5-10 | 1 | use `EF_RAISE_WIS` +1 |
| `POTION_CHA` | potion of charm | POTION | BONE | 5-10 | 1 | use `EF_RAISE_CHA` +1 |
| `POTION_HASTE` | potion of haste | POTION | ICE | 2-10 | 3 | use `EF_HASTE` +3 |
| `POTION_MIGHT` | potion of might | POTION | ORANGE | 2-10 | 3 | use `EF_MIGHT` +3 |
| `POTION_STONE` | potion of stoneskin | POTION | IRON | 4-10 | 2 | use `EF_STONE_SKIN` +3 |
| `POTION_REGEN` | regeneration potion | POTION | LIME | 3-10 | 2 | use `EF_RENEW` +1 |
| `POTION_FIRE` | potion of fire ward | POTION | ORANGE | 3-10 | 2 | use `EF_RESIST_FIRE` +1 |
| `POTION_COLD` | potion of cold ward | POTION | ICE | 3-10 | 2 | use `EF_RESIST_COLD` +1 |
| `POTION_POISON` | potion of poison | POTION | LIME | 1-7 | 3 | use `EF_POISON` +2 |
| `POTION_SLEEP` | potion of sleep | POTION | DARK | 2-10 | 2 | use `EF_SLEEP` +1 |
| `POTION_BLIND` | potion of blindness | POTION | DARK | 2-10 | 2 | use `EF_BLIND` +1 |
| `POTION_SLOW` | potion of slowness | POTION | BONE | 2-10 | 2 | use `EF_SLOW` +3 |
| `POTION_WEAK` | potion of weakness | POTION | BONE | 2-10 | 2 | use `EF_WEAKNESS` +2 |

#### Parchment (12)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `SP_LIGHT` | light parchment | PARCHMENT | BONE | 1-6 | 6 | use `EF_LIGHT` +1 |
| `SP_IDENTIFY` | identify parchment | PARCHMENT | BLUE | 1-10 | 7 | use `EF_IDENTIFY` +1 |
| `SP_MAP` | mapping parchment | PARCHMENT | EARTH | 1-10 | 5 | use `EF_MAP` +1 |
| `SP_TELEPORT` | teleport parchment | PARCHMENT | PURPLE | 2-10 | 4 | use `EF_TELEPORT` +1 |
| `SP_UNCURSE` | uncurse parchment | PARCHMENT | GOLD | 2-10 | 3 | use `EF_UNCURSE` +1 |
| `SP_ENCHANT` | enchant parchment | PARCHMENT | GREEN | 4-10 | 2 | use `EF_ENCHANT` +1 |
| `SP_SUMMON` | summon parchment | PARCHMENT | DARK | 2-10 | 2 | use `EF_SUMMON` +1 |
| `SP_MISSILE` | missile parchment | PARCHMENT | BONE | 1-6 | 5 | aim `EF_BOLT_MAGIC` +4 |
| `SP_FIRE` | fire parchment | PARCHMENT | ORANGE | 3-10 | 4 | aim `EF_BOLT_FIRE` +8 |
| `SP_FROST` | frost parchment | PARCHMENT | ICE | 3-10 | 4 | aim `EF_BOLT_COLD` +8 |
| `SP_ACID` | acid parchment | PARCHMENT | LIME | 4-10 | 3 | aim `EF_BOLT_ACID` +8 |
| `SP_SLEEP` | sleep parchment | PARCHMENT | PURPLE | 2-10 | 3 | aim `EF_SLEEP` +1 |

#### Wand (8)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `WAND_STRIKING` | wand of striking | WAND | BONE | 2-7 | 3 | aim `EF_BOLT_MAGIC` +4; count 6 |
| `WAND_SLOW` | wand of slowness | WAND | EARTH | 3-10 | 2 | aim `EF_SLOW` +3; count 5 |
| `WAND_SLEEP` | wand of sleep | WAND | PURPLE | 3-10 | 2 | aim `EF_SLEEP` +1; count 5 |
| `WAND_FIRE` | wand of fire | WAND | ORANGE | 4-10 | 2 | aim `EF_BOLT_FIRE` +6; count 5 |
| `WAND_FROST` | wand of frost | WAND | ICE | 4-10 | 2 | aim `EF_BOLT_COLD` +6; count 5 |
| `WAND_TELEPORT` | wand of teleport | WAND | GOLD | 5-10 | 1 | aim `EF_TELEPORT` +1; count 4 |
| `STAFF_POWER` | staff of power | STAFF | PURPLE | 7-10 | 1 | aim `EF_BOLT_MAGIC` +12; count 8 |
| `STAFF_HEALING` | staff of healing | STAFF | RED | 6-10 | 1 | aim `EF_HEALING` +8; count 6 |

#### Ring (18)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `RING_HEALTH_1` | ring of health +1 | RING | RED | 1-6 | 3 | fits RING; worn `EF_WORN_HP` +1 |
| `RING_HEALTH_2` | ring of health +2 | RING | RED | 3-10 | 2 | fits RING; worn `EF_WORN_HP` +2 |
| `RING_HEALTH_3` | ring of health +3 | RING | RED | 6-10 | 1 | fits RING; worn `EF_WORN_HP` +3 |
| `RING_ARMOUR_1` | ring of armour +1 | RING | IRON | 2-7 | 2 | fits RING; worn `EF_WORN_AC` +1 |
| `RING_ARMOUR_2` | ring of armour +2 | RING | IRON | 5-10 | 1 | fits RING; worn `EF_WORN_AC` +2 |
| `RING_STR_1` | ring of strength +1 | RING | ORANGE | 3-10 | 1 | fits RING; worn `EF_WORN_STR` +1 |
| `RING_DEX_1` | ring of agility +1 | RING | LIME | 3-10 | 1 | fits RING; worn `EF_WORN_DEX` +1 |
| `RING_CON_1` | ring of vigour +1 | RING | FLESH | 3-10 | 1 | fits RING; worn `EF_WORN_CON` +1 |
| `RING_INT_1` | ring of intellect | RING | BLUE | 4-10 | 1 | fits RING; worn `EF_WORN_INT` +1 |
| `RING_WIS_1` | ring of insight | RING | PURPLE | 4-10 | 1 | fits RING; worn `EF_WORN_WIS` +1 |
| `RING_SPEED_1` | ring of speed +1 | RING | ICE | 6-10 | 1 | fits RING; worn `EF_WORN_SPEED` +1 |
| `RING_FIRE` | ring of fire ward | RING | ORANGE | 3-10 | 1 | fits RING; worn `EF_WORN_RES_FIRE` +1 |
| `RING_COLD` | ring of frost ward | RING | ICE | 3-10 | 1 | fits RING; worn `EF_WORN_RES_COLD` +1 |
| `RING_VENOM` | ring of venom ward | RING | GREEN | 3-10 | 1 | fits RING; worn `EF_WORN_RES_POISON` +1 |
| `RING_RENEWAL` | ring of renewal | RING | GOLD | 5-10 | 1 | fits RING; worn `EF_WORN_REGEN` +1 |
| `RING_WEAKNESS` | ring of weakness | RING | DARK | 2-8 | 1 | fits RING; worn `EF_WORN_STR` -2; cursed |
| `RING_CLUMSY` | ring of clumsiness | RING | DARK | 2-8 | 1 | fits RING; worn `EF_WORN_DEX` -2; cursed |
| `RING_FRAILTY` | ring of frailty | RING | DARK | 2-8 | 1 | fits RING; worn `EF_WORN_HP` -3; cursed |

#### Amulet (5)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `AMULET_HEALTH` | amulet of health | AMULET | RED | 2-10 | 1 | fits NECK; worn `EF_WORN_HP` +3 |
| `AMULET_GUARDING` | amulet of guarding | AMULET | IRON | 3-10 | 1 | fits NECK; worn `EF_WORN_AC` +2 |
| `AMULET_ACID` | amulet of acid ward | AMULET | LIME | 4-10 | 1 | fits NECK; worn `EF_WORN_RES_ACID` +1 |
| `AMULET_WARDING` | amulet of warding | AMULET | PURPLE | 6-10 | 1 | fits NECK; worn `EF_WORN_RES_MAGIC` +1 |
| `AMULET_FOLLY` | amulet of folly | AMULET | DARK | 2-8 | 1 | fits NECK; worn `EF_WORN_WIS` -2; cursed |

#### Light (3)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `TORCH` | torch | TORCH | EARTH | 1-10 | 6 | fits LIGHT; worn `EF_WORN_LIGHT` +2; count 100 |
| `LANTERN` | oil lantern | LANTERN | EARTH | 2-10 | 3 | fits LIGHT; worn `EF_WORN_LIGHT` +3; count 200 |
| `MAGIC_LANTERN` | magic lantern | LANTERN | ICE | 6-10 | 1 | fits LIGHT; worn `EF_WORN_LIGHT` +4 |

#### Key (5)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `KEY_IRON` | iron key | KEY | IRON | 1-5 | 4 | group IRON |
| `KEY_BRASS` | brass key | KEY | ORANGE | 2-7 | 3 | group BRASS |
| `KEY_SILVER` | silver key | KEY | BONE | 4-9 | 2 | group SILVER |
| `KEY_GOLD` | gold key | KEY | GOLD | 6-10 | 2 | group GOLD |
| `KEY_SKELETON` | skeleton key | KEY | DARK | 7-10 | 1 | group ANY |

#### Tool (2)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `LOCK_PICK` | lock pick | LOCK_PICK | IRON | 1-8 | 4 | count 5; group +0 |
| `FINE_LOCK_PICK` | fine lock pick | LOCK_PICK | GOLD | 4-10 | 2 | count 8; group +3 |

#### Treasure (7)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `GOLD_SMALL` | handful of gold | COINS | GOLD | 1-4 | 16 | count 10 |
| `GOLD_PILE` | pile of gold | COINS | GOLD | 2-7 | 14 | count 30 |
| `GOLD_HEAP` | heap of gold | COINS | GOLD | 5-10 | 12 | count 80 |
| `GOLD_HOARD` | hoard of gold | COINS | GOLD | 8-10 | 6 | count 200 |
| `GEM_OPAL` | opal | GEM | BONE | 2-7 | 3 | count 60 |
| `GEM_RUBY` | ruby | GEM | RED | 4-10 | 3 | count 120 |
| `GEM_DIAMOND` | diamond | GEM | ICE | 7-10 | 2 | count 250 |

#### Quest (1)

| Kind | Name | Sprite | Bank | Depth | Rarity | Stats, hook, count |
|---|---|---|---|---|---|---|
| `QUEST` | crown | CROWN | PURPLE | 10-10 | 0 |  |

## 9. What each subclass does once built

| Subclass | Works | Data only for now |
|---|---|---|
| Melee weapons | wielded, attack, hit effects | two hands; rusty sword's curse |
| Armour, rings, amulets | AC, HP, abilities, speed, resistances | curses; renewal, light |
| Potions, food, drink | used, take effect | sleep and blindness grant a status no rule reads |
| Parchments, self | used up, with their message, but nothing happens | the 7 pending ops |
| Parchments, aimed; wands; staves | carried | aiming, charges |
| Ranged, ammo | equipped | firing, throwing, counts |
| Keys, lock picks | carried | locks |
| Light | equipped | radius, burning out |
| Treasure | carried | turning into gold |

## 10. Scope

**In:** R1-R23. Three commits, each building and passing its tests:

1. **Power:** `gen_hook_t`, `gen_hook_of()`, `effect_apply()`, `effect_power[]`,
   and the existing rows at amounts per power. No behaviour changes.
2. **The framework:** the kind record, `GEN_HOOK_AIM`, the slots, the count, the
   bands, `GEN_PICK()`, the screens and messages.
3. **The catalogue:** the effect rows, the kinds and the placeholder sprites,
   with docs/sprite_style.md 6.8, docs/memory_map.md, src/game/README.md and
   the header comments (attack.h, components.h, item.h).

**Out:** R24-R29.

## 11. Tests

PC tests. `test_gen`:

- For every kind: its name is at most 19 characters; a hook with an effect has a
  non-zero power and `ef < EF_COUNT`; a worn hook names a worn effect, and other
  hooks an effect that can be applied; rarity 0 only for the quest item; depth
  within 1 to `depth_max` and `depth_max` at most `LEVEL_DEPTH_BOTTOM`; `fits`
  within `SLOTS_HUMANOID`.
- By subclass: a wand or staff aims; ammo and launchers share a group; thrown ammo
  has dice and range 2 or more; a launcher has range 2 or more and fits
  `SLOT_RANGED`; a key has a key group.
- `create()`: a ranged kind gets `COMPONENT_RANGED` and no melee; a sword is as
  before; `item_count[]` follows `count`.
- `GEN_PICK()`: the depth tests moved from `gen_pick()`, plus: a kind past
  `depth_max` is never picked. For every depth, 1 to 10: a kind is found, and
  no subclass passes 25% of the weight.

`test_effects`:

- Instant: heal and damage scale. Worn: +2 and -3 change the attribute, and a
  mask ignores the power. Two worn items add up.
- Timed: power 2 poison ticks 2 for all its firings, after its source is gone;
  modifiers scale through a recompute; `on_end` uses the stored power.
- Refresh: a stronger power stays and a weaker one doesn't replace it; the timer
  restarts either way. Stacking instances keep their own.
- Clamps: amount x power past 127; negative base raises.
- Order: every timed effect is below `EF_TIMED_COUNT`, and none at or above it is.
- Pending ops run without aborting.

Also:

- `test_act`: use and hit pass the kind's power. A potion's amount still heals 6.
- `test_inventory`: the five new slots equip and list; a full set of 15 and a pack
  of 20 all show.
- `test_ui`: the verbs, and "an" before a vowel.
- All existing tests pass with the renames (R16).

CSpect checks, with the dev kit extended:

1. Healing draught and healing potion heal 3 and 6 with one effect.
2. Ring of health +2: max HP goes up by 2, and back down when taken off. Ring of
   frailty takes 3 off, and hit points stay within the maximum.
3. Plate armour +2 and an iron helmet: AC up by 11 on the PC screen.
4. Potion of haste: speed changes for 20 turns and turns are visibly faster, and
   the PC screen shows it. Drink a second: the time restarts.
5. Go down the stairs to depths 1, 5 and 10: new items appear, the deep ones
   are not starter gear, and placeholder sprites show the right banks.
6. Inventory: a sword, a bow, arrows, a cloak and leggings all equip, and their
   slot words show. A wand can't be used, and doesn't offer to.
7. "You read the", "You drink the", "You eat the", and "an oil lantern".
8. The build still reports 60 or more bytes free below the stack.

## 12. Risks

- **Common memory (R9).** 83 bytes is little: a change in a common file can
  stop the build. Commit 1 measures it first, and `gen_hook_of()` is designed
  to add nothing.
- **zsdcc.** `amount * power` multiplies two `int8_t`. Promote to `int16_t` before
  multiplying, read the struct fields into locals (T-ZSDCC-1), and read the asm
  of `effects_add()` and `run_op()`. PC tests miss these bugs.
- **`GEN_HOOK_POWER()` sign.** The high byte must go through `uint8_t` to
  `int8_t`, so -3 stays -3. Tested, and read in the asm.
- **Stack in `create()`.** It copies 12 bytes, less than today's 16.
- **Armour outruns monsters.** A full set of the best gear gives about +31 AC. On
  the d20 rule (roll + to-hit >= AC), monsters with to-hit 0 could then hit only on
  a 20. Monsters' to-hit must grow with depth, or AC will need a cap (R29).
- **Inert finds.** About 40% of a level's items do nothing yet (section 9).
  That's the price of generating them before their mechanics (Q3).
- **The treasure takes pack slots** until gold is picked up as gold.

## 13. Questions

All answered as proposed (2026-10-30). Q2 means sprite_style.md 6.8 becomes
items 40, monsters 50 (10 families).

The questions as asked:

- **Q1 (D2)** Power as a multiplier (proposed), or the kind's value replacing the
  definition's amount?
- **Q2 (6.5)** The 10 extra patterns for items: from monsters, 60 to 50, which is
  10 families (proposed); or from effects and features; or keep items at 30 and
  share more (armour pieces would go first)?
- **Q3 (section 9)** Generate the inert kinds at their real rarity (proposed, so the
  mix can be seen in play); or at rarity 0 until each mechanic is built, which
  plays better now but leaves section 8.2 untested?
- **Q4 (D7, D8)** One hook per item kind and no `health` part (proposed); or keep
  either, at 3 and 5 bytes a kind?
- **Q5 (R1)** Are "parchment" (`ITEM_PARCHMENT`, "identify parchment") and
  staves under `ITEM_WAND` the words you want?
- **Q6 (R29)** Are the catalogue's numbers right enough as a first pass (proposed),
  or do you want to set any depths and rarities before it is built?

## 14. As built: differences from the design

Built in the order of section 10. The build and the PC tests pass. The zsdcc
asm of `times()`, `scaled()`, the item pick, `gen_item_hook()` and the worn
hook's call to `effects_add()` was read (T-ZSDCC-3).

- **No multiply (6.3).** `amount * power` linked the library's `__mulint_callee`
  into common memory, 28 bytes, taking the free space from 83 to 54 (R9).
  `times()`, in page 120, adds in a loop instead. Powers are at most 127, and
  this runs only when an effect is applied, ticks or is recomputed.
- **No `strcmp()` for "a" and "an" (6.4).** It linked 23 bytes into common memory
  for the same reason. `set_article()` compares the characters by hand. It is
  called by `say_named()` and `say_did()`, so no message had to change, and it
  corrects any name after " a ", creatures too.
- **`GEN_HOOK_PACK()`** builds `gen_hook_of()`'s answer, with
  `GEN_HOOK_EF()` and `GEN_HOOK_POWER()` to read it.
- **`GEN_HOOK_COUNT` is 5,** so a creature or feature record's `hooks[]` grew from
  4 to 10 bytes, as D7 says.
- **The dev kit** holds 18 items to try (section 11), not one of each: the pack
  holds 20, and 2 places stay free for the quest item and a find.
- **Tests.**
  - `on_end` at a stored power is in the code but untested: no definition has an
    `on_end` op.
  - The new slots are tested in `test_act` (a real equip), not `test_inventory`.
  - "A full set of 15 and a pack of 20 all show" is a check of
    `SCREEN_INV_MAX`, not a filled screen.
  - `test_gen` now destroys each item in its all-kinds loops: there are more
    kinds than entities.
- **Known, not changed.** In the choosing lists, row letters run past "z" after
  26 rows (`'a' + i` in `draw_row()`). A full list can reach that, as it already
  could at 30 rows.

Memory, debug build, against the design (section 6.6):

| Where | Before | Built |
|---|---|---|
| Page 109 | 302 | 6,248, 76%, as computed |
| Page 121 | 486 | 2,465 |
| Page 120 | 4,816 | 5,166 |
| Page 108 | 886 | 1,088 |
| Slot 0 ends | `$19AF` | `$1A13` |
| Free below the stack | 83 | 73: the longer dev kit is the 9 bytes |
