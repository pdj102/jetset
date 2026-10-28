# Attributes, statuses and conditions

Status: agreed (2026-10-28). Q1-Q6 are answered (section 10), and the
decisions stand as proposed except Q5 (bits reserved now: R21). Nothing
is built yet.

This redesigns how attributes and statuses are stored and described, and
adds conditions. Parts A and B change no rules: the PC tests still pass,
and the game plays the same. Part C adds the saving throw. Each part is
its own commit:

- **Part A:** one set of terms for an attribute's values, with names and
  comments that follow them.
- **Part B:** each attribute's base moves into the component it belongs
  to, and the status mask moves into the stats component.
- **Part C:** conditions, the statuses that impair a creature, and saving
  throws that end them.

## 1. Goal

- Everything about an attribute lives with its component. All four of hit
  points' values (kind value, base, value, current) are in health.h.
- The word "base" means one thing only.
- One comment explains how attributes work, and the other headers point to
  it.
- The effects core stops reading and writing the game's tables, as its
  split already intends (effects_core.h: "could become an nge module").
- Creatures can have conditions that a saving throw, a cure or time ends,
  without the core knowing what any condition means.

## 2. How it works today

1. A kind's **base record** (`gen_creature_base_t` and its component parts
   `health_base_t`, `stats_base_t`) is constant data in a generator's page.
2. `health_add()` and `stats_add()` copy it twice: into
   `attr_base[ATTR_*][id]` (attrs_cdata.c, slot 0), the entity's permanent
   value, and into the component tables (`health_hp_max[]`, `health_ac[]`,
   the masks, `stats_speed[]`, `stats_ability[][]`), its derived values.
3. `effects_recompute()` rebuilds the derived values from scratch:
   `derive()` (effects_core.c) copies `attr_base` and adds the modifiers of
   worn items and active effects, and `effects_game_store()` clamps the
   results and writes them.
4. `effects_set_base()` and `OP_RAISE_BASE` change `attr_base` for good,
   then recompute.
5. Current hit points (`health_hp[]`) are not an attribute: damage and
   healing change them, and the recompute keeps them within the maximum.
6. `status_mask[]` (attrs.h) is the OR of the statuses the entity's
   effects grant.

What's wrong with it:

- **"Base" has two meanings.** A kind's constant *base record*
  (`health_base_t.hp`) and an entity's permanent *base value*
  (`attr_base`) are different things, and the comments use the same word
  for both.
- **`health_base_t.hp` and `mana_base_t.mp` are maximums,** but they are
  named like current amounts.
- **The bases are kept apart from their components.** health.h has an
  attribute's derived value and current amount, but not its base.
- **The core knows the game's storage.** `derive()`, `effects_set_base()`
  and `OP_RAISE_BASE` index `attr_base` directly. `effects_game_store()`
  already keeps the derived values on the game's side.
- **A silent hole.** `effects_set_base()` checks only
  `effects_game_has_attrs()` ("has health"). Raising `ATTR_STR` on an
  entity with health but no stats writes a base that is never used, and
  nothing complains.
- **Statuses aren't attributes.** Effects produce them, but they are
  declared in attrs.h.
- **The comments are hard to follow.** attrs.h explains it all in one
  dense paragraph, without the terms or how a value flows from one stage
  to the next. The table comments differ: only `health_hp_max` and the
  stats tables say they are derived.

## 3. Terms

| Term | Meaning | Hit points | AC |
|---|---|---|---|
| Attribute | A number or mask that equipment and effects can change; it has an id (`ATTR_*`) so effect data can name it | `ATTR_HP_MAX` | `ATTR_AC` |
| Kind record | A kind's constant record in its generator's page (today's "base record") | `gen_creature_kind_t` | |
| Kind value | An attribute's value in the kind record: it sets the base when the entity is created | `health_kind_t.hp_max` | `health_kind_t.ac` |
| Base | The entity's permanent value; only `effects_set_base()` changes it (a potion of vitality that permanently increases max HP) | `health_hp_max_base[]` | `health_ac_base[]` |
| Modifier | An amount added (or bits ORed) by a worn item's effect or an active effect, while it lasts | ring of health: +2 | |
| Value (derived) | Base + modifiers, clamped; the one rules read; only `effects_recompute()` writes it | `health_hp_max[]` | `health_ac[]` |
| Pool | An attribute that is a maximum for a resource: hit points, later MP | hit points | |
| Current | A pool's amount now; damage and healing change it; kept within the value; not an attribute | `health_hp[]` | |
| Status | A flag (a bit of the status mask) an active or worn effect grants while it lasts; not an attribute. The core knows it only as a bit | | |
| Condition | A status that impairs a creature: poisoned, later blinded and unconscious. It ends when its effect does: its time runs out, a saving throw is passed, or it is cured | | |
| Saving throw | A roll that ends a condition: 1d20 + an ability's modifier (`STATS_MOD()`) against a difficulty class | | |
| DC | Difficulty class: what a saving throw must reach | | |

## 4. Requirements

### Part A: terms, names, comments

- **R1** Code and docs use the terms in section 3. "Base" means only the
  entity's permanent value.
- **R2** Each attribute's tables follow one naming rule (D2): the plain
  name is the value rules read, the base has a `_base` suffix, and a
  pool's current amount is the pool's plain name.
- **R3** A kind record's field holding a pool's maximum is named for the
  maximum: `hp_max`, `mp_max`.
- **R4** attrs.h explains attributes once, using the terms (D5). The
  component headers and effects.h point to it instead of explaining it
  again.
- **R5** Every derived table says so in the same form:
  `/**< ... Derived (ATTR_AC). */`.

### Part B: ownership

- **R6** An attribute's base is a table of the component the attribute
  belongs to, next to its value.
- **R7** The attribute ids (`ATTR_*`) stay one numbered list, in attrs.h.
  Each id names its component and tables.
- **R8** The effects core reads and writes bases only through the seam. It
  includes nothing of the components, and `check-effects-core` keeps
  enforcing that.
- **R9 (New)** Setting or raising the base of an attribute whose component
  the entity lacks is asserted. *Reason: today it is silently written and
  never used (section 2).*
- **R10 (Changed)** The status mask and `STATUS_*` belong to the stats
  component (D9). The effects core treats statuses as bits, with no
  meanings. *Reason: conditions belong to creatures, and every creature
  has stats (user's proposal, Part C).*
- **R11** No behaviour changes in Parts A and B: the same values, events
  and clamping. The memory used in slot 0 stays the same.

### Out of scope (Parts A and B)

- **R12 (later)** Maximum MP as an attribute (`ATTR_MP_MAX`):
  docs/mana.md D2. When it comes, it follows this design: a
  `mana_mp_max_base[]` table and one row in the attribute table (D4).
- **R13 (later)** Making `effects_game_store()` table-driven too. It keeps
  its special cases (hit points kept within the maximum, the timer period,
  the ability clamp).

### Part C: conditions

- **R14** A condition is a kind of status: a status that impairs a
  creature. The game says which statuses are conditions
  (`STATUS_CONDITIONS`). Poisoned is the first.
- **R15** Only an entity with the stats component (a creature) has
  statuses. Effects still act on any entity with health (poison still
  damages a chest), but it gets no statuses.
- **R16** A condition lasts while an effect grants it, and ends when every
  effect granting it has ended, for one of these reasons:
  - its time runs out (a timed effect's last cycle);
  - a saving throw is passed (R17);
  - it is cured (`effect_cure()`, `OP_CURE`).
- **R17 (New)** An effect's definition may name a saving throw: an ability
  (`ABIL_*`) and a DC. The target rolls it each time the effect fires,
  after its tick op. On 1d20 + the ability's modifier >= DC, the effect
  ends. *Reason: the "saving throw is passed" ending in the request needs
  a roll and a time to make it.*
- **R18** The effects core knows nothing of what a status means, of
  abilities or of dice. It ORs the bits of the effects granting them,
  ends effects by the bits they grant (cure), and asks the game's side
  whether a saving throw is passed.
- **R19 (New)** `EV_EFFECT_ENDED` says why the effect ended: its time ran
  out, it was cured, or a saving throw was passed. *Reason: the messages
  can then say "You shake off the poison." rather than "The poison wears
  off." That text is out of scope (R23).*
- **R20** Poison gets a saving throw (Q4: which ability and DC;
  Constitution, DC 10 proposed).

### Out of scope (Part C)

- **R21 (later)** The rules blinded and unconscious change (sight and
  targeting; acting at all), each in its own step. Their bits,
  `STATUS_BLINDED` and `STATUS_UNCONSCIOUS`, are reserved now and are in
  `STATUS_CONDITIONS` (Q5). Nothing grants them yet, so no rule reads
  them, and the stats window doesn't show them yet.
- **R22 (later)** Other ways to end a condition or keep it off: a saving
  throw when it is applied (resisting it), being damaged (waking from
  unconsciousness), condition immunities (an undead can't be poisoned;
  this could be a mask attribute, `ATTR_COND_IMMUNE`).
- **R23 (later)** Different messages for each way an effect ends.
- **R24 (later)** More than 8 statuses: the mask and the definitions'
  `status` field become `uint16_t`.

## 5. Decisions

- **D1 Rename "base record" to "kind record" (R1).** The types become
  `*_kind_t`: `health_kind_t`, `stats_kind_t`, `mana_kind_t`,
  `attack_kind_t`, `gen_creature_kind_t`, `gen_item_kind_t`,
  `gen_feature_kind_t`. The macros become `HEALTH_KIND_HAS()` and
  `MANA_KIND_HAS()`. This touches about 25 files, mostly comments (the
  gen headers and data, the components, tests, docs/guidelines,
  docs/animations.md, docs/sprite_import.md, src/game/README.md). It is a
  mechanical rename that the build and tests check. **Option (Q1):** keep
  "base record" and its types, and define "base" in attrs.h as the
  entity's value. That's cheaper, but the two meanings stay side by side
  (`health_base_t.ac` sets `health_ac_base[]`).
  **Rejected:** "template" or "prototype": new words for something the
  code already calls a kind (`CR_*`, `kind.h`).

- **D2 Naming: the plain name is the value rules read (R2).** For example:
  `health_ac` (value), `health_ac_base` (base), `health_hp_max` (value),
  `health_hp_max_base` (base), `health_hp` (current). Rules read the
  values and current amounts most often, so those names stay short and
  don't change. A suffix keeps a component's tables for one attribute
  together (`health_hp*`). **Rejected:** prefixes (`base_hp_max`,
  `cur_hp`): they split a component's tables apart, and rename the names
  used most.

- **D3 Keep the word "derived"** for base + modifiers. The code already
  uses it (`effects_derived_t`, the comments). "Value" is short for it in
  prose. **Rejected:** "effective": a second word for the same thing.

- **D4 The core finds the bases through one table on the game's side
  (R6, R8, R9).** It replaces `effects_game_attr_is_mask[]`:

  ```c
  /* An attribute: its base table, the component that has it, and how modifiers combine */
  typedef struct effects_attr {
      uint8_t *base;        /* the component's base table: [NGE_ENTITY_MAX] */
      uint16_t component;   /* the entity has the attribute only with it */
      uint8_t  is_mask;     /* modifiers ORed (1) or added (0) */
  } effects_attr_t;

  extern const effects_attr_t effects_game_attrs[ATTR_COUNT];   /* page 121 */
  ```

  - `derive()` takes each base from `effects_game_attrs[a].base[id]` when
    the entity has its component, and 0 otherwise (`effects_game_store()`
    already ignores those).
  - `effects_set_base()` and `OP_RAISE_BASE` assert the component (R9),
    then write through the pointer.
  - The core still includes only attrs.h (`ATTR_*`, `ATTR_COUNT`), which
    has no tables any more.
  - Ability rows are `stats_ability_base[a]`, one pointer each.

  This is 12 rows of 5 bytes in page 121, about 60 bytes, against
  `effects_game_attr_is_mask[]`'s 12.
  **Rejected:** seam functions (`effects_game_base_get()`,
  `effects_game_base_set()`): two switches over every attribute, where
  the table is data and already sits next to the definitions.

- **D5 attrs.h becomes the attribute list and its explanation (R4,
  R7).** attrs_cdata.c is deleted. The header keeps its path: the
  attributes are the components', and the effects only change them. Its
  comment:

  ```c
  /*
   * An attribute is a number (or a mask) that equipment and effects can
   * change: max hit points, AC, speed, the abilities, the damage masks.
   * Each has, per entity:
   *
   *   kind value  constant, in its kind record (health_kind_t.hp_max):
   *               sets the base when the entity is created
   *   base        changed only for good, by effects_set_base() (a potion
   *               of vitality): health_hp_max_base
   *   value       base + the modifiers of equipped items' worn effects and
   *               of active effects, clamped: health_hp_max. The one rules
   *               read; only effects_recompute() writes it, and it is
   *               recomputed whenever a source changes (equip, an effect
   *               starts or ends, a base changes)
   *
   * A pool (hit points, later MP) also has a current amount (health_hp):
   * not an attribute, but a resource that damage and healing change,
   * kept within the value. A lower maximum takes it down; a higher one
   * doesn't heal.
   *
   * The tables live in the component each attribute belongs to; this
   * file only numbers them (ATTR_*), so effect definitions can name them.
   */
  ```

  Each id's comment names its component and tables:
  `#define ATTR_AC  1   /**< health: health_ac, health_ac_base; 0-255. */`.
  **Option (Q2):** move it to `game/effects/effects_attrs.h`, beside the
  module that uses the ids.

- **D6 (Replaced by D9.)** This first proposed moving the statuses to the
  effects module, in a new `effects_cdata.c`. Part C puts them in the
  stats component instead.

- **D7 The `_add()` functions set base and value from the kind value,**
  as they do today. No recompute is needed at creation: there are no
  modifiers yet.

- **D8 A condition is a status the game marks as one (R14).** The bits
  stay `STATUS_*`, and `STATUS_CONDITIONS` (stats.h) is the mask of those
  that are conditions: `STATUS_POISONED` today. A status that isn't a
  condition is a beneficial one (hasted, later). The difference matters
  only where a rule acts on conditions as a group: a potion that cures
  them all (`OP_CURE` with `STATUS_CONDITIONS`), condition immunities
  (R22), and the screen. **Rejected:** a separate condition mask beside
  the status mask: two masks, two fields in every definition, and two
  cures, for flags that work the same way.

- **D9 The status mask is in the stats component (R10, R15).** Today's
  `status_mask[]` becomes `stats_status[]` (stats_cdata.c, slot 0:
  common code reads it). `STATUS_*`, `STATUS_CONDITIONS` and
  `STATUS_HAS()` move to stats.h. `stats_add()` clears the mask, as
  `health_add()` does today. `effects_game_store()` writes the mask only
  with `COMPONENT_STATS`, as it already does for speed and the abilities.
  Every reader today is a creature (the player, the boss and goblins in
  the tests). **Rejected:** health (a door would have statuses); a new
  `effects_cdata.c` (D6): the mask is about the creature, not about the
  effects.

- **D10 Statuses stay derived; the core does no set or clear (R18).** A
  status is the OR of the statuses of every effect granting it (active
  effects, and worn effects of equipped items), and `effects_recompute()`
  works it out from scratch, like an attribute's value. This keeps the
  core generic: it handles bits it doesn't understand, and the game's
  side stores them. Ending a condition always means ending its effects
  (time, save, cure).
  **Rejected:** the core setting a bit when an effect starts and clearing
  it when it ends. That goes wrong when two sources grant the same bit:
  two poisonings (`EF_POISON` doesn't stack today, but a poisoned blade
  and a trap could be different effects), or a worn item and an effect.
  The first to end clears the bit while the other still grants it. A
  cure that clears the bit leaves its effect running and still doing
  damage. Avoiding all this would need a count per bit per entity.

- **D11 A saving throw is a field of the definition, rolled by the
  game's side (R17, R18).** `effect_def_t` gains
  `effect_save_t save = { arg, dc }`, where `dc` 0 means there is no
  saving throw. In `effects_on_timer()`, after the tick op, the core
  calls the seam `effects_game_save(target, arg, dc)`. If it returns 1,
  the core ends the effect (D12). The game's side rolls
  `1d20 + STATS_MOD(stats_ability[arg][target]) >= dc` with `rand()`, as
  attacks do. An entity without stats has no modifier.
  **Rejected:** a game op in `on_tick`: there is one tick op, and poison
  uses it for its damage. **Rejected:** the core rolling the d20:
  abilities and dice are the game's rules.

- **D12 `EV_EFFECT_ENDED`'s p2 is the reason (R19):** `EFFECT_END_EXPIRED`,
  `EFFECT_END_CURED`, `EFFECT_END_SAVED` (effects.h). p2 is 0 today, so
  `EFFECT_END_EXPIRED` is 0 and nothing that reads the event changes.
  `effect_end()` keeps its signature and means "expired". The core ends
  effects through an internal `end(effect, reason)`. *To check while
  building: whether anything outside the core calls `effect_end()` for
  another reason.*

## 6. Design

### 6.1 Part A: renames and comments

| Now | Becomes |
|---|---|
| `health_base_t`, `.hp` | `health_kind_t`, `.hp_max` |
| `mana_base_t`, `.mp` | `mana_kind_t`, `.mp_max` |
| `stats_base_t`, `attack_base_t` | `stats_kind_t`, `attack_kind_t` |
| `HEALTH_BASE_HAS()`, `MANA_BASE_HAS()` | `HEALTH_KIND_HAS()`, `MANA_KIND_HAS()` |
| `gen_{creature,item,feature}_base_t` | `gen_{creature,item,feature}_kind_t` |
| "base record" in comments and docs | "kind record" |
| the `b` parameters of the `_add()` functions | `k` |

Comments:

- attrs.h: D5's comment.
- health.h, stats.h, mana.h: the paragraph about attributes shrinks to one
  sentence pointing to attrs.h ("max hit points, AC and the masks are
  attributes (attrs.h)"), and the table comments follow R5. mana.h says
  that maximum MP is a pool's maximum but not yet an attribute
  (docs/mana.md D2).
- effects.h: "Derived values (...)" uses the terms and points to attrs.h.
- docs/guidelines/patterns.md, house.md, toolchain.md: the kind-record
  wording. docs/mana.md: its terms row, as a note that it was renamed.

`tile_base_t` (data/tile_data.h) is a different thing, a tileset's base
tiles, and keeps its name.

### 6.2 Part B: the tables

```c
/* health.h: tables in slot 0 (health_cdata.c), meaningful only with COMPONENT_HEALTH */
extern uint8_t health_hp[NGE_ENTITY_MAX];            /**< Current hit points: 0 is dead. */
extern uint8_t health_hp_max[NGE_ENTITY_MAX];        /**< Most hit points, 1 or more. Derived (ATTR_HP_MAX). */
extern uint8_t health_hp_max_base[NGE_ENTITY_MAX];   /**< Its base. */
extern uint8_t health_ac[NGE_ENTITY_MAX];            /**< Armour class. Derived (ATTR_AC). */
extern uint8_t health_ac_base[NGE_ENTITY_MAX];
/* ...immune, resist, vuln: each with its _base */

/* stats.h: tables in slot 0 (stats_cdata.c) */
extern uint8_t stats_speed[NGE_ENTITY_MAX];                       /**< Derived (ATTR_SPEED). */
extern uint8_t stats_speed_base[NGE_ENTITY_MAX];
extern uint8_t stats_ability[ABIL_COUNT][NGE_ENTITY_MAX];         /**< Derived (ATTR_ABILITY()). */
extern uint8_t stats_ability_base[ABIL_COUNT][NGE_ENTITY_MAX];
```

The bases are read-only outside their component and the effects module,
as `attr_base` is now.

### 6.3 Part B: the effects module

- effects_core.h: `effects_attr_t` and `effects_game_attrs[]` (D4) replace
  `effects_game_attr_is_mask[]`.
- effects_core.c:
  - `derive()` reads the bases through the table, 0 without the
    component.
  - `effects_add()` reads `is_mask` from the table.
  - `effects_set_base()` and `OP_RAISE_BASE` assert the component, then
    write through the table.
- effects_pdata.c: `effects_game_attrs[]`, one row per `ATTR_*`.
- stats.h, stats_cdata.c: `STATUS_*`, `STATUS_HAS()` and `stats_status[]`
  (D9). `effects_game_store()` writes the mask under `COMPONENT_STATS`.
- attrs_cdata.c: deleted. tests/Makefile's `check-effects-core` is
  unchanged: attrs.h is still allowed, and stats.h is still banned.
- The core's comments and `effect_def_t.status` say "statuses: bits the
  game defines", with no `STATUS_*` names.

### 6.4 Part C: conditions and saving throws

```c
/* stats.h */
#define STATUS_POISONED     0x01
#define STATUS_BLINDED      0x02   /**< Reserved: its rules come later (R21). */
#define STATUS_UNCONSCIOUS  0x04   /**< Reserved: its rules come later (R21). */

/** The statuses that are conditions (impairments). */
#define STATUS_CONDITIONS  (STATUS_POISONED | STATUS_BLINDED | STATUS_UNCONSCIOUS)

extern uint8_t stats_status[NGE_ENTITY_MAX];   /**< STATUS_* granted by its effects. Derived: read-only for game code. */

/** Creature id (with stats) has any of statuses s. */
#define STATUS_HAS(id, s)  ((stats_status[id] & (s)) != 0)

/* effects_core.h */
typedef struct effect_save {
    uint8_t arg;   /* what the game's side rolls with (an ABIL_*) */
    uint8_t dc;    /* what the roll must reach; 0 for no saving throw */
} effect_save_t;

/* in effect_def_t, after on_end */
    effect_save_t save;   /* timed: tried at each firing, after on_tick; passed ends it */

/* The seam: 1 if target passes saving throw arg against dc */
uint8_t effects_game_save(nge_entity_id_t target, uint8_t arg, uint8_t dc);

/* effects.h: EV_EFFECT_ENDED's p2 */
#define EFFECT_END_EXPIRED  0   /* its time ran out (or effect_end()) */
#define EFFECT_END_CURED    1   /* effect_cure() or OP_CURE */
#define EFFECT_END_SAVED    2   /* a saving throw was passed */
```

- `effects_on_timer()`: the tick op, then, if the effect is still alive,
  its target affectable and `save.dc != 0`, `effects_game_save()`. Passed
  ends it with `EFFECT_END_SAVED`, before the last-cycle check.
- `effect_cure()` ends with `EFFECT_END_CURED`.
- effects_game.c: `effects_game_save()` rolls
  `rand() % 20 + 1 + STATS_MOD(score) >= dc`, where `score` is
  `stats_ability[arg][target]` with `COMPONENT_STATS` and 10 (modifier 0)
  without. `arg < ABIL_COUNT` is asserted.
- effects_pdata.c: `EF_POISON` gets `.save = { ABIL_CON, 10 }` (Q4). An
  average creature (Con 10-11) then shakes it off on 55% of firings, so
  it usually lasts one or two of its four.
- Asserts: a definition with a saving throw is timed.

### 6.5 Memory

- Slot 0: no change. 12 × 100 bytes of bases and 100 bytes of status mask
  move between files.
- Page 121: about +50 bytes (D4), and 2 bytes per definition for the
  saving throw (+16).
- Page 120: the saving throw and the end reasons, about 0.1K; the asserts
  add a few bytes in debug builds.
- Page 110: no change (`stats_add()` clears one more table).

## 7. Scope

In: R1-R11 and R14-R20, the PC tests, and these docs: src/game/README.md,
docs/memory_map.md (the slot 0 tables), docs/guidelines (the
kind-record wording), docs/mana.md (terms).

Out: R12, R13, R21-R24.

## 8. Tests

PC tests: Part A changes names only. Part B:

- **`test_health`, `test_stats`, `test_effects`, `test_gen`:** the checks
  of `attr_base[ATTR_*][id]` read the components' base tables instead.
  The checks of `status_mask` read `stats_status`.
- **`test_effects`, new:**
  - Raising `ATTR_STR` on an entity with health but no stats is asserted
    (`TEST_EXPECT_HALT`, R9).
  - A recompute of an entity without stats leaves its abilities and
    speed alone.
  - Every row of `effects_game_attrs[]` has a base table and a component,
    and its `is_mask` matches today's (immune, resist and vuln only).
  - Poison on an entity with health but no stats: it is damaged, and
    there is no status to store.
- **All tests pass unchanged otherwise (R11).**

Part C:

- **`test_effects`:**
  - `effects_game_save()` with a seeded `rand()`: a roll that reaches the
    DC passes, and one below fails. The ability modifier counts (Con 1
    and Con 30), and an entity without stats rolls with modifier 0.
  - Poison with a saving throw that passes at the first firing: the tick
    damage is done, then the effect ends, the status clears, and
    `EV_EFFECT_ENDED` has p2 `EFFECT_END_SAVED`.
  - Failing every saving throw: it lasts its four firings and ends with
    `EFFECT_END_EXPIRED`.
  - A cure: `EFFECT_END_CURED`.
  - Two effects granting one status: ending one keeps the status (D10).
  - `dc` 0: no roll (`rand()` isn't called).
- **`test_act`, `test_ui`, `test_level`:** the poison tests that expect
  all four firings seed `rand()` so the saves fail, or check the status
  only after the first firing.

CSpect checks:

1. A new game: the stats window shows the same values as before.
2. Put on a ring of health: max HP goes up by 2. Take it off: it goes back
   down.
3. A potion of vitality raises max HP by 1.
4. Poison (a poisoned bite or the poison potion): "Poisoned" shows and HP
   drops at each firing. Over a few poisonings, some end early ("The
   poison wears off." for now, R23) and some last all four firings. A cure
   ends it at once.

## 9. Risks

- **The zsdcc miscompiles** (HL reuse when reading struct fields). D4's
  table is a struct read in a loop, in `derive()`. Read its asm once it
  builds; the fields can go into locals first, as `run_op()` does.
- **Rename merges.** Part A touches many files. Do it on a clean tree, in
  one commit, with no other changes.
- **Poison gets weaker (R20).** With a DC 10 save, the poisoned bite and
  trap do about half the damage they do now. Q4 can make the DC higher.
- **The same zsdcc risk for `effect_save_t`** read from a definition copy
  in `effects_on_timer()`: read the fields into locals.

## 10. Questions

Answered 2026-10-28:

- **Q1:** rename "base record" to "kind record" (D1 as proposed).
- **Q2:** keep attrs.h in `game/components/` (D5 as proposed).
- **Q3:** three commits: Parts A, B and C in turn.
- **Q4:** poison's saving throw is Constitution, DC 10 (R20).
- **Q5:** reserve `STATUS_BLINDED` and `STATUS_UNCONSCIOUS` now, with
  their rules later (R21, section 6.4).
- **Q6:** statuses stay derived (D10).

The questions as asked:

- **Q1 (D1)** Rename "base record" to "kind record" across the code
  (proposed), or keep "base record" and define the two meanings in
  attrs.h?
- **Q2 (D5)** Keep attrs.h in `game/components/` (proposed), or move it
  to `game/effects/`?
- **Q3** Three commits, Parts A, B and C in turn (proposed), or fewer?
- **Q4 (R20)** Poison's saving throw: Constitution, DC 10 (proposed),
  something harder (DC 12-13), or none for now (the mechanism without a
  user)?
- **Q5 (R21)** Add `STATUS_BLINDED` and `STATUS_UNCONSCIOUS` only with
  their rules, each in its own step (proposed), or reserve the bits now?
- **Q6 (D10)** Keep statuses derived (the OR of their effects, proposed),
  or have the core set and clear bits, with D10's problems?
