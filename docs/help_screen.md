# Help screen: requirements and design

Status: implemented (2026-11-09). Three questions were answered before
writing (section 9), and decisions D1-D9 were agreed the same day. CSpect
checks 1-7 passed (2026-11-09). The page turn time on hardware is not
measured yet. Section 10 lists where the build differs from this design.

Covers to-do objective **U1** (U1.1-U1.4) in [todo.md](todo.md). Builds
on [ui_screens.md](ui_screens.md) (the overlay screen framework,
`game/ui/screen.h`), where it is R24, and in part R13. The plan is
[help_screen_plan.md](help_screen_plan.md).

## 1. Goal

- In play, `?` opens a help screen. It is a few pages of fixed text: the
  keys, the keys in screens and prompts, the goal of the game, and what
  the abilities and stats do.
- The player can turn the pages and leave. Leaving doesn't use a turn.
- It is an ordinary overlay screen, built like the PC screen
  (`screen_pc_bank.c`): the four framework steps, panels, and the help
  line. Nothing new goes into nge.

## 2. Terms

| Term | Meaning |
|---|---|
| Help screen | The overlay screen this spec adds. It is called the *guide* in code (D2), so it can't be confused with the help line |
| Help line | Row 31: what the keys do now (`screen_help()`), as in ui_screens.md |
| Page | One screenful of help text: a heading on the title panel and up to 29 rows of body |
| Ability | Str, Dex, Con, Int, Wis, Cha: 1-30, with a modifier `(score - 10) / 2` (`STATS_MOD()`) |

## 3. Requirements

**New** or **Changed** marks a requirement that differs from the to-do
entry, with the reason.

### The screen

- **R1 (U1.1)** The help screen has several pages (four now, D5). Each
  page has a title with its number and the count ("Commands 1/4") and a
  body of text.
- **R2** In play, `?` (SYMBOL SHIFT + C) opens the help screen at page 1.
  It uses no turn, and the game is paused while it is open (ui_screens.md
  R14).
- **R3** Left and right (the cursor keys, or `h` and `l`) turn the page.
  The pages wrap around: right on the last page goes to the first, and
  left on the first goes to the last. The digit keys `1` to the page count
  go straight to that page. SPACE leaves, as on every screen
  (ui_screens.md Q3). Other keys do nothing.
- **R4** The help line reads `Left/right:page 1-4:go to SPACE:back`.
- **R5** Leaving shows play's screen as it was (ui_screens.md R4), with
  play's help line.
- **R6 (Changed)** The help line in play becomes
  `?:help i:inv g:get <>:stairs`. *Reason: the line is full at 40
  characters, and `?` must be shown. The help screen lists every key, so
  the play line only needs to point to it (question Q3).*

### The pages

- **R7 (U1.2)** A **Commands** page lists every key in play and what it
  does: moving (including diagonals and waiting), items, fighting and
  magic, the dungeon's features, looking, stairs, the character screen,
  help and quit. Development keys (`GAME_DEV_KIT`) are not listed.
- **R8 (New)** A **Screens and prompts** page lists the keys in the
  inventory (browse and choose), the spell list, the targeting cursor,
  the direction prompts and the help screen. *Reason: those keys are only
  on each screen's help line, which shows them one context at a time.
  Context help (`?` on each screen) is left for later (Q2, U1.5).*
- **R9 (U1.4)** A **Goal** page says how to win: go down to depth 10, take
  the crown, and climb out of the dungeon (up the stairs on depth 1) with
  it. It also says what happens to the crown if it is dropped, and that
  dying ends the game. These are the rules of
  [winning.md](winning.md).
- **R10 (U1.3, Changed)** An **Abilities and stats** page says what each
  ability does *in the rules as they are now*, how its modifier follows
  from its score, and what HP, MP, AC, speed, XP and gold are. An ability
  with no use yet says so. *Reason: the to-do entry's example (Str: melee
  hit and damage; Dex: ranged hit) isn't true of the code. Today Str only
  resists being held, and Cha does nothing. The page must not promise
  rules that don't exist (Q1). The rules change is a new to-do item
  (section 6).*
- **R11** Each page fits its panel: at most 29 rows of at most 40
  characters, with no wrapping. A PC test checks this (section 7).
- **R12** Headings are yellow, keys cyan, and the rest white, as on the
  other screens.

### Code

- **R13** The screen is `game/ui/screen_guide.h` and
  `screen_guide_bank.c`, with its text in `screen_guide_pdata.c`. It is
  opened by `player_command()`.
- **R14** No change to nge, and none to the framework (`screen.h`).
- **R15** PC tests in `test_ui` (section 7).

## 4. Decisions

- **D1 The pages are strings with control codes, in the odd page of the
  pair.** Each page is one `const char[]`: lines end with `\n`, and colour
  changes are the stream's own INK code (`NGE_CTRL_INK`, then
  `NGE_INK_*`, both non-zero), built by macros (`H` yellow, `K` cyan, `T`
  white) using string-literal joining. The whole page is one `nge_puts()`
  to the body's stream. The text is in page 105, which a banked call into
  104 maps into slot 7. The Layer 2 drawing maps its own page into slot 7
  and puts the old page back for each glyph, so the stream can read the
  text (N-BANK-5: it is passed only to common code). **Rejected:**
  - a table of lines with an ink byte each: more bytes, and a loop that
    does what the stream already does;
  - text in page 104 beside the code: 104 has about 2K left, and the four
    pages are about 3K (D3).

- **D2 Named `screen_guide`, not `screen_help`.** `screen_help()` is the
  help line (`screen.h`). A file `screen_help.h` with a function
  `screen_help_run()` would read as part of it. **Rejected:**
  `screen_help.h` (confusing); `screen_manual.h` (fine, but "guide" is
  shorter on a 40-column line of code).

- **D3 Code in page 104, text in page 105.** Page 116 is full. Page 104
  holds the PC screen and the spell list (about 5.9K), and the guide's
  code is about 0.4K. Page 105 is unused, and the text is about 3K.
  **Rejected:** a new pair (142/143): it uses a pair for 0.4K of code.
  It stays the fallback if 104 overflows when it is built.

- **D4 The body is cleared and drawn again on each page turn.** The
  body's stream clears (`nge_text_cls()`) and the page's string is
  written. The screen shows buffer 1 while it draws, so the turn is
  visible as a quick repaint. **Rejected:** drawing into a hidden buffer
  and switching: only two buffers exist (ui_screens.md D2), and buffer 0
  holds play's screen. The repaint is measured in check 4; if it is slow,
  a page can be drawn from its first line down, which looks deliberate.

- **D5 Four pages, Commands first.** The order is 1 Commands,
  2 Screens and prompts, 3 Goal, 4 Abilities and stats. A player
  pressing `?` mid-game most often wants a key, so page 1 is the keys.
  **Rejected:** Goal first. It is read once, and the title or creation
  screen is the place to say it (left for later).

- **D6 `?` works in play only (Q2).** From play, `?` opens page 1. On
  other screens and in prompts, `?` does nothing new: the inventory's
  choose mode ignores it, and prompts treat it as "other key: back".
  **Rejected:** context help now: it needs a context number in
  `screen_help()` (ui_screens.md R13), a `?` case on each screen, and a
  page per context. It is U1.5.

- **D7 The text describes today's rules (Q1)** (R10). **Rejected:**
  adding Str and Dex to attacks in this objective. That is a rules
  change with balance tests of its own, and it is not help.

- **D8 Page turning wraps around, and the digits jump (R3).** With four
  pages, wrapping means any page is at most two presses away, and the
  code is one modulo. **Rejected:** stopping at the ends, which needs the
  help line to change at each end to stay true (ui_screens.md R11).

- **D9 The text is written by hand, beside the rules it describes.** The
  pages name keys and rules that live in `player_bank.c`, `effects_pdata.c`,
  `feature.c` and `pc.c`. A test can't check the meaning, so each of
  those places gets a one-line comment: "described in the help screen
  (screen_guide_pdata.c)". **Rejected:** generating the Commands page
  from a key table. `player_command()` is a `switch`, and turning it into
  a table is a larger change than the help it would save.

## 5. Design

### 5.1 The module (`game/ui/screen_guide.h`)

```c
/** @brief Show the help screen at page 1 until SPACE. Uses no turn. */
void screen_guide_run(void) NGE_BANKED;
```

`screen_guide_bank.c` (`codeseg PAGE_104`, `constseg PAGE_104`):

```c
#define PANEL_TITLE  0   /* rows 0-1: the heading, and "n/4" on the right */
#define PANEL_BODY   1   /* rows 2-30: the page's text, clip mode */
#define PANEL_COUNT  2

void screen_guide_run(void) NGE_BANKED
{
    NGE_FILE *streams[PANEL_COUNT];
    uint8_t page = 0;
    uint8_t next;
    uint8_t c;

    screen_open();
    nge_util_assert(nge_panel_open_all(SCREEN_BUFFER_OVERLAY, panels, PANEL_COUNT, streams));
    screen_help("Left/right:page 1-4:go to SPACE:back");
    draw(page, streams);
    while ((c = nge_key_ascii(screen_wait_key())) != ' ') {
        next = turn(page, c);            /* the new page, or page if c isn't a page key */
        if (next != page) {
            page = next;
            draw(page, streams);
        }
    }
    nge_panel_close_all(streams, PANEL_COUNT);
    screen_close();
}
```

`turn()` is a pure function of `(page, key)`, testable on its own:
`CHAR_RIGHT`/`'l'` → `(page + 1) % GUIDE_PAGES`; `CHAR_LEFT`/`'h'` →
`(page + GUIDE_PAGES - 1) % GUIDE_PAGES`; `'1'`..`'0' + GUIDE_PAGES` → that
page. It is declared in `screen_internal.h` for the test, as
`screen_pc_ink()` is.

`draw()` clears both panels, writes the heading in yellow and `n/4` at
the right of the title panel, then `nge_puts(guide_pages[page].text, body)`.

### 5.2 The text (`screen_guide_pdata.c`, `constseg PAGE_105`)

```c
#define H  "\x10\x0D"   /* NGE_CTRL_INK, NGE_INK_YELLOW: a heading */
#define K  "\x10\x0E"   /* ...NGE_INK_CYAN: a key */
#define T  "\x10\x0C"   /* ...NGE_INK_WHITE: text */

typedef struct guide_page { const char *title; const char *text; } guide_page_t;
extern const guide_page_t guide_pages[GUIDE_PAGES];
```

The macros end each escape at a string boundary (`H "Moving"`), so the hex
escape can't swallow the letters that follow.

Draft text. The final wording is checked against the code in plan step 2.
Each line here is ≤ 40 characters.

**1 Commands**

```text
Moving
 arrows/hjkl  move; two keys: diagonal
 .            wait a turn
 < >          go up, down the stairs
Items
 g ,          pick up
 i            inventory
 w r          wear, take off
 e            exchange for what's worn
 d            drop
 u            use: drink, read, eat
Fighting and magic
 move into    attack
 f            fire a missile
 a            aim a wand
 m            cast a spell
 b            read a spell book: learn
The dungeon
 o            operate a lever, fountain
 c            close a door
 p            pick or unlock a lock
 s            search for traps
 D            disarm a trap
 x            look around
You
 C            your character
 ?            this help
 Q            quit the game
```

**2 Screens and prompts**

```text
Inventory (i)
 up/down jk   select an item
 w r e d u a  act on it (as listed)
 SPACE        back
Choosing an item (d w r u e a p)
 a-t          choose it
 up/down      select; ENTER chooses
Spells (m)
 letter       cast; or up/down, ENTER
Aiming, firing, looking (a f m x)
 arrows       move the cursor
 ENTER        choose the target
 other keys   back
Directions (c o p D)
 a direction  choose it
 other keys   back
This help (?)
 left/right   turn the page
 1-4          go to a page
 SPACE        back

The bottom line always shows the keys
you can use now.
```

**3 Goal**

```text
The crown
 The crown lies on depth 10, the
 deepest level of the dungeon.

To win
 1 Go down the stairs (>) to depth 10.
 2 Find the crown and pick it up (g).
 3 Climb back up (<). Leave by the up
   stairs on depth 1 with the crown.

 You can't leave without the crown.

If you drop the crown
 It stays on that depth. Levels are
 made anew on each visit, but the
 crown will be there when you return.

If you die
 The game is over.
```

**4 Abilities and stats**

```text
Abilities
 1-30; 10-11 is average. Each gives
 (score - 10) / 2 to its rolls:
 +1 at 12, -1 at 8.
 Str  resisting being held
 Dex  disarming traps, picking locks
 Con  HP at each level; resisting
      poison, weakness and blindness
 Int  MP at each level
 Wis  finding traps; resisting
      slowness and sleep
 Cha  no use yet
Stats
 HP     hit points: at 0 you die
 MP     mana: spells cost MP
 AC     armour class: higher is hit
        less often
 Speed  lower acts more often
 XP     from kills and gold: enough
        XP raises your level
Colours (C screen)
 cyan better than base, red worse
```

Facts behind page 4: `STATS_MOD()` (`stats.h`); Str: the Hold save
(`effects_pdata.c`); Dex: `feature_disarm_margin()`,
`feature_pick_works()`; Con: `pc.c` HP gain, and the saves against Poison,
Weakness and Blind; Int: `pc.c` MP gain; Wis: `feature_found_by()`, and
the saves against Slow and Sleep. Monsters' Int and Wis (AI brains,
waking) aren't the player's concern and aren't listed. Gold gives 1 XP a
piece (`pc_gain_gold()`, to check in step 2).

### 5.3 Play (`player_bank.c`, `game.c`)

- `player_command()`: `case '?': screen_guide_run(); break;` beside `'C'`
  (no turn used).
- `game_play_help()`: the new line (R6).

### 5.4 Memory

- **Common:** nothing new but the banked-call entry for
  `screen_guide_run()`, if the build makes one. Diff the map: a new
  `__*` or `asm_*` routine in common memory is a fault.
- **Page 104:** about 0.4K of code and literals (the titles, the help
  line). It must stay under 8K with asserts.
- **Page 105:** about 3K of text, its first use.
- `memory_map.md`: 104/105 becomes "the PC screen, the spell list and the
  help screen / the help screen's text".

## 6. Scope

In:

- R1-R15: the screen, its four pages, `?` in play, the new play help
  line;
- PC tests (section 7);
- docs: `ui_screens.md` (R24 built, R13 still later),
  `memory_map.md`, `src/game/README.md`, `screen_internal.h`'s file list,
  `todo.md`.

Later (new to-do items, added with this spec):

- **U1.5** Context help: `?` on each screen and prompt opens the page for
  it (ui_screens.md R13).
- **U1.6** Tell the player about `?` on the title or creation screen.
- **G1 (new topic G, Game rules)** Abilities in attacks: Str to melee hit and
  damage, Dex to ranged hit, and a use for Cha. The help page's text
  changes with it.

## 7. Tests

PC tests (`test_ui`):

- **Turning:** `turn()` for each key: right and `l` from page 0 to 1 and
  from 3 to 0; left and `h` from 0 to 3; `'1'`-`'4'` to pages 0-3; `'5'`,
  `'0'`, `'?'`, ENTER leave the page alone.
- **The text fits (R11):** for each page, walk the string as the stream
  would (skipping `NGE_CTRL_INK` and its byte): at most 29 lines, and at
  most 40 characters on any line. No other control code than INK and
  `\n`, and no line ends in a space.
- **The screen:** with fake keys `RIGHT`, `'4'`, `SPACE`: buffer 1 shows
  page 1's heading, then page 2's, then page 4's, with the right `n/4`;
  after SPACE buffer 0 is shown and unchanged, and play's help line is
  back.
- **`?` in play:** `player_command('?')` runs the screen and uses no turn
  (the player's timer is unchanged).

CSpect checks (the user runs them):

1. In play, the help line reads `?:help i:inv g:get <>:stairs`.
2. `?` opens the help screen at "Commands 1/4", whole at once, the game
   paused.
3. Right, right, right, right comes back to page 1. Left from page 1 goes
   to page 4. `3` goes to the Goal page.
4. A page turn redraws without a distracting paint (D4). Note how it
   looks.
5. Every page is readable: no clipped line, and the colours are as in
   R12.
6. SPACE: play's screen is back exactly, sprites and messages too, and no
   turn has passed.
7. Each key on the Commands page does what the page says (a read-through
   in play).

Hardware: the page turn time (check 4) at about 0.8 × CSpect's.

## 8. Risks

- **The text goes stale.** A rule or key change won't fail a test. D9's
  comments beside the rules are the guard, plus to-do item G1 naming the page.
- **Page 104 space.** About 2K left, so 0.4K should fit. If not, move the
  guide to pair 142/143 (D3).
- **`?` on the keyboard.** It is SYMBOL SHIFT + C. `nge_key_ascii()`
  already maps it (`nge_input_setup.c`), but no game key uses SYMBOL
  SHIFT yet, so check 2 is the first test of it in play.

## 9. Questions answered (2026-11-09, before writing)

- **Q1** The abilities page describes **the rules as they are now**. Cha
  is "no use yet" (D7, R10).
- **Q2** `?` works **in play only**, opening page 1. Context help is
  later, as U1.5 (D6).
- **Q3** Play's help line becomes **`?:help i:inv g:get <>:stairs`**
  (R6).

## 10. As built

Step 1 (2026-11-09): the screen, turning pages, and `?` in play, with a
one-line placeholder body per page.

- **Names carry the module's prefix** (G-NAME-1): `GUIDE_PAGES`,
  `guide_page_t`, `guide_pages[]` and `turn()` are `SCREEN_GUIDE_PAGES`,
  `screen_guide_page_t`, `screen_guide_pages[]` and `screen_guide_turn()`
  in `screen_internal.h`.
- **The titles are in page 105**, in `screen_guide_pages[]` with the
  text (as section 5.2's struct has them), not page 104's literals (as
  section 5.4 said). Page 104 holds only the help line's literal.
- **The title row:** the heading is centred in yellow, as on the other
  screens, and `n/4` is white in columns 37-39.
- **Memory:** page 104 grew by 0.55K (0.4K estimated), to 6.3K of 8K.
  Page 105 holds 164 bytes of placeholders. Common memory is unchanged.
- **`?` in play is tested on the PC:** `player_command()` with SYMBOL
  SHIFT + C runs the screen and leaves the player's turn unused. CSpect
  check 6 still covers play's screen coming back with sprites.

Step 2 (2026-11-09): the four pages' text, checked against the code.
Lines that changed from section 5.2's draft:

- **Commands:**
  - `move into` also opens a door: "attack; open a door".
  - `f` also throws ammunition: "fire or throw a missile".
  - `o` has no levers. It uses shrines, altars and fountains: "use a
    shrine or fountain".
  - `p` chooses a key or a lock pick: "unlock: a key or a pick".
  - `s` finds hidden features: "search for hidden traps".
- **Screens and prompts:**
  - In the inventory, exchange is `x`, not `e`: `w r x d u a`, "act on it
    (bottom line)".
  - The inventory gets a new line: ENTER does the obvious action ("use,
    aim, wear or take off", `obvious_action()`).
  - Choosing an item: "letter", not "a-t", because a list can be longer
    than the pack's 20.
  - Spells: `j k` move too.
  - The targeting cursor takes `hjkl` too, and SPACE chooses as ENTER
    does.
  - This help: "arrows h l".
- **Goal:** as drafted.
- **Abilities and stats:**
  - `STATS_MOD()` rounds down, so 9 gives -1 as 8 does: "(score - 10) /
    2, rounded down. 8-9 -1, 10-11 +0, 12-13 +1."
  - Wis: "finding hidden traps".
  - XP: "from kills, and 1 a gold piece" (`pc_gain_gold()` confirmed).
- **D9's comments** are also beside the inventory's, the spell list's and
  the cursor's keys, the direction prompts, and the stairs' crown check
  (`act_stairs.c`), as the pages describe those too.
- **Memory:** page 105 holds 2.7K. Page 104 and common memory are
  unchanged.

Spell books ([spells.md](spells.md) section 10, step 8, 2026-11-10):

- **Commands:** `b`, "read a spell book: learn", after `m`. The page has 28 of its 29 lines.
- **Screens and prompts:** the choosing heading is "Choosing an item (d w r e u a p b)". The
  inventory's key line stays `w r x d u a`, and ENTER's stays "use, aim, wear or take off":
  neither has room for `b` or "read" in 40 columns. The inventory's help line shows
  ` b:read` on a book.
