# Tileset import: requirements and design

Status: implemented, both steps of D6; CSpect checks 1-4 passed, with the
sprites' colours unchanged after the palette loader moved (2026-10-21).
Hardware checks are still to do. The decisions in section 4 were agreed as
proposed on 2026-10-21; D2 drops the PNG route for now. Section 8 lists
where the build differs from the design, and section 9 says how to change
the art.

Imports the map's metatiles from an Aseprite tileset, so new art goes from
Aseprite into the game with a save and a `make`. A script reads the
`.aseprite` file directly, converts its colours to the Next's, gives each
metatile a palette bank, and writes the tile definitions and the tilemap
palette as source files in `src/data/`. This covers tilesets only: levels
are generated, so tilemaps (Aseprite's tilemap cels) are not imported.

## 1. Goal

- The map's art is drawn in Aseprite, on a tilemap layer, in 16 x 16
  tiles, using colours of the Next's 512-colour palette.
- `make` turns the `.aseprite` file into the game's tile definitions and
  palette. There is no export step in Aseprite.
- The art can use any of the 512 colours; the script fits them into the
  tilemap's 16 banks of 16 colours, so the artist doesn't arrange a
  palette.
- The file reading and the bank fitting can be reused to import sprites
  from a tileset later (R16).

## 2. Terms

| Term | Meaning |
|---|---|
| Metatile | The graphic of one map cell: 16 x 16 pixels, 2 x 2 hardware tiles (cells_16.md). Metatile m is hardware tiles 4m to 4m+3 |
| Tileset | Aseprite 1.3's set of tiles that a tilemap layer draws from. Tile n of the tileset is metatile n |
| Bank | 16 entries of the tilemap palette, chosen by a map tile's attribute (bits 7-4). Pixel values 0-15 index the bank; 0 is transparent (`NGE_TILEMAP_TRANSPARENT`) |
| Next colour | A 9-bit colour, 3 bits each of red, green and blue. In 8 bits a channel is one of 0, 36, 73, 109, 146, 182, 219, 255 |

## 3. Requirements

### The source

- **R1** The source is `assets/Tilesheet.aseprite`, committed. The
  metatiles are the tileset of its tilemap layer, in tileset order. Other
  layers (Background) are ignored.
- **R2** The sprite can be in indexed or RGB colour mode. In indexed mode
  a pixel is a byte, so only the first 256 entries of a 512-colour
  palette can be painted; RGB mode, with the 512 colours as swatches,
  allows all of them. Grayscale mode is an error.
- **R3** A pixel is transparent if, in indexed mode, it is the sprite's
  transparent index (169 in today's file), or, in RGB mode, its alpha is
  0. Any other alpha below 255 is an error.
- **R4** Tile 0 is metatile 0, rock: it must be fully transparent
  (Aseprite keeps tile 0 empty).
- **R5** Every opaque pixel is an exact Next colour. Anything else is an
  error naming the tile, the pixel and the colour.

### The conversion

- **R6** A metatile uses at most 15 opaque colours (a bank's 16 entries
  less the transparent one); more is an error naming the tile and its
  count.
- **R7** Every metatile is given one bank (D1). Metatiles share a bank
  when their colours fit in its 15 entries together. More than 16 banks
  is an error.
- **R8** At most 64 metatiles, tile 0 included (cells_16.md R12: 256
  hardware tiles at `$4A00`). More is an error.
- **R9** The output is the same for the same input, and adding a
  metatile at the end of the tileset doesn't change the pixel values or
  banks of the others (D1).
- **R10** On an error the script writes nothing and exits with an error,
  so the build stops and no half-written file is left. On success it
  prints a summary: metatiles, banks used, and colours in each bank.

### The output

- **R11** `src/data/tile_defs_data.asm`: the metatiles, as now
  (`SECTION BANK_5`, `org 0x4A00`, `_tile_pattern`, 4 bits a pixel, the
  left pixel in the high nibble, metatile m as hardware tiles 4m to
  4m+3). The metatiles the tileset doesn't have are transparent, up to
  64.
- **R12** `src/data/tile_palette_data.asm`: the banks used, 16 entries
  each, as `nge_palette_entry_9_t` pairs (`RRRGGGBB`, then the low blue
  bit), in an odd page in the game's asset range (D4). Entry 0 of each
  bank is transparent (written as black).
- **R13** `src/data/tile_data.h`: the page and size of the palette, the
  number of metatiles, and each metatile's bank, as constants that cost
  no memory (D3).
- **R14** The generated files say at the top that they are generated, by
  which script from which file, and are not to be edited.

### The build and the game

- **R15** `make` runs the script when the `.aseprite` file or the script
  is newer than the output. The output is committed (D5).
- **R16 (later):** sprites imported from an Aseprite tileset, through the
  same file reader and bank fitting. Their format (8-bit patterns, or
  4-bit with a palette offset each) is cells_16.md R17's decision, made
  in that spec. This spec keeps the reader and the bank fitting apart
  from what is made of the tiles so they can be reused (5.1).
- **R17** The game colours each map tile by its metatile's bank from
  `tile_data.h`, in place of today's ink colours (banks 12-15). The
  build fails if the game names a metatile the tileset doesn't have.
- **R18** The tilemap palette is loaded from the generated data in place
  of `screen.c`'s `tile_palette[]` and the ink banks.

## 4. Decisions

- **D1 Banks are fitted automatically, first fit, in tileset order.**
  Each metatile in turn goes in the first bank whose colours, with its
  own, make 15 or fewer; if none fits, a new bank is opened. A bank's
  colours are kept in the order they were first added, so a metatile
  added later appends colours and never moves an earlier metatile's
  pixel values (R9). 16 banks of 15 colours hold far more than 64
  metatiles usually need. **Rejected:** banks chosen by the artist (the
  Aseprite palette arranged as 16 rows of 16): full control, but it gives
  up drawing from the 512 colours. **Rejected:** optimal packing (fewest
  banks): any change to any tile could reorder every bank and redo all
  the data, and saving banks gains nothing while 16 are free.
- **D2 The script reads the `.aseprite` file.** The format is documented
  (Aseprite's `docs/ase-file-specs.md`); the parts needed (header,
  layers, palette, tileset) take about 100 lines of standard Python
  (`struct`, `zlib`). There's no export step to forget, nothing to
  install (no Pillow), and no need to run Windows Aseprite from WSL.
  **Rejected for now:** a PNG exported from Aseprite (Export Tileset or
  the batch CLI): an extra manual step, and a second file that can be out
  of date.
- **D3 Banks are constants, and metatile names stay in the game.**
  Aseprite 1.3 has no clear way in its UI to give a tile a name or user
  data, so the metatiles' names stay where they are (`LEVEL_*` in
  `level_internal.h`, `level.h`), by number. `tile_data.h` defines
  `TILE_BANK_<m>` for each metatile and `TILE_BANK(m)`, which expands a
  named number to its constant, so `TILE_BANK(LEVEL_CORRIDOR)` is a
  compile-time number. **Rejected:** a bank table in memory (64 bytes):
  common memory is short, and every use is a named metatile. Reordering
  the tileset in Aseprite changes the numbers, so the names in the game
  must be changed with it; the build check (R17) catches only a missing
  tile, not a moved one.
- **D4 The palette is banked data, loaded by one palette loader for
  every palette.** It is up to 512 bytes, too much for common memory (605
  bytes free in the debug build). It goes in page 133 (`SECTION
  PAGE_133`, the first odd page of the game's asset range), and
  `nge_palette_load9()` (`hw/nge_palette`) maps it in for the upload
  (memory_map.md, "Sprite and palette data"), writing the banks from bank
  0 of tilemap palette 0. The loader takes the palette to write, so it
  also loads the sprite palette, and replaces `nge_sprite_load_palette()`:
  one way to load a palette. It is in common memory, so fades and
  palette cycling can call it often and from anywhere. **Changed after
  review:** first designed as `nge_map_load_palette()` in the map's page
  44, for the tilemap palette only.
- **D5 The generated files are committed.** The game, and the PC tests
  that include `tile_data.h`, build from them like any other source, and
  a change to the art shows in the diff of the palette and banks.
  `make` rebuilds them when the art changes, so they can't silently fall
  behind. **Rejected:** generating into `obj/` (never committed): a
  single source, but the PC tests and every build would then need the
  script, and the header's path would differ from the other data's.
- **D6 Two steps: the script, then the switch.** Today's tileset has 5
  metatiles, and the game uses 10 (rock to stairs down), so the
  placeholders can't be replaced yet.
  - Step 1: the script and its tests. `make tiles` runs it into
    `obj/tiles/`, for trying on the art, without touching the game.
  - Step 2, when the tileset has the game's metatiles in the game's
    order: the script writes into `src/data/`, the default build runs it
    (R15), and the game changes (R17, R18) are made. The placeholders and
    `scripts/double_art.py` are removed.
- **D7 The ink banks go from the map.** With the art's own colours, the
  map no longer needs banks 12-15, so all 16 banks are the art's. The
  game stops loading them (`nge_tm_load_ink_palette()` stays in nge for
  the tilemap stream, which the game doesn't use). A metatile in two
  colours is two metatiles in the tileset; recolouring by bank (one
  graphic, two palettes) is out for now.

## 5. Design

### 5.1 Scripts

- **`scripts/aseprite.py`**: reads a `.aseprite` file, for any importer.
  - The header: size, colour depth (8 indexed, 32 RGBA; 16 grayscale is
    an error), transparent index.
  - The palette (chunk `0x2019`; the old chunks `0x0004` and `0x0011` are
    ignored if `0x2019` is present).
  - The layers (chunk `0x2004`): name, type, and a tilemap layer's
    tileset index.
  - The tilesets (chunk `0x2023`): tile size, count, and the tiles'
    pixels (flag 2, zlib-compressed, the tiles stacked top to bottom). A
    tileset in an external file (flag 1) is an error.
  - Gives each tile as rows of RGBA pixels (alpha 0 or 255), so importers
    don't deal with colour modes.
  - Chunks it doesn't need (cels, tags, user data, slices) are skipped by
    their size.
- **`scripts/next_colour.py`**: RGB to a Next colour (exact, R5), the
  `nge_palette_entry_9_t` bytes, and the bank fitting (D1): given each
  graphic's colours, returns the banks and each graphic's bank and pixel
  values. Kept apart for sprites (R16).
- **`scripts/ase2tiles.py`**: the tile importer.

  ```
  python3 -I scripts/ase2tiles.py assets/Tilesheet.aseprite src/data [--layer NAME]
  ```

  With no `--layer`, the file must have one tilemap layer. It checks
  R3-R8, fits the banks, splits each 16 x 16 tile into its four hardware
  tiles, and writes the three files (5.2) to temporary names, renaming
  them only when all are written (R10).

### 5.2 Generated files

- **`tile_defs_data.asm`**: as today's (R11). Each metatile is headed
  `; metatile m: bank b`, and its tiles are labelled `sprite<n>` as now.
- **`tile_palette_data.asm`**:

  ```
  SECTION PAGE_133
  PUBLIC _tile_palette
  ._tile_palette
  ; bank 0: metatiles 0, 1, 3
  	db $00, $00, $6D, $00, ...
  ```

- **`tile_data.h`**:

  ```c
  #define TILE_PALETTE_PAGE   133  /**< SECTION PAGE_133 in tile_palette_data.asm. */
  #define TILE_PALETTE_BANKS  2    /**< Banks in tile_palette, 16 entries each. */
  #define TILE_METATILES      6    /**< Metatiles in the tileset, tile 0 included. */

  #define TILE_BANK_0  0
  #define TILE_BANK_1  0
  ...
  /** A metatile's bank, from a metatile number that is a constant. */
  #define TILE_BANK(m)   TILE_BANK_X(m)
  #define TILE_BANK_X(m) TILE_BANK_##m

  extern const nge_palette_entry_9_t tile_palette[];
  ```

### 5.3 The build

- **Step 1:** a `tiles` target writes into `obj/tiles/`. `make test` runs
  the scripts' tests too (section 7).
- **Step 2:** a rule, with all three files as its targets, from the
  `.aseprite` file and the scripts; `src/data/*.asm` are already found by
  the Makefile's wildcard. `make check` (`check-toolchain.sh`) checks for
  `python3`.

### 5.4 nge (step 2)

- **`hw/nge_palette`:** `nge_palette_load9(uint8_t page, uint8_t
  palette_select, uint8_t first_index, const nge_palette_entry_9_t
  *entries, uint16_t num_entries)`, in common memory. It maps `page` into
  `NGE_MMU_DATA_SLOT` (7, a new name in `hw/nge_mmu.h` for the slot every
  loader uses), calls `nge_write_palette9()`, and maps the old page back.
  `NGE_PALETTE_DATA_COMMON` (0) maps nothing. It asserts the page is odd.
- **`sprite/`:** `nge_sprite_load_palette()` goes; its caller
  (`actors.c`) calls `nge_palette_load9()` with
  `RPC_SELECT_SPRITES_PALETTE_0`. The pattern loaders use
  `NGE_MMU_DATA_SLOT`.

### 5.5 The game (step 2)

- **`screen.c`:** `tile_palette[]` and the `nge_tm_load_ink_palette()`
  call go; `screen_init()` calls `nge_palette_load9(TILE_PALETTE_PAGE,
  RPC_SELECT_TILEMAP_PALETTE_0, 0, tile_palette, TILE_PALETTE_ENTRIES)`
  (`TILE_PALETTE_ENTRIES`: the banks' entries, from `tile_data.h`).
- **`level_gen.c`, `level_bank.c`:** `put()` and the floor placement
  take a bank, `TILE_BANK(<metatile>)`, in place of an ink. Where the
  metatile is chosen at run time (floor or floor 2), each choice passes
  its own bank. `LEVEL_FLOOR_INK` goes.
- **Features (`gen_feature_pdata.c`, `feature.c`):** a look's `ink`
  becomes `bank`, set with `TILE_BANK()` of its tile.
- **`level.h` or `level_internal.h`:** a compile-time check that the
  highest metatile the game names is below `TILE_METATILES` (R17).

### 5.6 Memory

- **Bank 5:** unchanged: the tile definitions' 8K at `$4A00`.
- **Page 133:** the palette, 32 bytes a bank, 512 at most.
- **Common memory:** the loader, about 140 bytes in the debug build
  (zsdcc's code for its five arguments, and its assert), less
  `tile_palette[]` (8 bytes) and the ink palette loader and its table,
  which are no longer linked. Page 42 loses the sprite palette loader.

## 6. Scope

In:

- R1 to R15, R17, R18, in the two steps of D6;
- tests (section 7);
- docs:
  - `docs/memory_map.md`: page 133, the palette loader, the tile
    definitions generated;
  - `src/banks.inc`: page 133 in the allocation comment;
  - `level.h`'s comment on the metatiles (no longer the glyphs);
  - `cells_16.md`: a note that R13's placeholders were replaced here;
  - how to add a metatile: draw it at the end of the tileset, name its
    number in `level_internal.h`, `make`.

Out, for later:

- importing sprites (R16);
- recolouring a metatile by bank (D7);
- names or other data for tiles from Aseprite (D3);
- the PNG route (D2);
- more than 64 metatiles (cells_16.md R15).

## 7. Tests

The scripts' tests: `tests/scripts/test_ase2tiles.py` (`unittest`), run by
`make test`. They build small `.aseprite` files with a writer of their own
(`tests/scripts/ase_writer.py`), so no binary files are committed:

- **Reading:** indexed and RGBA files give the same tiles; the transparent
  index and alpha 0 are transparent; an uncompressed chunk skipped by its
  size; grayscale, an external tileset, and part-transparent alpha are
  errors.
- **Colours:** the eight channel levels convert exactly, and a colour
  between them is an error naming the tile and pixel; the 9-bit bytes for
  a few colours (black, white, pure red, green, blue, and a blue with the
  low bit set).
- **Banks:** two tiles that fit together share a bank; one that doesn't
  opens a new one; 16 colours in a tile, and a 17th bank, are errors;
  adding a tile at the end leaves the earlier tiles' banks and pixel
  values as they were (R9).
- **Output:** a metatile's four hardware tiles in order, nibbles left
  pixel high; padding to 64 transparent metatiles; more than 64 is an
  error; a non-empty tile 0 is an error; on an error nothing is written.
- **The real file:** `assets/Tilesheet.aseprite` reads, tile 0 is empty,
  and every colour is exact. Only these hold as the art changes; the test
  catches a misreading of the format that a hand-written test file would
  share.

PC tests (step 2):

- **`test_palette`:** `nge_palette_load9()` maps the page into the data
  slot, writes the entries to the palette selected (tilemap, sprites)
  from the index given, and maps the old page back; with
  `NGE_PALETTE_DATA_COMMON` it maps nothing; an even page is an error.
  The sprite palette loader's test in `test_sprite` goes.
- **`test_level`, `test_gen`, `test_feature`:** tiles carry their
  metatile's bank in the attribute's bits 7-4.

CSpect and hardware checks (step 2):

1. The map draws with the art's colours: floors, corridors, walls,
   doors, stairs.
2. Rock and transparent pixels show what is behind the tilemap.
3. Doors open and shut, and stairs show, in their own colours.
4. A change in Aseprite (a pixel's colour in one metatile), then `make`,
   shows in the game; nothing else changes colour.

## 8. As built: differences from the design

- **The art is in RGB mode**, with a palette entry of alpha 0 for
  transparent (R2, R3).
- **The game's numbers** (`level_internal.h`, `level.h`) were set by hand
  for the tileset of 15 metatiles, some of them trial designs. Floor,
  floor 2 and corridor share metatile 2: they are all open, and nothing
  tells them apart by number.
- **The build check (R17)** is a check typedef in `level_internal.h` over
  every metatile the level names. `TILE_BANK()` of a number past the
  tileset is a compile error too (`TILE_BANK_<m>` is undefined).
- **`make tiles`** regenerates the data in `src/data/` (step 1's
  `obj/tiles/` is gone). C files are compiled after the data is up to date
  (an order-only prerequisite); `make test` brings it up to date first.
- **The PC tests** link `screen.c`, which uses `tile_palette`, assembly the
  PC can't build: `tests/support/test_tile_data.c` stands in for it.
- **`double_art.py`** also wrote the placeholder sprites and Layer 2's font,
  which stay. Their comments point to commit 2d2d2fc for the script.
- **Common memory:** 549 bytes free in the debug build, 56 fewer than
  before. The map's loader in page 44 left 689; moving it to common memory
  (D4, after review) costs 140.

## 9. Changing the art

1. Draw in `assets/Tilesheet.aseprite`, on the tilemap layer "Tilemap 1",
   in Next colours (the 512-colour palette), at most 15 colours a tile.
2. A new metatile: add it at the end of the tileset, so no other tile's
   number moves, and name its number in `game/level/level_internal.h`
   (terrain) or `level.h` (features' looks).
3. Reordering or removing tiles changes the numbers after them: change
   the names with them.
4. `make`. The script prints the metatiles and the banks they use, or an
   error naming the tile and pixel (a colour that isn't a Next colour, too
   many colours, more than 64 tiles).
5. Commit the `.aseprite` file and the three generated files in
   `src/data/` together.

