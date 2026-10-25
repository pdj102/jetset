# Dungeon decoration: requirements and design

Status: step 1 done (D5, 2026-10-24) on the first design; its CSpect
checks (5.10) are still to do. All decisions were agreed on 2026-10-24.
Revised 2026-10-25 and built the same day: one base for each terrain,
named after it (D13); codes give each neighbour's terrain by a letter, or
`x` for any (D9), in place of join sets; rules are ordered by the
importer, most specific first (D12); a base's default is its variant 1,
in place of fallbacks (D14). The revision's CSpect checks (5.10)
passed on 2026-10-25, with the redrawn tileset. Section 7 lists where the build of the first design differed from
it, and section 8 the revision's changes and where its build differs.

Separates what a map cell **is** (its terrain: rock, wall, floor...) from
how it is **drawn** (one of several metatiles, its look). The generator
builds the dungeon in terrain only. A decorator then picks each cell's
look by rules on its neighbours (which wall piece: a top wall, a
corner...) and by chance among interchangeable looks (floor variations),
the same every time for the same level. Walkability comes from the
terrain, so a new look can never make a wall walkable or a floor solid.

The looks are named in Aseprite, and the names are the rules: a tile
named `wall_xwfw` is the wall with walls east and west, floor south and
anything north, a top wall. The tile importer turns the names into the
header's names and the decorator's rules, so no tile number is written by
hand, and Aseprite renumbering its tiles changes nothing in the code.

## 1. Goal

- The generator stays simple: it knows terrain, not art. Changing it
  doesn't touch the art, and changing the art doesn't touch it.
- Walls are drawn by their place: top, bottom and side walls, corners,
  ends, in the tileset's 3/4 view (walls with a visible face).
- Floors, corridors and walls can have several interchangeable looks,
  chosen at random but the same on every redraw of the level.
- A new look is drawn, named and built: the name says where it goes.
- No tile numbers in the code: every look is used by its name.

## 2. Terms

| Term | Meaning |
|---|---|
| Terrain | What a cell is, for the game's rules: `TERRAIN_ROCK`, `_WALL`, `_FLOOR`, `_CORRIDOR`, `_DOORWAY`, `_HOLE`. Each has a letter, for codes: `r`, `w`, `f`, `c`, `d`, `h` |
| Look | A metatile that draws a terrain, or a feature's state (a shut door): named in Aseprite, `LOOK_<NAME>` in the code |
| Base | The first part of a look's name. A terrain's base is its name (`wall`), and all its looks start with it; a feature's look is a base of its own (`door_shut`) |
| Code | Four letters, read north, east, south, west: each the letter of the terrain that neighbour must be, or `x` for any (5.2). Four more letters, after the first four, do the same for the corners, north-east, south-east, south-west, north-west |
| Rule | A look of a terrain's base: its code is its pattern. A look without a code is the rule that matches any neighbours, the terrain's default |
| Specificity | How many of a rule's 8 letters are not `x`. A terrain's rules are tried most specific first (D12) |
| Variant | One of the interchangeable tiles of a name, by number (`floor_2`); a name without a number is variant 1 |
| Decorating a cell | Choosing its look: of its terrain's rules, the first whose code matches its neighbours; then a weighted pick among that name's variants |

## 3. Requirements

### Terrain

- **R1** The level keeps each cell's terrain for as long as the level
  lasts, apart from the map (`level_terrain`, in the level's page data).
  The map holds only looks (metatile and bank), as now.
- **R2** The generator writes terrain only, and every decision it makes
  reads terrain: a corridor is dug only through rock, a doorway is
  `TERRAIN_DOORWAY`. It never names a look.
- **R3** Rooms' floors and corridors are different terrains, so they can
  look different; both are open. A doorway is its own terrain, open: the
  door, if there is one, is a feature on it, as now.
- **R3a** A hole (a pit, a chasm) is a terrain with no floor: walkers
  can't enter it, but a mover that passes `NGE_BLOCK_NO_FLOOR` (a flyer, a
  ghost) can. It doesn't block sight, when there is a sight rule. The
  generator places holes in room interiors (step 2), never on a doorway or
  next to one, and so that every open cell can still be reached by a
  walker.

### Walkability

- **R4** Each terrain has one block value, nge's blocking flags: rock and
  wall `NGE_BLOCK_SOLID`, hole `NGE_BLOCK_NO_FLOOR`, floor, corridor and
  doorway `NGE_BLOCK_NONE`. A game flag (`NGE_BLOCK_USER` up: blocks sight,
  later) is one more bit in a terrain's value. The map's block table is
  built from the terrains' bases: every look of a terrain's base gets
  that terrain's block value. No look is listed by hand.
- **R5** *(Changed: says why it can happen, and the test reads the
  tables.)* A name belongs to one terrain's base, but a tile may have
  several names (R17), and the block table is by tile. So one tile can be
  named by two terrains: floor and corridor share one, which is allowed,
  as both are open. A tile named by two terrains with different block
  values is an error (a bottom wall and a doorway on one tile: the walls
  can be walked on). A PC test catches it by walking every look of every
  terrain's base in the tables, not by decorating cells.
- **R6** Features' looks (doors, stairs) are named looks too, outside the
  decoration: open in the block table, the feature blocks (a closed door).

### Names (in Aseprite)

- **R17** A look is named with a slice over a placed tile on the
  tileset's canvas (Aseprite's Slice tool; double-click a slice to name
  it). The tile under the slice is the look. A tile may have several names
  (one picture for a left and a right wall); a name, with its variant,
  names one tile.
- **R18** *(Changed: letters for terrains, corners in place of a
  diagonal, D9.)* A name is
  `<base>[_<code>[_<corners>]][_<variant>][_w<weight>]`, in lower case
  letters, digits and `_` (5.2):
  - `<base>`: one or more words: a terrain's name (`wall`, `floor`), or a
    feature's look (`door_shut`);
  - `<code>`: four letters, north, east, south, west: a terrain's letter,
    or `x` for any. A terrain's base only: a feature's look has no code;
  - `<corners>`: four more letters, north-east, south-east, south-west,
    north-west, the same way; after a code only. Left out, all four are
    `x`;
  - `<variant>`: a number from 1, for interchangeable looks of the same
    name. Left out, it is 1;
  - `<weight>`: `w` and a number, 1-255, the variant's share of the
    picks among its name's variants (1 if left out): a variant is chosen
    weight / total weight of the time (5.2, variants and weights).
- **R19** *(Changed: rewritten, D13, D14.)* The tile importer writes, in
  `src/data/`:
  - in `tile_data.h`, for every name (a base, with its code and corners if
    it has them): `LOOK_<NAME>`, its variant 1's metatile, and
    `LOOK_<NAME>_BANK`, that metatile's bank attribute. `<NAME>` is the
    name in upper case without its variant and weight: `wall_xwfw_2_w3`
    is a variant of `LOOK_WALL_XWFW`. And, for every terrain in the list
    (R20), `TILE_TERRAIN_<NAME>`, its number by the list's order;
  - in `tile_rules_data.c`, the decorator's tables: for each terrain, in
    the list's order, its rules most specific first (R8), each with its
    variants and their weights (5.4).
- **R20** *(Changed: terrains, not bases, and nothing is optional, D13.)*
  The game's names are listed for the importer in `assets/tile_names.txt`
  (5.4): the terrains, in `TERRAIN_*` order, each with its letter (its
  name is its base); and the other looks the game names (the features').
  It is an input of the tile import, like the tileset.
- **R21** *(Changed: rewritten, D14.)* Every name the game uses has a
  tile, so every cell gets a look:
  - every terrain in the list has a slice named with its base alone
    (`wall`, or `wall_1`): its default, the rule that matches any
    neighbours, tried last;
  - every other look in the list has a slice named with its base alone;
  - every name with variants has a variant 1 (`floor_xxhx_2` needs a
    `floor_xxhx`): it is the name's `LOOK_`;
  - the list's terrains are the game's: `level_deco.c` checks, at compile
    time, that `TILE_TERRAIN_<NAME>` is `TERRAIN_<NAME>` for every terrain.
- **R22** The importer checks every slice, and reports **errors**:
  - **a critical look missing:** a terrain's default or a listed look
    with no slice, or a name with variants but no variant 1 (R21);
  - **a slice the wrong size or in the wrong place:** larger than one tile
    (16 x 16), crossing from one tile into the next, or outside the
    canvas. A cell outside the tilemap layer's cel is the empty tile, tile
    0, as Aseprite shows it; tile 0 is metatile 0, transparent, so a name
    on it is allowed (`rock`);
  - **a name that is malformed:** characters other than lower case
    letters, digits and `_`; a name that begins with a known base (R20)
    but whose next part isn't a code, corners, a variant or a weight
    (`wall_xwfq`: `q` is no terrain's letter, a mistyped code, not a new
    base); a code on a feature's look; a variant of 0; a weight outside 1-255;
  - *(New.)* **a name's weights adding up to more than 256:** the pick
    is from an 8-bit hash, so the variants past 256 would never be
    chosen (5.2);
  - **a name used twice:** one name and variant on two different tiles
    (`floor` and `floor_1` are the same name).
- **R23** And **warnings**:
  - **an unrecognised name:** a slice whose base isn't in the list (R20):
    it is ignored, and generates nothing;
  - **a repeated slice:** the same name and variant twice on one tile:
    the second is ignored;
  - **a slice with no name** (Aseprite's default `Slice 1`, ...):
    ignored;
  - *(New, D12.)* **two rules that both match some cells**, of the same
    specificity, where no more specific rule matches those cells: the
    first by name is used for them. The warning names both rules and the
    cells' code (`wall_xwwx` and `wall_xxww` both match `xwww`), so the
    artist can draw a look for that case, or make one of the codes exact.
- **R24** The importer reports every problem it finds, not only the first,
  each with the slice's name and its canvas cell (column and row, in
  tiles), so the artist can find it: `error: slice "wall_xwfq" at (3, 2):
  "xwfq" is not a code`. Errors first, then warnings.
- **R25** Any error stops the import: nothing is written, the importer
  exits with status 1, and the build stops (R14). Warnings don't: the
  output is written and the build goes on. The summary also lists the
  tiles that have no name (trial drawings, unused), for information.

### Decoration

- **R7** Decoration runs once the terrain is complete, before features
  are placed (they draw their looks over the cells they are on).
- **R8** *(Changed: one base, letter codes, no fallback, D9, D12-D14.)* A
  cell's look comes from its terrain's base (5.6): of its rules, the first
  whose code (and corners) matches the cell's 8 neighbours' terrains.
  The importer orders the rules most specific first, and rules of the
  same specificity by name; the default matches any neighbours and comes
  last, so every cell gets a look (R21). Then a pick weighted among that
  name's variants.
- **R9** Neighbours off the map count as rock.
- **R10** The pick is a hash of the cell's position and the level's
  decoration seed, not the next random number: the generator's random
  sequence doesn't change when the decoration does, and decorating a
  cell again gives the same look.
- **R11** A cell can be decorated on its own: when a cell's terrain changes
  in play (a feature removed, later a wall dug out), it and its 8
  neighbours are decorated again.
- **R12** The block table and the decoration come from the same tables
  (R4): `level_init()` builds the block table from them.

### Later

- **R13 (later):** doors and stairs with looks by their place (a door in a
  top wall, in a side wall): codes of their own, matched by the same
  rules.
- **R14 (later):** more metatiles than 64 (cells_16.md R15), if the
  looks need them.
- **R15 (later):** falling into a hole: a walker that enters one (pushed,
  or by choice) falls to the next depth, through a level request
  (level.h, `LEVEL_ARRIVE_ANY`), as the development kit's fall does.
- **R16** *(Changed: no bases or join sets to give, up to 25 terrains.)*
  Adding a terrain is a constant, its block value, its line in the list
  with its letter, and where the generator places it (5.8); the decorator
  and the block table need no change, and the tests of R5 and R8 cover it
  by themselves. Up to 25 terrains: a letter each, `x` being any.

## 4. Decisions

- **D1 Terrain in its own array, not in the map.** The map in page 51
  is full (80 x 48 tiles of 2 bytes, and the block table), so it can't
  hold a third byte a cell. Terrain is read by the generator and the
  decorator, which run in the level's pages, never every frame: it goes
  in page 103, `LEVEL_WIDTH x LEVEL_HEIGHT` bytes (2,400), where 8K has
  138 bytes used. Run-time rules keep reading the block table by look,
  as now. **Rejected:** terrain in the map's `tile_id` until decoration
  replaces it: decorated and undecorated cells would be told apart only
  by the moment, and a cell couldn't be decorated again later (R11).
  **Rejected:** a look-to-terrain table (256 bytes) instead of the
  array: a look may serve two terrains (floor and corridor sharing one,
  as now), so the look can't say which it is.
- **D2 One decorator for rules and chance.** A code picks the case (stage
  2); the name's variants are the chances (stage 3). One table walk does
  both, and a look with no variants is a name with one tile.
- **D3 (revised 2026-10-25) Patterns on the 8 neighbours' terrains.** The
  tileset is in 3/4 view: a top wall shows its face, a bottom wall its
  top, and both have walls left and right. What tells them apart is where
  the floor is: below a top wall, above a bottom wall, only diagonally at
  a corner. A code gives each side's terrain, or any, and its corners the
  diagonals' (D9). A terrain whose looks depend on different neighbours
  (a floor with a hole south of it shows a lip, one with a wall north of
  it a shadow) has a code for each, in its one base (`floor_xxhx`,
  `floor_wxxx`). The 8 neighbours' terrains are read once a cell.
  **Rejected:** a 4-bit mask of wall neighbours alone: no corners, and no
  top or bottom. **Rejected:** a 256-entry table a terrain: big, and
  almost every entry the same. **Rejected:** join sets, the first design:
  each base had a set of terrains that "join" (`x`) and the rest didn't
  (`o`). The same code meant different things in different bases, and a
  terrain that cared about two kinds of neighbour needed two bases, tried
  in order, with a fallback in the last. **Rejected:** joining only the
  cell's own terrain, with no join sets: rock and floor would both be
  `o`, so a top wall and a bottom wall would have the same code (`oxox`),
  and so would a left wall and a right wall (`xoxo`).
- **D4 The hash and its seed.** The decoration seed is drawn once, as
  the first random number of `level_generate()`, so each visit to a depth
  looks different (or the same, with the development kit's seed), and the
  generator's own numbers come after it whatever the decoration does.
  The pick is `hash(x, y, seed) % total weight` (5.6).
- **D5 Two steps.** Step 1: terrain, the names and rules from slices, the
  decorator, and names on today's tiles that draw the levels as today
  (5.3); the tests move to terrain. Step 2: the art's wall pieces, floor
  variations and holes, named, and the generator's holes.
- **D6 Feature looks stay outside the decoration for now (R6, R13).** Doors
  and stairs are drawn by their features, over the decorated cell, with
  named looks (`LOOK_DOOR_SHUT`); when a feature goes, its cell is
  decorated again (`level_put_floor()` becomes that).
- **D7 (revised) The rules come from the names; the block values are in
  C.** First decided as C tables of rules, because Aseprite can't give a
  tile data in its UI (tileset_import.md D3). Slices can name a tile on
  the canvas, and a name that says its neighbours is a rule: the importer
  generates the rules, so a new piece is drawn, named and built, with no
  C to change. What is about the game, not the art, stays in C: each
  terrain's block value (`game/level/level_deco.c`). The terrains
  themselves are in both, `TERRAIN_*` and the list (R20), and checked
  against each other at compile time (R21).
- **D8 A hole is a terrain, not a feature.** It is part of the place,
  drawn by its neighbours like a wall (its edges show which side the floor
  is), and nge already has its blocking flag (`NGE_BLOCK_NO_FLOOR`). A
  feature is for things with state or actions (a door opens, stairs are
  climbed); a hole has neither until R15, and even then falling is the
  hole's terrain acting on whoever enters. **Rejected:** a hole as a
  feature entity: an entity a cell, and no edges.
- **D9 (revised 2026-10-25) Looks are named by their neighbours'
  terrains.** Each letter of a code is the terrain that neighbour must be
  (`r` rock, `w` wall, `f` floor, `c` corridor, `d` doorway, `h` hole), or
  `x`, any. A code says the case exactly, means the same in every base,
  and sorts with its kind: `wall_xwfw` is a wall with walls each side and
  floor below, whatever is above. `x` for any keeps the codes of the
  common cases short, and absorbs what the generator puts outside a room
  (rock, or a corridor passing). Corners are a second group of four
  letters, so a code can say all 8 neighbours (`wall_wwww_xfxx`: walls on
  four sides and floor only to the south-east). **Rejected:** names by
  place (`wall_top`, `wall_corner_tl`): they read well, but each needs its
  rule written by hand, and new shapes (ends, joins, pillars) need new
  words. **Rejected:** `x`/`o` codes by join sets (D3). **Rejected:** a
  set of terrains for one letter (`s` for wall or rock): one more thing to
  define and learn; two rules, or `x`, do the same.
- **D10 Names come from slices, not tile numbers.** Aseprite renumbers its
  tiles when one is added or removed, so numbers kept in the code go
  wrong silently (a wall that draws as floor, or walks like one). A slice
  stays on its tile's place on the canvas, so the names follow the tiles.
  **Rejected:** numbers in `level_internal.h`, as now. **Rejected:** a
  separate list of names by number: the same problem, in another file.
- **D11 The importer knows the game's names from a list, and finds every
  problem at once.** A missing look would otherwise be a compile error
  where its `LOOK_` name is used, which says what is missing but not that
  the cause is in Aseprite. With the list, the importer says it plainly
  ("no slice named `wall`, the wall's default"), and can tell a mistyped
  name of a known base (an error) from a name the game doesn't use yet (a
  warning: an artist can name pieces before the game needs them). All
  problems are reported in one run, because fixing them means a trip to
  Aseprite each. **Rejected:** reading the names from the C
  (`level_deco.c`): the importer would parse C. **Rejected:** treating
  unknown names as errors: the artist couldn't draw and name ahead of the
  code.
- **D12 (new, 2026-10-25) The importer orders the rules, most specific
  first.** A rule with more letters other than `x` matches fewer cells, so
  where it matches it is the better look: a pillar (`wall_ffff`) before a
  top wall (`wall_xwfw`), the default (all `x`) last. A rule that matches
  only cells another also matches always has more letters, so it is
  always tried first, with no ordering to get right. Rules of the same
  specificity rarely match the same cells; where they do, name order
  decides and the importer warns (R23). **Rejected:** ordering by hand,
  in the list: every look named twice, once in Aseprite and once in the
  list. **Rejected:** the slices' order: Aseprite doesn't show it, and
  moving a slice would change the art.
- **D13 (new, 2026-10-25) One base for each terrain, named after it.** With
  letter codes, one base says every case of a terrain, whatever its
  neighbours (D3): the floor's lip is `floor_xxhx`, not a `floor_lip`
  base. So a terrain's base is its name, the list gives the terrains
  rather than bases, and the base tables are in terrain order. The C
  keeps only each terrain's block value: no bases, their order, or join
  sets. **Rejected:** several bases a terrain, the first design: their
  order, their join sets, and a fallback in the last, for what a code now
  says alone.
- **D14 (new, 2026-10-25) A terrain's default is its base alone, variant
  1, and is required.** The default is the rule with no code: it matches
  any neighbours, sorts last (D12), and so gives a look to every cell no
  other rule matches. Every terrain must have it (R21), so there is no
  fallback to look for, and no cell can be left without a look. A name
  without a variant number is variant 1, and every name has a variant 1,
  its `LOOK_`. **Rejected:** fallbacks, the first design: a separate part
  of every base, optional in some, with the rule that the last base of a
  terrain had one. **Rejected:** a variant number required on every name:
  `door_shut_1` for a look with one tile.
- **No rotated or mirrored looks.** A metatile is 2 x 2 hardware tiles:
  rotating it would mean swapping its four tiles too, not only setting
  their bits, and in the 3/4 view the pieces aren't rotations of each
  other (a top wall shows its face, a bottom wall doesn't).

## 5. Design

### 5.1 Terrain (`game/level/level_internal.h`, `level_pdata.c`)

```c
/* What a cell is, for the game's rules: the generator's map. In the order of
   assets/tile_names.txt, whose letters name them in the looks' codes */
#define TERRAIN_ROCK      0   /* r */
#define TERRAIN_WALL      1   /* w */
#define TERRAIN_FLOOR     2   /* f: a room's floor */
#define TERRAIN_CORRIDOR  3   /* c */
#define TERRAIN_DOORWAY   4   /* d: where a corridor meets a room: a door may stand on it */
#define TERRAIN_HOLE      5   /* h: no floor: a pit, a chasm */
#define TERRAIN_COUNT     6

extern uint8_t level_terrain[LEVEL_HEIGHT][LEVEL_WIDTH];   /* page 103 */
```

The generator's `put()` becomes `set_terrain(x, y, t)`; `put_corridor()`
tests `TERRAIN_ROCK`; `door()` sets `TERRAIN_DOORWAY`; `clear()` fills
with rock. The rooms' interior search (`try_floor()`) is unchanged: it
uses the room list.

### 5.2 Look names

`<base>[_<code>[_<corners>]][_<variant>][_w<weight>]`. The importer
takes the longest base in the list (R20) that the name starts with, then
reads the parts after it: four letters of terrains' letters and `x`, the
code; four more, the corners; digits, the variant; `w` and digits, the
weight. In the code, `LOOK_` and the name in upper case, without its
variant and weight.

A code matches a cell when each of its letters is `x` or the letter of
that neighbour's terrain (rock off the map, R9). Without corners, the
corners are `x`.

The walls, for Rogue's rectangular rooms (rock outside, floor inside,
doorways in the walls, corridors that may pass outside):

| Name | N E S W | Piece |
|---|---|---|
| `wall_xwfw` | any, wall, floor, wall | top wall: floor below, its face shows |
| `wall_fwxw` | floor, wall, any, wall | bottom wall |
| `wall_wfwx` | wall, floor, wall, any | left wall: floor to the east |
| `wall_wxwf` | wall, any, wall, floor | right wall |
| `wall_xwwx` | any, wall, wall, any | top-left corner |
| `wall_xxww` | any, any, wall, wall | top-right corner |
| `wall_wwxx` | wall, wall, any, any | bottom-left corner |
| `wall_wxxw` | wall, any, any, wall | bottom-right corner |
| `wall_xdfw`, `wall_xwfd` | a doorway east, west | a top wall ending at a doorway |
| `wall_fdxw`, `wall_fwxd` | a doorway east, west | a bottom wall ending at a doorway |
| `wall_dfwx`, `wall_wfdx` | a doorway north, south | a left wall ending at a doorway |
| `wall_dxwf`, `wall_wxdf` | a doorway north, south | a right wall ending at a doorway |
| `wall_fwfw`, `wall_wfwf` | floor on two opposite sides | a wall one cell thick, across or upright |
| `wall_ffff` | floor all round | a pillar |
| `wall_wwww` | walls all round | a junction, or inside a thick wall; with corners, which junction (`wall_wwww_xfxx`) |
| `wall` | the default: any | any case not drawn yet |

Rogue's rooms need the first eight and the default; the doorway ends
draw the cells beside a doorway, which otherwise take the default. The
others can be drawn when the generator makes other shapes. The four
corners match some cells two at a time, where three walls meet (`xwww`, a
T): the importer warns about these (R23) until the T is drawn
(`wall_xwww`), or the corners are made exact (`wall_rwwr`: rock north
and west, so no corridor may pass).

**Variants and weights.** The variants of a name are the tiles with the
same base, code and corners, and different numbers: `floor`, `floor_2`,
`floor_3` are one name, `LOOK_FLOOR`, with three tiles. When a cell
matches that name, one of its tiles is picked, and the weights say how
often each is. They are relative: each variant is chosen its weight
divided by the total of the name's weights, so only their ratios matter.

| Slices | Total | Chances |
|---|---|---|
| `floor`, `floor_2`, `floor_3` | 3 | a third each (no weights: all 1) |
| `floor_w6`, `floor_2_w3`, `floor_3` | 10 | 60%, 30%, 10% |
| `floor_w8`, `floor_2` | 9 | 8 in 9, 1 in 9 (today's floor) |
| `floor_w99`, `floor_2` | 100 | 99%, 1%: a rare crack |

To assign them:

- decide how often each variant should show, as a ratio (plain floor six
  times as often as cracked, cracked three times as often as mossy), and
  write the ratio as whole numbers (`w6`, `w3`, `w1`, the last left out);
- give the plain, most common tile the largest weight, and a rare detail
  1;
- keep the total at most 256 (an error otherwise, R22). Totals that
  divide 256 (2, 4, 8, 16...) give exact shares; others are off by at
  most one pick in 256 for each unit of weight (`floor_w8` and `floor_2`,
  total 9: `floor_2` gets 28 of the 256 hash values, 10.9%, where 1 in 9
  is 11.1%), too little to see;
- weights count within one name only: `floor_xxhx`'s variants are picked
  among themselves, apart from `floor`'s.

Each cell's pick comes from its position and the level's seed (R10,
5.6), not a counter, so the shares hold over a level, not exactly in any
one room: over a 60 x 40 level, a 1 in 4 variant shows on 15-35% of the
cells (5.10).

Other bases: `rock`; `floor`, `floor_2_w7`, `floor_3`... (variants, by
weight); `floor_xxhx` (a lip: a hole to the south); `corridor`;
`doorway`; `hole`, with codes for its edges (`hole_fxxx`: the far edge,
the pit's wall showing; `hole_xxfx`: the near lip...); and the features'
looks, `door_shut`, `door_open`, `stairs_up`, `stairs_down`.

### 5.3 The tileset's names, renamed (D9, D14)

Step 1 named today's tiles by the first design's codes. The revision
renames them; the tiles are unchanged:

| First design | Revised | Piece |
|---|---|---|
| `wall_xxox` | `wall_xwfw` | top wall |
| `wall_oxxx` | `wall_fwxw` | bottom wall |
| `wall_xoxx`, `wall_xxxo` | `wall_wfwx`, `wall_wxwf` | left and right walls |
| `wall_xxxx_se`, `_sw`, `_ne`, `_nw` | `wall_xwwx`, `wall_xxww`, `wall_wwxx`, `wall_wxxw` | top-left, top-right, bottom-left, bottom-right corners |
| `wall_oxox`, `wall_xoxo` | `wall_fwfw`, `wall_wfwf` | one cell thick, across and upright |
| `wall_oooo` | `wall_ffff` | pillar |
| `wall_xoox` | `wall_xdfw`, `wall_wfdx` | a wall ending at a doorway: each first-design code with two sides open was two places. Here a top wall with a doorway east, a left wall with one south |
| `wall_xxoo` | `wall_xwfd`, `wall_wxdf` | a top wall with a doorway west, a right wall with one south |
| `wall_ooxx` | `wall_fdxw`, `wall_dfwx` | a bottom wall with a doorway east, a left wall with one north |
| `wall_oxxo` | `wall_fwxd`, `wall_dxwf` | a bottom wall with a doorway west, a right wall with one north |
| `floor`, `floor_1_w7`, `floor_2` | `floor_w8`, `floor_2` | `floor` and `floor_1` are now one name (R22): 1 in 9 is `floor_2`, as now |

`wall`, `rock`, `corridor`, `doorway`, `hole` and the features' names are
unchanged.

### 5.4 The tile importer (`scripts/ase2tiles.py`, `scripts/aseprite.py`)

- **Reading (`aseprite.py`):** slices (chunk `0x2022`: name, and the
  first key's rectangle) and the tilemap layer's cel (cel kind 3: the
  canvas's tiles, 32 bits each, the tile number in the low bits).
- **The game's names (`assets/tile_names.txt`, R20):** one line each, `#`
  for comments:

  ```
  # Terrains, in TERRAIN_* order: the letter that names them in codes,
  # and their name, which is their base
  terrain r rock
  terrain w wall
  terrain f floor
  terrain c corridor
  terrain d doorway
  terrain h hole
  # Other looks the game names: the features'
  look door_shut
  look door_open
  look stairs_up
  look stairs_down
  ```

  Every name listed must have its slice (R21). A letter is one lower case
  letter other than `x`. Errors of the list, each with its line number:
  an unknown keyword, a name or a letter used twice, a letter that is not
  a letter or is `x`. Adding a terrain or a look the game names is a line
  here and its use in the code.
- **Names:** each slice's rectangle must lie within one canvas cell of the
  tilemap layer's cel (R22); the tile there is the look. Names are parsed
  (5.2), checked against the list, and grouped by base, then by name, then
  by variant.
- **Order:** a terrain's rules are sorted by specificity, most first, and
  by name where it is the same (D12). Two rules of the same specificity
  overlap when no position has two different letters; the cells they
  share are the code with each position's letter that isn't `x`. The
  importer warns (R23) unless a more specific rule matches all of those
  cells: it has, at each position, `x` or the shared code's letter.
- **Checks (R22-R25):** every slice is checked and every problem
  collected; the list's required names are checked last. Then errors and
  warnings are printed, errors first, each with the slice and its cell;
  with any error the importer writes nothing and exits with status 1.
- **Output, added to today's (`src/data/`, R19):**
  - `tile_data.h`: `LOOK_<NAME>` and `LOOK_<NAME>_BANK` for every name;
    `TILE_TERRAIN_<NAME>` for every terrain, and `TILE_TERRAINS`, their
    count; `TILE_ANY`, a neighbour any terrain; the tables' types and
    declarations.
  - `tile_rules_data.c`: the rule tables, constants in page 103 with the
    level's data (`#pragma constseg PAGE_103`, as the generators' tables
    are in their data pages):

    ```c
    const tile_look_t tile_looks[];   /* tile, bank attribute, weight */
    const tile_rule_t tile_rules[];   /* want[8] (a terrain, or TILE_ANY), first look, looks */
    const tile_base_t tile_bases[];   /* a terrain's: first rule, rules (the last is its default) */
    ```

    `want` is in the order N, NE, E, SE, S, SW, W, NW, as the
    decorator reads the neighbours. `tile_bases` has one entry a terrain,
    in terrain order: `tile_bases[TERRAIN_WALL]` is the wall's. The
    features' looks have names in the header but no tables.
- **The build:** `tile_rules_data.c` is a fourth output of the tile rule
  (`make tiles`), `assets/tile_names.txt` one more input, and the
  Makefile builds `src/data/*.c` too. Warnings show in the build's output
  and don't stop it. The PC tests
  link it with the level module.

### 5.5 The decorator (`game/level/level_deco.c`, page 102)

```c
/* Each terrain's block value (R4): its base's looks block that */
static const nge_block_t terrain_blocks[TERRAIN_COUNT] = {
    [TERRAIN_ROCK]     = NGE_BLOCK_SOLID,
    [TERRAIN_WALL]     = NGE_BLOCK_SOLID,
    [TERRAIN_FLOOR]    = NGE_BLOCK_NONE,
    [TERRAIN_CORRIDOR] = NGE_BLOCK_NONE,
    [TERRAIN_DOORWAY]  = NGE_BLOCK_NONE,
    [TERRAIN_HOLE]     = NGE_BLOCK_NO_FLOOR,
};
```

With it, a compile-time check that `TILE_TERRAINS` is `TERRAIN_COUNT` and
each `TILE_TERRAIN_<NAME>` its `TERRAIN_<NAME>` (R21): a terrain missing
from the list, or out of order, is a compile error.

```c
/* Choose cell (x, y)'s look by its terrain's rules: writes the map tile */
void level_decorate(uint8_t x, uint8_t y);

/* Decorate every cell: once the terrain is complete (R7) */
void level_decorate_all(void);

/* A cell's terrain changed: decorate it and its 8 neighbours (R11) */
void level_decorate_around(uint8_t x, uint8_t y);

/* Every look's block value into the map's block table (R4, R12) */
void level_deco_blocks(void);
```

### 5.6 Matching and picking

- **The neighbours:** the 8 neighbours' terrains, read once, in the
  order N, NE, E, SE, S, SW, W, NW (rock off the map, R9).
- **The match:** the rules of `tile_bases[terrain]`, in order: the first
  where, at each of the 8 positions, `want` is `TILE_ANY` or that
  neighbour's terrain. The last rule is the default, all `TILE_ANY`, so
  one always matches.
- **The pick:** `h = hash(x, y, level_deco_seed)`, 0-255, then the look
  at `h % total weight` along the matched name's variants: each variant
  takes as many values in a row as its weight, so variants `w3` and `w1`
  take 0-2 and 3. A total over 256 would leave the last values out of
  reach (R22). The hash, 8-bit,
  cheap on the Z80:

  ```c
  h = (uint8_t)(x * 151 + y * 59) ^ seed;
  h ^= h >> 3;
  h *= 109;   /* Z80N mul */
  h ^= h >> 4;
  ```

  Its spread is tested on the PC (5.9); if a level shows stripes, the
  constants change.
- **Cost:** a rule usually fails at its first or second letter that isn't
  `x`. With some 20 rules for walls and one or two for the rest, about
  700 T-states a cell on average, 2,400 cells: about 1.7M T-states, 60 ms
  at 28 MHz, once a level.

### 5.7 The generator and the level (`level_gen.c`, `level_bank.c`, `level.c`)

- `level_generate()`: `level_deco_seed = (uint8_t)rand()` first (D4);
  terrain as now; then `level_decorate_all()` after `make_corridors()`,
  before `place_stairs()` (R7).
- `level_init()`'s `set_up_blocks()`: `nge_map_clear_blocks(NGE_BLOCK_SOLID)`,
  `level_deco_blocks()`, then the features' looks, open, as now.
- `level_put_floor()` (a feature gone) becomes "decorate the cell again":
  its terrain is unchanged (a doorway, or floor under stairs), so
  `level_decorate_around()` gives it its look back.
- The tile numbers in `level_internal.h` and `level.h` (`LEVEL_FLOOR`,
  `LEVEL_TILE_DOOR_SHUT`...) go, and `level_tile_check` with them: the
  features use `LOOK_DOOR_SHUT` and `LOOK_DOOR_SHUT_BANK`, and nothing else
  names a look.

### 5.8 Adding a terrain (R16), or a look

A terrain:

1. A `TERRAIN_*` constant before `TERRAIN_COUNT` (up to 25).
2. Its block value in `terrain_blocks`.
3. Its line in `assets/tile_names.txt`, in the same place, with an unused
   letter.
4. Where the generator places it, in terrain only.
5. Its looks, drawn and named in the tileset: at least its default, its
   name alone. Other terrains whose looks depend on it get codes with its
   letter (as `floor_xxhx` names the hole).

A look for an existing terrain: draw it, place it on the canvas, put a
slice on it with its name, `make`. Nothing in the code changes. If `make`
warns that the name is unrecognised, its base isn't in the list: a
misspelt base, or a new terrain to add (above).

Limits: 25 terrains (a letter each), 255 rules and 255 looks in the
tables (`uint8_t` indices), and the tileset's 64 metatiles (R14).

### 5.9 Memory

- **Page 103:** `level_terrain`, 2,400 bytes, the seed, and the rule
  tables: 3 bytes a look, 10 a rule, 2 a terrain; step 2's walls,
  floors and holes about 40 looks and 30 rules, under 0.5K. As built, 31
  looks and 30 rules: 406 bytes.
- **Page 102:** the decorator, about 0.5K, and `terrain_blocks`.
- **Common memory:** unchanged.

### 5.10 Tests

- **The importer** (`tests/scripts/test_ase2tiles.py`, files built by
  `ase_writer.py`, which learns slices and tilemap cels):
  - slices read, and the tile under each;
  - names parsed: base (the longest listed), code, corners, variant (1 if
    left out), weight;
  - each error of R22 (a terrain's default or a look missing, a name
    without variant 1, a slice larger than a tile, one crossing two tiles,
    one off the canvas; a slice outside the cel names tile 0; a code with
    a letter no terrain has, corners without a code, a code on a feature's
    look, a variant of 0, a weight of 0 or 256, a name's weights adding
    up to 257, a name on two tiles,
    `floor` and `floor_1` both), each named with its slice and cell;
  - each warning of R23 (an unknown base, a repeated slice, an unnamed
    slice, two overlapping rules of the same specificity; none when a
    more specific rule covers their overlap): the output is still
    written, without the ignored slices;
  - several problems in one file are all reported, errors before
    warnings; with an error nothing is written;
  - the list's syntax: an unknown keyword, a repeated name or letter, the
    letter `x`, each an error of the list, with its line number;
  - a tile with two names;
  - rules in order (most specific first, by name on a tie, the default
    last), `want` for a code with and without corners; variants'
    weights;
  - the generated header and tables for a small tileset;
  - the real tileset reads, and every slice in it parses.
- **`test_level`:** its checks of the layout (rooms walled, corridors
  through rock, doorways, every room reachable) read `level_terrain`, not
  the map: they test the generator, whatever the art. Reachability is a
  walker's: holes are not open (R3a). With holes (step 2): none on or
  next to a doorway, only in room interiors. New: every map tile is a
  look of its cell's terrain's base (or a feature's look); every cell's
  block value is its terrain's.
- **`test_deco`** (new, the level module and the tables linked):
  - a cell's neighbours, with those off the map as rock;
  - a top wall, each corner, a side wall and a wall end match their names
    (with the real tables, where the art has them), and a cell no rule
    matches takes the default;
  - a more specific rule wins over a less specific one that also matches
    (a pillar over a top wall, where the art has both);
  - every terrain's base ends with its default (R8, R21);
  - no tile is named by two terrains with different block values (R5):
    every look of every rule of every terrain, from the tables, blocks
    its terrain's value;
  - weights: over a 60 x 40 level, two variants weighted 1 and 3 are
    chosen 15-35% and 65-85% of the time (the hash's spread);
  - decorating a cell twice gives the same look (R10);
  - the terrain a seed makes doesn't depend on the tables (D4);
  - `level_decorate_around()` redecorates the 3 x 3 cells only.
- **Step 1 is nearly a refactor:** levels are built and drawn by the same
  rules as today, but each seed gives a different dungeon from today's,
  once: the decoration seed is one more number drawn first (D4), and the
  floor variant is chosen by the hash. From then on, changes to the
  decoration never change the dungeon a seed makes.

CSpect checks (step 1): levels look and play as before; walls block, at
every place; doors open and close, and a removed feature leaves its
cell's look. Step 2 adds checks of the wall pieces in every position.
After the revision: the same levels draw the same, apart from the cells
beside a doorway, which take the doorway ends where they are drawn.

## 6. Scope

In (step 1, D5):

- R1-R12 and R16-R21: terrain, the decorator, names from slices and the
  rules made from them, the hole terrain with its placeholder look, not
  placed;
- the names of 5.3 on today's tileset (slices to add in Aseprite), and
  `assets/tile_names.txt`;
- the tests;
- docs: `docs/memory_map.md` (page 103), `level.h`'s comment on map
  tiles, `docs/tileset_import.md` (names from slices, D3 there replaced;
  section 9, a new look).

In (the revision, 2026-10-25): the changes of section 8.

Out, for later:

- step 2: the art's wall pieces, floor variations and holes, named; the
  generator's holes (R3a);
- falling into holes (R15);
- doors and stairs by their place (R13);
- more than 64 metatiles (R14);
- prefab rooms: hand-made rooms placed by the generator, in terrain, so
  they are decorated like the rest.

## 7. As built: differences from the design

The first design, built 2026-10-24. Items marked *superseded* are
replaced by the revision (section 8).

- **The tileset already has step 2's wall pieces.** Its names (5.3, first
  design) give the four corners, top, bottom and side walls, a pillar and
  walls one cell thick, so step 1 draws walls by their place already; the
  cells beside a doorway still take the fallback (`wall`, a top wall's
  look).
- **`level_put_floor()` is `level_redraw()`:** it puts back the cell's
  look by its terrain, which isn't always floor (a doorway).
- **A slice outside the tilemap layer's cel names tile 0** (R22 as
  corrected): rock is the empty tile, and `rock` was named where the
  canvas has no cel.
- *Superseded (D14: a name without a number is variant 1).* **A name
  without a variant number is a variant too.** `floor` (weight 1) with
  `floor_1_w7` and `floor_2` is three variants, two on the same tile:
  `floor_2` shows 1 time in 9. Without the plain `floor` slice it would be
  1 in 8, as before (`floor_1` is then the required name's tile).
- **Variants have no names of their own:** `LOOK_FLOOR` is the first
  variant's tile; the others are only in the tables. (The revision: it is
  variant 1's.)
- *Superseded (5.4: the features' looks have no tables).* **`look` and
  `base` lines in the names file are treated alike:** a feature's look is
  a base with only a fallback, and its tables are generated but unused.
- **R5 was broken by the tileset (found and fixed 2026-10-25):** tile 23 was both
  `wall_oxxx` (a bottom wall) and `doorway`, so bottom walls were open,
  and `test_level` failed (depths 1, 4, 6, 10). The bottom wall moved to a
  tile of its own. `test_deco`'s check of R5
  decorates a cell among neighbours of one terrain, which never makes a
  bottom wall, so it missed it; 5.10's test reads the tables instead.
- **`tile_data.h` keeps `TILE_BANK()`** beside the new `LOOK_*_BANK`
  names, for code that has a metatile number.
- **The decorator walks its tables by pointer.** An index into a table in
  a banked page made zsdcc split the 24-bit address into bytes, which
  z80asm reports as "integer range" (the code was right). A pointer and
  `+=` compiles to 16-bit loads, with no warning.
- **Tests:**
  - `test_level` checks every cell's blocking against its terrain's,
    which would have caught a wall that can be walked on; and that every
    cell not under a feature redraws to the same look.
  - `test_deco` runs on the real tables: the wall pieces in a room, off
    the map as rock, a pillar, every terrain among every neighbour, the
    floor variant's share, the same look each time, and redrawing around
    a cell.
  - Two checks of 5.10 aren't in C: a terrain's bases in order (the art
    has no `floor_lip` yet; the importer's tests check the tables' order),
    and the terrain not depending on the tables (the tables are generated,
    so a test can't swap them; it holds by construction: the seed is
    drawn first, and the decorator draws no random numbers).
- **Memory:** page 102 holds 6,455 of 8,192 bytes, page 103 2,692 (the
  terrain 2,400, the tables 154); common memory is unchanged (546 bytes
  free in the debug build, 7,291 in the release build).

## 8. The revision: changes to the build

Built 2026-10-25: these are done.

- **`assets/tile_names.txt`:** `terrain <letter> <name>` lines in
  `TERRAIN_*` order, and `look` lines; no `required` (5.4).
- **The importer** (`scripts/ase2tiles.py` and the names module):
  - parse letter codes and corners, by the list's letters, and the base
    as the longest listed one; a name without a variant is variant 1;
  - R21's checks: every terrain's default and every look, and every
    name's variant 1;
  - rules sorted by specificity, then by name, the default last; the
    overlap warning (R23);
  - `tile_rule_t` with `want[8]`; `tile_base_t` without the fallback, one
    a terrain, in terrain order; no tables for the features' looks;
  - `TILE_TERRAIN_<NAME>`, `TILE_TERRAINS` and `TILE_ANY` in the header;
    `LOOK_BASE_*` go.
- **`level_deco.c`:** `deco_base_t`, `deco_terrain_t` and
  `SOLID_TERRAIN` go, for `terrain_blocks[]` and the compile-time check
  (5.5); `level_decorate()` compares the neighbours with each rule's
  `want`, and has no fallback; `level_deco_blocks()` walks one base a
  terrain.
- **`level_internal.h`:** `terrain_set_t` and `TERRAIN_BIT()` go; the
  terrains' letters in their comments.
- **The tileset:** its slices renamed (5.3).
- **Tests:** `test_ase2tiles.py` and `test_deco.c` as in 5.10, with the
  new names (`LOOK_WALL_XWWX`...).
- **Docs:** `docs/tileset_import.md` section 9 (a new look's name).

As built, the differences from the design:

- **The tileset's names differ from 5.3's table.** The art was renamed
  and redrawn with the revision: it has the T junctions (`wall_xwww`,
  `wall_wxww`, `wall_wwxw`, `wall_wwwx`) and `wall_wwww`, which settle
  where two corners overlap, and all eight doorway ends (5.2), so the
  cells beside a doorway have pieces of their own; `wall`, the default, is
  a tile of its own. The floor is `floor_1_w7` and `floor_2_w1`: `floor_2`
  shows 1 time in 8, not 9.
- **Eight overlap warnings on the real tileset** (R23): four where a wall
  meets a room's wall from outside (`fwww`, `wfww`, `wwwf`, `wwfw`), and
  four where two doorway ends meet on a wall one cell thick (`dfwf`,
  `fdfw`, `fwfd`, `wfdf`). Rogue's separate rooms make neither.
- **Corners without a code can't be written:** the first group of four
  letters after a base is always the code, so R22's error for it went.
- **`terrain_blocks` is walked by pointer** in `level_deco_blocks()`, as
  the tables are (section 7): indexing it gave the "integer range" warning
  again.
- **`level_decorate()` asserts if no rule matches**, which the default
  makes impossible; the map's tile is then left as it was.
- **Tests:**
  - `test_deco` checks R5 from the tables, every look of every rule of
    every terrain against its terrain's block value, and that each
    terrain's last rule is all `TILE_ANY`; and that walls all round take
    `wall_wwww` over the less specific rules that also match (D12).
  - `test_tile_names.py`: letter codes and corners, a variant 1 for every
    name (`floor` and `floor_1` the same), weights up to 256, the order
    (most specific, by name, the default last), the overlap warning and a
    more specific rule silencing it, the names file's terrains and
    letters.
- **Memory:** page 102 holds 5,987 bytes (468 fewer: the matching is
  simpler), page 103 2,944 (the tables 406, from 154).
