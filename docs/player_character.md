# The player character component: requirements and design

Status: implemented; the PC tests and the CSpect checks in section 7
passed (2026-10-28). D1 to D11 agreed as proposed, which answers Q4 to Q8. Q1:
a placeholder for the class (R5, D12). Q2: XP only for the player's own
kills (R7). Q3: XP for gold, now (R12a, D13). Section 10 lists where the
build differs from the design.

Adds the player character component (PC): the data and rules that only the
player has (their name, level, experience points and gold) as one game
component. Today the name is a UI variable (`screen_name`,
game/ui/screen_main.h), the gold a placeholder in the stats window, and
there are no levels or experience points.

## 1. Goal

- The data only the player has is in one component, with its rules, and
  nothing else is in it: what monsters, items and features act on stays in
  the shared components (stats, health, mana, inventory, attack).
- The player gains experience points (XP) by killing monsters, and gains
  levels from them, as in classic D&D: each level gives more hit points.
- The player has gold, which later code (gold on the floor, shops) adds
  and spends through one small API.
- The stats window shows the level, the XP and the gold.

## 2. Terms

| Term | Meaning |
|---|---|
| PC | Player character: the component, and its prefix in code (`pc_`). The player entity is still `actor_player` (game/actors.h) |
| Level | The character's experience level, 1 to `PC_LEVEL_MAX`. Not a dungeon level, which this spec calls the depth |
| XP | Experience points: gained by killing monsters; enough of them give the next level |
| XP value | What a monster kind is worth when the player kills it: in its kind record |
| Level table | The XP needed for each level |
| Hit die | The die rolled for the hit points a level gives (classic D&D: d8 for a fighter) |
| Shared component | A component any entity can have (stats, health, mana, inventory, attack) |

## 3. Requirements

**New** marks a requirement that goes beyond the request, with the reason.

### What goes in the component

- **R1** A value goes in the PC component only if no other entity could
  ever have it. A value that a monster, item or feature could have, or that
  a rule shared with monsters reads, goes in a shared component, even when
  only the player has it today (MP: the mana component).
- **R2 (New)** A rule that acts on the player from outside (a monster's
  attack that drains a level, later) may change PC data: the test in R1 is
  whether others could *have* the value, not who changes it. *Reason: the
  request's rule ("data needed by interactions with monsters goes in shared
  components") would put the level in a shared component because of level
  drain, though no monster has a level.*

### The component

- **R3** The PC component is a game component, `COMPONENT_PC`, the next
  number (`NGE_COMPONENT_USER + 10`), following
  game/components/components.h.
- **R4** Only the player has it: given when the player is created for a new
  game (`actors_create()`), never removed. A new game sets every value
  afresh.
- **R5** It holds:

  | Field | Range | Starts at |
  |---|---|---|
  | Name | 1 to 6 characters | what the title asks for; "Rogue" if none |
  | Level | 1 to `PC_LEVEL_MAX` | 1 |
  | XP | 0 to 65,535 | 0 |
  | Gold | 0 to 65,535 | 0 |
  | Class (placeholder) | `PC_CLASS_*`: only `PC_CLASS_FIGHTER` for now | `PC_CLASS_FIGHTER` |
  | Deepest depth reached **(New)** | 1 to `LEVEL_DEPTH_BOTTOM` | 1 |

  *Reason for the depth: the game-over screen and a score show how deep the
  player got; only the player has it.* The class is a placeholder (Q1, D12):
  stored, but no rule reads it until character generation (R16).
- **R6** The API:
  - reading every field;
  - testing whether an entity is the player (`PC_IS(id)`);
  - gaining n XP;
  - gaining and spending n gold, and testing whether the player has n.

### Experience and levels

- **R7** When a monster dies and the player killed it (`EV_DIED` with the
  player as its source), the player gains its kind's XP value. A kill by
  anything else (a trap, a monster, poison) gives nothing (Q2: agreed).
- **R8** Each creature kind has an XP value in its kind record; the
  player's is 0.
- **R9** When the XP reach the level table's figure for the next level, the
  player goes up a level; one gain can give several levels. At
  `PC_LEVEL_MAX` the XP still go up, but the level stops.
- **R10** A new level raises maximum hit points by one roll of the hit die
  plus the Constitution modifier, at least 1, and current hit points by the
  same amount.
- **R11** XP stop at 65,535 rather than wrapping.

### Gold

- **R12** Gaining gold stops at 65,535. Spending more than the player has
  is the caller's mistake: asserted in debug builds; in release builds the
  gold stops at 0 (as `mana_spend()`).
- **R12a** XP for gold, as classic D&D (Q3): gaining n gold also gains n XP,
  with R9-R11 (new levels). Spending gold takes no XP away.

### Events

- **R13 (New)** Gaining XP posts `EV_XP_GAINED`; each new level posts
  `EV_LEVEL_UP`; a change of gold posts `EV_GOLD_CHANGED`, before the XP
  it gives. *Reason: the
  stats window and the messages ("Welcome to level 2.") react to events, as
  they do to damage and MP (G-ARCH-3).*

### The UI

- **R14** The stats window shows the name (as now), the level, the XP and
  the gold, and follows every change (R13).
- **R15** The name the title asks for is stored in the component;
  `screen_name` goes.

### Out of scope

Later, each in its own spec:

- **R16 (later)** Character classes (fighter, cleric, magic-user, thief)
  and perhaps race, from character generation: the hit die, the level
  table, and later spells and what can be worn depend on the class. Only
  the player has a class (monsters have hit dice in their kind records), so
  it is in the PC component (R1): a placeholder now (R5, D12).
- **R17 (later)** To-hit and saving throws that improve with level. Monsters
  have them too, so they are attributes in shared components (attack,
  stats) whose bases a new level raises; the PC component keeps only the
  level.
- **R18 (later)** MP gained by level, for a spellcaster: maximum MP becomes
  an attribute first (docs/mana.md, D2).
- **R19 (later)** Level drain (an undead's touch): takes a level, and the XP
  down to that level's figure.
- **R20 (later)** Gold on the floor and in monsters' packs (a gold item,
  picked up into the PC's gold, giving its XP through R12a), how much gold
  each depth holds, and shops (gold from a sale gives no XP: D13).
- **R21 (later)** Hunger, if the game has it: only the player eats, so it
  goes in the PC component.
- **R22 (later)** A score and the game-over details: what killed the
  player, how many monsters they killed.

Not in the PC component, because other entities have them or could (R1):
alignment (detect evil, protection from evil act on monsters too), the
abilities, hit points, AC, MP, speed, statuses, the inventory and
equipment, light and vision, spells known if monsters cast spells.

## 4. Decisions

- **D1 One set of variables, not tables (R4).** Every other component has a
  table per field, `[NGE_ENTITY_MAX]` (100) entries. Only entity 0 has
  this one, so each field is one variable: `pc_level`, not
  `pc_level[id]`. The fields come to 14 bytes, against about 1.3K as
  tables. components.h's description of a component gains this variant (a
  *single* component). **Rejected:** tables, for consistency: 99 rows never
  used, in slot 0's 2.3K free.

- **D2 The variables are in slot 0 (`pc_cdata.c`),** with the other
  components': common code reads them (`player_on_event()`), and slot 0 is
  the ECS state a saved game will keep (docs/memory_map.md).
  **Rejected:** page data in page 115 (the player module's unused odd
  page): common code couldn't read it, and it wouldn't be saved with the
  rest.

- **D3 It takes a component number (R3):** the 15th of 16 component bits,
  leaving one for the game. `PC_IS(id)` tests the bit, one mask test, and
  replaces `KIND_IS_PLAYER()` (game/components/kind.h), which tests the
  kind component and the kind index. When the bits run out, the mask
  becomes 32 bits (`nge_component_mask_t`, ecs/nge_entity.h). *(Since
  replaced by three byte tables, 24 components:
  docs/component_tables.md.)* **Option (Q4):** a game entity flag instead (`NGE_ENTITY_FLAG_USER`: 5 free), or no
  marker at all, `KIND_IS_PLAYER()` as now; no component bit used.

- **D4 It is called `pc`: `COMPONENT_PC`, game/components/pc.{h,c},
  `pc_xp`.** **Rejected:** `player`: game/player is the module of the
  player's commands and turn, with `player_turn` and `player_*()`
  functions; the names would clash. `character`: long, and `char_` reads as
  the C type. The docs already say "PC tests" for the tests on the PC; the
  spec and comments say "the PC component" where it could be unclear.

- **D5 XP and gold are 16 bits (R5).** Classic D&D's level tables (B/X: a
  fighter needs 240,000 XP for level 9) don't fit, but the dungeon is 10
  depths deep, and wouldn't give that many. The game has its own level
  table (D6). Both stop at 65,535 (R11, R12). **Option (Q5):** 32 bits, for
  classic tables: 4 more bytes, slower sums (rare: only on a kill or a
  pickup), 6-digit display.

- **D6 Classic monster XP values, a short level table (R8, R9).** Monsters
  are worth what B/X gives (by hit dice: below 1 HD 5, 1 HD 10, 2 HD 20,
  3 HD 35, 4 HD 75), so a rat and a goblin are worth 5. The level table is
  the game's, classic in shape (doubling, then slower), scaled so that a
  player who clears most of each depth is about level 10 at the bottom:

  | Level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
  |---|---|---|---|---|---|---|---|---|---|---|
  | XP | 0 | 20 | 40 | 80 | 160 | 320 | 640 | 1,200 | 2,000 | 3,000 |

  `PC_LEVEL_MAX` is 10. The figures are a first guess to tune in play (Q6).
  With XP for gold (D13), the pace also depends on how much gold each depth
  holds: the table and the gold are tuned together when gold items come
  (R20).

- **D7 The game's event handler gives XP for kills (R7).** It already
  reacts to `EV_DIED` for the player (game/game.c); for a creature with the
  player as source, it reads the kind's XP value (`gen_creature_xp()`,
  banked, as `gen_name()` reads the name) and calls `pc_gain_xp()`. The
  health component doesn't learn about XP.

- **D8 A new level goes through the effects (R10).** `pc_gain_xp()` raises
  the maximum hit points with `effects_set_base(ATTR_HP_MAX)` (the one way
  to change a base, game/components/attrs.h), stopping at 255, then
  `health_heal()` gives the same amount, which posts `EV_HEALED`. The hit
  die is d8 (`PC_HIT_DIE`) until classes come (R16); the Constitution
  modifier is `STATS_MOD()` of the player's Constitution as it is then,
  with any ring or effect.

- **D9 Three events (R13), the amounts in p1 and p2.** An event's p1 and p2
  are a byte each, and an XP gain can be more than 255 later, so
  `EV_XP_GAINED` carries it as p1 (low byte) and p2 (high byte).
  `EV_LEVEL_UP` has p1 the new level, one event per level gained.
  `EV_GOLD_CHANGED` carries nothing: the stats window reads `pc_gold`, and
  the action that gained it (picking gold up, later) says how much.
  **Rejected:** `EV_GOLD_GAINED` and `EV_GOLD_SPENT`, as mana has: nothing
  tells them apart yet.

- **D10 The level goes under the name, the XP above the gold (R14),** and
  the rows below move down one:

  ```
  row 0   Name: Rogue
  row 1   Level      1      new, in the empty row
  row 2   HP   12/ 12
  row 3   MP    5/  5
  row 4   AC       10
  rows 5-10  Str ... Cha
  row 12  XP         0      new
  row 13  Gold       0      was row 12
  row 14  Depth      1      was row 13
  row 15  Turn       0      was row 14
  row 17  Poisoned          was row 16
  ```

  **Option (Q7):** the XP on the empty row 11, under Cha, and nothing
  moves.

- **D11 A development key to check the display (CSpect).** Nothing gives
  gold yet, and levels need many kills. Under `GAME_DEV_KIT` (G-DEV-1),
  `E` gives the XP for the next level and 10 gold. **Option (Q8):** no key;
  the PC tests cover it, and CSpect shows XP from kills only. With XP for
  gold (D13) the 10 gold give 10 XP too: `E` gives the XP for the next
  level less 10, then the gold, so each press is still one level.

- **D12 The class is a placeholder (R5, Q1).** `pc_class`, one byte, with
  `PC_CLASS_FIGHTER` 0 the only class (`PC_CLASS_COUNT` 1), set by
  `pc_add()`. Nothing reads it yet: the hit die (`PC_HIT_DIE`) and the level
  table stay single until character generation makes them per class
  (R16), and the stats window doesn't show it while there is one class.
  **Rejected:** per-class tables now, with one row each: code for choices
  that don't exist yet.

- **D13 XP for gold on gaining it, in `pc_gain_gold()` (R12a, Q3).** Classic
  D&D gives the XP when treasure is brought out of the dungeon; the game
  has no town to bring it to, so the XP come when the gold is gained.
  `pc_gain_gold()` adds the gold (`EV_GOLD_CHANGED`), then calls
  `pc_gain_xp()` with the amount, so a pickup that crosses a figure gives
  a level. Today every source of gold is treasure; when shops come (R20), a
  sale adds gold through a second function that gives no XP.
  **Rejected:** the XP given by the pickup action: every later source of
  treasure (a chest, a quest reward) would have to remember it.

## 5. Design

### 5.1 The component (`game/components/pc.{h,c}`, `pc_cdata.c`)

```c
#define PC_NAME_MAX   6    /**< Longest name: it follows "Name: " on one stats line. */
#define PC_LEVEL_MAX  10
#define PC_HIT_DIE    8    /**< The die rolled for each level's hit points (R16: by class, later). */

/* Classes: values of pc_class (a placeholder: D12) */
#define PC_CLASS_FIGHTER  0
#define PC_CLASS_COUNT    1

/* Variables in slot 0 (pc_cdata.c): read-only outside the component; meaningful while the player exists */
extern char     pc_name[PC_NAME_MAX + 1];
extern uint8_t  pc_level;       /**< 1 to PC_LEVEL_MAX. */
extern uint16_t pc_xp;
extern uint16_t pc_gold;
extern uint8_t  pc_class;       /**< PC_CLASS_*: not read yet (D12). */
extern uint8_t  pc_depth_max;   /**< The deepest depth reached. */

#define PC_IS(id)        NGE_ENTITY_HAS_ANY_COMPONENTS((id), COMPONENT_PC)
#define PC_HAS_GOLD(n)   (pc_gold >= (n))

void pc_add(nge_entity_id_t id, const char *name) NGE_BANKED;   /* level 1, fighter, no XP or gold, depth 1 */
void pc_gain_xp(uint16_t amount) NGE_BANKED;                    /* EV_XP_GAINED, then EV_LEVEL_UP per level */
void pc_gain_gold(uint16_t amount) NGE_BANKED;                  /* stops at 65,535; EV_GOLD_CHANGED; then pc_gain_xp(amount) */
void pc_spend_gold(uint16_t amount) NGE_BANKED;                 /* asserts PC_HAS_GOLD(); EV_GOLD_CHANGED */
void pc_reach_depth(uint8_t depth) NGE_BANKED;                  /* pc_depth_max = the larger */
```

- Banked in page 110, with the other component functions. The functions
  take no entity: there is one PC. They assert that `actor_player` has the
  component.
- The level table is a `const uint16_t` table in page 110.
- `COMPONENT_PC` is `NGE_COMPONENT_USER + 10`; `COMPONENT_COUNT` becomes 11.
- `KIND_IS_PLAYER()` callers (the health component, the messages) use
  `PC_IS()`; `KIND_IS_PLAYER()` goes.

### 5.2 Events (`game/events.h`)

`EV_XP_GAINED`, `EV_LEVEL_UP` and `EV_GOLD_CHANGED`, the next three
numbers, with rows in the table at the top of the header: the target the
player, no source.

### 5.3 The generator (`game/gen`)

- `gen_creature_kind_t` gains `uint16_t xp`: the rat and the goblin 5, the
  player 0.
- `gen_creature_xp(uint8_t kind) NGE_BANKED`: reads it from the creature
  kinds' page.

### 5.4 The game

- `actors_create()` calls `pc_add(actor_player, name)` with the name the
  title asked for.
- The level module calls `pc_reach_depth()` when the player arrives on a
  level (on `EV_ARRIVED`).
- `game.c`'s event handler: `EV_DIED` for a creature that isn't the
  player, with the player as source: `pc_gain_xp(gen_creature_xp(...))`
  (D7).

### 5.5 The UI

- The title types the name into `pc_name` (through a stack buffer, as now);
  `screen_name` and `SCREEN_NAME_MAX` go.
- `screen_main.h`: `screen_show_level(uint8_t)`, `screen_show_xp(uint16_t)`,
  `screen_show_gold(uint16_t)`; the rows of D10. `screen_draw_stats()`
  draws the name from `pc_name`, and no longer the gold placeholder.
- `player_show_stats()` shows the level, the XP and the gold.
  `player_on_event()`: `EV_XP_GAINED` redraws the XP, `EV_LEVEL_UP` the
  level, `EV_GOLD_CHANGED` the gold.
- The messages: `EV_LEVEL_UP` says "Welcome to level 2."

### 5.6 Memory

- Slot 0: +14 bytes.
- Page 110: the functions and the level table, about 0.3K with asserts.
- Common: three cases in `player_on_event()`, one in `game.c`'s handler.
- Page 116: three `screen_show_*()`, about 150 bytes.
- The creature kinds' page: 2 bytes per kind.

## 6. Scope

In: R1-R15, D11's key if agreed, PC tests, and docs: `src/game/README.md`
(components row, `dev.h` row), `docs/memory_map.md` (slot 0),
components.h (the single component variant, D1), `game/dev.h` (the key).

Out: R16-R22.

## 7. Tests

PC tests:

- **`test_pc`** (new, linked like `test_mana`):
  - `pc_add()`: the component set; level 1, a fighter, no XP or gold,
    depth 1, the name copied (6 characters at most).
  - XP: a gain below the next level posts `EV_XP_GAINED` (amount in p1 and
    p2) and no `EV_LEVEL_UP`; a gain to exactly the figure gives the level;
    one gain across three figures gives three levels and three events.
  - At `PC_LEVEL_MAX`: XP go up, the level doesn't. At 65,535 the XP stop.
  - A new level: maximum and current hit points up by 1 to 8 plus the
    Constitution modifier, at least 1 (Constitution 3: still 1); the
    maximum stops at 255.
  - Gold: gains add and stop at 65,535; spending takes it down; spending
    too much halts (`TEST_EXPECT_HALT`); each posts `EV_GOLD_CHANGED`.
  - XP for gold: gaining 15 gold gives 15 XP, `EV_GOLD_CHANGED` before
    `EV_XP_GAINED`; gold that crosses a figure gives the level; spending
    gold leaves the XP as they were.
  - `pc_reach_depth()`: keeps the larger.
  - Asserts when the player has no component.
- **`test_gen`:** the rat's and goblin's XP value is 5, the player's 0.
- **`test_game`** (or where `game.c`'s handler is tested): the player
  killing a rat gains 5 XP; a rat killed by a trap (no source) or by
  another monster gives none.
- **`test_ui`:** the rows of D10; each event redraws its row.

CSpect checks:

1. A new game with a typed name: the stats window shows the name, `Level 1`,
   `XP 0`, `Gold 0`, and the other rows in their new places.
2. Kill a rat: `XP 5`. Kill rats until 20 XP: `Level 2`, "Welcome to experience level
   2.", and the HP maximum higher, the current HP up by the same.
3. With D11's key: `E` goes up a level each press and adds 10 gold (the
   XP for it included), up to level 10; after that the XP still rise, by
   10 each press.
4. A new game after game over: level 1, no XP or gold, the new name.

## 8. Risks

- **The dead monster's kind when the handler runs (D7).** If `EV_DIED` is
  delivered after the monster is released, `kind_index[]` may belong to a
  new entity. The messages already name the dead creature on `EV_DIED`, so
  it is probably still there; the build checks it, and if not, the XP
  value goes in the event (p1 and p2).
- **The level table's pace (D6)** is a guess until the dungeon has its
  monsters; tuning it changes one table.

## 9. Questions

All answered (2026-10-28): Q1 a class placeholder (D12); Q2 the player's
own kills only, as proposed; Q3 XP for gold now (R12a, D13); Q4 to Q8 as
proposed, by agreeing D3, D5, D6, D10 and D11.

- **Q1 (R5)** The fields: name, level, XP and gold as asked, and the deepest
  depth (proposed). Any to add now (class, R16), or to drop?
- **Q2 (R7)** XP only for the player's own kills (proposed), or for every
  monster that dies on the player's level, as classic D&D gives the party
  XP for what it defeats?
- **Q3 (R20)** Classic D&D's 1 XP per gold piece recovered: wanted, later?
  It decides how much gold the dungeon holds.
- **Q4 (D3)** A component bit for the PC (proposed, leaves 1 of 16), a
  game entity flag, or no marker (`KIND_IS_PLAYER()` as now)?
- **Q5 (D5)** 16-bit XP and gold with the game's own level table
  (proposed), or 32-bit with classic tables?
- **Q6 (D6)** The level table, `PC_LEVEL_MAX` 10, and B/X monster XP values
  as proposed?
- **Q7 (D10)** XP above the gold with the rows below moved down (proposed),
  or on row 11 with nothing moved?
- **Q8 (D11)** A development key `E` for XP and gold (proposed), or none?

## 10. As built: differences from the design

- **The functions take the entity** (`pc_gain_xp(id, amount)`), as every
  other component's do, and assert `PC_IS(id)`, rather than reading
  `actor_player`: components don't depend on game/actors.h (G-ENT-4).
  Callers pass `actor_player`.
- **The creature generator adds the component** for a kind with
  `CREATURE_PLAYER`, as it sets the persistent flag, so every player the
  generator makes (the PC tests' too) is `PC_IS()`. `pc_add(id)` names a
  new character "Rogue"; `actors_create(name)` then calls
  `pc_set_name()`. The name the title asks for is kept in `game.c`
  (`screen_title_run(name)`), so an empty answer keeps the last game's.
- **`EV_LEVEL_GAINED`, not `EV_LEVEL_UP`:** events are named in the past
  tense (P-EVENT-1), and it matches `EV_XP_GAINED`.
- **The message is "Welcome to experience level 2.":** the depth is
  already called "level" ("You go down to level 2.").
- **The kill XP and the depth are in the player module, not `game.c`
  (D7) or the level module (5.4):** `player_on_event()` calls
  `player_on_pc_event()` (banked, page 114) for every event; it gives the
  XP for a creature the player killed, records the deepest depth on
  `EV_ARRIVED`, and redraws the XP, level and gold. One banked call in
  common code, with the choosing in page 114: the first version, with
  the cases in common code, took 219 bytes of common memory. The PC tests
  reach it through `test_ui`.
- **The risk in section 8 doesn't arise:** event targets stay valid until
  the events are handled (event/nge_event.h), so the dead monster's kind
  is still there.
- **`screen_draw_stats()` only clears the window;** `screen_show_name()`
  draws the name, from `player_show_stats()` with the other values.
- **A big gain posts about three events a level** (`EV_ATTRS_CHANGED`,
  `EV_HEALED`, `EV_LEVEL_GAINED`), and the event queue holds 16: a gain
  across five levels or more at once would fill it. Kills and the `E` key
  give one level at a time; a large gold pile could give more, so gold
  items (R20) must handle it.
- **Guidelines:** G-ENT-4 names `PC_IS()` instead of `KIND_IS_PLAYER()`,
  which is gone; N-DATA-3 and P-COMP-1 allow the single component's
  variables (D1).
- **Memory (debug build):** slot 0 +14 bytes (the ECS data now ends at
  `$16F3`); common memory +48 bytes, 364 free (412 before); page 110
  +1,352 bytes, about 380 left, the tightest page now; page 114 +370,
  page 116 +220, page 118 +111, page 106 +118.
