# Character creation: requirements and design

Status: implemented (2026-11-08). Q1 to Q6 were answered on 2026-11-08
(section 9). The answers changed R1, R2, D3, D8 and D11. Step 1 of the
plan (the classes and kits, without the screen) is built: its PC tests and
CSpect checks passed (2026-11-08). Step 2 (the creation screen) is built:
its PC tests and CSpect checks 1-6 passed (2026-11-08). The fix for the
gap between panels, made afterwards, is checked by the PC tests. Step 3
closed the docs. Hardware check 7 is still to do. Section 10 lists where the build
differs from the design. The build steps are in
[character_creation_plan.md](character_creation_plan.md). To-do item C1
([todo.md](todo.md)).

Builds on [player_character.md](player_character.md) (the PC component and
its class placeholder, R16, D12), [pc_screen.md](pc_screen.md),
[ui_screens.md](ui_screens.md) (overlay screens), [items.md](items.md) and
[mana.md](mana.md) / [spells.md](spells.md).

## 1. Goal

- When a new game starts, the player makes their character: picks a class,
  optionally changes its name, sees what the class gives them, and starts.
- A framework that later classes and random rolls slot into: a class is
  data, and the screen shows whatever the class record makes.

Today every game starts with one fixed player (`CR_PLAYER`: Str 16 Dex 14
Con 14 Int 10 Wis 10 Cha 10, 12 HP, 5 MP, Magic Bolt). In debug builds it
also carries the development kit (game/actors.c). The title asks for the
name. `pc_class` exists but only holds `PC_CLASS_FIGHTER`, and nothing
reads it.

## 2. Terms

| Term | Meaning |
|---|---|
| Class | What the character is: Fighter, Mage, Dev. `pc_class`, a `PC_CLASS_*` value |
| Class record | A class's row in the class table: its name, default name, creature kind and kit |
| Default name | The name a class offers until the player types one ("Bran" for a Fighter) |
| Kit | The items a class starts with. Each is either equipped or carried |
| Preview | The character the creation screen shows: the real player entity, made for the chosen class |
| Creation screen | The new overlay screen where the class is chosen and the name changed |

## 3. Requirements

**New** or **Changed** marks a requirement that differs from the request,
with the reason.

### Flow

- **R1 (Changed, Q2)** Starting a new game goes: title (any key) →
  creation screen → play. The title no longer asks for the name.
- **R2 (Changed, Q2)** Each class has a default name, which the screen
  shows until the player types one. The **change name** key opens a name
  prompt on the creation screen: 1 to 6 characters, ENTER to finish. An
  empty answer keeps the name as it was.
- **R2a (New)** A typed name stays when the player moves to another class,
  and is offered again in the next game. *Reason: the player chose it.
  Losing it on a class change would mean typing it again (Q6).*
- **R3** On the creation screen the player can move between the classes,
  and the screen updates for the class chosen.
- **R4** ENTER confirms the class and starts the game.
- **R5 (New)** The creation screen opens on the class chosen last time
  (Fighter in the first game). *Reason: playing the same class again is
  then just ENTER.*

### Classes

- **R6** There are three classes. Stats are fixed, with nothing rolled:

  | | Fighter | Mage | Dev |
  |---|---|---|---|
  | Abilities | high Str, Dex; low Int, Wis | high Int, Wis; low Str, Dex | 12 in each (Q3) |
  | Mana | none | yes | yes |
  | Kit | short sword, leather armour, shield | dagger, sling, stones | sword, plate armour +2, short bow, arrows (Q4) |

  The figures are in D4.
- **R7 (New)** A class with mana knows Magic Bolt, as today's player does.
  A class without mana knows no spells. *Reason: a caster with no spell
  couldn't use its mana. A fighter with a spell couldn't cast it.*
- **R8 (New)** The kit's weapons, armour, launcher and ammunition start
  equipped. *Reason: the player shouldn't have to spend their first turns
  putting on the kit.*
- **R9 (Changed, Q5)** The Dev class is offered only in debug builds
  (`GAME_DEV_KIT`). It also carries the rest of today's development kit
  and its resistances. Fighter and Mage get only their own kit, in every
  build. *Reason: Dev is a testing class. Fighter and Mage should be
  checked in CSpect exactly as a player will get them.*

### The screen

- **R10** The creation screen shows, for the chosen class:
  - the name and class as a title ("Bran the Fighter");
  - HP, MP (or "none"), AC and speed;
  - the six abilities with their modifiers;
  - the kit, each item named as the inventory names it, marked if it's
    equipped.
- **R11 (New)** It also shows the spells the class knows. *Reason: it's the
  other thing besides abilities and kit that differs between classes.*
- **R12** The values shown are the character's real values. AC, for
  example, includes the armour and shield the kit puts on (D1).
- **R13** It is an overlay screen (ui_screens.md): rows 0-30, with the help
  line on row 31.

### Out of scope

Later, each in its own spec or to-do item:

- **R14 (later)** More classes (cleric, thief).
- **R15 (later)** Rolled abilities (3d6, 4d6 drop lowest) and rolled HP.
  The preview makes the character for real (D1), so a roll is one more
  step when the character is made, with a key to roll again.
- **R16 (later)** Per-class rules: hit die, level table, the spells learned
  at each level, what each class can wear or wield.
- **R17 (later)** A sprite or colours per class.
- **R18 (later)** A one-line description of each class on the screen.

## 4. Decisions

- **D1 The preview is the real character, made again on each change
  (R3, R12).** Moving to a class empties the world (the ECS reset that
  `play_start()` does today) and makes the player of that class as entity
  0, kit included. The screen then draws from the components, as the PC
  screen does. On ENTER the player is already made, so play carries on from
  there. **Rejected:**
  - Drawing from the class data. AC with armour, attack with a weapon, and
    later rolled values would all need the rules worked out a second time.
  - Three players made at once. Only one can be entity 0, and they would
    use three times the timers and effects.

- **D2 A class is a creature kind plus a class record.** Each class has a
  creature kind with `CREATURE_PLAYER` (abilities, HP, mana, spells), so
  the generator does all of the making. A class record in the new module
  names the class, its default name, its kind and its kit. Adding a class
  means adding a kind and a record. **Rejected:** one player kind, with a
  class record that overrides abilities, mana and spells. That would mean
  new generator code to strip the mana component and the spells that the
  kind gives.

- **D3 (Changed, Q3) `CR_PLAYER` becomes the Dev class's kind, with 12 in
  every ability. `CR_FIGHTER` and `CR_MAGE` are added at the end (21,
  22).** Dev keeps today's HP, MP and Magic Bolt. The PC tests that make a
  `CR_PLAYER` (12 test files) keep it. Tests that depend on today's
  abilities are updated to 12s: to-hit, damage, the Con modifier on level
  up, and the PC screen's rows. `make test` finds them.

- **D4 The figures** (agreed, Q1; to tune in play):

  | | Fighter | Mage | Dev |
  |---|---|---|---|
  | Str Dex Con | 16 14 14 | 9 10 12 | 12 12 12 |
  | Int Wis Cha | 9 9 10 | 16 14 10 | 12 12 12 |
  | HP | 12 | 8 | 12 |
  | MP | none | 8 | 5 |
  | Spells | none | Magic Bolt | Magic Bolt |
  | Speed, AC base | 10, 10 | 10, 10 | 10, 10 |
  | Default name | Bran | Elwyn | Dev |

  Each class's kit (item kinds):

  | Class | Equipped | Carried |
  |---|---|---|
  | Fighter | `IT_SHORT_SWORD`, `IT_LEATHER_ARMOUR`, `IT_SHIELD` | |
  | Mage | `IT_DAGGER`, `IT_SLING`, `IT_STONES` | |
  | Dev | `IT_SWORD`, `IT_PLATE_ARMOUR_2`, `IT_SHORT_BOW`, `IT_ARROWS` | the rest of today's `dev_kit[]` |

- **D5 A kit item is equipped only if its record says so (R8).** Each kit
  entry is a kind plus an equip flag. It is made with `gen_item()`, owned
  by the player, then equipped through `act_equip()` (`ACT_DO`), so slots
  and worn effects follow the one rule. **Rejected:** equipping whatever
  fits. The Dev kit's ring of frailty is cursed and would be put on.

- **D6 The kit's events are thrown away.** Making the character posts
  `EV_EQUIPPED` and `EV_ATTRS_CHANGED`. After each preview the queue is
  cleared (`nge_event_init()`), so play doesn't open with "You wield a
  short sword." `game_play_redraw()` draws the stats afresh.

- **D7 A new module, `game/chargen/`, in pages 140/141.** It holds the class
  table, the making (`chargen_make()`) and the creation screen
  (`chargen_run()`), all in page 140, so the screen reads the class table
  directly. Page 141 is unused. **Rejected:**
  - The screen in `game/ui/`. Page 116 has about 1.6K free, and the screen
    would need copies of the class table across pages.
  - The making in `actors.c`. That is common memory, which this change
    frees instead (D9).

- **D8 (Changed, Q2) The keys.**
  - Up and down (also `j` `k`) choose from a list on the left. These are
    the inventory list's keys. The list stops at both ends.
  - `n` is change name: the name prompt on rows 28-30, as on today's
    title. The game's text input (`nge_line`) handles the typing, and
    ENTER ends it.
  - ENTER starts the game. No other key does anything.
  - The help line reads "Up/down: class   N: change name   ENTER: start".
    While the player types it reads "Type a name, then ENTER".

- **D9 `actors_create()` becomes `actors_create_persistents()`.** It
  makes only the quest item and the debug goblin. Player creation and
  `dev_kit[]` move to page 140, which saves common memory. `play_start()`
  calls, in order: `chargen_run()` (which makes the player, entity 0), then
  `actors_create_persistents()`, then the rest as now.

- **D10 The class is set by `pc_set_class()` (page 110).** `pc_add()` still
  starts with Fighter. `PC_CLASS_MAGE` 1 and `PC_CLASS_DEV` 2 are added,
  so `PC_CLASS_COUNT` becomes 3. The class names live once, in the class
  table, and `chargen_class_name()` copies one. The PC screen's
  `class_names[]` goes.

- **D11 (Changed, Q2) The title shows only the game's name and "Press any
  key to start".** `screen_title_run()` loses its parameter, the name
  prompt (`ask_name()` moves to the chargen module) and its three welcome
  messages. The messages move to the end of `chargen_run()`, because they
  belong to the start of play.

- **D12 (New, R2, R2a) The name is held by `game.c` and starts empty.**
  Empty means "use the class's default". `chargen_run()` passes the
  default to `chargen_make()` while the name is empty, and the typed name
  once there is one. A typed name is written into `game.c`'s `name`, so
  the next game offers it again. **Option (Q6):** each class keeps its own
  name, so moving away from a class you renamed shows the default again.

## 5. Design

### 5.1 The module (`game/chargen/chargen.h`)

```c
/* The classes the creation screen offers: Dev in debug builds only (R9) */
#define CHARGEN_CLASSES   (GAME_DEV_KIT ? 3 : 2)

/** Empty the world and make the player of class cls (PC_CLASS_*) as entity 0,
    named name, with its kit; the event queue cleared (D1, D6). */
void chargen_make(uint8_t cls, const char *name) NGE_BANKED;

/** The creation screen: from class cls until ENTER. name (common memory,
    PC_NAME_MAX + 1) is empty for the class default, and takes a typed name.
    Leaves the player made (chargen_make()) and returns the class. */
uint8_t chargen_run(uint8_t cls, char *name) NGE_BANKED;

/** Copy class cls's name ("Fighter") into buf, CHARGEN_NAME_MAX + 1 bytes. */
void chargen_class_name(uint8_t cls, char *buf) NGE_BANKED;
```

The class table in page 140, by `PC_CLASS_*`:

```c
typedef struct chargen_kit { uint8_t kind; uint8_t equip; } chargen_kit_t;
typedef struct chargen_class {
    const char *name;              /* "Fighter" */
    const char *default_name;      /* "Bran": 1 to PC_NAME_MAX characters */
    uint8_t creature;              /* CR_FIGHTER, CR_MAGE, CR_PLAYER */
    const chargen_kit_t *kit;      /* ended by { IT_NONE, 0 } */
} chargen_class_t;
```

`chargen_make()`:

1. `game_reset_world()`, which is `play_start()`'s ECS reset
   (`nge_entity_init()`, `nge_render_init()`, `nge_timer_init()`,
   `nge_event_init()`) moved into a function of its own in game.c.
2. `gen_creature(class.creature, NGE_OWNER_WORLD)`, asserting entity 0.
   Then `pc_set_class()` and `pc_set_name()`.
3. For each kit entry: `gen_item(kind, actor_player)`, then `act_equip()`
   if the entry's equip flag is set, asserting success. For Dev, also the
   resistances that `actors_create()` sets today.
4. `nge_event_init()`.

A name change doesn't remake the character. It calls `pc_set_name()` and
redraws the title row.

### 5.2 The screen

```
row  0          CREATE YOUR CHARACTER          (yellow, centred)
row  2              Bran the Fighter           (centred)
row  4  Class        HP   12      AC   13
row  5  > Fighter    MP   none    Speed normal
row  6    Mage
row  7    Dev        Str 16  +2   Int  9   0
row  8               Dex 14  +1   Wis  9   0
row  9               Con 14  +1   Cha 10   0
row 11               Spells  none
row 13               Kit
row 14               short sword       wielded
row 15               leather armour    worn
row 16               iron shield       worn
rows 28-30  What is your name? _           (only while changing it)
row 31  Up/down: class   N: change name   ENTER: start
```

The left panel is columns 0-11 and the right panel columns 13-39. The
cursor is drawn in yellow. On a move, `chargen_make()` runs, then the
title and right panel are redrawn (the list redraws only its cursor). Each
change is one key press, so its cost doesn't matter. The modifiers are
`STATS_MOD()`, and the item names come from the inventory's naming
function (with counts, "pouch of stones (20)"). After a name is typed, the
prompt panel is cleared again.

### 5.3 The game loop (`game.c`)

```c
static char name[PC_NAME_MAX + 1];              /* empty: the class default (D12) */
static uint8_t last_class = PC_CLASS_FIGHTER;   /* R5 */

game_run():
    for (;;) { screen_title_run(); over_run(play_run()); }

play_start():
    srand(...);
    last_class = chargen_run(last_class, name);   /* the player made: entity 0 */
    actors_create_persistents();
    player_init(); level_clear_request(); ended = 0;
    ... as now from nge_render_follow()
```

### 5.4 Memory

- Page 140: the module with the name prompt, about 2K with asserts. Page
  141 is unused.
- Page 116: the title loses `ask_name()` and the messages, about 200 bytes
  saved.
- Common: `dev_kit[]` and the dev block leave `actors.c` (about 100 bytes
  saved). `game_reset_world()` is about the same size as the code it
  replaces.
- Page 107: two creature kinds, about 90 bytes. Page 127: two sequence
  rows, 12 bytes.
- Page 110: `pc_set_class()`, about 20 bytes. Page 110 is the tightest page
  (about 380 free).
- Page 104: the class names go, a few bytes saved.

## 6. Scope

In: R1-R13, the PC tests, and the docs: `src/game/README.md` (a chargen
row, and the `ui` title, `actors` and `dev.h` rows), `docs/memory_map.md`
(pages 140/141), `game/dev.h` (the kit is now Dev's), `actors.h`, `pc.h`,
`screen_title.h`, and [todo.md](todo.md) (C1's status).

Out: R14-R18.

## 7. Tests

PC tests:

- **`test_chargen`** (new):
  - For each class, `chargen_make()` leaves the player as entity 0 with
    `PC_IS()`, its `pc_class`, its kind's abilities and HP, and the name.
    The Fighter has no mana and no spells. The Mage has 8 MP and Magic
    Bolt. The Dev has 12 in every ability.
  - The kit: each equipped item in its slot (`item_slot[]`), stones in
    `SLOT_AMMO` and the sling in `SLOT_RANGED`. The Dev's extras are
    carried, not equipped. The Fighter's AC includes the armour and
    shield.
  - The event queue is empty afterwards.
  - Making Mage after Fighter leaves one player and none of the Fighter's
    items (the entity count).
  - `CHARGEN_CLASSES` is 2 with `GAME_DEV_KIT` 0, so Dev isn't offered.
  - `chargen_class_name()` gives each class's name.
- **`test_gen`:** `CR_FIGHTER` and `CR_MAGE` have `CREATURE_PLAYER` and
  are never generated at random. `CR_PLAYER` has 12 in every ability.
- **`test_pc`:** `pc_set_class()` sets the class and asserts on a bad one.
- **`test_ui`:**
  - The screen's rows for each class.
  - Down and up move the cursor, stopping at the ends, and redraw the
    preview.
  - With an empty name, each class shows its default ("Elwyn the Mage").
  - `n`, then a typed name and ENTER: the title row and `pc_name` change,
    `name` holds it, and moving to another class keeps it (R2a).
  - `n`, then ENTER alone: the name is unchanged.
  - ENTER returns the class. The screen opens on the class passed in.
  - The PC screen's title reads "Elwyn the Mage".
  - The title only waits for a key: no prompt, no messages.
- **Every test file using `CR_PLAYER`:** the expectations that followed
  from Str 16, Dex 14 and Con 14 are moved to 12s (D3).

CSpect checks:

1. New game: the title shows no prompt, and any key opens the creation
   screen on "Bran the Fighter", with the Fighter's figures and the short
   sword, leather armour and shield equipped.
2. Down: "Elwyn the Mage", MP 8, Magic Bolt, dagger, sling and stones.
   Down again: Dev (debug build) with 12s and the full development kit. Up
   and down stop at the ends.
3. `n`, type "Ana", ENTER: the title reads "Ana the Dev". Up: "Ana the
   Mage".
4. ENTER on Mage: the stats window shows "Ana" and the Mage's HP and MP,
   the inventory shows the kit equipped, and the PC screen reads "Ana the
   Mage". `f` fires the sling without any equipping first. The welcome
   messages appear, with no "You wield" ones.
5. A Fighter game has no MP row in the stats window and can't cast (`m`).
6. Game over, then a new game: the screen opens on Mage, named "Ana".
7. Hardware (release build): only Fighter and Mage are offered, and
   neither carries the development kit.

## 8. Risks

- **Resetting the world while an overlay screen is open (D1).**
  `nge_render_init()` hides the sprites at the next upload. The screen
  covers them anyway, but CSpect check 2 confirms that nothing flickers.
- **`act_equip()` outside play.** It must not need the player's turn or a
  location: the player isn't placed yet. If it does, the making sets
  `item_slot[]` and calls `effects_recompute()` itself, as act_equip.c
  does. That costs a second copy of the slot rule, so it is only the
  fallback.
- **The event queue (32)** while the Dev's 17 items are made and 4
  equipped. Today 17 `gen_item()` calls fit, and the equips add about 8
  events. The test asserts nothing overflows.
- **The test churn from the Dev's 12s (D3)** may be wide. If it runs past
  a handful of expectations, the tests get their own creature kind with
  today's figures instead. That would be noted in "As built".

## 9. Questions

Answered on 2026-11-08:

- **Q1 (D4)** The figures: agreed as proposed.
- **Q2 (D8)** The name is typed on the creation screen with a change name
  key, and each class has a default name. This changed R1, R2, D8 and D11,
  and added R2a and D12.
- **Q3 (D3)** Dev has 12 in every ability.
- **Q4 (D4)** "Good armour" is plate armour +2, and "box" is a short bow.
- **Q5 (R9)** Dev is offered in debug builds only, and the development kit
  is on Dev alone.

- **Q6 (R2a, D12)** A typed name stays for every class and for the next
  game, as proposed. The default names Bran and Elwyn are agreed.

## 10. As built: differences from the design

Step 1 (2026-11-08):

- **`chargen_make()` empties the world itself** with the nge inits
  (`nge_entity_init()`, `nge_render_init()`, `nge_timer_init()`,
  `nge_event_init()`), rather than calling a `game_reset_world()` in
  game.c (5.1). The module then depends only on nge and the game's
  components, and the PC tests can link it without game.c.
  `play_start()` no longer calls those inits.
- **Until the screen comes (step 2),** a new game starts as the Dev in debug
  builds and as the Fighter in hardware builds (`START_CLASS`, game.c),
  with the name the title asks for.
- **The kits are arrays with a count** (`KIT()`), not lists ended by
  `IT_NONE`: there is no `IT_NONE`.
- **`CHARGEN_CLASSES` is set by `#if GAME_DEV_KIT`.** The class table
  always has the Dev's row. Without `GAME_DEV_KIT` it holds only the four
  equipped items, and `chargen_make()` refuses it.
- **Test changes from the Dev's 12s (D3):** five expectations, in
  `test_pc` (the hit point and MP gains on a new level) and `test_ui` (the
  stats window's Str and Cha). `test_anim` and `test_gen` now allow for
  the two new player kinds. The fallback in section 8 wasn't needed.
- **Memory:** in the debug build, common memory has 166 bytes more free
  (4,172 to 4,338), because the development kit left `actors.c`. Page 140
  holds 967 bytes in debug builds and 506 in hardware builds.

Step 2 (2026-11-08):

- **The help line is "Up/down:class N:change name ENTER:start"** (D8),
  in the game's compact `key:action` style. The spec's version is 46
  characters, and the row has 40.
- **Equipped items are shown in cyan**, as on the inventory screen, with
  "Kit (cyan: in use)" as the heading, rather than a word beside each
  item. A kit item's name and count leave no room for a word. A count is
  shown when it is more than 1: "pouch of stones (20)".
- **The Dev's kit is longer than the panel** (17 items, 14 rows). The
  equipped items come first, then the pack, and the last row reads "and 4
  more in the pack".
- **The screen is `chargen_screen_bank.c`,** in page 140 beside
  `chargen_bank.c`, with the class table shared through
  `chargen_internal.h`.
- **The panels cover rows 0-30 with no gaps.** The head panel is rows
  0-3 and the class list is columns 0-12. A gap between panels is never
  cleared, so in CSpect the title's "Press any key" left an 's' in column
  12, beside the first kit item. `test_ui` now fills the overlay with
  junk before the screen opens, and checks that none of it is left.
- **The title's panel now covers rows 0-30.** Its prompt panel went with
  the name prompt.
- **`GAME_DEV_KIT` 0 isn't covered by the PC tests.** They build with
  debug settings, so the two-class offer is checked only by building for
  hardware and by CSpect check 7.
- **Memory:** page 140 holds 3,879 bytes in debug builds and 3,089 in
  hardware builds. Common memory is 1 byte smaller: 4,337 bytes free in
  debug builds.
