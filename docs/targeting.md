# Targeting: requirements and design

Status: implemented (sections 5 to 7 describe the code as built). All the
CSpect checks in section 7 passed on 2026-10-19.

Changed since by the game loop's redesign ([game_loop.md](game_loop.md)):
targeting is now one of several choosers. It waits with `game_wait_key()`
instead of a frame loop of its own, and the cursor is overlay sprite
`SCREEN_OVERLAY_CURSOR` (`game/screen.h`), whose record is in common memory
and which the frame start uploads. Sections 4 and 5 describe the first
version.

Covers the targeting mode: the player moves a cursor over the map and
chooses a tile or cancels. Also covers the first command that uses it,
look. Ranged attacks and spells will use it later.

## 1. Goal

Some commands need a tile, not a direction: look at something across the
room, fire at a monster, cast a spell at a spot. Each one asks the player to
choose the tile in the same way and gets the same kind of answer back: the
tile, or "no tile". What the tile is used for is up to the command.

## 2. Terms

| Term | Meaning |
|---|---|
| Targeting | The mode in which the player moves the cursor and chooses a tile or cancels: one call, `target_choose()` |
| Cursor | The sprite marking the tile being targeted |
| Caller | The command that starts targeting (look; later fire, cast) |
| Purpose | What the caller wants a tile for (`TARGET_LOOK`, later `TARGET_FIRE`): it picks the prompt and the help text |
| Origin | The tile the range is measured from: usually the player's |
| Start | The tile the cursor starts on: usually the origin. Later it might be the last target or the nearest monster |
| Range | The furthest distance from the origin a tile can be chosen at, in tiles, or `TARGET_RANGE_ANY` |
| Invalid tile | `NGE_MAP_X_INVALID`, `NGE_MAP_Y_INVALID` (0xFF, map/nge_map.h): the x and y given back when no tile is chosen. Off every map: maps are at most 80 x 48 |
| Within range | A tile is within range r of another when dx² + dy² ≤ r² + r (dx and dy the differences in column and row, made positive): `nge_map_within()`, the one range test in the game |
| Scroll margin | `TARGET_SCROLL_MARGIN` tiles (4): how close the cursor can come to the view's edge before the view scrolls |

## 3. Requirements

Numbered so tests and commits can name them. **Changed** marks a requirement
that differs from the original request, and **New** one that wasn't in it.
Section 4 gives the reasons.

### The cursor

- **R1** The movement keys (the cursor keys, or h j k l) move the cursor one
  tile. A held key repeats, at the keyboard's usual repeat rate.
- **R2** The cursor starts on the start tile given by the caller. The start
  must be on the map, and asserting it is the caller's job.
- **R3** **Changed:** the camera follows the cursor, scrolling only when it
  nears the view's edge. A move that would take the cursor within
  `TARGET_SCROLL_MARGIN` tiles of the view's edge scrolls the view one tile
  the same way, as far as the map edges allow. So the cursor reaches the
  edge of the view only where the view is at the edge of the map. The start
  tile is already in the view (the camera is centred on the player), so the
  view doesn't move at the start.
- **R4** The cursor can't leave the map. A move off the edge leaves it where
  it is.
- **R5** **Changed:** the cursor shows whether its tile can be chosen. It is
  green within range and red outside it. Range is measured from the origin,
  not the start, with `nge_map_within()` (section 2), so the range is a
  circle. With `TARGET_RANGE_ANY`, every tile is within range.
- **R5a** **New:** every range and distance test in the game uses
  `nge_map_within()`, so a range means the same tiles everywhere. That
  covers ranged attacks, spells, and later a monster's reach and sight
  radius.
- **R6** **New:** the cursor is drawn above the entities and is cut off at
  the edges of the map view, like them.

### Choosing and cancelling

- **R7** ENTER or SPACE chooses the cursor's tile. **Changed:** choosing a
  tile out of range is refused ("That is out of range."), and targeting goes
  on.
- **R8** Any other key cancels ("Never mind."), as the direction prompts do.
- **R9** **Changed:** `target_choose()` returns 1 when a tile is chosen, with
  its x and y in the context. It returns 0 on cancel, with x set to
  `NGE_MAP_X_INVALID` and y to `NGE_MAP_Y_INVALID`.

### The context

- **R10** **Changed:** the caller passes a context (`target_t`, on its own
  stack) that holds:
  - in: the purpose, the origin, the range, and the start (in x and y);
  - out: the chosen tile (in x and y), or the invalid tile.

### Around targeting

- **R11** **New:** no time passes while targeting. Targeting itself uses no
  turn. The caller decides whether its own command uses one.
- **R12** **New:** while targeting, the message window shows the purpose's
  prompt ("Look at what?") and the help bar shows its keys. On exit, the help
  bar shows play's help again.
- **R13** **New:** on exit, the cursor is hidden and the camera centres on
  the player again.
- **R14** **New:** key events waiting at entry and at exit are thrown away.
  Keys typed for play don't move the cursor, and a confirm key held down
  doesn't leak into play.
- **R15** **New:** the screen stays live while targeting: sprites keep
  animating, and the map and sprites are redrawn when the camera moves.

### Look

- **R16** The look command (key x) targets with purpose `TARGET_LOOK`. The
  origin and start are the player's tile, and the range is
  `TARGET_RANGE_ANY`.
- **R17** On a chosen tile, look gives one message for each entity on it,
  in ID order, in the existing form "You see a ring." The cursor isn't
  counted, and neither is the player ("You see yourself." if the tile is
  theirs). A tile with no entities gives "You see nothing there."
- **R18** Look uses no turn, whether a tile is chosen or not.

## 4. Changes to the suggested requirements, and why

- **Purpose instead of a prompt string (R10, R12).** The callers are banked
  code with their literals in their own page (player, page 114). If the
  caller passed a pointer to its prompt, targeting would read it from its
  own page, with the caller's page mapped out. A purpose number lets the
  targeting module own its prompts and help texts. It is also the one
  place to hang purpose-specific behaviour later (a fire purpose showing a
  line of fire) without changing the context.
- **Origin separate from start (R5, R10).** Today they are the same tile.
  Once firing starts the cursor on the last target or the nearest monster,
  range must still be measured from the player. Keeping them separate now
  costs two bytes and means no caller changes later.
- **One range test for the whole game (R5, R5a).** "Within range" needs a
  metric, and it must be the same everywhere: a bow with range 5 and a
  monster that sees 5 tiles should mean the same tiles. It is a function in
  nge's map module, because it is about map tiles, not about targeting.
  Section 5, "The range test", gives its cost.
  - **A rounded circle:** dx² + dy² ≤ r² + r keeps every tile closer than
    r + ½ (the tile-centre circle). Plain ≤ r² leaves single tiles poking
    out at the four axis points, and makes range 1 miss the diagonal
    neighbours (2 > 1). With + r, range 1 is all 8 neighbours.
  - **Not Chebyshev** (the larger of dx and dy): it is cheaper, but makes a
    square, so diagonal shots reach 41% further.
  - **Not Manhattan** (dx + dy): it matches 4-way walking, but makes a
    diamond, so diagonal shots are cut to 71%.
- **The invalid tile is nge's (R9).** The value that means "not a map tile"
  belongs with the map, so every module that needs it (targeting, later
  pathfinding, "no last target") uses the same one. Section 8 explains why
  it moves from `nge_tilemap.h`.
- **Out of range is refused, not returned (R7).** Every ranged caller would
  otherwise repeat the check and decide what to do. The red cursor already
  tells the player, so refusing keeps them in targeting to try again.
- **A return value as well as the invalid tile (R9).**
  `if (target_choose(&t))` says what it tests. x and y are still set to the
  invalid tile, as asked, so a stale x and y can't be mistaken for a choice.
- **The requirement numbers run on with no gap** (the list had no 4).
- **R11 to R15 were missing.** Each is something the code must do anyway, and
  writing it down makes it testable. Without R13 the view would stay where
  the cursor left it. Without R14, holding ENTER, or a typed-ahead key,
  would leak through.

### Blocking, not a game state

Agreed: targeting is a call that returns when the player has chosen, not a
state. As a state, the caller would have to be resumed on exit, through a
table of purposes and handlers in game.c. As a call, the caller's code reads
in order:
`if (target_choose(&t)) { do something with t.x, t.y }`.

The condition: game.h says nothing in a state's update waits. Targeting
becomes the one documented exception. It is allowed because:

- it is only called during the player's turn, when no time passes anyway;
- it runs its own frame loop that does the frame's presentation (wait for
  the blanking period, upload the sprites and the cursor, redraw the map,
  read the keyboard, build the next sprites) and none of its logic (no
  clock, no events, no change of level, no release of dead entities);
- it posts no events, so nothing builds up in the queue while it runs.

The frame's presentation is in one function in game.c
(`game_frame_start()`: the vblank wait and what was `pre_update()`), which
both the main loop and targeting call, so there is still only one copy.

The key handling is separate from the frame loop (`target_key()`, section
5), so the PC tests drive it with key events and no hardware. If targeting
ever needs to become a state, that function is its update as it stands.

## 5. Design

### Module

`game/target/`: `target.h`, and `target_bank.c` in a new page pair, 124/125
(code, with the purposes' prompts and help texts; 125 is unused). It is
called by player commands, in page 114, through banked calls. The deepest
banked call chain is main loop → `player_command()` → `target_choose()` →
`target_key()` → `player_key_direction()` or `nge_key_ascii()`: 4 levels,
well inside the 8-level limit.

```c
#define TARGET_RANGE_ANY      0xFF   /* every tile is within range */
#define TARGET_SCROLL_MARGIN  4      /* tiles kept between the cursor and the view's edge */

/* Purposes: what a tile is wanted for */
#define TARGET_LOOK           0
#define TARGET_PURPOSE_COUNT  1

typedef struct target {
    uint8_t purpose;    /* in: TARGET_* */
    uint8_t origin_x;   /* in: range is measured from here */
    uint8_t origin_y;
    uint8_t range;      /* in: tiles, or TARGET_RANGE_ANY */
    uint8_t x;          /* in: the cursor's start; out: the tile chosen, or NGE_MAP_X_INVALID */
    uint8_t y;          /* in: the cursor's start; out: the tile chosen, or NGE_MAP_Y_INVALID */
} target_t;

/* Run targeting until the player chooses (1) or cancels (0) */
uint8_t target_choose(target_t *t) NGE_BANKED;

/* One key event: TARGET_STEP_MOVING (the cursor moved, or didn't), _CHOSEN or _CANCELLED */
uint8_t target_key(target_t *t, nge_key_event_t ev) NGE_BANKED;

/* The cursor's tile is within range of the origin: nge_map_within(), or 1 with TARGET_RANGE_ANY */
uint8_t target_in_range(const target_t *t) NGE_BANKED;
```

In `map/nge_map.h`, in common memory, as any module may call it:

```c
#define NGE_MAP_X_INVALID  0xFF   /* not a map column: maps are at most NGE_MAP_STRIDE wide */
#define NGE_MAP_Y_INVALID  0xFF   /* not a map row: maps are at most NGE_MAP_HEIGHT_MAX high */

/* (x1, y1) is within range r of (x0, y0): dx² + dy² <= r² + r */
uint8_t nge_map_within(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t r);
```

It takes two tiles, not dx and dy, so no caller has to work out differences
and signs. It is a function, not a macro, because a macro would use each
argument twice.

`target_choose()`:

```
flush keys; show the purpose's prompt and help; place the cursor
loop:
    game_frame_start()          wait for vblank, sprites, map, keyboard
    upload the cursor sprite
    on a key event: step = target_key(t, ev)
        moved: scroll if the cursor is in the margin; recolour it (target_in_range)
    nge_render_update()         the entities' sprites through the moved camera
until step is not MOVING
hide the cursor; centre the camera on the player; play's help; flush keys
```

The movement keys mean the same here as in play, so `key_direction()`
moves out of `player_bank.c` into one shared function
(`player_key_direction()`, banked, page 114).

### The range test

`nge_map_within()` is cheap enough to be the only range test, on every path
that calls it:

- zsdcc compiles `(uint16_t)dx * dx` for `+zxn` into the Z80N's hardware
  multiply, not a library call. That's `mul d,e`, shown in the listing as
  `mlt de`, at 8 T-states. A test compile of the check gives three
  multiplies, an add and a 16-bit compare: about 150 T-states including the
  call. That is about 0.2% of a frame at 3.5 MHz, once per cursor move or
  monster turn.
- A table of squares is no faster here, so it isn't worth its bytes.
- It can't overflow. The biggest dx² + dy² on the biggest map is
  79² + 47² = 8450, and r² + r at r = 255 is 65280. Both fit in 16 bits, so
  `TARGET_RANGE_ANY` (255) needs no special case. A compile-time check in
  `nge_map.h` keeps (`NGE_MAP_STRIDE` − 1)² + (`NGE_MAP_HEIGHT_MAX` − 1)²
  within 16 bits if the map's size changes.
- On a plain Z80 (no `mul`), each multiply would be a library call costing
  a few hundred T-states. The Next build always has the instruction, and the
  PC tests don't care.

### Scrolling

After each move, the cursor is compared with the view's edges, in map
tiles:

- If it is less than `TARGET_SCROLL_MARGIN` from the left edge
  (`t->x - nge_map.cam_x < TARGET_SCROLL_MARGIN`), the module calls
  `nge_map_move_camera(-1, 0)`.
- The right, top and bottom edges work the same way.

`nge_map_move_camera()` already stops at the map's edges, so near the edge
of the map the cursor walks on to the edge of the view. The view is 28 x 27
tiles, so a margin of 4 leaves 20 x 19 tiles where the cursor moves without
scrolling.

### The cursor

- **The sprite:** hardware sprite 64, the first one after the render
  system's 0 to 63. Its number is a constant in `screen.h`, which owns the
  screen's layout and so the split of sprites. Higher sprite numbers are
  drawn on top, so it shows above the entities. The map's sprite clip window
  (`nge_map_clip_sprites()`) cuts it off at the view's edges, as it does
  for them (R6).
- **Not an entity:** an entity would take an entity slot, and would be found
  by `nge_location_find()` on its tile, so look would see it. It would also
  need cleaning up if a change of level ever ran during targeting. One
  sprite record, a local of `target_choose()`, has none of these problems:
  the stack is in common memory, where `nge_sprite_upload()` can read it.
  `nge_map_to_screen()` gives its position. On exit it is uploaded hidden
  straight away, not in the blanking period: one sprite, at worst one frame
  half drawn.
- **The patterns:** 8 (`DEMO_SPRITE_CURSOR_IN`, green) and 9
  (`DEMO_SPRITE_CURSOR_OUT`, red), in `sprite_data.asm`: the corners of an
  8 x 8 tile in the top-left of a 16 x 16 pattern, like the other
  entities' patterns. Corners rather than a whole frame, so what is on the
  tile still shows. `DEMO_SPRITE_PATTERNS` went from 8 to 10.
- **The help bar:** "Arrows: move  ENTER: look  Other: back" (SPACE also
  chooses, but there is no room to say so).

### Look

- `player_command()`: on x, the player's tile becomes the origin and the
  start, with range `TARGET_RANGE_ANY`, then
  `if (target_choose(&t)) messages_look(t.x, t.y);`. Look uses no
  `player_act()`: it is not an action, because it changes nothing in the
  world and takes no time (R18).
- `messages_look(x, y)` (page 118, next to `say_named()`): loops over
  `NGE_LOCATION_FOR_EACH_AT(id, LEVEL_AREA, x, y, COMPONENT_KIND)`. The kind
  mask leaves out anything that isn't a game entity. Then it gives the R17
  messages.
- The play help bar has no room for another key, so x goes in the title's
  hints, as d and f did: "i inv, d drop, f fire, x look, Q quit." fits one
  message line.
- `messages_look()` uses each kind's full name, so it says "You see a ring
  of health.".

## 6. Scope

In:

- the targeting module: `target_choose()`, `target_key()`,
  `target_in_range()`, the cursor sprite, the scrolling, and the look
  purpose's prompt and help;
- `nge_map_within()`, its compile-time check and its tests, in nge's map
  module;
- `NGE_MAP_X_INVALID` and `NGE_MAP_Y_INVALID` in `nge_map.h`, replacing
  `NGE_TILEMAP_X_INVALID` and `NGE_TILEMAP_Y_INVALID` (section 8);
- `game_frame_start()` taken out of `game_run()`, and `game_play_help()`
  out of play's entry action, so targeting can put play's help back.
  game.h's "nothing waits" paragraph and guideline P-STATE-1 name
  targeting as the second exception, after the title's name prompt;
- `player_key_direction()` shared;
- the look command and `messages_look()`;
- the cursor pattern(s);
- PC tests (section 7);
- `docs/memory_map.md`: page pair 124/125 and sprite 64.

Out, for later:

- fire and cast purposes. When they come, the fire direction prompt
  (`GAME_FIRE`) goes, and `act_t` gets a target tile;
- line of fire: the cursor coloured for a blocked path, the path drawn;
- a start on the nearest monster or the last target, and keys to cycle
  through the monsters in view;
- field of view: look sees every tile today, even unexplored ones. Once
  there is one, an unseen tile gives "You can't see there.";
- describing the tile itself (wall, floor) when nothing is on it;
- a cap on look's messages, for a pile bigger than the 4-line message
  window;
- "a" or "an" by name: all messages use "a" today.

## 7. Tests

PC tests (`tests/test_target.c`, with the screen faked as in `test_ui`):

- R2, R9: start, then cancel → returns 0, x = `NGE_MAP_X_INVALID`,
  y = `NGE_MAP_Y_INVALID`;
- R1, R4: moves in each direction, and moves at each map edge and corner
  stay on the map;
- R3: a move into the margin scrolls the camera by one tile, and the cursor
  stays at the margin. At the map's edge the camera stays put and the
  cursor reaches the view's edge;
- R5: `target_in_range()` is 1 with `TARGET_RANGE_ANY` anywhere on the map,
  and is measured from the origin when the start is elsewhere.

In `tests/test_map.c`, for `nge_map_within()` (R5a), with tiles given as
offsets from the origin:

- r = 0: only the tile itself;
- r = 1: all 8 neighbours are within, and (2, 0) isn't;
- r = 5 (limit 30): (5, 0), (5, 2) and (4, 3) are within. (6, 0), (5, 3)
  and (4, 4) aren't. These cases pin down the rounding;
- the same answers with the two tiles swapped, and with the target left of
  or above the origin;
- r = 255 from (0, 0) to (79, 47): within.
- R7: choosing in range returns `CHOSEN` with the tile. Choosing out of range
  returns `MOVING`, with the cursor where it was and the refusal shown;
- R8: a non-movement, non-confirm key → `CANCELLED`;
- R17: `messages_look()` on a tile with two items, on an empty tile, and on
  the player's tile (the messages, in order).

CSpect checks (reported separately):

- x shows the prompt, the help and a green cursor on the player;
- the cursor moves without scrolling in the middle of the view;
- the view scrolls once the cursor is within 4 tiles of an edge, and the
  entities' sprites stay on their tiles while it scrolls;
- the cursor stops at the map edges and is cut off at the view's edges;
- ENTER on a tile with items lists them, and any other key says
  "Never mind.";
- after exit: the camera is back on the player, play's help is shown, the
  turn counter hasn't changed, and a held ENTER does nothing.

## 8. Review notes on the changes to the requirements

- **The invalid constants are in the wrong module.**
  - `nge_tilemap.h` is the hardware tilemap: the 40 x 32 screen grid that
    the map view, stats and messages are drawn in.
  - The cursor's x and y are map tiles: `nge_map`'s grid, up to 80 x 48,
    seen through the view. A map column such as 60 isn't a tilemap column
    at all, so `NGE_TILEMAP_X_INVALID` names the wrong coordinates.
  - The constants move to `nge_map.h` as `NGE_MAP_X_INVALID` and
    `NGE_MAP_Y_INVALID`. A compile-time check that 0xFF is at least
    `NGE_MAP_STRIDE` and `NGE_MAP_HEIGHT_MAX` keeps them off every map if
    the map's size changes.
  - Nothing uses the tilemap constants yet.
