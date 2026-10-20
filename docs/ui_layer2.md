# The UI on Layer 2, and the pixel camera: requirements and design

Status: implemented; all the CSpect checks in section 7 passed (2026-10-21).
D1, D2, D3, D5 and D6 were agreed on 2026-10-20, and R2's panels stay
black. Section 8 lists where the build differs from the design.

Step 6 of [game_loop.md](game_loop.md). Moves the stats, the messages,
the help bar, the item screens and the title from the tilemap to Layer 2,
so the tilemap holds only the map. Adds a camera that
can stand between tiles, which the map and the sprites follow. The game
still moves the camera a whole tile at a time: smooth movement comes with
the animations (step 7), which this step makes possible.

The decision to use Layer 2 (D9) and the measurements behind this design
are in game_loop.md, section 10.

## 1. Goal

- The UI doesn't move when the map scrolls, because it is on another
  layer.
- The map can be shown from any pixel, not just from a tile's edge, with
  the sprites staying on their tiles.
- The game plays and looks as it does now. The UI is drawn in the same
  places, with the same text and colours.

## 2. Terms

| Term | Meaning |
|---|---|
| Layer 2 | The 320 x 256, 8-bit bitmap layer, above the sprites and the tilemap, in pages 16-25. Transparent where nothing is drawn |
| Cell | An 8 x 8 block of Layer 2, on the tilemap's grid: 40 x 32 cells |
| Layer 2 stream | An output stream on a rectangle of cells, drawing text with the same control codes as the tilemap stream (`io/nge_l2_stream.h`, new) |
| Text colour | `NGE_INK_*`: a stream's ink, mapped to a Layer 2 colour by the Layer 2 stream |
| Paper | A Layer 2 stream's background colour, set when it is opened |
| Pixel camera | The view's top-left in map pixels: `cam_x * 8 + fine_x`, `cam_y * 8 + fine_y`, where `fine_x` and `fine_y` are 0-7 |
| Fine offset | `fine_x`, `fine_y`: the tilemap's hardware scroll offset |

## 3. Requirements

Numbered so tests and commits can name them.

### The UI

- **R1** The stats window, the message window, the help bar, the item
  screens and the title are drawn on Layer 2. Nothing but the map is
  drawn on the tilemap.
- **R2** The screen looks as it does now: the same places (map view 28 x
  27 cells, stats 12 x 27, messages 40 x 4, help bar 40 x 1), the same
  text and the same text colours. The windows' backgrounds are opaque
  and black, as now (agreed: other colours are a later design change).
- **R3** The screen module's interface (`game/screen.h`) doesn't change
  for its callers: `screen_message()`, `screen_help()`, `screen_show_*()`,
  and the streams `screen_stats`, `screen_messages`, `screen_panel`. Code
  that writes to them (the item screens, the title's name prompt, its
  echo through `nge_gets()`) keeps working unchanged.
- **R4** The Layer 2 stream understands every control code the tilemap
  stream does (`nge_tm_stream.h`): clear to end of line, backspace, new
  line (scrolling at the bottom), clear, carriage return, ink, attribute,
  clip, at. It wraps and clips the same way.
- **R5** An item screen opens over the map window as an opaque panel, and
  closing it makes the panel transparent again. The map and the sprites
  underneath aren't touched, so nothing needs drawing again.
- **R6** The title covers the map window with an opaque panel, so no
  level shows (game_loop.md, R13).
- **R7** Within a frame, any number of updates to the stats, help and
  messages may be made. Each is drawn when it is made: there is no
  queue. A frame with one message and a stats change stays within a
  frame on hardware. More messages may take more frames (measured: about
  17 ms each, mostly the scroll), as nothing waits for the screen but
  the screen itself.

### The pixel camera

- **R8** `nge_map` keeps the camera in pixels. Today's functions, which
  set it by whole tiles (`nge_map_set_camera()`, `nge_map_move_camera()`,
  `nge_map_centre_camera()`), keep working, with a fine offset of 0. A new
  function sets it to any pixel, clamped so the view stays on the map.
- **R9** The map is drawn through the pixel camera: the tiles of the
  camera's tile position, plus one more column and row, with the
  tilemap's hardware scroll set to the fine offset in the same blanking
  period. The extra column and row lie under the stats and message
  panels.
- **R10** Sprites are drawn through the pixel camera, so they stay on
  their tiles at every fine offset. A sprite partly off the view's left
  or top edge shows the part that is in view: the render system and
  `nge_map_to_screen()` already work that way.
- **R11** In play, the camera still moves a whole tile at a time: the
  fine offset stays 0. Smooth movement belongs to the animations (step
  7). A development key lets the camera be moved a pixel at a time to
  check R9 and R10.

### Clean-up

- **R12** The DMA code comes out of `layer2/`: on hardware it is no faster
  than the CPU for any of Layer 2's operations (game_loop.md, section 10).
- **R13** The prototype (`game/proto/`, page pair 126/127, Shift+L on the
  title) is removed, now its questions are answered. Its findings are in
  game_loop.md.
- **R14** The tilemap stream's control-code functions get names that
  don't say "tilemap", since they work on any text stream:
  `nge_tm_ink()` becomes `nge_text_ink()`, and so on, in a new
  `io/nge_text.h` with the `NGE_CTRL_*` codes and the `NGE_INK_*` text
  colours. The tilemap stream keeps its own header for opening tilemap
  streams.

### Later

- **R15 (later, if text is too slow):** a Layer 2 stream buffers a line
  and draws it in one `nge_layer2_text()` call at the end of the line or
  on a flush, saving a banked call a character (D2). It needs the buffer
  out of common memory, or the room for it there.

## 4. Decisions

- **D1 A stream, not drawing calls.** The UI already writes through
  streams with control codes. A Layer 2 stream with the same codes (R4)
  keeps all of that code, R3's interface, and the title's line editor
  echo, unchanged. **Rejected:** having `screen.c` call
  `nge_layer2_text()` directly. That would mean changing every caller,
  and writing a second echo for the name prompt.
- **D2 The stream draws a character at a time.** Its output function is
  called through a pointer, so it must be in common memory (N-BANK-3). It
  calls the banked `nge_layer2_glyph()` for each character, which costs a
  banked call (a few hundred T-states) on top of the glyph's roughly
  2,000. **Rejected for now:** a line buffer per stream, drawn at the end
  of the line. It would save that call, but cost 40 bytes of common memory
  per stream, and only 784 bytes of common memory are free in debug
  builds. It is a future requirement (R15) if text drawing turns out too
  slow.
- **D3 No message queue (R7).** One scroll per message (about 11 ms on
  hardware) is affordable at the rate messages come. **Later, if it
  shows:** queue a frame's messages and scroll once for all of them. That
  needs the queue in a banked page, as there is no room in common memory.
- **D4 Inks stay text colours.** `NGE_INK_WHITE` and the others stay the
  names callers use. The tilemap stream maps them to palette banks (for
  the map's tiles), and the Layer 2 stream to RRRGGGBB colours (a 4-entry
  table).
- **D5 Panels cover; they don't hide or redraw.** Layer 2 is above the
  sprites, so an opaque panel covers the sprites under it. An item screen
  no longer hides the sprites (`nge_render_set_view(NGE_RENDER_NO_VIEW)`)
  or redraws the map when it closes. Making a 28 x 27-cell panel opaque
  or transparent is a fill of about 48K bytes: about 37 ms, 2 frames, when
  a screen opens or closes.
- **D6 The pixel camera now; smooth movement later (R11).** The pixel
  camera depends on the UI being off the tilemap, so it belongs with this
  step. Its first user, a camera following a sliding player, belongs to
  the animations. Building the mechanism now, checked with a development
  key, keeps step 7 about animation only.
- **D7 The tilemap is redrawn whole on a tile move,** as now: a copy of
  29 x 28 tiles, about 2 ms. **Later, if needed:** use the tilemap's
  wrap-around to draw only the new column or row.

## 5. Design

### 5.1 nge: Layer 2 (`layer2/`, page 90)

As the prototype left it, without the DMA (R12):

- `nge_layer2_init()`: 320 x 256, pages 16-25, identity palette, the
  font's column bitmasks, cleared to transparent, shown, layer order LSU.
  Called once by `main()`, after `screen_init()`.
- `nge_layer2_fill()`, `nge_layer2_glyph()`, `nge_layer2_text()`,
  `nge_layer2_scroll_up()`: cells by value; strings in common memory.
- `nge_layer2_set_dma()` goes.

### 5.2 nge: the Layer 2 stream (`io/nge_l2_stream.{h,c}`, common)

```c
/* An output stream on a rectangle of Layer 2 cells, with paper as its background colour */
NGE_FILE *nge_open_layer2(const nge_rect_t *cells, uint8_t paper);
```

- **State,** in the stream union (`nge_stream_dev.h`), like the tilemap
  stream's: the area, the cursor, the ink colour, the paper, the escape
  state, clip mode. About 10 bytes per stream.
- **Output** (`l2_put()`, common):
  - a printable character: `nge_layer2_glyph()` at the cursor;
  - the control codes as R4 lists them. Clear and clear to end of line are
    `nge_layer2_fill()` with the paper colour. A new line at the bottom,
    in scroll mode, is `nge_layer2_scroll_up()`.
  - Wrapping and clipping work exactly as in `nge_tm_stream.c`: a delayed
    wrap, and clip mode dropping characters past the edge.
- **Ink:** `NGE_INK_*` (12-15) maps to RRRGGGBB through a 4-entry table;
  any other value is white.
- **Layers:** `io/` may include `layer2/` (N-LAYER-1: `layer2` is with
  `tilemap`, below `io`).

### 5.3 nge: shared text codes (`io/nge_text.h`, R14)

`NGE_CTRL_*`, `NGE_INK_*`, and the control-code helpers
`nge_text_ink()`, `nge_text_attr()`, `nge_text_at()`, `nge_text_cls()`,
`nge_text_clreol()` and `nge_text_clip()`. These are the bodies of
today's `nge_tm_*()`, which only `putc` a code, so they work on any
stream. The callers in `screen.c`, `ui_bank.c` and `title_bank.c` are
renamed. `nge_tm_stream.h` keeps `nge_open_tilemap()` and
`nge_tm_load_ink_palette()`.

### 5.4 The game: the screen module

- **Streams:** `screen_init()` opens the four streams with
  `nge_open_layer2()` in the same rectangles, then fills the stats,
  messages and help areas with their paper colour. The map view stays
  transparent.
- **The tilemap:** `screen_init()` still sets the tilemap up and loads its
  palettes, for the map. It no longer clears it for text.
- **`main()`:** calls `nge_layer2_init()` after `screen_init()`.

### 5.5 The game: item screens and the title

- **`ui_bank.c`:**
  - `open_screen()` fills the map window opaque (the panel's paper) and
    draws the list as now;
  - `close_screen()` fills it transparent and shows play's help;
  - no `nge_render_set_view()`, and no `game_play_redraw()`.
- **`game_play_redraw()`:** only for a new level (the stats, the depth,
  the map, the sprites' area). The item screens call `game_play_help()`
  instead.
- **`title_bank.c`:** `show_title()` fills the map window opaque instead
  of clearing it on the tilemap, and starting a game makes it transparent
  (in `play_start()`, through `game_play_redraw()`).

### 5.6 nge: the pixel camera (`map/`)

- **State:** `nge_map` gains `fine_x` and `fine_y` (0-7), next to `cam_x`
  and `cam_y`.
- **Setting it:**
  - `nge_map_set_camera_px(uint16_t x, uint16_t y)` sets both parts,
    clamped to the map;
  - the tile functions set the fine offset to 0.
- **Redrawing:**
  - `dirty` still means "redraw the tiles": set when `cam_x` or `cam_y`
    changes, or a tile changes;
  - a second flag, `scroll_dirty`, means "write the fine offset", set
    when `fine_x` or `fine_y` changes;
  - `nge_map_update()`, in the blanking period, does whichever is needed:
    redraw, and `nge_tilemap_set_scroll(fine_x, fine_y)`.
- **Drawing:** the view's tiles plus one column and one row, the extra
  ones only where the map has them. `nge_map_init()` asserts the window
  leaves room for them on the tilemap (x2 + 1 < 40, y2 + 1 < 32).
- **`nge_map_to_screen()`** subtracts the pixel camera.

### 5.7 nge: the render system

`collect()` takes the view's position as the pixel camera: `cam_x * 8 +
fine_x` instead of `cam_x * 8`. Nothing else changes (R10).

### 5.8 The game: checking the pixel camera (R11)

A development key (`GAME_DEV_KIT`, listed in `dev.h`), V for view, runs a
loop of the game's frames:
- while an arrow key is held, the camera moves a pixel a frame;
- any other key centres the camera on the player again and returns.

No time passes.

### 5.9 Memory

- **Common memory:** about +250 bytes for the Layer 2 stream's output
  function and colour table, and -100 bytes or so from the prototype's
  library routines (`strncat`, the 32-bit arithmetic), which go with it.
  The debug build has 784 bytes free now.
- **Pages:** 126/127 are freed. Page 90 loses the DMA code.

## 6. Scope

In:

- R1 to R14, as in section 5;
- PC tests (section 7);
- docs:
  - `docs/memory_map.md`: pages 126/127 freed, the Layer 2 stream;
  - `src/nge/README.md`: `io/` gains the Layer 2 stream and the text
    codes; `layer2/` loses "prototype stage";
  - `src/game/README.md`, `dev.h`: the V key; the prototype gone;
  - `game_loop.md`: step 6 done.

Out, for later:

- smooth camera movement, and the player's slide (step 7);
- a message queue with one scroll per frame (D3);
- a line buffer in the Layer 2 stream (R15, D2);
- drawing only the new column or row on a tile move (D7);
- panel colours other than black (R2), and any change to the layout.

## 7. Tests

PC tests:

- **The fake Layer 2:** `layer2/nge_layer2_draw.c` writes to slot 7 by
  address, which would crash on the PC. So it joins `LIB_EXCLUDE` in
  `tests/Makefile`, like `nge_cspect_trap.c`. `tests/support` gets a fake
  with the same functions, recording into a 40 x 32 grid of characters
  and colours, with a reader like `test_tilemap_row()`. Every test that
  draws the screen uses it.
- **`test_l2_stream`** (new): the Layer 2 stream on the fake.
  - Each control code (R4).
  - Wrapping and the delayed wrap, clip mode, and scrolling at the
    bottom: compared with the same text on a tilemap stream
    (`test_tm_stream`'s cases).
  - Ink colours mapped through the table; paper on clear.
- **`test_map`:** `nge_map_set_camera_px()` clamping. The fine offset and
  the two flags. The tile functions reset the fine offset. The drawn
  area's extra column and row, and none past the map's edge.
  `nge_map_to_screen()` through a fine offset.
- **`test_render`:** a sprite's position through a fine offset, and
  partly off the left and top edges.
- **`test_ui`, `test_target`:** as now. The screens' text is read from
  the faked Layer 2 grid instead of the tilemap.

`layer2/` itself (the drawing in slot 7, the assembly) is checked on
hardware and CSpect, not on the PC, like the device drivers (N-TEST-1).

CSpect and hardware checks:

1. The title covers the map window. The stats, messages and help look as
   they do now.
2. Every screen and prompt works: the inventory and the four choosers,
   close, fire, look. Closing a screen shows the map and sprites as they
   were, with nothing redrawn.
3. Messages scroll; the name prompt's typing and backspace work.
4. The V key: the map and sprites move together a pixel at a time, in all
   directions, up to the map's edges. Sprites show partly at the view's
   left and top edges. The panels never move.
5. Moving, stairs and game over work as before.

## 8. As built: differences from the design

- **`screen_init()` calls `nge_layer2_init()`,** not `main()` (5.4):
  setting Layer 2 up clears it, so it must come before the windows' black
  backgrounds are filled, and the screen module owns that order.
- **nge is linked as a library** (`obj/nge.lib`, built by the Makefile).
  The linker takes every object file it is given, used or not, so the
  tilemap stream (about 2K with its assert messages), which the game no
  longer uses, overflowed common memory in debug builds. As a library,
  only the nge modules something uses are linked. Debug builds now have
  823 bytes free, release builds 7,473.
- **The tilemap's ink palette loader is a file of its own**
  (`io/nge_tm_palette.c`), so loading the map's colours doesn't need the
  tilemap stream.
- **The Layer 2 stream is about 1.5K** of common memory, more than 5.9's
  estimate of 250 bytes: zsdcc's code for reaching struct fields through a
  pointer is bulky. If common memory gets tight, its control-code handling
  (everything but the printable-character fast path) can move to a banked
  page.
- **A fine offset needs room past the window,** and that is asserted when
  a fine offset is set (`nge_map_set_camera_px()`), not when the map is
  set up (5.6): a map whose window fills the tilemap (as some PC tests
  use) works as before, with whole tiles only. The extra column and row
  are drawn only where the tilemap has room.
- **The title no longer hides the sprites:** its opaque panel covers them,
  as the item screens' does (D5).
- **The development key V** is in `player_bank.c` (`pan_view()`), with the
  other development keys.
- **The fake Layer 2** (`tests/support/test_layer2.c`) checks only that a
  call's cells are in range, counting a check only when they aren't, so a
  test's check count stays its own.
