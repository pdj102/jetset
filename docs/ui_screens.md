# In-game screens: requirements and design

Status: implemented; the CSpect checks in section 7 passed (2026-10-26).
The opening time on hardware (check 8) is not measured yet. Agreed on 2026-10-26, with the questions answered in section 9.
Section 10 lists where the build differs from the design.

Builds on [ui_layer2.md](ui_layer2.md) (the UI on Layer 2) and
[cells_16.md](cells_16.md) (today's layout). It replaces the item screens
(`game/ui/`) and the screen module (`game/screen.{h,c}`) with a screen
framework, and adds the first new screen: one inventory screen in place of
the five item screens.

## 1. Goal

- One way to build an in-game screen: a list of panels, a help line, and
  the code that is particular to that screen. A new screen (stats, help,
  end of game, mini map) is a new file and a row in a table, not new
  plumbing.
- Screens cover the whole display, apart from the help line, and the game
  is paused while one is shown.
- Inventory, equip, take off, drop and use are one screen.
- Generic parts (the panels, the list, the Layer 2 buffers) are in nge;
  Jetset's screens and words are in the game.

## 2. Terms

| Term | Meaning |
|---|---|
| Cell | An 8 x 8 block of Layer 2: the display is 40 x 32 cells |
| Screen | Everything shown on the display above the help line (rows 0-30): the main screen, the inventory, the title... |
| Main screen | The screen of play: the map, the stats and the messages. The base screen, always under the others |
| Overlay screen | Any other screen: shown over the main screen, pausing the game |
| Panel | A rectangle of cells (`nge_rect_t`) in a screen, with a paper colour and a mode. A text panel has a Layer 2 stream; the map panel is a transparent hole onto the tilemap and the sprites |
| Help line | Row 31, always shown: what the keys do now (the "help pane" in the request) |
| Help screen | A future overlay screen of help pages |
| Layer 2 buffer | One 320 x 256 Layer 2 image (80K). Two are proposed (D2): 0 for the main screen, 1 for overlay screens |
| Paused | No game time passes, no animation advances, no sprite changes |

## 3. Requirements

Numbered so tests and commits can name them. **Changed** or **New** marks
a requirement that differs from the request, with the reason.

### Screens

- **R1** The UI is a set of screens. Each screen covers rows 0-30 of the
  display. Row 31 is the help line, shown under every screen.
- **R2** The main screen is the base screen: the map panel (28 x 26), the
  stats (12 x 26) and the messages (40 x 5), as today. It is shown
  whenever no other screen is.
- **R3** Opening an overlay screen pauses the game. It covers the whole
  of rows 0-30: the map, the stats and the messages are hidden, not shown
  through.
- **R4** Closing an overlay screen shows the main screen exactly as it
  was, including any messages and stats changed while it was covered,
  with no visible redraw.
- **R5 (New)** An overlay screen may open another (the inventory opens
  help; later the stats open help). Closing it shows the one beneath.
  At most 3 screens are open at once: main, one overlay, and one over
  that. *Reason: a context help screen must be reachable from any
  screen, and return to it.*
- **R6** A screen is divided into panels. Each panel is a `nge_rect_t`
  of cells. Panels of one screen don't overlap and together cover rows
  0-30 (gaps are filled with the screen's paper).
- **R7** A text panel draws through a Layer 2 stream
  (`io/nge_l2_stream.h`), with its own paper colour and either clip mode
  (fixed fields) or scroll mode (a log).
- **R8** The map panel is the one exception: a transparent hole in Layer
  2 over the tilemap stream and the hardware sprites, which the map and
  the render system draw (`map/nge_map.h`, `ecs/systems/nge_render.h`).
  Only the main screen has one now; the framework allows any screen to
  have one (the title's demo, later).
- **R9** A screen draws itself entirely from game state when it opens,
  and redraws only the panels it changes after that.
- **R10 (New)** A screen opens in at most 6 frames on hardware (about
  0.1 s), and nothing of it shows until it is complete. A cursor move
  redraws within 2 frames. *Reason: the full-screen fill is 80K of
  Layer 2; without a limit, opening a screen could visibly paint.*

### The help line

- **R11** The help line shows what the keys do in the current context:
  the screen, and within it the state (the inventory shows only the
  actions valid for the selected item).
- **R12** Every screen sets the help line when it opens, and again when
  its context changes. Closing a screen puts back the help of the screen
  beneath.
- **R13 (later)** From any screen, `?` opens the help screen at the
  page for the current context. The context is a number each screen
  passes with its help line, so help pages can be added without
  changing the screens.

### Pause and actions

- **R14** While an overlay screen is shown, no game time passes, no
  animation advances and no sprite moves (today, waiting for a key runs
  the game's frames, which advance the animations).
- **R15 (New)** A screen only chooses; the game acts. An action that uses
  a turn (equip, take off, drop, use) closes the screen and returns the
  action and its item to the command that opened it, which does it, as
  `ui_choose_item()` does today. *Reason: keeps one place where turns are
  used (game/game.h), and the world reacts to each action before the
  next. Option: see D7.*
- **R16 (New)** A screen offers only actions the game would allow: it
  asks the action in check mode (`ACT_CHECK`, game/actions/act.h). A key
  for an action that isn't allowed shows the refusal's reason on the
  screen, in the same words as in play. *Reason: the screen and the
  action can never disagree, and no rule is written twice.*

### The inventory screen

- **R17** One inventory screen replaces the inventory, drop, equip,
  take off and use screens.
- **R18** It lists what the player has in use (by slot) and what is in
  the pack (by ID), with the pack's count and capacity, and a cursor on
  one item.
- **R19 (Changed, Q5)** A details panel shows the selected item's name.
  What can be done with it is on the help line (R11). *Reason: more
  details (where it is worn, attack, AC, a description) come later, with
  item descriptions.*
- **R20** From it the player can equip, take off, drop and use the
  selected item, and leave.
- **R21** The main screen's item keys still work, as shortcuts: `i`
  opens the inventory to browse; `d`, `w`, `r`, `u` open it with that
  action chosen, listing only the items it applies to, so one letter
  finishes the command as today (D8).
- **R22** A list longer than its panel scrolls with the cursor (the
  player can carry 20 and use 10).

### Future screens (the framework must allow them; not built now)

- **R23 (later)** Stats screen: the player's full stats.
- **R24 (later)** Help screen: pages of help, by context (R13).
- **R25 (later)** Title screen: full screen, with an optional demo in a
  map panel (R8).
- **R26 (later)** Character generation: choices and the name prompt, a
  line editor on a panel stream.
- **R27 (later)** End of game: died, quit or escaped, with a summary.
- **R28 (later)** Mini map: the level's 60 x 40 tiles drawn on Layer 2,
  needing a pixel or half-cell fill in `layer2/` (generic, added then).

### Code

- **R29** The framework is built on nge (`layer2/`, `io/nge_l2_stream`,
  `io/nge_text`, `input/`) and duplicates none of it. nge gains only
  generic parts (section 5.1); nothing in nge names Jetset's screens,
  items or words.
- **R30** The framework is `game/ui/screen.{h,c}`; each screen has its own
  files in `game/ui/`. `game/screen.{h,c}` and `game/ui/ui*.{h,c}` go.
- **R31** Callers of the main screen don't change: `screen_message()`,
  `screen_show_*()`, `screen_overlay_*()` keep their names and
  meanings (as ui_layer2.md R3 did).
- **R32** PC tests for the nge parts and the screens (section 7).

## 4. Decisions

- **D1 A blocking screen run, not a state in the main loop.** An overlay
  screen runs its own loop (`screen_run()`) until it closes, and returns
  its answer, as the choosers do today (game/game.h). **Rejected:** screens
  as states of the main loop, entered and left by flags. That suits
  screens that run alongside play, which none do: they all pause it, and
  the commands that open them want an answer back.

- **D2 Two Layer 2 buffers: the main screen in 0, overlays in 1.** The
  Next shows Layer 2 from whichever 16K bank NextReg $12 names, so
  switching buffers is one register write, in the blanking period.
  Overlays are drawn in buffer 1 while buffer 0 is still shown, then
  shown in one step (R10). Closing switches back: buffer 0 was never
  touched, so the main screen needs no redraw (R4), and messages and
  stats written while an overlay is up go to buffer 0, unseen until it
  closes. Cost: 10 pages (26-35: 26-31 are already reserved for Layer 2,
  32-35 come from the spare 32-39). **Rejected:**
  - one buffer, redrawing the main screen on close: the message window
    has no history to redraw from (it would need a log of 5 lines of
    40, 200 bytes, and colours), and the stats, map hole and help would
    repaint visibly;
  - one buffer, saving the covered cells to spare pages and copying them
    back: about 80K copied each way, 4 frames, for what a register
    write does.

- **D3 Each Layer 2 stream draws on the buffer it was opened on.**
  `nge_open_layer2()` gains a buffer argument; the stream keeps it and
  passes it to the drawing calls. **Rejected:** a "current draw buffer"
  switch in `layer2/`: a message written during an overlay would land on
  the overlay.

- **D4 Dispatch by a switch on the screen's number, not a table of
  function pointers.** Each screen's code is banked in its page, and
  functions called through a pointer must be in common memory
  (N-BANK-3), where 823 bytes are free. `screen_run()` (banked) calls
  each screen's `NGE_BANKED` functions from a `switch`. **Rejected:** a
  per-screen table of handlers: it would need a common trampoline per
  handler.

- **D5 nge gets panels and a list, not screens.** The generic parts are
  what any game's screens need and what can be tested alone: Layer 2
  buffers, a paper control code, panels (open a table of rects as
  streams, fill, close), and a list with a cursor and scrolling (logic
  only). The screen stack, the help line and the run loop stay in the
  game: they call the game's frame and dispatch, and moving them into
  nge would need callbacks through pointers (D4). **Rejected:** an nge
  screen manager with a dispatch function, as `nge_event_process()`
  takes one: it would be a thin loop around game code, and the only
  reuse would be the stack of 3 numbers.

- **D6 Streams are opened when a screen opens and closed when it
  closes.** The main screen keeps its streams (stats, messages, help);
  an overlay opens one per text panel, plus its help line on buffer 1.
  `NGE_FILE_MAX` goes from 6 to 10 for the game (it is `#ifndef` in
  `io/nge_stream.h`, so the game's build sets it): 3 main + 2 help + 4
  overlay panels + the keyboard. About 17 bytes of common memory each:
  +68 bytes. **Rejected:** one stream moved from panel to panel (a "set
  area" call): saves the bytes, but each panel's cursor and ink would
  have to be put back by hand on every switch.

- **D7 A turn-using action closes the screen (R15).** **Rejected:** keeping
  the screen open and doing several actions in one visit: either the
  world doesn't react between them (equip three items for the price of
  one), or the world steps with the screen up and the player can't see
  what happens. See question Q1.

- **D8 One screen, two modes (R21).** *Browse* (from `i`): the cursor
  selects, keys name the action. *Choose* (from `d`, `w`, `r`, `u`): the
  action is fixed, the list holds only items it applies to, and a letter
  chooses at once (today's flow), or the cursor and ENTER. **Rejected:**
  dropping the shortcut keys (every item action becomes `i`, cursor, key:
  three keys for what is now two).

- **D9 In browse mode, letters are action keys and the cursor selects.**
  Items can't have letters there, as `d`, `e`, `r`, `u` would be both an
  item and an action. In choose mode letters are items (no action keys
  are needed). See question Q2.

- **D10 The highlighted row is drawn in paper colours.** The Layer 2
  stream gets a PAPER control code (`nge_text_paper()`), so the cursor row
  is drawn ink-on-highlight. The tilemap stream ignores it. **Rejected:**
  a sprite bar under Layer 2 (Layer 2 is on top, D5 of ui_layer2.md); a
  `>` marker only (allowed as well, but a bar reads better at 40
  columns).

- **D11 The framework's names keep the `screen_` prefix; the main
  screen's functions keep theirs (R31).** The framework is
  `game/ui/screen.h` (`screen_run()`, `screen_help()`), the main screen
  `game/ui/screen_main.h` (`screen_message()`, `screen_show_hp()`...), the
  inventory `game/ui/screen_inv.h`. File names are unique, as the PC tests
  need. **Rejected:** renaming the main screen's functions
  (`main_message()`...): 50 call sites change for no gain.

- **D12 The title moves to an overlay now; the end of game stays on the
  main screen until R27.** The title draws on `screen_panel` (the opaque
  panel over the map view), which goes. As an overlay on buffer 1 it
  keeps its text and name prompt, on its own panels. The end of game only
  writes a message and help on the main screen, which keeps working.

## 5. Design

### 5.1 nge

**Layer 2 buffers (`layer2/`, page 90)**

```c
#define NGE_LAYER2_BUFFERS  2   /* buffer b in pages 16 + 10b to 25 + 10b */

void nge_layer2_init(void) NGE_BANKED;              /* both buffers transparent, buffer 0 shown */
void nge_layer2_show_buffer(uint8_t b) NGE_BANKED;  /* NextReg $12; call in the blanking period */
void nge_layer2_fill(uint8_t b, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t colour) NGE_BANKED;
void nge_layer2_glyph(uint8_t b, uint8_t col, uint8_t row, uint8_t c, uint8_t ink, uint8_t paper) NGE_BANKED;
/* nge_layer2_text() and nge_layer2_scroll_up() take b first too */
```

Each drawing call adds `b * 10` to the page it maps. `memory_map.md`:
pages 16-35 Layer 2, 36-39 spare.

**Layer 2 stream (`io/nge_l2_stream`)**

- `nge_open_layer2(uint8_t buffer, const nge_rect_t *cells, uint8_t paper)`.
- The state gains `buffer` (1 byte).
- A PAPER control code (`NGE_CTRL_PAPER`, `nge_text_paper()` in
  `io/nge_text.h`): sets the paper for the characters that follow;
  clearing still fills with the paper set at open. The tilemap stream
  skips the code and its argument.

**Panels (`ui/nge_panel`, new folder, banked page 92)**

```c
typedef struct nge_panel {
    nge_rect_t area;     /* cells */
    uint8_t    paper;    /* RRRGGGBB, or NGE_LAYER2_TRANSPARENT for a hole (the map panel) */
    uint8_t    mode;     /* NGE_PANEL_CLIP, NGE_PANEL_SCROLL, NGE_PANEL_HOLE */
} nge_panel_t;

/* Fill every panel with its paper and open a stream on each text panel, into streams[] (NULL for a hole) */
uint8_t nge_panel_open(uint8_t buffer, const nge_panel_t *panels, uint8_t n, NGE_FILE **streams) NGE_BANKED;
void nge_panel_close(NGE_FILE **streams, uint8_t n) NGE_BANKED;

/* Text helpers on any stream of a panel: centred, and padded to a width (clears what was there) */
void nge_panel_centre(const nge_panel_t *p, uint8_t row, const char *s, NGE_FILE *stream) NGE_BANKED;
```

Panel tables are `const` in the screen's page; `nge_panel_open()` copies
each record before mapping anything (N-BANK-5). New layer: `ui/` may
include `io/` and `layer2/`; `ecs/` and the game may include `ui/`:

```
core <- hw <- tilemap, layer2, sprite, devices, input, sound <- io <- map, event, ui <- ecs <- game
```

**List (`ui/nge_list`, logic only, PC tested)**

```c
typedef struct nge_list {
    uint8_t count;   /* items */
    uint8_t rows;    /* rows the panel shows */
    uint8_t top;     /* first item shown */
    uint8_t cursor;  /* selected item */
} nge_list_t;

#define NGE_LIST_NONE    0   /* the key isn't the list's */
#define NGE_LIST_MOVED   1   /* the cursor moved: redraw the old and new rows */
#define NGE_LIST_SCROLLED 2  /* top changed: redraw every row */

void    nge_list_init(nge_list_t *l, uint8_t count, uint8_t rows);
uint8_t nge_list_key(nge_list_t *l, nge_key_event_t ev);   /* up, down, page up/down (with CAPS), home/end */
void    nge_list_set(nge_list_t *l, uint8_t item);         /* move the cursor, scrolling to show it */
```

The game draws the rows; the list never draws, so it needs no callback.

### 5.2 The game: the framework (`game/ui/screen.{h,c}`, `screen_bank.c`)

```c
#define SCREEN_MAIN   0
#define SCREEN_INV    1
#define SCREEN_TITLE  2
/* later: SCREEN_HELP, SCREEN_STATS, SCREEN_CHARGEN, SCREEN_OVER, SCREEN_MINIMAP */

typedef struct screen_result { uint8_t action; nge_entity_id_t item; } screen_result_t;

screen_result_t screen_run(uint8_t screen, uint8_t arg) NGE_BANKED;   /* open, run until closed, answer */
void screen_help(const char *text, uint8_t context);   /* common: banked callers pass literals (N-BANK-5) */
```

`screen_run()`:

1. Ends any open game frame (`game_frame_end()`), so the main screen's
   sprites are consistent when play resumes.
2. Pushes the screen (stack of 3), calls its `enter()` (the switch, D4):
   it opens its panels on buffer 1 and draws them, and sets the help.
3. In the blanking period, shows buffer 1.
4. Loop, paused (R14): `nge_frame_wait_vblank()`, `nge_input_update()`,
   one key event to the screen's `key()`, which returns *stay*, *close*
   or *close with an answer*. No render, map or animation update.
5. On close: the screen's `leave()` closes its streams; pop. If a screen
   is beneath on buffer 1 it is entered again (redrawn, R9); if it is the
   main screen, buffer 0 is shown and its help put back (R12).
6. Flushes the keys both ways (as `ui_bank.c` does now).

The help line: one stream on each buffer, in the screen's help colour;
`screen_help()` writes to the one on the buffer shown and keeps the
context number for R13.

### 5.3 The main screen (`game/ui/screen_main.{h,c}`)

Today's `game/screen.c`, less `screen_panel` and `screen_show_map()`
(the map panel is a hole for good, as nothing draws over it on buffer 0).
Its panels become a `nge_panel_t` table (the rects in the request), opened
on buffer 0 by `screen_init()`. Messages, stats and the overlay sprites
work as now. Common memory, as now.

### 5.4 The inventory screen (`game/ui/screen_inv_bank.c`, page 116)

```
col 0                      23 24              39
row 0  |          Worldly Possessions           |   title
row 1  |                                        |
row 2  | In use                   | short sword  |   list | details
       |  short sword (in hand)   |              |
       |  leather armour (worn)   |              |
       |                          |              |
       | Pack 2/20                |              |
       |  potion of healing       |              |
       |  ring of health          |              |
row 28 |                          |              |
row 29 | You must take that off first.          |   status (one line)
row 30 |                                        |
row 31 | j/k:select w:wear r:off d:drop u:use SPACE:back   help line
```

- **Panels:** title (0-1), list (0-23, 2-28), details (24-39, 2-28),
  status (29-30).
- **Items:** collected as `collect()` does now (in use by slot, then the
  pack); in choose mode, filtered by the action (today's `LIST_*`).
- **Browse keys:** cursor up/down (and `k`/`j`) move; `w` wear, `r`
  take off, `d` drop, `u` use, ENTER the item's obvious action (wear or
  take off, or use); `SPACE` back (Q3), on every screen.
- **Choose keys:** letters `a`-`t` choose at once; cursor and ENTER
  choose; `SPACE` back ("Never mind." as now).
- **Checks (R16):** each action key asks `act_do(action, &a, ACT_CHECK)`;
  refused, the reason's text goes to the status panel. The help line
  lists the actions allowed for the selected item, redrawn as the cursor
  moves.
- **Answer:** `{action, item}`; `player_bank.c`'s item commands call
  `screen_run(SCREEN_INV, mode)` and do the action, replacing
  `ui_choose_item()` and `ui_show_inventory()`.

### 5.5 The title (`game/ui/screen_title_bank.c`, page 104)

The title's text and the name prompt move from the map panel and the
message window to the title screen's own panels on buffer 1 (D12). The
line editor echoes to a panel stream, running the paused loop.

### 5.6 Memory

- **Common:** +68 bytes of stream slots (D6), +the buffer byte and PAPER
  handling in the Layer 2 stream (estimate 60 bytes); -the panel stream
  and `screen_show_map()`. To measure.
- **Pages:** 26-35 Layer 2 buffer 1 (`src/banks.inc` and
  `memory_map.md`: Layer 2 becomes 16-35, spare 36-39); 92/93 nge `ui`; 116/117 the
  framework and the inventory; 104/105 the title. Later screens: a pair
  each where they outgrow 116 (the help screen's text in an odd page).

## 6. Scope

In:

- R1-R12, R14-R22, R29-R32;
- nge: Layer 2 buffers, the PAPER code, `ui/` panels and list;
- the framework, the main screen, the inventory screen, the title moved
  to an overlay;
- PC tests (section 7);
- docs: `memory_map.md` (pages 26-39, 92/93), `src/nge/README.md` (`ui/`,
  the layer chain), `src/game/README.md`, `game/game.h` (choosers),
  `ui_layer2.md` (pointer to this spec).

Later:

- R13 and R24 (the help screen and `?`), R23 (stats), R25-R27 (title
  demo, character generation, end of game), R28 (mini map, with a pixel
  fill in `layer2/`);
- more in the details panel (Q5): where the item is worn, attack, AC, a
  description (there is no description table yet);
- colours other than black for panels.

## 7. Tests

PC tests:

- **Fake Layer 2** (`tests/support/test_layer2.c`): one grid per buffer,
  and which buffer is shown.
- **`test_l2_stream`:** a stream draws only on its buffer; PAPER sets the
  paper of the following characters, not of clearing.
- **`test_panel`** (new): open fills each panel, holes transparent;
  streams opened per text panel and closed; centring; too few slots
  fails cleanly.
- **`test_list`** (new): moves, ends, page up/down, scrolling to keep
  the cursor shown, a list shorter than its panel, an empty list.
- **`test_ui`:** the inventory in both modes: what each lists, the
  cursor, an action allowed and refused (the status text), the answer
  returned; closing shows buffer 0, untouched; a message written while
  the screen was up is on buffer 0 only.

CSpect and hardware checks:

1. `i`: the inventory appears whole, at once; the map, stats and
   messages are hidden. SPACE: play's screen is back exactly, sprites
   too.
2. Cursor moves in a list longer than the panel (carry 20 items).
3. Wear, take off, drop, use from browse mode; each closes the screen and
   takes a turn; messages appear in the message window.
4. `d`, `w`, `r`, `u`: choose mode, a letter finishes the command as
   before.
5. A refused action (wear a potion) shows the reason on the screen; the
   screen stays.
6. While the inventory is up, no monster moves and no animation plays.
7. The title and the name prompt work as before.
8. Opening time on hardware within R10.

## 8. Risks

- **NextReg $12 in CSpect:** CSpect is expected to honour the Layer 2
  bank register; check with a test pattern first (an early CSpect check),
  as the whole of D2 rests on it.
- **Common memory:** 823 bytes free in debug builds; D6's slots and the
  stream changes must be measured as soon as they are written.

## 9. Questions answered (2026-10-26)

- **Q1 (D7)** A turn-using action closes the inventory: **agreed.**
- **Q2 (D9)** In browse mode the cursor selects and letters are actions:
  **agreed.** In choose mode letters choose items.
- **Q3** **SPACE** leaves a screen, on every screen.
- **Q4 (D12)** The title moves to an overlay now: **agreed.**
- **Q5** The details panel shows **only the item's name** for now; more
  details later (R19, section 6).
- **Q6 (D2)** 10 pages for the second Layer 2 buffer (26-35), leaving
  36-39 spare: **agreed.**

## 10. As built: differences from the design

- **Screens are run functions, with no dispatch (D1, D4).** Each overlay
  screen is a banked function the command or the main loop calls
  directly (`screen_inv_run()`, `screen_title_run()`), as the choosers
  were, so there is no `switch` and no enter, key and leave hooks: a
  screen's state lives on its stack or in page data across its frames.
  The framework is four steps (`screen_open()`, `screen_frame()`,
  `screen_wait_key()`, `screen_close()`) and the help line; the "stack"
  is a count of open overlays, and a screen that opens another redraws
  itself when it returns. `screen_run()` and `screen_result_t` don't
  exist.
- **nge's `ui/` is in common memory, not page 92 (5.1).** A banked screen
  passes it tables and strings in its own page, and its list in its page
  data: banked in another page, `ui/` couldn't read them (N-BANK-5).
  About 250 bytes of common memory.
- **The list only moves (5.1).** `nge_list_move()` and `nge_list_set()`
  take a number of items; the game decides which keys move it (the cursor
  keys, and `j` and `k` in browse mode, where they aren't item letters).
  There are no page up or down, home or end keys.
- **PAPER sets the paper for clearing too (5.1).** Simpler than two
  papers: a highlighted row is PAPER, the text, clear to the end of the
  line (the bar), PAPER back.
- **No help context number yet (R13, 5.2).** `screen_help()` keeps its
  one argument; the number comes with the help screen.
- **The title is in `game/ui/screen_title_bank.c`, page 116 (5.5).** All
  the screens are in one page while they fit (4.7K of 8K); `game/title/`
  is gone and pages 104/105 are free.
- **The main screen's set-up and stats are banked (5.3),** in page 116
  (`screen_main_bank.c`), to make room in common memory: `screen_init()`,
  `screen_draw_stats()` and `screen_show_*()` are `NGE_BANKED`. The
  messages, the overlay sprites and `screen_map_view` (read by nge_map
  from other pages) stay common.
- **`screen_message_to()` (R16).** The inventory shows a refusal's reason
  by sending the message window's output to its status panel while it
  calls `messages_act_failed()`, so the words still come only from
  `messages/` (G-FLOW-3). Refusals made in a screen no longer reach the
  message window.
- **`game_pause_frame()` (R14).** The paused frame is in `game.c`, beside
  the other frames, and `screen_frame()` calls it: P-LOOP-1 now allows
  it, and a new pattern, P-SCREEN-1, lists what an overlay screen does.
- **The inventory's layout (5.4).** The list is columns 0-27 and the
  details 28-39, so the longest name with its slot ("ring of health
  (right)") fits on a row; there are no "In use" and "Pack" headings (the
  slot words show what is in use), and the pack's count is on the details
  panel's last row. The browse help line is `jk:select w:wear d:drop
  u:use SPACE:back`, listing only the allowed actions: at most 40
  characters. Its list, cursor and streams are page data (page 117, 42
  bytes), off the stack.
- **Choose mode ignores other keys.** A letter past the list, or any key
  but a letter, the cursor keys, ENTER and SPACE, does nothing; SPACE
  leaves without "Never mind.".
- **`NGE_FILE_MAX` is set in the Makefile** (`NGE_CONFIG`, every build
  mode); the PC tests use 12.
- **Memory:** common memory free in debug builds 428 bytes (546 before),
  in release builds 7,210. zsdcc warns "conditional flow changed by
  optimizer" on the buffer arithmetic in `nge_layer2_draw.c`
  (`map_col()`, `nge_layer2_show_buffer()`); the generated code was read
  and is correct.
