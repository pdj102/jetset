# 16 x 16 cells: requirements and design

Status: implemented; CSpect checks 1-8 passed, with no tearing (2026-10-21).
Check 8 on hardware is still to do: hardware runs faster than CSpect, so it
is expected to pass. The decisions in section 4 were agreed as proposed on
2026-10-21. Section 8 lists where the build differs from the design.

Changes the size of the game's grid cell from 8 x 8 pixels to 16 x 16, so
map tiles and entities can have better graphics. The game still plays on
one grid: a cell holds one map tile and at most one creature, and every
rule (moving, blocking, generation, targeting, ranges) stays in cells.
Only drawing and the camera deal in pixels.

## 1. Goal

- Each map cell and each entity is drawn 16 x 16 pixels.
- The game's rules don't change: they work in cells, as now.
- The art in this step is a placeholder: today's 8 x 8 art, pixel-doubled.
  New art can then be dropped in without code changes.

## 2. Terms

| Term | Meaning |
|---|---|
| Cell | One square of the game's grid: a map position, `nge_location`'s x and y. Now 16 x 16 pixels |
| Hardware tile | One 8 x 8 tile of the Next's tilemap (40 x 32 of them), and one tile definition (32 bytes, 4 bits a pixel) |
| Metatile | The graphic of one cell: 2 x 2 hardware tiles. Metatile m is hardware tiles 4m to 4m+3 (D2) |
| Cell tile | What the map stores for a cell: `nge_tile_t`, with a metatile number as its `tile_id` and the attribute (palette offset) for all four hardware tiles |
| Window | The map's rectangle on the tilemap, in hardware tiles (as now) |
| View | The cells the window shows: `view_w` x `view_h`, now half the window's size each way |

## 3. Requirements

### The grid

- **R1** The map, the block table, entity locations, the generator,
  targeting and every action keep working in cells. No game rule changes
  because a cell is bigger.
- **R2** One constant gives the cell's size in pixels (16), and another
  the hardware tile's (8). Every use of today's `NGE_MAP_TILE_PIXELS` uses
  the one it means: cell pixels for the camera, locations, slides and
  sprites; hardware tile pixels for the window, the clip window and
  tilemap coordinates.
- **R3** A level keeps its size in cells (60 x 40, `LEVEL_WIDTH`,
  `LEVEL_HEIGHT`), so it is now 960 x 640 pixels (D5).

### Drawing the map

- **R4** The map stores one cell tile per cell, as now (2 bytes, 80 x 48
  at most, in page 51). Drawing writes each cell as its metatile's four
  hardware tiles, all with the cell's attribute.
- **R5** The view is 14 x 13 cells: a window of 28 x 26 hardware tiles at
  the top left, as today's 28-column map window less its bottom row
  (D3, D4).
- **R6** The pixel camera works with 16-pixel cells: `fine_x` and `fine_y`
  are 0-15. Drawing adds a column and a row of cells past the view (two
  hardware columns and rows), where the map has them, for a fine offset
  to show. They lie under the stats and message panels.
- **R7** Redrawing the whole view on a cell move takes no longer than
  today's (about 2 ms on hardware), or is moved to assembly until it does.

### Entities

- **R8** An entity's sprite is a full 16 x 16 pattern, placed on its cell
  with no offset. Locations' fine positions are 0-15.
- **R9** A slide still takes 4 frames a cell, the key repeat rate, so the
  game's pace doesn't change: 4 pixels a frame (D6). A lunge goes 8
  pixels, at 4 a frame.
- **R10** The targeting cursor's two patterns mark the corners of a
  16 x 16 cell.

### The screen

- **R11** The UI keeps its places and sizes, except the map window's
  bottom row (hardware row 26), which joins the message window: messages
  get 5 lines, rows 26-30 (D4).

### Graphics

- **R12** The map's graphics are metatiles in the tile definitions at
  `$4A00`: 256 hardware tiles, so 64 metatiles. Metatile 0 is transparent
  (rock). The tilemap font is no longer in the tile definitions: nothing
  draws text on the tilemap since the UI moved to Layer 2.
- **R13** The placeholder metatiles are today's map glyphs (`.`, `,`,
  `#`, `-`, `|`, `+`, `'`, `<`, `>`), pixel-doubled, in the same ink
  colours (palette banks, as now). The placeholder sprites are today's
  8 x 8 sprites, pixel-doubled into the whole 16 x 16 pattern.
- **R14** The map's tile numbers become metatile numbers, named in one
  place (`level_internal.h`, `level.h`): they are no longer ASCII codes.

### Later

- **R15 (later, if 64 metatiles are too few):** the tile definitions move
  to bank 7 (bit 7 of register `0x6F`): 512 hardware tiles, 128
  metatiles. That would also free `$4A00`-`$69FF` in bank 5 for the
  program: 8K more common memory. Needs checking on CSpect and hardware
  first (D2).
- **R16 (later):** walls drawn by their neighbours (corners, joins), when
  real art comes. The generator would store which wall piece a cell has;
  the rules see only "wall".
- **R17 (later):** the sprite format for real art: 8-bit patterns (64 in
  the pattern memory) or 4-bit (128, 16 colours each through a palette
  offset). This step keeps today's 8-bit patterns (D7).

## 4. Decisions

- **D1 One grid, in 16-pixel cells.** Everything that has a position
  works in cells; only drawing knows a cell's size. **Rejected:** 8 x 8
  map tiles with 16 x 16 entities. An entity would cover 2 x 2 map tiles,
  and every check (blocking, moving, generation, targeting, look) would
  have to handle footprints, for no gain in play.
- **D2 Metatile m is hardware tiles 4m to 4m+3** (top left, top right,
  bottom left, bottom right), so drawing works out the four tile numbers
  from m with no table. With 512-tile mode, as now, a cell's tile number
  0-127 maps to hardware tiles 0-511 (bit 8 is bit 0 of the attribute).
  Today's tile definition space holds 256 hardware tiles: 64 metatiles,
  enough for the dungeon's tiles and colour variants by palette.
  **Rejected for now:** a metatile table (four hardware tiles per
  metatile, about 1K for 128 metatiles). It lets metatiles share hardware
  tiles, but costs a lookup per cell and memory; worth it only if tile
  space runs short, and R15 is the first answer to that. **Rejected:**
  storing the map at hardware tile resolution (four entries per cell):
  four times the memory (30K at 80 x 48) and every rule would read a
  quarter of a cell.
- **D3 The view: 14 x 13 cells**, from today's 28 x 27-tile map window.
  This is a quarter of what the player sees now (28 x 27 cells). It is
  the largest view the layout allows without shrinking the stats panel.
  **Rejected:** a wider map window (for example 30 columns, 15 cells)
  with a 10-column stats panel: the stats lines would need shortening.
  Worth revisiting once it has been played.
- **D4 The spare row goes to the messages.** The map window loses its
  bottom row of hardware tiles (a 27-row window can't hold whole cells).
  Giving it to the message window means one more line of messages.
  **Rejected:** a blank row between the map and the messages: it wastes
  the row.
- **D5 Levels keep their size in cells (R3).** The game plays the same;
  only less of the level is on screen, and the camera scrolls more often.
  A 60 x 40 level is about 4 x 3 views. **Open, after playing:** smaller
  levels, or smaller rooms, if exploring feels slow. That would be a
  change to the generator only.
- **D6 Slides keep their time, not their speed (R9).** 4 pixels a frame
  keeps a cell move at 4 frames, matching the key repeat, so holding a
  key moves as fast as now. **Rejected:** 2 pixels a frame, 8 frames a
  cell: smoother, but halves walking speed and lengthens every turn the
  world waits on an animation.
- **D7 This step changes the size, not the art.** The placeholders are
  today's art doubled (R13), so the change can be checked on its own,
  and the art can come after, without code changes. The sprite format
  (R17) is the art's decision.
- **D8 nge supports 16-pixel cells only.** The cell size is a constant,
  but nothing keeps 8-pixel cells working: one drawing path to test and
  keep fast. **Rejected:** a build option for 1 x 1 or 2 x 2 cells: two
  drawing paths and two sets of tests for one game.
- **D9 Two steps: the split, then the size.** First R2's two constants
  with both at 8 (a refactor: the build behaves exactly as before, and
  the tests pass unchanged). Then cells become 16. The first step shows
  which uses of `NGE_MAP_TILE_PIXELS` meant which.

## 5. Design

### 5.1 nge: sizes (`map/nge_map.h`, `ecs/components/nge_location.h`)

```c
/** Pixels per cell, each way. */
#define NGE_MAP_CELL_PIXELS  16

/** Hardware tiles per cell, each way. */
#define NGE_MAP_CELL_TILES   2
```

- `NGE_MAP_TILE_PIXELS` goes. Hardware tile pixels are a tilemap
  constant: `NGE_TILEMAP_TILE_PIXELS` (8), in `tilemap/nge_tilemap.h`.
- `NGE_LOCATION_FINE` is `NGE_MAP_CELL_PIXELS`.
- `NGE_MAP_CAMERA_PX()`, `NGE_MAP_CAMERA_PY()`: cell pixels.

The uses today (section 7 lists the tests):

| Where | Becomes |
|---|---|
| `nge_map.c`: camera clamping and centring, `nge_map_set_camera_px()` | cell pixels |
| `nge_map.c`: `nge_map_to_screen()` | window in tile pixels, map and camera in cell pixels |
| `nge_map_setup.c`: the clip window | tile pixels |
| `nge_render.c`: the view's size, the window's position | cell pixels, tile pixels |
| `anim_bank.c`: the slide | cell pixels |
| `target_bank.c`: the cursor's position | cell pixels |

### 5.2 nge: the map (`map/`)

- **Set-up:** `nge_map_init()` takes the window in hardware tiles, as
  now, and asserts its width and height are even. `view_w` and `view_h`
  are half of them: cells.
- **State:** `cam_x`, `cam_y` in cells; `fine_x`, `fine_y` 0-15. The
  tilemap's hardware scroll is set to the fine offset, as now.
- **Room for a fine offset:** asserted when a fine offset is set, as
  now, but for two hardware columns and rows (`x2 + 2 < 40`,
  `y2 + 2 < 32`). With the window at columns 0-27 and rows 0-25, the
  extra cells are at columns 28-29 and rows 26-27.
- **`nge_map_set()`:** unchanged (a cell's change in view marks the view
  dirty).
- **`nge_map_draw()`:** for each cell row of the view (plus one), writes
  two tilemap rows. For a cell with tile number m and attribute a:

  | Position | Tile ID (low 8 bits) | Attribute |
  |---|---|---|
  | top left | `4m` | `a` with bit 0 set to bit 6 of m |
  | top right | `4m + 1` | the same |
  | bottom left | `4m + 2` | the same |
  | bottom right | `4m + 3` | the same |

  With 64 metatiles (R12) bit 6 of m is always 0, but drawing sets it so
  R15 needs no change here. Written in C first. If it is slower than R7
  allows, the inner loop moves to assembly.
- **Size:** 15 x 14 cells drawn, 840 hardware tiles (1,680 bytes):
  slightly more than today's 29 x 28 (812).

### 5.3 nge: the render system and locations

- `collect()`'s visibility test uses the view in cell pixels; the sprite
  position is `NGE_LOCATION_PX(id) - NGE_MAP_CAMERA_PX()`, as now.
- `nge_location_set_fine()` and `nge_location_move()` (a move in
  pixels) work in 0-15 through `NGE_LOCATION_FINE`.
- Slide and lunge offsets are `int8_t`: a cell is 16 pixels, well within
  range.

### 5.4 The game

- **`screen.c`:** `screen_map_view` is columns 0-27, rows 0-25.
  `message_area` is rows 26-30 (R11). `screen_map_view`'s users (the
  title's and item screens' panels) cover 28 x 26 tiles.
- **`level_internal.h`, `level.h`:** the tile numbers become metatile
  numbers (R14). Proposed order, so related tiles sit together:

  | m | Tile |
  |---|---|
  | 0 | rock (transparent) |
  | 1, 2 | floor, floor 2 |
  | 3 | corridor |
  | 4, 5, 6 | wall horizontal, wall vertical, corner |
  | 7, 8 | door shut, door open |
  | 9, 10 | stairs up, stairs down |

  The colours stay attributes (`LEVEL_FLOOR_INK` and the features'
  looks), as now.
- **`anim.h`:** `ANIM_SLIDE_SPEED` 4, `ANIM_LUNGE_PIXELS` 8,
  `ANIM_LUNGE_SPEED` 4 (R9).
- **`target_bank.c`:** the cursor's position in cell pixels. Its camera
  moves at the view's edge are in cells, as now.

### 5.5 Graphics data

- **`tile_defs_data.asm`:** 64 metatiles, 4 hardware tiles each, in
  metatile order. Unused metatiles are transparent. The font is removed.
- **`sprite_data.asm`:** the 8 x 8 sprites (player, potion, sword, rat,
  ring) pixel-doubled to fill their patterns; the cursors redrawn as the
  corners of 16 x 16. The 16 x 16 walker is unchanged.
- **The doubling** is done once, by a script in `scripts/`
  (`double_art.py`: reads today's asm, writes the doubled asm), and its
  output is committed. The script is kept for reference, but the asm is
  the source from then on: new art replaces it.

### 5.6 Memory

- **Common memory:** about the same. `nge_map_draw()` grows by perhaps 100
  bytes (more if it moves to assembly), nothing else changes size.
- **Bank 5:** the tile definitions keep their 8K at `$4A00`.
- **Sprite patterns:** unchanged in number (10 of 64 8-bit patterns).

## 6. Scope

In:

- R1 to R14, in the two steps of D9;
- PC tests (section 7);
- docs:
  - `docs/memory_map.md`: the tile definitions hold metatiles, not the
    font;
  - `src/nge/README.md`: `map/`'s cells and metatiles;
  - `docs/ui_layer2.md`: a note that the map window and message window
    sizes changed here.

Out, for later:

- tile definitions in bank 7 (R15);
- walls drawn by their neighbours (R16);
- real art, and its sprite format (R17);
- smaller levels or rooms (D5), and a different screen layout (D3).

## 7. Tests

PC tests:

- **Step 1 (D9):** every test passes unchanged with both constants at 8.
- **`test_map`:**
  - The camera in cell pixels: clamping, centring, `fine_x` 0-15.
  - `nge_map_draw()`: a cell with tile number m and attribute a writes
    `4m`..`4m+3`, two by two, with the attribute; a tile number of 64 or
    more sets the attribute's bit 0. The extra column and row of cells,
    and none past the map's edge.
  - `nge_map_init()` asserts an odd window size.
  - `nge_map_to_screen()` with a window not at 0,0, through a fine
    offset.
  - The tests' hard-coded `* 8`s use the constants.
- **`test_location`:** fine positions 0-15; moving across a cell edge.
- **`test_render`:** a sprite's position through a fine offset of up to
  15, and partly off the left and top edges.
- **`test_anim`:** a slide starts 16 pixels back and takes 4 frames; a
  lunge goes 8 pixels.
- **`test_level`, `test_gen`, `test_feature`:** as now, with the new tile
  numbers.
- **`test_ui`, `test_target`:** as now; the messages have 5 lines.

CSpect and hardware checks:

1. The map draws in 16 x 16 cells: floors, corridors, walls, doors,
   stairs, in their colours. Rock is transparent.
2. Sprites sit exactly on their cells: player, creatures, items.
3. Walking: a slide takes the same time as before; holding a key moves
   steadily. The camera follows smoothly to the map's edges.
4. The V key: the map and sprites move together a pixel at a time
   through a whole cell, in all directions.
5. Attacks lunge towards the target; targeting's cursor frames a cell and
   scrolls the view at its edge.
6. The item screens and the title cover the map window exactly; the
   message window shows 5 lines.
7. Stairs, a fall (F), and game over work as before.
8. Hardware only: a full redraw on a cell move fits in the frame (R7).

## 8. As built: differences from the design

- **The names keep "tile" (R2, 5.1).** The code already calls a map square
  a tile everywhere, and "cell" means an 8 x 8 block of Layer 2
  (ui_layer2.md), so `NGE_MAP_TILE_PIXELS` stays, now 16, and means the map
  tile. The hardware tile's size is `NGE_TILEMAP_TILE_PIXELS` (8), and
  `NGE_MAP_TILE_SPAN` (2) is hardware tiles per map tile. Step 1 (D9) was
  done this way and the tests passed unchanged before the size changed.
- **Layer 2 has its own font (R12).** The design missed that Layer 2 built
  its text font from the tile definitions (tiles 32-127) when it started.
  Taking the font out of them would have broken all the UI's text. The font
  is now a constant table of column bitmasks in page 90
  (`layer2/nge_layer2_font.c`, with the drawing that reads it), generated
  from the old tile font. So the tile definitions hold only metatiles, all
  64 are free, and Layer 2's text no longer depends on the tilemap. The
  tilemap stream still writes character codes as tile numbers, so it would
  need a font in the tile definitions; the game doesn't use it.
- **The stats window is 12 x 26 (R11).** Row 26 joins the message window
  across the whole width, so the stats window loses it too. Its lines use
  rows 0-16, so nothing moves.
- **Tile numbers (5.4):** the corner stays the horizontal wall's tile, as
  before, so it has no metatile of its own and the numbers are dense: rock
  0, floors 1 and 2, corridor 3, walls 4 and 5, doors 6 and 7, stairs 8
  and 9.
- **`nge_map_draw()`'s row loop is assembly from the start (R7).** zsdcc's
  code kept its pointers in the stack frame, about 450 T-states a map tile;
  the assembly takes about 250 (with the Z80N's `add hl, nn`). A full
  redraw is about 53,000 T-states, under 2 ms at 28 MHz, but still about
  1.5 times today's LDIR copy (about 34,000). Hardware check 8 measures it.
  The PC tests run the C version of the same loop.
- **Common memory:** the debug build has 605 bytes free, 92 fewer than
  before (the drawing code). The release build has 7,289.
- **The script (5.5)** takes the old data files as arguments, from the
  commit before the change (`git show 9ed3730:...`), as its output replaces
  them. It also writes Layer 2's font.
- **For review after playing:** targeting scrolls the view when the cursor
  is within `TARGET_SCROLL_MARGIN` (4) tiles of an edge. In a 13-row view
  that leaves 5 rows in the middle; a margin of 3 may suit the smaller view
  better.
- **Later (docs/tileset_import.md):** R13's placeholder metatiles were
  replaced by art drawn in Aseprite and imported by a script, and
  `double_art.py` was removed (it is in commit 2d2d2fc). The placeholder
  sprites and Layer 2's font, which it also wrote, stay.
- **Later (docs/sprite_import.md):** R17 decided: 4-bit patterns, imported
  from Aseprite, each shown through a palette bank. The placeholder sprites
  (`sprite_data.asm`) are gone.
