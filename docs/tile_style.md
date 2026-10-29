# Tile style guide

Status: agreed (2026-10-29): D1 to D10 and Q1 to Q6 as proposed, the
stairs turned (section 9), and the pit on a tile of its own (Q3).
Implemented the same day: the replacement tileset is in
`assets/Tilesheet.aseprite`; `make` and `make test` pass. CSpect checks
1-3 and 5 (section 9) passed on 2026-10-29; check 4, the pit, waits until
the generator makes pits. The masters (6.9) were agreed the same day.

The rules every metatile in `assets/Tilesheet.aseprite` follows, so tiles
drawn at different times, by the artist or by Claude, look like one set,
and the sprites (docs/sprite_style.md) stand out on them. How the art gets
into the game is docs/tileset_import.md; how tiles are named and chosen is
docs/dungeon_decoration.md. This guide says how to draw them, and above
all how to draw within the tilemap's palette.

A request for art names this guide: "draw `floor_4` and `hole_xfxx`
following docs/tile_style.md".

## 1. Goal

- One look across walls, floors, corridors, doors, stairs and pits: the
  same view, wall height, light and materials.
- A level reads at a glance on a TV: where the rooms are, where the
  walls are, which cells can be walked on, how many cells away a thing is.
- The map is the background: calm and low in colour, so sprites, which
  are outlined and coloured, are what the eye goes to.
- The whole tileset fits in few palette banks and few metatiles, leaving
  room for what comes later: more looks, remembered (dimmed) tiles,
  depth themes.

## 2. Terms

| Term | Meaning |
|---|---|
| Metatile | One 16 x 16 tile of the tileset: one map cell (tileset_import.md) |
| Bank | 16 entries of the tilemap palette. A metatile draws through one bank: 15 colours and transparent |
| Tile palette | The colours the tileset is drawn from (6.5.2): a short list, chosen, not the 512 |
| Void | Where nothing is drawn: transparent pixels, shown as the fallback colour, black |
| Cap | The top of a wall, seen from above: a 4-pixel band along the room's edge |
| Face | The front of a top wall, seen from the room: bricks, rows 4-15 |
| Flag | A floor's flagstone: one a cell |
| Ramp | A material's shades, light to dark |
| Master | An agreed tile that new tiles are drawn to match (6.9) |
| Grid | A tile written as 16 rows of 16 characters, one a pixel: `.` transparent, a letter the tile palette's key (6.5.2) |

## 3. Requirements

- **R1** Every metatile follows the view, geometry, light, palette and
  seam rules of section 6. A tile that breaks one says why in section 8.
- **R2** Tiles match the sprites: the same 3/4 view and light from the top
  left (sprite_style.md D1, D4), and the same wood colours.
- **R3** Sprites read on every tile they can stand on (floor, corridor,
  doorway, open door, stairs), and on the wall faces behind them: their
  near-black outline and their colours against the tile's (6.6).
- **R4** The tileset keeps within the palette limits of 6.5.1, and within
  the budgets of 6.8: colours, banks, metatiles.
- **R5** Tiles join without seams: a wall's cap, face and bricks continue
  into the next cell's, and floors tile in any arrangement (6.3).
- **R6** New art is checked against the masters (6.9), at 1x and zoomed,
  in a room with sprites in it, before it is committed.
- **R7** Claude draws to this guide by the workflow of section 7: grids
  and a preview for review, then the file, then `make`.

## 4. Decisions

- **D1 Keep today's geometry.** The 3/4 view of the agreed tileset
  (dungeon_decoration.md, checked in CSpect on 2026-10-25): every wall is
  a 4-pixel cap along the room's edge, and a top wall (floor to its south)
  also shows its brick face below the cap, 12 rows high. Side and bottom
  walls show only their cap. Beyond the cap is void. The decorator's names
  and every wall piece's shape stay as they are; only how they are drawn
  changes. **Rejected:** walls that fill their cell (the cap 16 pixels
  wide on side walls): a top wall's cap must fit above its face in the
  same cell, so caps would be 4 pixels on top walls and 16 on side walls,
  and rooms would look lopsided. **Rejected:** a face on bottom walls too
  (their outside face, over the void below): it reads as a second room's
  top wall.
- **D2 One short tile palette, chosen on purpose.** The importer accepts
  any of the 512 colours and fits them into banks, but each colour used
  by few tiles costs a slot in every bank those tiles land in, and shades
  picked by eye drift between tiles drawn on different days. The tileset
  is drawn from the 10 colours of 6.5.2; a new colour is added to that
  table first, with its role. **Rejected:** free colours: the palette
  limits would show only when a bank overflows, and the set would lose its
  consistency long before.
- **D3 Values, light to dark: caps, floor, faces, void.** Caps are the
  lightest (levels 5-7), the floor mid grey (4), wall faces and corridors
  darker (2-3), void black. The room's outline (caps on void) is the
  strongest contrast on the screen, so the shape of a level reads first;
  the floor, where sprites stand, sits in the middle, where both a sprite's
  near-black outline and its light colours show against it.
  **Rejected:** today's light floor (level 5, 182): sprites' steel and bone
  (sprite slot 3, (5,5,6)) are the same value and vanish into it.
  **Rejected:** a dark floor: the outline disappears and the dungeon goes
  murky.
- **D4 Neutral stone; warm lights, cool darks; colour for meaning.** The
  stone is grey, the cap's lit edge leans warm (7,7,6) and the deepest
  shade cool (1,1,2), as the sprites' ramps do. Hue is kept for things
  that mean something: wood (doors), later water, lava, moss, blood.
  **Rejected:** a tinted dungeon (brown or blue stone): it competes with
  the monsters' colourways, and a tint is better done as a depth theme,
  one palette for the whole map (Q6).
- **D5 Void is transparent, never black pixels.** Rock, the depth of a
  pit, the dark at the foot of the stairs down: alpha 0. A drawn black
  costs a slot in the bank and would differ from the fallback colour if
  that ever changes (a fade, a theme).
- **D6 Light from the top left.** As the sprites. Caps are lit on their
  top and left edges and shaded on their bottom and right; a brick's top
  row is lit; a cap's overhang throws a 1-pixel shadow on the face below;
  a free-standing thing (a pillar) throws its shadow down and right.
- **D7 A grid on the floor.** Each floor cell is one flag with a 1-pixel
  dark grout line on its top and left, and a 1-pixel lit edge inside it.
  In a turn-based game the player counts cells (a throw's range, a
  monster's distance), and the grout also parts the floor from the caps
  beside it. **Rejected:** no grid: calmer, but distances can't be read.
  **Rejected:** today's grout on all four sides: 2-pixel lines, heavier
  than the walls.
- **D8 A calm background.** A floor tile is its base shade with at most
  about 12 pixels of detail; detail is in the variants, which are rare by
  weight (dungeon_decoration.md 5.2). Corridors are rougher and darker
  (cobbles, levels 2-4) than rooms, so a corridor and a room's floor are
  told apart at a glance. **Rejected:** busy floors (cracks and rubble
  everywhere): sprites and items get lost in them.
- **D9 At most 8 banks for the map's art; aim for 1 or 2.** The import's
  first fit (tileset_import.md D1) never moves a tile's pixel values
  (R9), so a later bank with every colour darkened draws the same tiles
  dimmed by changing only their attribute: the remembered tiles of a
  field of view. Keeping the art in banks 0-7 keeps banks 8-15 free for
  that, or for palette effects. **Rejected:** using banks freely: 16 are
  there, but a dimmed copy of each would not fit.
- **D10 Masters, once agreed.** As sprite_style.md D9: the top wall, the
  floor and the shut door, once the replacement set is agreed (6.9).

## 5. Questions

- **Q1 The geometry (D1).** Keep the thin caps and top-wall faces as
  today? Proposed: yes. A full redesign (thicker walls, faces on more
  walls) would also change the decorator's names and the generator's
  shapes.
- **Q2 The floor's value (D3).** Proposed: a mid grey flag, level 4
  (146), darker than today's 182.
- **Q3 Pits.** Proposed: `hole` is void, and a new `hole_fxxx` draws the
  pit's far wall, a dark rim, where the floor is to the north.
  *(Changed when built: `hole` can't share tile 0 with rock, as first
  proposed: the block table is per tile, and a pit blocks differently from
  rock. It is a tile of its own, void but for 3 `k` pixels, so Aseprite
  never takes it for the empty tile.)* The other sides need nothing: the
  floor's edge against the void is the lip. Other edges (`hole_xfxx`...)
  only if previews show they're needed.
- **Q4 Doors in side walls.** Before, and now, every doorway and
  door is an arch seen from the front, also in a left or right wall.
  Proposed: keep it until dungeon_decoration.md R13 (doors by their
  place) is taken up; then a side door is a cap with a door seen edge on.
- **Q5 Floor variants.** Proposed: three: plain (`floor_w6`), cracked
  (`floor_2_w1`) and worn (`floor_3_w2`): two cells in three are plain.
- **Q6 Depth themes (later).** A different tile palette for deeper levels
  (cold blue stone, red brick), loaded in place of this one, with the same
  pixel values. Proposed: later; the importer would need a second palette
  mapped colour for colour. Section 6.5.3 keeps it possible.

## 6. The rules

### 6.1 Canvas

- Metatiles are 16 x 16, on the tilemap layer "Tilemap 1", in RGB mode,
  transparent as alpha 0 (tileset_import.md section 9).
- Tile 0 is empty: rock.
- Every tile the game uses is named with a slice (dungeon_decoration.md
  5.2). Several names may share a tile, if they are the same terrain's,
  or of terrains that block alike: the level's block table is per tile
  (dungeon_decoration.md R5). A tile drawn twice is a waste of the 64.
- A tile that is all transparent but isn't rock has a few `k` pixels, so
  it isn't the empty tile to Aseprite or to the block table (the pit).
- Every tile is placed on the canvas once, under its slices. The canvas
  has a sample room, the features in row 7, and an empty row 8, outside
  the tilemap's cel, where `rock` is named.
- Tiles are drawn whole: the hardware can mirror and rotate each 8 x 8
  tile, but the game gives a metatile's four tiles one attribute, and the
  light from the top left (D6) isn't symmetrical anyway. A left and a
  right wall are two tiles.

### 6.2 Geometry

All wall pieces are made of three parts at fixed places, so they join:

| Part | Where | Rows, columns |
|---|---|---|
| Horizontal cap | Top and bottom walls, corners, T's | rows 0-3, the full width or to the corner |
| Vertical cap, floor to the east (a left wall) | Left walls, left corners | columns 12-15 |
| Vertical cap, floor to the west (a right wall) | Right walls, right corners | columns 0-3 |
| Face | Under a horizontal cap with floor to the south | rows 4-15 |
| Arch | Doorways and doors | jambs columns 0-3 and 12-15; opening columns 4-11 from row 4; the arch's curve in rows 2-4 |

The face, top to bottom: row 4 the cap's shadow (`k`); two brick courses
of 3 rows, each followed by a mortar row (`d`), rows 5-12; a course of 2
rows, 13-14; row 15 the foot (`k`), where the wall meets the floor.

Bricks are 8 pixels long with a 1-pixel mortar joint; each course is
offset 4 from the one above, so the bond continues across cells.

Free-standing features drawn as tiles (the pillar, stairs) sit on a floor
flag (the tile includes the floor around them) and keep a wall's cap and
face proportions: a pillar is a cap of 10 x 5 over a face of 9 rows.

Stairs go across the screen, so the two read apart at a glance and both
have their top on the right:

- **`stairs_up` rises left to right.** A flight seen from the side,
  standing on the flag: 4 steps 3 pixels wide and 3 high, columns 2-13,
  from row 11 to row 2. Each tread is lit `h` over `q`, with a `d` shadow
  under its nosing; the flight's side is `m`, its foot `k`; it throws a
  shadow down and to the right.
- **`stairs_down` descends right to left.** A stairwell cut into the
  flag, columns 2-13, rows 2-14: 3 treads 4 pixels wide, the top one at
  the right, lit (`h`, `q`), the next a step darker (`p`, `l`), the last
  only its edge (`l`) before the void; each drop from the tread above is
  `k`. The far wall of the well (`d`, edged `k`) shows deeper as the
  treads go down.

### 6.3 Seams

- A cap band that leaves a tile's edge enters the next tile at the same
  rows or columns (6.2). It is shaded as continuing (no lit or shaded
  edge across the join); where it ends (a corner, the void), it is
  closed with its edge shade.
- Face rows are the same in every tile with a face, so courses line up.
- Floors: the grout is on row 0 and column 0 only, so any floor tile
  beside any other makes a 1-pixel grid line.
- Corridors: the cobbles may run to all four edges; check them tiled 3 x 3
  in a row and in a column (corridors are one cell wide).
- Check new tiles in a mock room with every neighbour they can have
  (section 7, step 3), not alone.

### 6.4 Light and shading

- Light from the top left (D6).
- Cap: lit `h` along its top and left edges, base `q`, shade `p` along its
  bottom and right edges. A 4-pixel band is lit, base, base, shade.
- Face: a brick's top row is lit `l`, its body `m`, the mortar `d`; the
  cap's shadow (row 4) and the foot (row 15) are `k`.
- Floor flag: grout `d` on row 0 and column 0, a lit edge `f` on row 1 and
  column 1, the body `l`.
- Shadows thrown by a free-standing tile fall down and to the right, 1
  pixel, in `d`.
- No dithering, no anti-aliasing (as sprite_style.md D5). No outline: the
  map isn't outlined; edges are made by value (cap against void, grout
  against flag).

### 6.5 The palette

The point of this guide. The tilemap's palette is small and works
differently from the sprites'; most ways a tileset goes wrong are ways
of spending it.

#### 6.5.1 The limits

| Limit | From | What it means for drawing |
|---|---|---|
| 15 colours and transparent a metatile | 4-bit tiles; the importer gives a metatile one bank (tileset_import.md R6) | A tile has 15 colours at most. In practice far fewer: the busiest uses 8 (the open door) |
| 16 banks of 15 colours, 240 in all, for the whole map | Tilemap palette 0; it holds only the map (text is on Layer 2, the ULA is off) | Every distinct colour in the tileset is a slot somewhere. A colour used by one tile still costs one |
| Banks are filled first fit, in tileset order, colours appended (tileset_import.md D1) | The importer | Tiles that share colours share a bank. A tile with colours of its own may open a new bank, with copies of the shared colours in it too. The importer's summary says how many banks are used: watch it |
| 8 levels a channel: 0, 36, 73, 109, 146, 182, 219, 255 | 9-bit Next colours, exact only (R5) | There are only 8 neutral greys: the stone ramp has to be built on them. The smallest tint is one level on one channel ((7,7,6) is a warm white). Any other colour is an import error |
| Transparent is pixel 0, shown as black | Tilemap transparency by index (NextReg `$4C` = 0); nothing is under the tilemap, so the fallback colour shows (NextReg `$4A` = black) | Void is transparent (D5). Black pixels aren't needed and cost a slot |
| No reserved colour | The tilemap's 4-bit pixels are transparent by index, not by colour | Layer 2's transparent colour (`$E3`) can be used in tiles |
| A metatile is one colourway | Recolouring by bank is out (tileset_import.md D7) | A metatile in two colours is two metatiles: it costs metatiles (6.8), not banks |
| Pixel values never move when tiles are added (R9) | First fit, appending | A bank can be copied later with its colours changed (dimmed, themed) and still draw the same tiles: the reason for D9 |
| The sprites' palette is separate | Sprite palette 0 (sprite_import.md 5.2) | A tile can use the same colour as a sprite (the wood does) without sharing a slot; matching colours is a choice, made in 6.5.2 |

#### 6.5.2 The tile palette

Levels 0-7 a channel (R, G, B). The key is the grid letter.

| Key | Colour | Name | Use |
|---|---|---|---|
| `h` | (7,7,6) | Cap lit | The lit (top, left) edge of caps; stair treads' edges |
| `q` | (6,6,6) | Cap | Caps' body; stair treads |
| `p` | (5,5,5) | Cap shade | The shaded (bottom, right) edge of caps |
| `f` | (5,5,5) | Flag lit | A flag's lit edge, floor specks: the same colour as `p` |
| `l` | (4,4,4) | Stone light | The floor's flags; a brick's lit row; cobbles' tops |
| `m` | (3,3,3) | Stone | Bricks; cobbles; stair risers; cracks in flags |
| `d` | (2,2,2) | Stone dark | Mortar; grout; cobble gaps; thrown shadows |
| `k` | (1,1,2) | Stone deepest | The shadow under a cap; the foot of a face; inside an arch; a pit's wall; iron bands |
| `W` | (5,3,1) | Wood lit | Doors (sprite slot 7) |
| `w` | (4,2,1) | Wood | Doors |
| `v` | (2,1,0) | Wood dark | Planks' gaps; a door's edge |

10 colours: the whole set is one bank, with 5 free slots.

How to use it:

- **The core is one bank.** Walls, floors, corridors, doorways, stairs
  and pits use only the stone (`h` to `k`), 8 colours. Keep it so: a new
  stone tile adds no colour.
- **A family of special tiles adds a short ramp**, 2 or 3 colours (water:
  light, mid, dark), entered here first. While bank 0 has room (5 slots
  now), they join it; past that, the family opens a second bank with the
  stone colours it also uses. Put a new family's tiles after the core in
  the tileset, so first fit packs the core first.
- **No near-duplicates.** Two colours one level apart in one channel are
  the same colour on a TV; use the one in the table.
- **Not in tiles:** pure white (7,7,7) and the sprites' outline ink
  (0,0,1). They are the sprites' highlights and edges; on the map they
  would compete with them (6.6).
- Check with `make`: the summary lists the banks and their colours. A
  stone tile that opens a bank has a stray colour.

#### 6.5.3 Kept for later

- **Dimmed tiles** (a field of view's remembered cells): a copy of each
  used bank, every colour about half as bright, in banks 8-15. The map
  shows a remembered cell by its attribute alone. Needs D9.
- **Depth themes** (Q6): a second palette with the same layout, loaded for
  deeper levels. Needs the tile palette to be short (D2), so a theme is a
  list of 10 or 15 replacements, not hundreds.
- **Palette cycling** (water, lava, a torch's flicker): its ramp would
  need fixed entries in a bank; first fit doesn't give that today, so it
  would be an importer change.

### 6.6 Contrast with the sprites

- Sprites have a near-black outline (sprite slot 1, (0,0,1)) and white
  highlights; tiles keep away from both ends (6.5.2): the darkest large
  areas a sprite stands on are level 2-3 (corridors), the lightest level 4
  (floor).
- The floor (4,4,4) is mid grey: a sprite's outline is dark against it,
  its steel and bone (5,5,6), (7,7,6) light against it, and its colourway
  saturated where the floor is grey.
- Wall faces (2-3) are behind sprites standing just south of a top wall:
  the outline still parts them, as on the corridors.
- Caps (5-7) are where a sprite only reaches in a lunge (sprite_style.md
  section 8); they needn't hold a sprite.
- Check every new tile with sprites on it: the player, a rat and an item
  (section 7, step 3).

### 6.7 Variants

- A variant changes few pixels from its name's plain tile (a crack, a
  worn patch, a missing brick): at most about 12 pixels, in the tile's own
  colours. The grout, caps and courses stay, so it tiles with the plain
  one.
- The plain tile has the largest weight (D8); a variant that catches the
  eye (a skull, a puddle) is weight 1 among a total of 16 or more.
- Variants are what make a big room less flat; a tileset needs only a few
  of them for its most common looks (floor, top wall, corridor).

### 6.8 Budgets

| Budget | Limit | Before | Now (section 9) |
|---|---|---|---|
| Metatiles | 64, tile 0 included (tileset_import.md R8) | 25 | 28 |
| Banks | 8 for the art (D9), of 16 | 1 | 1 |
| Colours | 15 a bank | 9 | 10 |

Proposed share of the 64 metatiles:

| Use | Metatiles |
|---|---|
| Rock (tile 0) | 1 |
| Wall pieces | 22 (today's 18, and the T's and junctions as needed) |
| Floors and pits | 6 (plain and variants, a pit and its far rim, edges later) |
| Corridors, doorway | 4 |
| Doors, stairs | 4 now; 8 with side doors (Q4) |
| Variants of walls and corridors | 8 |
| Later features (fountains, altars, water, traps) | the rest, about 15 |

### 6.9 Masters

Agreed 2026-10-29 (D10). New tiles are compared with them:

| Class | Master |
|---|---|
| Wall | `wall_xwfw` (top wall) (agreed 2026-10-29) |
| Floor | `floor` (variant 1) (agreed 2026-10-29) |
| Feature | `door_shut` (agreed 2026-10-29) |

## 7. Workflow with Claude

As sprite_style.md section 7, with tiles:

1. **The request** names the tiles by their slice names, and this guide.
2. **Grids.** Claude writes each tile as a grid in the tile palette's keys
   (6.5.2), with notes on the rules that decide it.
3. **Preview.** Claude renders the tiles at 6x, and a mock room at 3x:
   the new tiles in place with every neighbour they can have, the
   decorator's choice of tile per cell, and sprites on the floor (the
   player, a rat, an item). It also reports the colours and banks the set
   would use.
4. **Into the file.** Once agreed, Claude writes the tiles into
   `assets/Tilesheet.aseprite` with Aseprite's command line (a Lua
   script): existing tiles are changed in place, so their slices still
   name them; new tiles are placed on the canvas and named with a slice.
5. **Check.** `make` (the importer's errors, its banks summary), then the
   artist opens the file in Aseprite and runs the game in CSpect.
6. **Commit** the `.aseprite` file and the generated files in `src/data/`
   together (tileset_import.md section 9).

## 8. Exceptions

None yet.

## 9. The replacement tileset

Drawn by the rules above; the grids are in section 10. It was previewed
for review as the tiles at 6x and a mock level at 3x with sprites, beside
the same level in today's tiles; previews aren't kept, and are made again
at step 3 of section 7.

What changed:

| Tiles | Before | Now |
|---|---|---|
| Walls (every piece) | White and light grey caps, a grey brick face | The same shapes; caps shaded by the light (lit, base, shade), the face in three courses with a shadow under the cap and a foot |
| `wall` (the default) | A top wall with red specks | The top wall, with its sides closed |
| `wall_ffff` (pillar) | A hollow square | A pillar: cap and face on a floor flag, with a shadow |
| `floor` | Light flags (182) with 2-pixel grout | Mid flags (146), 1-pixel grout top and left, a lit edge (D3, D7) |
| Floor variants | `floor_2_w1`: a chip | `floor_2_w1` a crack, `floor_3_w2` worn specks, `floor_w6` plain |
| `corridor` | The floor tile | Dark cobbles (D8) |
| `doorway` | An arch over void | An arch over a threshold flag |
| `door_shut`, `door_open` | An arch with a brown door | The arch with a planked door with iron bands; open, the door edge on at the left and the threshold beyond |
| `stairs_up` | Steps rising right to left | A flight seen from the side, rising left to right: lit treads, its side in stone, a shadow down and right |
| `stairs_down` | Steps descending left to right, into black | A stairwell in the floor, descending right to left: the top tread lit at the right, each lower one darker, the last into the void |
| `hole` | A square pit with a frame | Void, on a tile of its own (Q3); new `hole_fxxx`, the far wall's rim |

Colours: 10, one bank (before, 9, one bank): the doors' wood has 3 shades
(before, 2), and the stone 7 (before, 7 greys).

As built (2026-10-29):

1. The tiles were written into `Tilesheet.aseprite` by a Lua script
   (section 7, step 4): tiles 1-24 changed in place, so their slices still
   name them. The old pit (tile 1) is now `hole_fxxx` (its slice renamed);
   `floor_1_w7` is `floor_1_w6`. New: `floor_3_w2` (tile 25, canvas cell
   (5, 1)), `corridor` (tile 26, (7, 7); it shared the floor's tile
   before) and `hole` (tile 27, (6, 7)). The canvas grew a row (128 x 144)
   so `rock` is named on an empty cell, (0, 8).
2. `make`: 28 metatiles, 1 of 16 banks, 10 colours. `make test` passes;
   `test_deco.c`'s check of the floor variants' share changed from about
   12% (1 in 8) to about 33% (3 in 9).
3. CSpect checks (2026-10-29: 1-3 and 5 passed; 4 not run, as the
   generator makes no pits yet: it waits for them):
   1. A level's rooms, corridors, doors and stairs draw in the new tiles;
      caps, faces and floor grout join without seams across cells.
   2. Sprites read on the floor, the corridors and in front of top walls.
   3. Floor variants show about one cell in three; corridors and rooms
      are told apart.
   4. A pit shows its far rim, and void elsewhere.
   5. Stairs up rise left to right; stairs down descend right to left.
4. The masters (6.9): agreed.

## 10. Grids

The tile palette's keys (6.5.2); `.` is transparent.

Top wall `wall_xwfw` (also `wall_xwfd`, `wall_xdfw`, `wall_fwfw`); the
default `wall` is the same with its cap's sides closed (`h` on column 0,
`p` on column 15):

```
hhhhhhhhhhhhhhhh
qqqqqqqqqqqqqqqq
qqqqqqqqqqqqqqqq
pppppppppppppppp
kkkkkkkkkkkkkkkk
llllllldllllllld
mmmmmmmdmmmmmmmd
mmmmmmmdmmmmmmmd
dddddddddddddddd
llldllllllldllll
mmmdmmmmmmmdmmmm
mmmdmmmmmmmdmmmm
dddddddddddddddd
llllllldllllllld
mmmmmmmdmmmmmmmd
kkkkkkkkkkkkkkkk
```

Left wall `wall_wfwx` (and its doorway ends) and top-left corner
`wall_xwwx`; the right wall and right corners mirror the shapes, with the
light still from the top left (`h` on column 0, `p` on column 3):

```
............hqqp    ............hhhh
............hqqp    ............hqqq
............hqqp    ............hqqq
............hqqp    ............hqqq
............hqqp    ............hqqp
     ...                 ...
............hqqp    ............hqqp
```

Bottom wall `wall_fwxw`: rows 0-3 as the top wall's, void below. Bottom
corners: the 4 x 4 where the caps meet, shaded by the same rules.

Floor `floor_w6`, `floor_2_w1`, `floor_3_w2`:

```
dddddddddddddddd    dddddddddddddddd    dddddddddddddddd
dfffffffffffffff    dfffffffffffffff    dfffffffffffffff
dfllllllllllllll    dflllllllmllllll    dfllllllllllllll
dfllllllllllllll    dfllllllmlllllll    dfllllllllllllll
dfllllllllllllll    dflllllllmllllll    dflllflllllllllf
dfllllllllllllll    dflllllllmmlllll    dfllllllllllllll
dfllllllllllllll    dfllllllllllmlll    dflllllllmllllll
dfllllllllllllll    dflllllllllllmll    dflllllllllllfll
dfllllllllllllll    dfllllllllllllml    dfllllllllllllll
dfllllllllllllll    dfllllllllllllll    dflfllllllllllll
dfllllllllllllll    dfllllllllllllll    dfllllllllllllll
dfllllllllllllll    dfllllllllllllll    dfllllllmlllllll
dfllllllllllllll    dfllllllllllllll    dfllllllllllllll
dfllllllllllllll    dfllllllllllllll    dfllllllllllfmll
dfllllllllllllll    dfllllllllllllll    dfllllllllllllll
dfllllllllllllll    dfllllllllllllll    dfllllllllllllll
```

Corridor `corridor` and pillar `wall_ffff`:

```
dmmlldmmmldmmmld    dddddddddddddddd
mllldmlllldmlllm    dffhhhhhhhhhhfff
mlllmmlllmmllldd    dflhqqqqqqqqplll
dmmldddmmldmmmdm    dflhqqqqqqqqplll
lddmmlllddmmlldm    dflhqqqqqqqqplll
lmmllllmdmllllml    dflhpppppppppdll
mllllllmdmllllmd    dflkkkkkkkkkkdll
dlllllmddmllllmd    dflllldlllldldll
dmlllmddddmmmmdd    dflmmmdmmmmdmdll
lddmmmddmlldddmm    dfldddddddddddll
mmldmllmllllmdml    dflldlllldllldll
mllmllllllllmdll    dflmdmmmmdmmmdll
lllmlllmllllmdll    dfldddddddddddll
dmmdmlmddmmmddmm    dflllldlllldldll
dlldmmddmmldmmlm    dflkkkkkkkkkkdll
mdmmdddmlllmdddd    dfllddddddddddll
```

Doorway `doorway`, `door_shut`, `door_open`:

```
hhhhhhhhhhhhhhhh    hhhhhhhhhhhhhhhh    hhhhhhhhhhhhhhhh
hqqqqqqqqqqqqqqp    hqqqqqqqqqqqqqqp    hqqqqqqqqqqqqqqp
hqqqqhhhhhhqqqqp    hqqqqhhhhhhqqqqp    hqqqqhhhhhhqqqqp
hqqhhkkkkkkppqqp    hqqhhkkkkkkppqqp    hqqhhkkkkkkppqqp
hqhkkddddddkkpqp    hqhkkWWvWWvkkpqp    hqhkkvdddddkkpqp
hqhkllllllllkpqp    hqhkvWwvWwvwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwwvwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkkkkkkkkkkpqp    hqhkWkllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwwvwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwwvwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwwvwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwkkwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwkkwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkkkkkkkkkkpqp    hqhkWkllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwwvwkpqp    hqhkWvllllllkpqp
hqhkllllllllkpqp    hqhkWwwvwwvwkpqp    hqhkWvllllllkpqp
```

Stairs `stairs_up` (rising left to right), `stairs_down` (descending
right to left), and the pit's far rim `hole_fxxx`:

```
dddddddddddddddd    dddddddddddddddd    mmmmmmmmmmmmmmmm
dfffffffffffffff    dfffffffffffffff    kkkkkkkkkkkkkkkk
dflllllllllhhhll    dfkkkkkkkkkkkkfl    dkdkkdkkkdkkdkkk
dflllllllllqqqdl    dfddddddddkkkkfl    kk.kkk.kkkk.kk.k
dflllllllllpdddl    dfddddddddhqqqfl    .k..k...k...k..k
dfllllllhhhmmmdl    dfddddkkkkhqqqfl    ....k.......k...
dfllllllqqqmmmdl    dfddddpllkhqqqfl    ................
dfllllllpddmmmdl    dfkkkkpllkhqqqfl    ................
dflllhhhmmmmmmdl    df..lkpllkhqqqfl    ................
dflllqqqmmmmmmdl    df..lkpllkhqqqfl    ................
dflllpddmmmmmmdl    df..lkpllkhqqqfl    ................
dfhhhmmmmmmmmmdl    df..lkpllkhqqqfl    ................
dfqqqmmmmmmmmmdl    df..lkpllkhqqqfl    ................
dfpddmmmmmmmmmdl    df..lkpllkhqqqfl    ................
dfkkkkkkkkkkkkdl    df..lkpllkhqqqfl    ................
dflddddddddddddl    dfllllllllllllll    ................
```

The pit `hole`: transparent but for `k` at (5, 5), (10, 9) and (3, 12).
