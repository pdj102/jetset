# The player character screen: requirements and design

Status: implemented; the PC tests and the CSpect checks in section 7
passed (2026-10-28). Agreed on 2026-10-28: D1 to D10 as proposed,
which answers Q1, Q3, Q5 and Q6; Q2 and Q4 yes (section 9). Section 10
lists where the build differs from the design.

Builds on [ui_screens.md](ui_screens.md) (the overlay screen framework:
this is its R23, the stats screen), [player_character.md](player_character.md)
(the PC component), [attributes.md](attributes.md) (kind value, base,
value; statuses and conditions) and [mana.md](mana.md).

## 1. Goal

- One overlay screen that shows everything about the player character at
  a glance: who they are, how far they have come, their attributes and
  pools, and what is affecting them now.
- Where effects or worn items have moved an attribute away from its base,
  the player sees it at once, and sees whether it helps or hurts.
- No items: what is carried and equipped is the inventory screen's.

## 2. Terms

As in [attributes.md](attributes.md), section 3:

| Term | Meaning |
|---|---|
| Base | An attribute's own number, changed only for good (a potion of vitality, a new level): `health_hp_max_base` |
| Value | Base plus the modifiers of worn items' effects and active effects, clamped: `health_hp_max`. What the rules read |
| Current | A pool's amount now, within its value: `health_hp`, `mana_mp` |
| Changed | An attribute whose value differs from its base |
| Better / worse | A change that helps the player / hurts them. For most attributes higher is better; for speed (ticks between turns) lower is better; a vulnerability is always worse |
| Status | A flag an active effect (or worn item) grants while it lasts: `stats_status` |
| Condition | A status that impairs (`STATUS_CONDITIONS`): poisoned; blinded and unconscious later |
| Active effect | A timed effect on the player: an entity they own, with `COMPONENT_EFFECT` and a timer |

## 3. Requirements

**New** or **Changed** marks a requirement that differs from the request,
with the reason.

### The screen

- **R1** The PC screen is an overlay screen (game/ui/screen.h): it covers
  rows 0-30, pauses the game, and shows the main screen again exactly as
  it was when it closes.
- **R2** It opens from the main screen with one key (D1) and uses no turn.
  SPACE closes it, as on every screen (ui_screens.md, Q3).
- **R3 (New)** It only shows: it has no actions and no cursor. *Reason:
  everything on it is a result of play (equipping, drinking, levelling);
  nothing is chosen here.*
- **R4** It is drawn once, whole, when it opens (ui_screens.md R9, R10):
  nothing changes while the game is paused, so nothing is redrawn.
- **R5** Everything fits on one page (D2): no scrolling and no tabs.

### What it shows: the character

- **R6** Name, class and level, as a title: "Rogue the Fighter".
- **R7** Level; XP; the XP the next level needs (`pc_xp_needed(level + 1)`),
  or that the level is the highest (`PC_LEVEL_MAX`).
- **R8** Gold.
- **R9 (New)** The current depth and the deepest depth reached
  (`pc_depth_max`). *Reason: PC data the stats window shows only half of;
  the deepest depth is the player's progress.*

### What it shows: attributes and pools

- **R10** Hit points: current, value (the maximum) and base.
- **R11** Mana points, if the player has the mana component: current,
  maximum and base. Maximum MP isn't an attribute yet (mana.md D2), so
  its base is shown as its maximum, never changed, until it becomes one
  (attributes.md R12).
- **R12** Armour class: value and base.
- **R13** Speed: value and base, with a word for what the number means
  (D4): fast, normal, slow.
- **R14** The six abilities: score (value), its modifier (`STATS_MOD()` of
  the value) and base.
- **R15 (New)** Damage defences: for each of the 8 damage types, whether
  the player is immune, resistant or vulnerable (the value masks), and
  which of those come from effects or items rather than the base (D6).
  *Reason: they are attributes (`ATTR_IMMUNE`, `ATTR_RESIST`,
  `ATTR_VULN`) that rings and potions will change, and the player can't
  see them anywhere else.*
- **R16 (New)** The melee attack the player makes now: to hit, damage
  dice and bonus, damage type; the wielded weapon's attack, or their own
  (`act_melee.c`'s choice). Not the weapon's name. *Reason: it is the
  character's fighting strength, the number that matters most in play;
  the item itself stays on the inventory screen. See Q4.*

### What it shows: changes from the base

- **R17** Every attribute shown with a base (R10-R15) shows its value in
  a colour that says how it compares to the base: the normal colour at
  its base, one colour when better, another when worse (D3).
- **R18 (Changed)** Better and worse are judged for the player, not by
  the number's direction: a lower speed number is better; an added
  vulnerability is worse. *Reason: the request asks to show effects
  "increasing/decreasing" an attribute; colouring speed 5 (faster) as a
  decrease would warn the player of a benefit.* The base column always
  shows the number, so the direction is still visible.
- **R19** An ability's modifier takes its score's colour.
- **R20** A key to the colours is on the screen (row 30).

### What it shows: statuses, conditions and effects

- **R21** The statuses the player has now (`stats_status`), by name.
  Conditions are shown in the "worse" colour; other statuses (none yet)
  in the normal colour. "None" when there are none.
- **R22 (New)** The player's active effects: each one's name, the turns
  left, and its saving throw if it has one (ability and DC). *Reason: a
  status says the player is poisoned; the effect says for how long, and
  how they might shake it off. An effect with no status (regeneration)
  shows only here.*
- **R23** An effect that lasts for ever shows "always" for its turns.
  Several instances of one effect (stacking) are one row (D7).
- **R24** If there are more active effects than rows, the last row says
  how many more (D8).

### Not shown

- **R25** The inventory and equipment: no item names, no list of what is
  worn. Worn items' modifiers are part of the values (R17), and the screen
  doesn't say which item gave them (D5).
- **R26** The turn count and other game state that isn't the character's:
  they stay in the stats window.

### Code

- **R27** The screen is `game/ui/screen_pc.h` and `screen_pc_bank.c`,
  `screen_pc_run()`, following `screen_inv` (ui_screens.md section 10). It
  reads the components' tables and PC variables (slot 0) and asks the
  effects module for what it keeps in its own page (D5, D9).
- **R28** The colour rule (R17, R18) is one function, PC tested (D3).
- **R29** PC tests for what the screen draws and in which colour
  (section 7).

### Later

- **R30 (later)** `?` opens the help screen at this screen's page
  (ui_screens.md R13).
- **R31 (later)** A breakdown of a changed value by source: base, items,
  each effect (D5).
- **R32 (later)** Class features, saving throws by level, MP by level,
  as the PC component gains them (player_character.md R16-R18): each
  becomes a row here.
- **R33 (later)** A scrolling effect list, if the player can have more
  active effects than fit (D8).

## 4. Decisions

- **D1 `C` opens the screen (R2).** Shifted letters are free beyond the
  development keys (`Q`, `X`, `F`, `V`, `M`, `E`), and `C` for "character"
  is the roguelike convention. **Rejected:** `s` ("stats"): lowercase
  letters are the commands used every turn, and `s` may be wanted for
  search; `@`: needs SYMBOL SHIFT on the Next's keyboard. See Q1.

- **D2 One fixed page, two columns at the top (R5).** The data fits 40 x
  31 with 5 rows of effects (section 5.2). **Rejected:** pages or tabs
  (character / attributes / effects): more keys and code for data that
  fits; a scrolling list of every line: nothing would stay in place
  between visits.

- **D3 Colour by better or worse, with the base always shown (R17,
  R18).** The value is white at its base, cyan when better, red when worse;
  the base column is white. The Layer 2 stream has four inks (white,
  yellow, cyan, red, `io/nge_text.h`): yellow is for headings, as on the
  inventory screen. One function decides:

  ```c
  /* The ink for a value against its base: NGE_INK_WHITE, NGE_INK_CYAN (better) or NGE_INK_RED (worse) */
  uint8_t screen_pc_ink(uint8_t value, uint8_t base, uint8_t lower_is_better);
  ```

  **Rejected:** colour by the number's direction (R18's reason); a marker
  character after the value (`^`, `v`): the font has no arrows, and with
  the base column beside it the colour is enough; showing the base only
  when it differs: the request asks for both, and a fixed column reads
  better.

- **D4 Speed is shown as its number and a word (R13).** Speed is ticks
  between turns (`stats_speed`, 10 normal): under 10 "fast", 10 "normal",
  over 10 "slow". **Rejected:** turning it into moves per turn ("x2"):
  the effects and the docs speak in ticks, and the base would need the
  same conversion.

- **D5 Values are compared with bases only; sources are not named (R25,
  R31).** The difference includes worn items and active effects together,
  which is what the base and value tables hold. Naming the sources means
  running the effects' sum per source (`effects_game_sources()`,
  `effects_add()`), about a page 120 function more, and naming items,
  which R25 leaves to the inventory. **Option (Q3):** a "from items /
  from effects" split now. A known limit: a ring's +2 and a curse's -2
  cancel and show white.

- **D6 Defences as a grid (R15).** Three rows (immune, resist, vulnerable)
  by eight columns (the damage types, three letters each): `*` where the
  value mask has the bit. A bit in the base is white; a bit only in the
  value is cyan for immune and resist, red for vulnerable. Masks are ORed
  (attributes.md), so effects can add bits but not remove them: there is
  no "removed" case. **Rejected:** a line per row listing names
  ("Resist: fire poison"): 8 names don't fit 40 columns; one row of
  outcomes ("Fire: half"): it hides that resist and vulnerable cancel.

- **D7 Effects get a short name, kept with their definitions (R22).**
  `effect_def_t` gains `name` (at most 14 characters, page 121), read
  through `effect_text()` with a new `EFFECT_TEXT_NAME`, as the start and
  end texts are. Only timed effects need one (instant ones leave nothing;
  worn ones are items): "Poison", "Regeneration". Instances of one
  effect are one row, "Regeneration x2", with the longest time left.
  **Rejected:** a table of names in the screen, by `EF_*`: the effects'
  words would be in two places; one row per instance: two regeneration
  sources would take two of five rows.

- **D8 Five effect rows; more becomes "and n more" (R24).** The player has
  two active effects at most today (poison, regeneration). **Rejected:** a
  scrolling list (`ui/nge_list.h`) now: it needs a cursor and keys on a
  screen that has neither (R3). R33 if the effects grow.

- **D9 Turns left from the timer (R22).** An active effect's timer fires
  `cycles` more times, the next in `count` ticks, then every `period`:

      turns left = ceil(((cycles - 1) * period + count) / NGE_TIMER_TICKS_PER_TURN)

  `cycles` 0 is "always". An effect with a saving throw may end sooner:
  the row says so with its save ("CON 10"), not with a lower number. The
  screen reads the timer tables (slot 0) and the effect's kind through
  `effect_kind_of()`; the save through a new
  `effect_save_of(ef, &ability, &dc)`, banked in page 120, so the screen
  doesn't include the effects' internals. **Rejected:** storing the end
  turn in the effect: the timer already knows.

- **D10 In page 116, with the other screens (R27).** Estimate 1.2K with
  its literals, against about 3K free. If it doesn't fit, it moves to the
  free pair 104/105, and `memory_map.md` says so. **Rejected:** its own
  page from the start: one more pair used while 116 has room.

## 5. Design

### 5.1 Files

- `game/ui/screen_pc.h`: `void screen_pc_run(void) NGE_BANKED;`
- `game/ui/screen_pc_bank.c` (page 116): the panels table, the layout,
  `screen_pc_ink()`, and the run function: `screen_open()`, open the
  panels on `SCREEN_BUFFER_OVERLAY`, draw, `screen_help("SPACE:back")`,
  `screen_wait_key()` until SPACE, close the streams, `screen_close()`.
- `game/player/player_bank.c`: `case 'C': screen_pc_run();` (no turn).
- `game/effects/effects.h`: `EFFECT_TEXT_NAME`; `effect_save_of()`.
  `effects_core.h`: `name` in `effect_def_t`. `effects_pdata.c`: the names.
- Names of things the screen prints (abilities, damage types, statuses,
  classes) are literals in page 116; the stats window has its own
  ability and status words today, and the two may share a table later
  (G-FLOW-3 holds as long as the messages module doesn't need them).
- Banking: the name buffer for `effect_text()` is on the stack (N-BANK-5).

### 5.2 Layout

40 x 31 cells, one text panel per block (`ui/nge_panel.h`), black paper.
Headings yellow; labels and numbers white unless D3 colours them.

```
col 0                  19 20                 39
row 0  |          Rogue the Fighter             |   title, centred
    1  |                                        |
    2  | Level         3   |      Now  Mod Base |   character | abilities
    3  | XP          105   | Str   17   +3   16 |
    4  | Next        160   | Dex   12   +1   12 |
    5  | Gold        125   | Con   14   +2   14 |
    6  | Depth         2   | Int    9   -1   11 |   Int red: lowered
    7  | Deepest       4   | Wis   10   +0   10 |
    8  |                   | Cha    8   -1    8 |
    9  |                                        |
   10  |         Now  Value  Base               |   attributes
   11  | HP        8     14    12              |   14 cyan: a ring
   12  | MP        5      5     5              |
   13  | AC              12    10              |
   14  | Speed           10    10  normal      |
   15  | Attack  +2 to hit  1d6+1 slash         |
   16  |                                        |
   17  | Damage  Blu Sl Pi Fi Co Po Ac Ma       |   defences
   18  | Immune                                 |
   19  | Resist              *  *               |   Po cyan: an effect
   20  | Vuln.         *                        |
   21  |                                        |
   22  | Status  Poisoned                       |   red: a condition
   23  |                                        |
   24  | Effect          Turns  Save            |   active effects
   25  | Poison              9  CON 10          |
   26  | Regeneration   always                  |
   27  |                                        |
   28  |                                        |
   29  |                                        |
   30  | Cyan: better than base  Red: worse     |   key (R20), in its colours
   31  | SPACE:back                             |   help line
```

- **Title (row 0):** `"<name> the <class>"`; "Level n" is on row 2, so it
  isn't repeated.
- **Character (0-19, rows 2-8):** at `PC_LEVEL_MAX`, Next shows "max".
- **Abilities (20-39, rows 2-8):** the modifier signed, always ("+0").
- **Attributes (rows 10-15):** "Now" only for HP and MP. MP's row is
  left out when the player has no mana component. Attack: the to-hit
  signed, the dice as `NdS`, the bonus only when not 0, the type's
  name; no colour (it has no base, R16).
- **Defences (rows 17-20):** two-letter types, to keep the grid
  3 columns a type: 8 + 8 x 3 = 32 columns.
- **Status (row 22):** names separated by spaces; at most 3 statuses
  today, 28 characters.
- **Effects (rows 24-29):** five rows (D8), the sixth "and n more".
  Name in columns 0-14, turns right-aligned to 20, save from 23.

### 5.3 Reading the data

| Shown | Read from |
|---|---|
| Name, class, level, XP, gold, deepest | `pc_name`, `pc_class`, `pc_level`, `pc_xp`, `pc_gold`, `pc_depth_max` |
| Next | `pc_xp_needed(pc_level + 1)` (page 110, banked) |
| Depth | `level_depth` |
| HP | `health_hp`, `health_hp_max`, `health_hp_max_base` |
| MP | `MANA_IS()`, `mana_mp`, `mana_mp_max` |
| AC | `health_ac`, `health_ac_base` |
| Speed | `stats_speed`, `stats_speed_base` |
| Abilities | `stats_ability`, `stats_ability_base` |
| Defences | `health_immune`/`_resist`/`_vuln` and their `_base` |
| Attack | the wielded weapon's `attack_*[ATTACK_MELEE]` if it has one, else the player's |
| Statuses | `stats_status`, `STATUS_CONDITIONS` |
| Effects | entities owned by the player with `COMPONENT_EFFECT`: `effect_kind_of()`, `effect_text(EFFECT_TEXT_NAME)`, `effect_save_of()`, `nge_timer_count`/`_period`/`_cycles` |

The attack's choice of weapon must match `act_melee.c`'s; if that choice
isn't a function today, it becomes one both call (`attack_of(id)`,
page 110) rather than being copied.

### 5.4 Memory

- Page 116: about 1.2K (code and literals); measure.
- Page 121: 14 bytes or less per effect name, about 30 bytes today.
- Page 120: `effect_save_of()`, about 40 bytes.
- Common: none; slot 0: none. Streams: the screen's panels fit within
  `NGE_FILE_MAX` as the inventory's do (4 text panels or fewer: title,
  left, right, body).

## 6. Scope

In: R1-R29; `EFFECT_TEXT_NAME` and names for the timed effects;
`effect_save_of()`; the `C` key; PC tests; docs: `memory_map.md` (page
116), `ui_screens.md` (R23 points here), `src/game/README.md` if it lists
the screens, the help text that lists the keys if there is one.

Later: R30-R33.

## 7. Tests

PC tests:

- **`test_screen_pc`** (new, or in `test_ui`):
  - `screen_pc_ink()`: equal is white; higher better is cyan, lower red;
    with lower-is-better the reverse.
  - A new player: the title, level 1, XP 0, the next level's XP, gold 0,
    every value white, "Status  None", regeneration "always".
  - Strength raised for good (`effects_set_base()`): the new base and
    value, white.
  - A ring of health equipped: HP's value cyan, its base unchanged.
  - Poisoned: "Poisoned" red; the Poison row with its turns (from the
    timer) and "CON 10".
  - Speed lowered (faster) by an effect: cyan; raised: red.
  - A resistance granted by an effect: its `*` cyan; one in the base:
    white.
  - Level 10: Next shows "max".
  - Six active effects: five rows, then "and 1 more"; two regenerations
    are one row "x2".
  - Opening and closing: buffer 0 untouched, shown again.
- **`test_effects`:** each timed effect has a name; `effect_save_of()`
  for poison is CON 10, for regeneration none.

CSpect checks:

1. `C` in play: the screen appears whole; the values match the stats
   window. SPACE: play's screen is back exactly, and no turn has passed.
2. Drink a potion of strength: Str's base and value go up, white.
3. Wear the ring of health: HP's maximum cyan, the base unchanged; take
   it off: white again.
4. Get poisoned: "Poisoned" in red, the Poison row counting down between
   visits; it goes when the poison ends.
5. With the development key `E`, reach level 10: Next shows "max".

## 8. Risks

- **Page 116's room (D10):** measure as soon as the screen is written.
- **The effects' saving throw is a game-side idea (effects_core.h,
  "the seam"):** `effect_save_of()` returns the definition's `arg` as an
  ability; if the core moves to nge, the screen asks the game's side
  instead.

## 9. Questions

All answered (2026-10-28): Q1 `C` (D1); Q2 yes, depth and deepest depth
shown (R9); Q3 base and value only (D5, R31 later); Q4 yes, the melee
attack shown (R16); Q5 cyan and red, the base always shown (D3); Q6 one
row per effect, "x2" (D7).

- **Q1 (D1)** `C` to open the screen, or another key?
- **Q2 (R9)** Depth and deepest depth on this screen: wanted?
- **Q3 (D5)** Show only base and value (proposed), or split the difference
  into "from items" and "from effects" now?
- **Q4 (R16)** The current melee attack on this screen (proposed), or
  leave it to the inventory, as it comes from the weapon?
- **Q5 (D3)** Cyan for better and red for worse, the base always shown?
- **Q6 (D7)** One row per effect, stacked instances counted ("x2")?

## 10. As built: differences from the design

- **Page 104, not 116 (D10).** The screen is about 4.1K with asserts, not
  the 1.2K estimated, and page 116 had 3K free: it overflowed by 1,080
  bytes. It is alone in the free pair 104/105 (`src/banks.inc`,
  `memory_map.md`); page 116 is as it was.
- **Its own ability names.** Page 104 can't read page 116's table
  (N-BANK-5), so the screen has a copy. It walks them by pointer: indexing
  a table in a banked page gave z80asm's "integer range" warning, as in
  dungeon_decoration.md section 7.
- **`attack_weapon(id)`, not `attack_of()` (5.3).** It returns the
  wielded melee weapon, or `NGE_ENTITY_INVALID`, in the attack component
  (page 110). The melee action needs the weapon itself (it goes in the
  action's item), so it calls this and falls back to the actor, as the
  screen does.
- **`effect_save_of(ef, &ability)` returns the DC,** 0 for none, rather
  than filling both through pointers.
- **The saving throw reads "Con 10",** with the abilities' names as the
  abilities panel shows them, not "CON 10".
- **Five effect rows, the last of them "and n more" (D8, 5.2).** The
  layout has five rows (25-29) before the key on row 30, so no sixth row
  is left: more than five kinds show four and "and n more". The player
  can have two kinds today (poison, regeneration), so this path isn't
  tested yet.
- **No MP: the row is left blank,** not closed up, so the rows below
  don't move.
- **The main screen's help line doesn't list `C`:** it is 40 characters
  already ("g:get i:inv u:use w:wear r:off <>:stairs").
- **Tests are in `test_ui`,** not a new `test_screen_pc`, as the other
  screens' are; `test_attack` covers `attack_weapon()`, and `test_effects`
  the names and `effect_save_of()`.
- **Memory (debug build):** page 104 about 4.1K; page 110 +143 bytes
  (`attack_weapon()`), about 240 left; page 120 +180 bytes, page 121 +36;
  common memory 337 bytes free (364 before): the banked-call stubs of the
  three new banked functions.
