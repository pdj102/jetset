# Sprite import: requirements and design

Status: implemented, both steps of D10; CSpect checks 1-6 passed
(2026-10-22). Hardware checks are still to do. The decisions in section 4
and the palette spec in section 5.2 were agreed as proposed on 2026-10-22.
Section 8 lists where
the build differs from the design, and section 9 says how to change the
art.

Imports the game's sprites from an Aseprite file, so sprite art and its
animations go from Aseprite into the game with a save and a `make`. Each
frame is one 4-bit hardware pattern; tags name the sprites and give their
animations. Every palette bank follows one layout of 16 slots, so one
pattern shows in different colours through different banks: a red, blue or
green potion, a goblin and an orc, from the same drawing.

Builds on the tileset import (docs/tileset_import.md): the same file reader,
colour conversion and output conventions.

## 1. Goal

- Sprites are drawn in Aseprite, 16 x 16, one frame per pattern, with
  animations as tagged runs of frames.
- `make` turns the `.aseprite` file into the game's sprite patterns,
  sprite palette and a header naming each sprite and its animation.
- As many sprites as the hardware allows: 4-bit patterns, 128 of them.
- One drawing, many colourings: the bank a sprite is shown in chooses its
  colours, by a palette layout every bank shares.

## 2. Terms

| Term | Meaning |
|---|---|
| Pattern | One 16 x 16 hardware sprite image. 4-bit: 128 bytes, pixel values 0-15, 128 in the pattern memory |
| Bank | 16 entries of the sprite palette (palette offset n: entries 16n to 16n+15), chosen per sprite with `NGE_SPRITE_PALETTE(n)`. A 4-bit pixel value v shows entry 16n + v; 0 is transparent |
| Slot | A pixel value 0-15: a place in every bank, with one role (section 5.2) |
| Colourway | A bank that gives the layout's variable slots their colours: red, blue, bone... |
| Frame, tag | Aseprite's: a frame is one image of the timeline; a tag names a run of frames and how it plays |
| Run | Patterns first to first + count - 1, shown each for rate frames: what `nge_render_set_anim()` and `nge_render_play()` take |

## 3. Requirements

### The source

- **R1** The source is `assets/Sprites.aseprite`, committed: indexed colour
  mode, a 16 x 16 canvas, transparent index 0.
- **R2** Frame n is pattern n, in timeline order. At most 128 frames.
- **R3** The sprite's palette is the sprite palette: up to 256 entries,
  row b (entries 16b to 16b + 15) is bank b. A pixel's slot is its palette
  index & 15. Transparent pixels are the transparent index, 0. A pixel
  painted with slot 0 of another row (16, 32...) is an error naming the
  frame and the pixel: it would be transparent in the game but shows in
  Aseprite in that entry's colour.
- **R4** A frame's opaque pixels all come from one palette row: the row is
  a preview of the frame in that bank. More than one row is an error naming
  the frame and the rows.
- **R5** A frame is the visible layers combined, bottom to top: a layer's
  opaque pixels cover those below. Hidden layers and reference layers are
  left out, and so are the layers in a hidden group. A tilemap layer, a
  layer or cel with an opacity below 255, or a cel kind the reader doesn't
  know is an error. Cels may lie partly off the canvas; what is off it is
  left out.

### Tags

- **R6** Every frame is in at least one tag; a frame in none is an error
  (it would be a pattern with no name). Tags may overlap.
- **R7** A tag's name becomes a C name: upper case, with `.`, `-` and
  spaces as `_`, after `SPRITE_` (`rat.walk` is `SPRITE_RAT_WALK`). A name
  that isn't then a C identifier, or two tags with the same C name, is an
  error.
- **R8** A tag gives a run: its first frame's pattern, its count of
  frames, and its rate, the frames' duration in frames at 50 Hz (the
  duration in ms / 20, rounded, 1 to 255). The tag's frames must all have
  the same duration.
- **R9** A tag plays forward: Aseprite's reverse and ping-pong directions
  are errors (the render system plays runs forward). Aseprite's repeat
  count is ignored: the game decides whether a run loops (the rest
  animation) or plays once (a sequence).
- **R10** A tag's bank is the row its frames were drawn in (R4); frames of
  one tag in different rows are an error naming the tag. It is a default:
  the game can show the sprite in any bank.

### The output

- **R11** Patterns, 64 to an 8K page (`SECTION PAGE_137`, `PAGE_139`: odd
  pages, assembled for slot 7), two pixels a byte, the left pixel in the
  high nibble.
- **R12** The palette: every bank the sprite's palette has, 16 entries
  each, as `nge_palette_entry_9_t`, in page 135. Entries past the end of
  the Aseprite palette, in its last row, are black. A palette colour that
  isn't a Next colour is an error naming the entry.
- **R13** A header, `sprite_data.h`: the pages and counts to load, and for
  each tag `SPRITE_<NAME>` (first pattern), `_COUNT`, `_RATE` and `_BANK`.
- **R14** As for the tiles: on an error nothing is written and the build
  stops with a message; on success a summary is printed (patterns, pages,
  tags, banks used); the generated files say they are generated and are
  not to be edited; the same input gives the same output.
- **R15** A starter palette, `assets/sprite_palette.gpl` (a GIMP palette,
  which Aseprite loads: Palette, Load Palette), with the 16 banks of
  section 5.2, to begin `Sprites.aseprite` from. From then on the
  `.aseprite` file's palette is the source: the artist changes colours
  there.

### The build and the game

- **R16** Step 1: `make sprites` writes the output into `obj/sprites/`;
  the game doesn't use it. Step 2, when `Sprites.aseprite` has the game's
  sprites: the output goes into `src/data/`, committed, made by the
  default build when the art or the scripts change (as tileset_import.md
  R15), and the game switches to it (5.5).
- **R17 (later):** sprites larger than 16 x 16 (several hardware sprites
  per entity, or relative sprites).

## 4. Decisions

- **D1 Frames and tags.** Aseprite's timeline is made for animation: the
  artist plays a tag to see it, and sets each frame's duration. A tag maps
  onto a run directly, so the game's sprite records (`gen_render_t`,
  `gen_seq_t`) take a tag's four names and nothing is counted by hand.
  **Rejected:** a tileset, as for the map: no animation in Aseprite, and
  runs named by hand in the game.
- **D2 4-bit patterns.** 128 patterns in the pattern memory, twice the
  8-bit ones, and recolouring by bank (D4). 15 colours a sprite, through
  a layout made for it. **Rejected:** 8-bit patterns: 64 patterns, and
  one palette of 255 colours that can't recolour a sprite.
- **D3 Indexed colour mode.** The shared slots (white, the greys...) have
  the same colour in every bank, so a pixel's colour can't say which slot
  it is: its palette index can. The artist draws with one row of the
  palette and sees the sprite in that bank. **Rejected:** RGB mode, as the
  tiles are drawn: slots would have to be guessed from colours, and a
  colour used in two slots would be ambiguous.
- **D4 One slot layout for every bank (5.2), not fitted banks.** The
  tileset's banks are fitted automatically (tileset_import.md D1), which
  gives each tile the colours it was drawn in. Recolouring needs the
  opposite: a slot means the same part of a sprite in every bank, so a
  potion's liquid is slot 11 whatever its colour. **Rejected:**
  `next_colour.fit_banks()` for sprites.
- **D5 A tag's bank is the row it was drawn in (R10), as a default.** The
  header says which colourway the artist meant, and the game uses it or
  another. One row a frame keeps the preview honest: a frame mixing rows
  would look different in the game from Aseprite. The script doesn't check
  that the colours follow the layout: the palette is art, and a wrong
  colour shows in Aseprite at once.
- **D6 Every frame in a tag (R6), forward only, one duration a tag (R8,
  R9).** A frame without a tag can't be named, so it is a mistake (a stray
  frame) or a pattern the game would number by hand. The render system
  has one rate a run and plays forward; a ping-pong would need its frames
  repeated, which the artist can do with linked cels.
- **D7 No deduplication: frame n is pattern n.** Runs must be consecutive
  patterns, so a frame used in two animations is two patterns, unless the
  tags overlap. **Rejected:** merging identical frames: it would break
  runs, and gain little when tags can overlap.
- **D8 The palette in its own page (135), the patterns in 137 and 139.**
  64 4-bit patterns fill an 8K page exactly, so with the palette in a page
  of its own, each pattern page is one loader call. The game's asset range
  (132-223) has room.
- **D9 Bank uses are fixed (5.2): 0 the player, 1 a white flash, 2 a
  shadow, 3-15 colourways.** The flash and the shadow give every sprite a
  hit flash and a silhouette with no art: switch the bank for a few frames
  (`nge_render_set_flags()`). The player has a bank of its own colours.
- **D10 Two steps (R16), as the tiles had.** Step 1: the script and its
  tests, for trying on the art. Step 2, when the art has the game's
  sprites: the switch.

## 5. Design

### 5.1 Drawing in Aseprite

1. New sprite: 16 x 16, colour mode Indexed. Load
   `assets/sprite_palette.gpl` (Palette, Load Palette). Set the
   transparent colour to index 0.
2. Draw each sprite with one row of the palette: row 3 for a red monster,
   row 0 for the player. Layers are fine; hide guide layers.
3. Add a frame for each pattern. Tag each sprite's frames: `potion`,
   `rat.walk`, `rat.attack`, `player.idle`. Set the frames' duration
   (Frame Properties): 500 ms is a rate of 25, two steps a second.
4. Save as `assets/Sprites.aseprite`.

### 5.2 The palette bank spec

Every bank has the same 16 slots. Slots 1-8 are the same colours in every
colourway (but not in banks 0-2), so outlines, metal, wood and gold look
the same on everything. Slots 9-15 are the colourway: a 4-shade primary
ramp for a sprite's main material, and a 3-shade secondary ramp for its
second one.

| Slot | Role | Colour (R, G, B levels 0-7) |
|---|---|---|
| 0 | Transparent | - |
| 1 | Outline: a near-black ink, cool rather than pure black, so silhouettes read on dark floors | (0, 0, 1) |
| 2 | White: highlights, eyes, glints | (7, 7, 7) |
| 3 | Light grey, cool: steel, glass, bone in light | (5, 5, 6) |
| 4 | Dark grey, cool: iron, stone, shade on metal | (2, 2, 3) |
| 5 | Red: blood, eyes, hearts, gems | (6, 1, 1) |
| 6 | Gold: coins, hilts, buckles, flame tips | (7, 6, 1) |
| 7 | Brown: wood, leather, handles | (5, 3, 1) |
| 8 | Dark brown: wood in shade, hair, dirt | (3, 1, 0) |
| 9 | Primary highlight | colourway |
| 10 | Primary light | colourway |
| 11 | Primary mid | colourway |
| 12 | Primary dark | colourway |
| 13 | Secondary light | colourway |
| 14 | Secondary mid | colourway |
| 15 | Secondary dark | colourway |

How art uses it:

- **Potion:** glass in the greys and white, cork in brown, the liquid in
  the primary ramp. A red, blue or green potion is the bank.
- **Sword:** blade in the greys and white, hilt in gold and brown, a gem
  in the primary ramp.
- **Monster:** body in the primary ramp, belly and markings in the
  secondary, eyes in red or white. A rat in earth (12) or in dark (15);
  a goblin in green (6), an orc in iron (13).
- **Dungeon features as sprites (a chest, a torch, a fountain):** wood in
  the browns, metal in the greys, the flame or water in the primary ramp.
- **UI overlays:** the target cursor drawn once in the primary ramp: green
  (6) in range, red (3) out of range.
- **Effects (sparks, fire, frost, a poison cloud):** one pattern each,
  coloured by orange (4), ice (9) or lime (7).
- Text (damage numbers, messages) stays on Layer 2.

The ramps shift hue as pixel art does: highlights lean warm (towards
yellow), shades lean cool (towards blue or violet), within the 8 levels a
channel. Starting colours:

| Bank | Name | Primary 9-12 (highlight, light, mid, dark) | Secondary 13-15 (light, mid, dark) | Use |
|---|---|---|---|---|
| 0 | player | (5,6,7) (2,4,7) (1,2,5) (0,1,3) | (7,6,4) (6,4,3) (4,2,2) | The player's own: a blue tunic and skin to start. The layout is only a guide here |
| 1 | flash | every slot 1-15 (7,7,7) | | A hit flash: the sprite as a white silhouette |
| 2 | shadow | every slot 1-15 (1,0,2) | | A dark silhouette: unseen, invisible, remembered |
| 3 | red | (7,5,4) (7,2,2) (5,0,1) (3,0,1) | (7,6,4) (6,4,2) (4,2,1) cream | Red potions, fire imps, out-of-range cursor |
| 4 | orange | (7,7,3) (7,4,0) (6,2,0) (3,1,0) | (7,7,5) (7,6,1) (5,4,0) yellow | Fire, flames, torches |
| 5 | gold | (7,7,5) (7,6,1) (6,4,0) (4,2,0) | (6,3,1) (4,2,1) (2,1,0) bronze | Treasure, gold items, golden creatures |
| 6 | green | (5,7,3) (2,6,1) (1,4,1) (0,2,1) | (6,6,2) (4,4,1) (2,2,0) olive | Goblins, slimes, plants, in-range cursor |
| 7 | lime | (6,7,2) (4,7,0) (2,5,0) (1,3,1) | (6,4,6) (4,1,4) (2,0,2) purple | Poison, acid, toxic clouds |
| 8 | blue | (5,6,7) (2,4,7) (1,2,5) (0,1,3) | (4,7,7) (1,5,6) (0,3,4) cyan | Blue potions, water, magic |
| 9 | ice | (6,7,7) (3,7,7) (1,5,6) (0,3,4) | (3,4,7) (1,2,6) (0,1,3) deep blue | Frost, ice creatures, crystals |
| 10 | purple | (6,5,7) (5,2,6) (3,1,4) (1,0,2) | (7,4,6) (6,2,4) (3,0,2) pink | Wizards, arcane things, bats |
| 11 | flesh | (7,6,5) (7,4,4) (5,3,3) (3,1,2) | (7,3,3) (5,1,2) (3,0,1) lips, wounds | Humans, pigs, worms |
| 12 | earth | (6,5,3) (5,3,1) (3,2,1) (2,1,0) | (7,6,4) (5,4,3) (3,2,2) tan | Rats, dogs, leather goods |
| 13 | iron | (6,6,7) (4,4,5) (3,3,4) (1,1,2) | (6,3,1) (4,1,0) (2,0,0) rust | Armour, golems, orcs |
| 14 | bone | (7,7,6) (6,6,4) (5,4,3) (3,3,2) | (4,4,5) (2,2,3) (1,1,2) grave | Skeletons, ghosts, parchment |
| 15 | dark | (3,2,4) (2,1,3) (1,0,2) (0,0,1) | (7,2,2) (5,0,1) (3,0,0) red glow | Shadow creatures with glowing eyes |

These are a start: the colours in `Sprites.aseprite`'s palette are the
source (R15), and are the artist's to change.

### 5.3 Scripts

- **`scripts/aseprite.py`**, extended (its API for the tiles stays):
  - every frame, not only the first: its duration (frame header), and its
    cels (chunk `0x2005`: raw, linked to an earlier frame's cel, or
    compressed; position and opacity). A tilemap cel is kept as a kind the
    sprite importer refuses;
  - the tags (chunk `0x2018`: from, to, direction, name);
  - each layer's flags (visible, reference), child level and opacity;
  - `sprite.frame_indices(n)`: frame n's visible layers combined (R5), as
    rows of palette indices, `None` where transparent. For indexed
    sprites.
- **`scripts/asm_out.py`**, new: `db()` and `write_all()`, moved out of
  `ase2tiles.py` (no change to its output).
- **`scripts/ase2sprites.py`**, new:

  ```
  python3 -I scripts/ase2sprites.py assets/Sprites.aseprite <out dir>
  ```

  It checks R1-R10 and R12, and writes the three files (5.4).
- **`scripts/sprite_palette.py`**, new: writes `assets/sprite_palette.gpl`
  from section 5.2's tables (R15). Run once by hand; its output is
  committed.

### 5.4 Generated files

- **`sprite_patterns_data.asm`**: `SECTION PAGE_137` with
  `_sprite_patterns_0` and patterns 0-63, then `SECTION PAGE_139` with
  `_sprite_patterns_1` and patterns 64 on, if there are any. Each pattern
  is headed `; pattern n: <tags>, bank b`.
- **`sprite_palette_data.asm`**: `SECTION PAGE_135`, `_sprite_palette`,
  each bank headed `; bank b`.
- **`sprite_data.h`**:

  ```c
  #define SPRITE_PALETTE_PAGE     135  /**< SECTION PAGE_135 in sprite_palette_data.asm. */
  #define SPRITE_PALETTE_ENTRIES  256  /**< Entries in sprite_palette: load them from index 0. */
  #define SPRITE_PATTERNS         70   /**< 4-bit patterns, from pattern 0. */
  #define SPRITE_PATTERN_PAGES    2    /**< Pages of patterns: SPRITE_PATTERNS_PAGE_0 on. */

  #define SPRITE_PATTERNS_PAGE_0   137  /**< Patterns 0-63: sprite_patterns_0. */
  #define SPRITE_PATTERNS_COUNT_0  64
  #define SPRITE_PATTERNS_PAGE_1   139  /**< Patterns 64 on: sprite_patterns_1. */
  #define SPRITE_PATTERNS_COUNT_1  6

  #define SPRITE_RAT_WALK        5    /**< Tag rat.walk: frames 5-6. */
  #define SPRITE_RAT_WALK_COUNT  2
  #define SPRITE_RAT_WALK_RATE   12
  #define SPRITE_RAT_WALK_BANK   12
  ...
  extern const uint8_t sprite_patterns_0[];
  extern const uint8_t sprite_patterns_1[];
  extern const nge_palette_entry_9_t sprite_palette[];
  ```

  In step 2 it replaces the demo's `sprite_data.h` (base names are unique
  across the project, N-FILE-2).

### 5.5 The game (step 2, later)

- **Loading (`actors_load_graphics()`):** `nge_sprite_load_patterns4()`
  once a page, and `nge_palette_load9(SPRITE_PALETTE_PAGE,
  RPC_SELECT_SPRITES_PALETTE_0, 0, sprite_palette,
  SPRITE_PALETTE_ENTRIES)`.
- **Sprite records:** `gen_render_t` gets the mode
  (`NGE_SPRITE_PATTERN4`) and its flags carry `NGE_SPRITE_PALETTE(bank)`;
  `gen_shared_bank.c` passes both to `nge_render_add()`. The kind records
  (`gen_*_pdata.c`) use the tags' names, and their colourways.
- **Bank names:** the game names the banks of 5.2 in one place
  (`BANK_PLAYER`, `BANK_FLASH`, `BANK_SHADOW`, `BANK_RED`...).
- **The cursor (`target_bank.c`):** one pattern, shown in green or red.
- **The demo's recoloured kinds:** the goblin and the recoloured items
  get colourways.
- The demo sprite files (`src/data/sprite_data.asm`) go.

### 5.6 Memory

- **Pages 135, 137, 139:** the palette (512 bytes at most) and the
  patterns (8K a page).
- **Pattern memory:** all 16K for 128 4-bit patterns.
- **Common memory:** unchanged in step 1. Step 2 adds nothing: the
  loaders exist.

## 6. Scope

In (step 1):

- R1-R15, `make sprites` (R16's step 1);
- the scripts' tests (section 7);
- docs: this spec; `docs/memory_map.md` and `src/banks.inc` reserve pages
  135, 137 and 139.

Out, for later:

- step 2 (5.5);
- sprites larger than 16 x 16 (R17);
- checking that colours follow the layout (D5);
- loading only some sprites (for example a level's monsters): all 128
  patterns fit, so all are loaded.

## 7. Tests

`tests/scripts/test_ase2sprites.py` (`unittest`, run by `make test`), with
files built by `tests/scripts/ase_writer.py`, extended with frames,
durations, cels (raw, linked, compressed) and tags:

- **Reading:** frames and their durations; a linked cel shows its frame's
  image; layers combine bottom to top; hidden layers, a hidden group's
  layers and reference layers are left out; a cel off the canvas is
  clipped; tags' ranges, directions and names.
- **Rules:** each error of R1-R10 and R12, naming what it should: a frame
  in no tag, two rows in a frame, a tag over two rows, two durations in a
  tag, ping-pong, a bad or repeated name, 129 frames, a 32 x 32 canvas, RGB
  mode, a layer at half opacity, a palette colour that isn't a Next
  colour.
- **Output:** a tag's name, first pattern, count, rate (500 ms is 25; 10
  ms is 1) and bank; nibble order; 64 patterns a page, a second page from
  pattern 64; the palette's banks, the last padded with black; nothing
  written on an error.
- **The starter palette:** `assets/sprite_palette.gpl` has 256 entries,
  all Next colours, with banks 1 and 2 single-coloured and slots 1-8 the
  same in banks 3-15.
- `tests/scripts/test_ase2tiles.py` passes unchanged (the move to
  `asm_out.py`), and `make tiles` writes the same files.

Checks by hand in step 1: the generated `.asm` assembles (`zcc -c +zxn`)
and `sprite_data.h` compiles, as was done for the tiles. There are no
CSpect checks until step 2.

CSpect and hardware checks (step 2):

1. The player, rats and goblins show in their colours: the player in its
   own bank, rats in earth, goblins in green. The player blinks slowly at
   rest (its idle run).
2. Walking: the player's move sequence plays with each slide, and holding
   a key walks as fast as before.
3. Attacking: the player's attack frame shows with the lunge.
4. Items on the floor and in use: the four potions in their colours
   (healing red, antidote green, elixir gold, strength orange), the sword
   in iron, the ring in gold, a dead rat (its own frame) in earth.
5. Targeting (F, L): the cursor is green in range and red out of range.
6. Nothing shows a black square or a wrong colour: transparent pixels are
   transparent.

## 8. As built: differences from the design

- **`gen_render_t` keeps its fields (5.5).** Its flags carry
  `NGE_SPRITE_PALETTE(BANK_*)` already, and every sprite is now 4-bit, so
  `gen_render()` adds `NGE_SPRITE_PATTERN4` itself: no field for the mode.
- **The header also gives `SPRITE_PATTERN_PAGES`**, so the loading can
  test for a second page with `#if` (C-PRE-3).
- **The bank names** are in `game/actors.h`, next to the render layers:
  `BANK_PLAYER` to `BANK_DARK`, and `BANK_COUNT`. A check typedef in
  `actors.c` stops the build if the art's palette doesn't have all 16
  banks.
- **The colourways chosen:** the player in `BANK_PLAYER`, rats in earth,
  goblins in green; the healing potion red, the antidote green, the elixir
  gold, the potion of strength orange (one `potion` pattern for all four:
  they were told apart by mirroring and rotating before); the sword iron,
  the ring gold; the dead rat its own frame (`rat.dead`), in earth.
- **The player's sequences** come from the art (`player.move`,
  `player.attack`); the other kinds have none yet. `test_anim` checks
  they are copied, and that the move sequence is no longer than the slide
  (4 frames), so walking isn't slowed.
- **`screen_overlay_set()`** takes the sprite's flags, for the cursor's
  bank, and shows a 4-bit pattern.
- **The flash and shadow banks** are named but not used yet.
- **Page 101** is free: the demo sprites are gone. `make tiles` now also
  depends on `asm_out.py`, which it imports.
- **Common memory:** 546 bytes free in the debug build, 3 fewer.

## 9. Changing the art

1. Draw in `assets/Sprites.aseprite` (5.1): one palette row a frame, and
   every frame in a tag.
2. A new sprite or animation: add its frames and a tag. Frames are
   patterns in timeline order, so frames added in the middle renumber
   those after them; the names (`SPRITE_<TAG>`) follow, so the game
   doesn't change.
3. Using it in the game: the kind record (`gen_*_pdata.c`) names the tag:
   `{ SPRITE_<TAG>, SPRITE_<TAG>_COUNT, SPRITE_<TAG>_RATE,
   NGE_SPRITE_PALETTE(BANK_*), LAYER_* }`, or a creature's sequence
   `{ SPRITE_<TAG>, SPRITE_<TAG>_COUNT, SPRITE_<TAG>_RATE }`. A renamed or
   removed tag is a compile error where it is used.
4. A move sequence should last no longer than the slide (4 frames, 80
   ms), and an attack sequence no longer than the lunge (8 frames, 160
   ms): the world waits for them.
5. `make`. The script prints the patterns, tags and banks, or an error
   naming the frame, tag or pixel.
6. Commit the `.aseprite` file and the three generated files in
   `src/data/` together.

