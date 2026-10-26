# The mana component: requirements and design

Status: implemented; the CSpect checks in section 7 passed (2026-10-26). D1 to D9 agreed as proposed, which answers Q2 to Q4; Q1 was left
open, and the player has 5 MP. Section 10 lists where the build differs
from the design.

Adds mana points (MP), a resource spent to cast spells, as a game
component modelled on the health component's hit points
([game/components/health.h](../src/game/components/health.h)). Spells, and
getting MP back over time, come later; this step gives them the resource,
gives it to the player, and shows it.

## 1. Goal

- An entity with the mana component has mana points: a current amount and
  a maximum.
- Code that casts a spell later can check, spend and restore MP through
  one small API, with the rules in one place (G-ARCH-1).
- The player has MP, and sees them in the stats window.

## 2. Terms

| Term | Meaning |
|---|---|
| MP | Mana points: the entity's current amount, 0 to its maximum |
| Maximum MP | The most MP the entity can have: from its kind's base record |
| Spend | Take MP away, for a spell: the caller has checked there are enough |
| Restore | Give MP back, up to the maximum |
| Base record | A kind's constant record in its generator's page (`gen_creature_base_t`, game/gen) |

## 3. Requirements

**Changed** or **New** marks a requirement that differs from the request,
with the reason.

### The component

- **R1** The mana component is a game component, `COMPONENT_MANA`, the next
  game component number (`NGE_COMPONENT_USER + 9`), following
  game/components/components.h.
- **R2** An entity with it has a current MP and a maximum MP, each 0-255.
- **R3** Its maximum MP comes from its kind's base record. A kind whose base
  has 0 MP doesn't get the component.
- **R4** An entity starts with its current MP at its maximum.
- **R5** Current MP never goes below 0 nor above the maximum.
- **R6** The API:
  - reading an entity's current and maximum MP;
  - testing whether an entity has the component, and whether it has at
    least n MP;
  - spending n MP;
  - restoring n MP, up to the maximum, returning how many were restored.
- **R7** Spending more MP than the entity has is the caller's mistake: it
  is asserted in debug builds; in release builds the MP stop at 0 (R5).
- **R8 (New)** Spending posts `EV_MANA_SPENT`, and restoring
  `EV_MANA_RESTORED` when any MP came back, with the amount, as damage and
  healing post `EV_DAMAGED` and `EV_HEALED`. *Reason: the stats window, and
  later the messages ("You feel your power return."), react to events
  (G-ARCH-3); a spell's code shouldn't have to redraw the screen.*

### The player

- **R9** The player's base record gives them MP (Q1: how many), so the
  player has the component from the start of every game.
- **R10** Monsters have none for now (their base MP is 0).

### The UI

- **R11** The stats window shows the player's MP as it shows hit points:
  `MP    5/  5`.
- **R12** The display follows every change of the player's MP (R8), and is
  drawn with the rest of the stats for a new game or a new level.

### Out of scope

- **R13 (later)** Spells and the actions that cast them, which check and
  spend MP.
- **R14 (later)** Getting MP back over time (an innate effect, as `EF_REGEN`
  gives hit points), and items or effects that restore MP (a potion) or
  change the maximum (a ring): see D2.
- **R15 (later)** Monsters with MP, and messages for spending and
  restoring.

## 4. Decisions

- **D1 The tables are in slot 0 (`mana_cdata.c`),** like health's: common
  code reads them (the player's event handler, `player_on_event()`) and
  later every spell action. Two tables of `NGE_ENTITY_MAX` (100) bytes:
  `mana_mp[]`, `mana_mp_max[]`. Slot 0 has about 2.5K free.
  **Rejected:** page data in a mana module's page: common code couldn't read
  it.

- **D2 Maximum MP is a plain value for now, not an attribute.** Hit points'
  maximum is an attribute (`ATTR_HP_MAX`, game/components/attrs.h), so
  equipment and effects can change it, and `effects_recompute()` keeps hit
  points within it. Nothing changes maximum MP yet (R14), so it is set once
  by `mana_add()`. When the first item or effect needs to change it, it
  becomes `ATTR_MP_MAX`, with the same rule as hit points: a lower maximum
  takes current MP down with it, a higher one gives nothing back. **Option
  (Q2):** make it an attribute now: about 100 more bytes in slot 0, a few
  lines in `effects_game_store()`, and its tests, for no use yet.

- **D3 Read the tables; no getter functions (R6).** As with `health_hp[]`,
  the tables are read-only outside the component, and read directly:
  `mana_mp[id]`, `mana_mp_max[id]`. Two macros make callers read clearly:
  `MANA_IS(id)` (the component; components.h's model) and
  `MANA_HAS(id, n)` (at least n MP). **Rejected:** `mana_get()`: a banked
  call (page 110) to read one byte.

- **D4 `mana_spend()` asserts, then clamps (R7).** The caller checks first
  (`MANA_HAS()`); spending more is a bug, caught by the assert in debug
  builds. In a release build, where asserts are compiled out, the MP stop at
  0 rather than wrapping to 255. It returns nothing: what was asked for is
  what was spent. **Rejected:** returning 0 and spending nothing when there
  aren't enough: a check the caller has already made, hidden in the spend.

- **D5 Two events, as health has (R8).** `EV_MANA_SPENT` (p1: the MP spent)
  and `EV_MANA_RESTORED` (p1: the MP restored), the target the entity, no
  source. Nothing is posted for 0. **Rejected:** one `EV_MANA_CHANGED`:
  enough for the display, but the messages later tell spending from
  restoring.

- **D6 The base is its own record, `mana_base_t`,** a field `.mana` of the
  creature base (`gen_creature_base_t`), and `MANA_BASE_HAS(b)` says
  whether it gives the component (`mp != 0`), as `HEALTH_BASE_HAS()` does.
  The creature generator calls `mana_add()` when it does. **Rejected:** an
  `mp` field in `health_base_t`: two components in one record.

- **D7 MP goes under the hit points (R11),** and the armour class moves
  down a row, into the row that is empty now:

  ```
  row 2   HP   12/ 12
  row 3   MP    5/  5      new
  row 4   AC       10      was row 3
  row 5   Str      16      as now
  ```

  **Rejected:** MP on row 4, under AC: no row moves, but the two resources
  are apart.

- **D8 A player without mana shows no MP row.** The stats window draws the
  MP row only if the player has the component, so the code is right when a
  character class without magic comes (character generation, later). Today
  the player always has it (R9).

- **D9 A development key to check the display (CSpect).** Nothing spends MP
  in the game yet, so the display can't be seen to change. Under
  `GAME_DEV_KIT` (G-DEV-1), `M` spends 1 MP, or restores them all when there
  are none. **Option (Q3):** no key; the PC tests cover it, and CSpect shows
  only the starting value.

## 5. Design

### 5.1 The component (`game/components/mana.{h,c}`, `mana_cdata.c`)

```c
/** Base mana: the mana part of a kind's base record. */
typedef struct mana_base {
    uint8_t mp;   /**< Maximum MP, the same for every entity of the kind; 0 for none. */
} mana_base_t;

#define MANA_BASE_HAS(b)  ((b)->mp != 0)

/* Tables in slot 0 (mana_cdata.c): read-only for game code; meaningful only with COMPONENT_MANA */
extern uint8_t mana_mp[NGE_ENTITY_MAX];       /**< MP now, 0 to mana_mp_max. */
extern uint8_t mana_mp_max[NGE_ENTITY_MAX];   /**< The most MP it can have. */

#define MANA_IS(id)      NGE_ENTITY_HAS_ANY_COMPONENTS((id), COMPONENT_MANA)
#define MANA_HAS(id, n)  (mana_mp[id] >= (n))   /* with the component */

void    mana_add(nge_entity_id_t id, const mana_base_t *b) NGE_BANKED;   /* full: mp = mp_max = b->mp */
void    mana_spend(nge_entity_id_t id, uint8_t amount) NGE_BANKED;      /* asserts MANA_HAS(); EV_MANA_SPENT */
uint8_t mana_restore(nge_entity_id_t id, uint8_t amount) NGE_BANKED;    /* up to mp_max; EV_MANA_RESTORED; how many */
```

- Banked in page 110, with the other component functions; they assert
  their entity is in use and has the component, as `check_health()` does.
- `COMPONENT_MANA` is `NGE_COMPONENT_USER + 9`; `COMPONENT_COUNT` becomes
  10 (of 12 the game can have).

### 5.2 Events (`game/events.h`)

`EV_MANA_SPENT` and `EV_MANA_RESTORED`, the next two numbers, with rows in
the table at the top of the header.

### 5.3 The generator (`game/gen`)

- `gen_creature_base_t` gains `mana_base_t mana`.
- `create()` (gen_creature_bank.c) calls `mana_add(id, &b.mana)` when
  `MANA_BASE_HAS(&b.mana)`, after `health_add()`.
- `gen_creatures[CR_PLAYER]` gets `.mana = { .mp = 5 }` (Q1); the monsters
  leave it 0.

### 5.4 The UI

- `screen_main.h`: `screen_show_mp(uint8_t mp, uint8_t mp_max)
  NGE_BANKED`, drawing row 3 as `screen_show_hp()` draws row 2;
  `STATS_ROW_AC` becomes 4 (`screen_main_bank.c`).
- `player_show_stats()` (player_bank.c) shows MP when the player has the
  component (D8).
- `player_on_event()` (player.c, common): `EV_MANA_SPENT` and
  `EV_MANA_RESTORED` for the player redraw the MP row, as `EV_DAMAGED` and
  `EV_HEALED` redraw HP.

### 5.5 Memory

- Slot 0: +200 bytes (two tables).
- Page 110: the functions, about 0.2K with asserts.
- Common: two cases in `player_on_event()`, a few bytes.
- Page 116: `screen_show_mp()`, about 50 bytes.

## 6. Scope

In: R1-R12, D9's key if agreed, PC tests, and docs: `src/game/README.md`
(components row, `dev.h` row), `docs/memory_map.md` (slot 0 tables),
`game/dev.h` (the key).

Out: R13-R15.

## 7. Tests

PC tests:

- **`test_mana`** (new, linked like `test_health`):
  - `mana_add()`: full, at the base's maximum; the component set.
  - Spending: MP down, `EV_MANA_SPENT` with the amount; spending all of
    them leaves 0; spending 0 posts nothing.
  - Spending too much: the assert halts (`TEST_EXPECT_HALT`).
  - Restoring: up to the maximum, the amount returned and posted; at the
    maximum, 0 returned and nothing posted.
  - `MANA_HAS()` at, below and above the current MP.
  - Asserts on an entity without the component.
- **`test_gen`:** the player has mana at 5 of 5; a rat and a goblin don't.
- **`test_ui`:** the stats rows: HP row 2, MP row 3, AC row 4; an
  `EV_MANA_SPENT` for the player redraws the MP row.

CSpect checks:

1. A new game: the stats window shows `HP   12/ 12`, `MP    5/  5`,
   `AC       10`, then the abilities as before.
2. With D9's key: `M` takes MP down one at a time to 0, then back to the
   maximum; the row follows each press.
3. A new level, and a new game after game over: MP shown full.

## 8. Risks

None known: the component copies health's pattern, and the display a row
of it.

## 9. Questions

Q2 to Q4 are answered by D2, D9 and D7 (agreed 2026-10-26): a plain
maximum, the `M` key, MP under HP with AC moved down.

- **Q1 (R9)** How many MP should the player start with? 5 is proposed (a
  few spells' worth at 1-3 MP each).
- **Q2 (D2)** Maximum MP as a plain value now (proposed), or an attribute
  now, ready for items that change it?
- **Q3 (D9)** A development key `M` to spend and restore MP for the CSpect
  check (proposed), or none?
- **Q4 (D7)** MP under HP with AC moved down a row (proposed), or MP on the
  empty row 4 with nothing moved?

## 10. As built: differences from the design

The design was built as written. Details it left open:

- **`mana_spend()` asserts before it changes anything,** so in debug builds
  an overspend halts with the MP as they were; the clamp at 0 only matters
  when asserts are compiled out, and the PC tests can't reach it.
- **The `M` key does nothing for a player without mana** (D8's later
  classes), rather than asserting.
- **The effects core's include ban** (`check-effects-core` in
  `tests/Makefile`) now lists `game/components/mana.h` with the other
  components.
- **Memory:** slot 0 +200 bytes (the ECS data now ends at `$16E5`); common
  memory free in debug builds 412 bytes (448 before), in release builds
  7,194; pages 110 and 116 a few hundred bytes each.

