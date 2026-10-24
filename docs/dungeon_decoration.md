# Dungeon decoration: requirements and design

Status: step 1 done (D5, 2026-10-24); its CSpect checks (5.10) are still
to do. All decisions were agreed on 2026-10-24. Section 7 lists where the
build differs from the design.

Separates what a map cell **is** (its terrain: rock, wall, floor...) from
how it is **drawn** (one of several metatiles, its look). The generator
builds the dungeon in terrain only. A decorator then picks each cell's
look by rules on its neighbours (which wall piece: a top wall, a
corner...) and by chance among interchangeable looks (floor variations),
the same every time for the same level. Walkability comes from the
terrain, so a new look can never make a wall walkable or a floor solid.

The looks are named in Aseprite, and the names are the rules: a tile
named `wall_xxox` is the wall whose north, east and west neighbours join
it and whose south one doesn't, a top wall. The tile importer turns the
names into the header's names and the decorator's rules, so no tile
number is written by hand, and Aseprite renumbering its tiles changes
nothing in the code.

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
| Terrain | What a cell is, for the game's rules: `TERRAIN_ROCK`, `_WALL`, `_FLOOR`, `_CORRIDOR`, `_DOORWAY`, `_HOLE` |
| Look | A metatile that draws a terrain, or a feature's state (a shut door): named in Aseprite, `LOOK_<NAME>` in the code |
| Base | The first part of a look's name, before its code: `wall` in `wall_xxox`. A base's looks are the choices for one kind of drawing |
| Code | Four letters, read north, east, south, west: `x` if that neighbour joins the cell, `o` if not (5.2) |
| Joins | A neighbour joins a cell if its terrain is in the join set of the base being matched: for walls, wall and rock |
| Rule | A base's look with a code: its pattern. A look without a code is the base's fallback |
| Decorating a cell | Choosing its look: through its terrain's bases in order, the first rule whose pattern matches; else the last base's fallback; then a weighted pick among the looks with that name and their variants |

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
  built from the terrains' bases: every look of a terrain's bases gets
  that terrain's block value. No look is listed by hand.
- **R5** A look used by two terrains with different block values is an
  error, caught by a PC test.
- **R6** Features' looks (doors, stairs) are named looks too, outside the
  decoration: open in the block table, the feature blocks (a closed door).

### Names (in Aseprite)

- **R17** A look is named with a slice over a placed tile on the
  tileset's canvas (Aseprite's Slice tool; double-click a slice to name
  it). The tile under the slice is the look. A tile may have several names
  (one picture for a left and a right wall); a name, with its variant,
  names one tile.
- **R18** A name is `<base>[_<code>][_<diagonal>][_<variant>][_w<weight>]`,
  in lower case letters, digits and `_` (5.2):
  - `<base>`: one or more words, the kind of drawing (`wall`, `floor`,
    `floor_lip`, `door_shut`);
  - `<code>`: four letters of `x` and `o`, north, east, south, west;
  - `<diagonal>`: `ne`, `se`, `sw` or `nw`, after a code only: that
    diagonal neighbour doesn't join (the open corner of a room);
  - `<variant>`: a number, for interchangeable looks of the same name;
  - `<weight>`: `w` and a number, 1-255, the variant's chances among the
    others (1 if left out).
- **R19** The tile importer writes, for every name, `LOOK_<NAME>` (the
  metatile, without variant and weight; the first variant) and
  `LOOK_<NAME>_BANK`, and, for every base, its rules (5.4).
- **R20** The game's names are listed for the importer in
  `assets/tile_names.txt` (5.4): the bases the game decorates with, and
  for each whether its fallback is required; and the other looks the game
  names (the features'), required or not. It is an input of the tile
  import, like the tileset.
- **R21** A base with coded looks needs no fallback of its own if a later
  base of its terrain has one; every terrain's last base has a fallback,
  marked required in the list (R20), and a PC test checks the list
  against `deco_terrains` (R8).
- **R22** The importer checks every slice, and reports **errors**:
  - **a critical look missing:** a required base's fallback, or a
    required look, with no slice;
  - **a slice the wrong size or in the wrong place:** larger than one tile
    (16 x 16), crossing from one tile into the next, or outside the
    canvas. A cell outside the tilemap layer's cel is the empty tile, tile
    0, as Aseprite shows it; tile 0 is metatile 0, transparent, so a name
    on it is allowed (`rock`);
  - **a name that is malformed:** characters other than lower case
    letters, digits and `_`; a name that begins with a known base (R20)
    but whose next part isn't a code, a diagonal, a variant or a weight
    (`wall_xxoy`: a mistyped code, not a new base); a diagonal without a
    code; a weight outside 1-255;
  - **a name used twice:** one name and variant on two different tiles.
- **R23** And **warnings**:
  - **an unrecognised name:** a slice whose base isn't in the list (R20):
    it is ignored, and generates nothing;
  - **a repeated slice:** the same name and variant twice on one tile:
    the second is ignored;
  - **a slice with no name** (Aseprite's default `Slice 1`, ...):
    ignored.
- **R24** The importer reports every problem it finds, not only the first,
  each with the slice's name and its canvas cell (column and row, in
  tiles), so the artist can find it: `error: slice "wall_xxoy" at (3, 2):
  "xxoy" is not a code`. Errors first, then warnings.
- **R25** Any error stops the import: nothing is written, the importer
  exits with status 1, and the build stops (R14). Warnings don't: the
  output is written and the build goes on. The summary also lists the
  tiles that have no name (trial drawings, unused), for information.

### Decoration

- **R7** Decoration runs once the terrain is complete, before features
  are placed (they draw their looks over the cells they are on).
- **R8** A cell's look comes from its terrain's bases, tried in order
  (5.5): the first look whose code (and diagonal) matches the cell's
  neighbours, by that base's join set; if none in any base, the last
  base's fallback. Then a pick weighted among that name's variants. Every
  terrain's last base has a fallback, so every cell gets a look (a PC
  test).
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
  top wall, in a side wall): bases of their own, decorated by the same
  rules.
- **R14 (later):** more metatiles than 64 (cells_16.md R15), if the
  looks need them.
- **R15 (later):** falling into a hole: a walker that enters one (pushed,
  or by choice) falls to the next depth, through a level request
  (level.h, `LEVEL_ARRIVE_ANY`), as the development kit's fall does.
- **R16** Adding a terrain is a constant, its block value and bases, and
  where the generator places it (5.8); the decorator and the block table
  need no change, and the tests of R5 and R8 cover it by themselves. Up to
  16 terrains.

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
- **D3 Patterns on the 8 neighbours, by join sets.** The tileset is in
  3/4 view: a top wall shows its face, a bottom wall its top, and both
  have walls left and right. What tells them apart is where the open
  floor is: below a top wall, above a bottom wall, only diagonally at a
  corner. A code gives the 4 sides, and a diagonal one corner, of the
  neighbours that join, by a join set (walls: wall and rock). One
  terrain's cases can depend on different neighbours (a floor with a hole
  south of it shows a lip, one with a wall north of it a shadow), so a
  terrain tries several bases in order, each with its own join set
  (`floor_lip` by holes, then `floor` by walls). The 8 neighbours'
  terrains are read once a cell. **Rejected:** a 4-bit mask of wall
  neighbours alone: no corners, and no top or bottom. **Rejected:** a
  256-entry table a terrain: big, and almost every entry the same.
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
- **D7 (revised) The rules come from the names; the join sets and block
  values are in C.** First decided as C tables of rules, because Aseprite
  can't give a tile data in its UI (tileset_import.md D3). Slices can
  name a tile on the canvas, and a name that says its neighbours is a
  rule: the importer generates the rules, so a new piece is drawn, named
  and built, with no C to change. What is about the game, not the art,
  stays in C: each terrain's block value, its bases in order, and each
  base's join set (`game/level/level_deco.c`).
- **D8 A hole is a terrain, not a feature.** It is part of the place,
  drawn by its neighbours like a wall (its edges show which side the floor
  is), and nge already has its blocking flag (`NGE_BLOCK_NO_FLOOR`). A
  feature is for things with state or actions (a door opens, stairs are
  climbed); a hole has neither until R15, and even then falling is the
  hole's terrain acting on whoever enters. **Rejected:** a hole as a
  feature entity: an entity a cell, and no edges.
- **D9 Looks are named by neighbour codes.** A code (`xxox`) says the
  case exactly and sorts with its kind, and the same code means the same
  for every base: `x` is a neighbour that joins, by the base's join set.
  `x`, not `w`, for "joins", because `w` is also west in the diagonals.
  All 16 codes are possible: `xxxx` is a room's corner (all four sides
  solid, its floor only diagonal, `wall_xxxx_se`) or the inside of a thick
  wall. **Rejected:** names by place (`wall_top`, `wall_corner_tl`): they
  read well, but each needs its rule written by hand, and new shapes
  (ends, joins, pillars) need new words.
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
  ("no slice named `wall`, the wall's fallback"), and can tell a mistyped
  name of a known base (an error) from a name the game doesn't use yet (a
  warning: an artist can name pieces before the game needs them). All
  problems are reported in one run, because fixing them means a trip to
  Aseprite each. **Rejected:** reading the names from the C
  (`level_deco.c`): the importer would parse C. **Rejected:** treating
  unknown names as errors: the artist couldn't draw and name ahead of the
  code.
- **No rotated or mirrored looks.** A metatile is 2 x 2 hardware tiles:
  rotating it would mean swapping its four tiles too, not only setting
  their bits, and in the 3/4 view the pieces aren't rotations of each
  other (a top wall shows its face, a bottom wall doesn't).

## 5. Design

### 5.1 Terrain (`game/level/level_internal.h`, `level_pdata.c`)

```c
/* What a cell is, for the game's rules: the generator's map */
#define TERRAIN_ROCK      0
#define TERRAIN_WALL      1
#define TERRAIN_FLOOR     2   /* a room's floor */
#define TERRAIN_CORRIDOR  3
#define TERRAIN_DOORWAY   4   /* where a corridor meets a room: a door may stand on it */
#define TERRAIN_HOLE      5   /* no floor: a pit, a chasm */
#define TERRAIN_COUNT     6

/* A set of terrains: up to 16 (R16) */
typedef uint16_t terrain_set_t;
#define TERRAIN_BIT(t)    ((terrain_set_t)(1u << (t)))

extern uint8_t level_terrain[LEVEL_HEIGHT][LEVEL_WIDTH];   /* page 103 */
```

The generator's `put()` becomes `set_terrain(x, y, t)`; `put_corridor()`
tests `TERRAIN_ROCK`; `door()` sets `TERRAIN_DOORWAY`; `clear()` fills
with rock. The rooms' interior search (`try_floor()`) is unchanged: it
uses the room list.

### 5.2 Look names

`<base>[_<code>][_<diagonal>][_<variant>][_w<weight>]`, read from the end:
a last part `w` and digits is the weight; then digits, the variant; then
`ne`, `se`, `sw` or `nw`, the diagonal; then four of `x` and `o`, the
code; the rest is the base. In the code, `LOOK_` and the name in upper
case, without its variant and weight.

The walls, with wall and rock joining (the user's 15 patterns, with `x`
for solid, and `xxxx`):

| Name | N E S W | Piece |
|---|---|---|
| `wall_xxox` | solid, solid, open, solid | top wall: floor below, its face shows |
| `wall_oxxx` | open, solid, solid, solid | bottom wall |
| `wall_xoxx` | solid, open, solid, solid | left wall: floor to the east |
| `wall_xxxo` | solid, solid, solid, open | right wall |
| `wall_xxxx_se` | all solid, south-east open | top-left corner |
| `wall_xxxx_sw` | all solid, south-west open | top-right corner |
| `wall_xxxx_ne` | all solid, north-east open | bottom-left corner |
| `wall_xxxx_nw` | all solid, north-west open | bottom-right corner |
| `wall_xxxx` | all solid, no diagonal | inside a thick wall |
| `wall_xoox`, `wall_ooxx`, `wall_xxoo`, `wall_oxxo` | two sides open | a top or bottom wall ending at a doorway, or a corner of a wall one cell thick |
| `wall_xoxo`, `wall_oxox` | open on two opposite sides | a wall one cell thick, upright or across |
| `wall_xooo`, `wall_oxoo`, `wall_ooxo`, `wall_ooox` | three sides open | a wall's end |
| `wall_oooo` | all open | a pillar |
| `wall` | fallback | any case not drawn yet |

Rogue's rectangular rooms need the first eight and the fallback; the
generator's doorways also make the two-side ends (`wall_xoox`: a top wall
with a doorway to its east). The others can be drawn when the generator
makes other shapes.

Other bases: `rock`; `floor`, `floor_1_w7`, `floor_2`... (variants, by
weight); `floor_lip_ooxo` (a hole to the south, by the `floor_lip` base's
join set, holes); `corridor`; `doorway`; `hole` with codes by holes
joining (`hole_oxxx`: the far edge, the pit's wall showing; `hole_xxox`:
the near lip...); and the features' looks, `door_shut`, `door_open`,
`stairs_up`, `stairs_down`.

### 5.3 Step 1's names: today's levels

Slices on today's tiles that draw the levels as the generator does now:

| Name | Today's look |
|---|---|
| `rock` | `LEVEL_ROCK` |
| `wall` (fallback: top, bottom, corners) | `LEVEL_WALL_H` |
| `wall_xoxx`, `wall_xxxo` (side walls) | `LEVEL_WALL_V`, one tile, two names |
| `floor_1_w7`, `floor_2` | `LEVEL_FLOOR`, `LEVEL_FLOOR2`: today's one in eight |
| `corridor` | `LEVEL_CORRIDOR` |
| `doorway` | `LEVEL_FLOOR`'s tile |
| `hole` | a tile of its own: a placeholder until step 2 (it can't be rock's: R5) |
| `door_shut`, `door_open`, `stairs_up`, `stairs_down` | the features' looks |

Differences from today, all next to a doorway or a passing corridor,
which are open where today's generator drew by position alone: a side
wall's cell beside a doorway has an open side to the north or south, so
it matches no code and takes the fallback (`LEVEL_WALL_H`'s look, where
today it is `LEVEL_WALL_V`'s); a corner a corridor runs past has an open
side, so it can match a side wall's code. Step 2's wall ends are drawn
for exactly these cells.

### 5.4 The tile importer (`scripts/ase2tiles.py`, `scripts/aseprite.py`)

- **Reading (`aseprite.py`):** slices (chunk `0x2022`: name, and the
  first key's rectangle) and the tilemap layer's cel (cel kind 3: the
  canvas's tiles, 32 bits each, the tile number in the low bits).
- **The game's names (`assets/tile_names.txt`, R20):** one line each, `#`
  for comments:

  ```
  # Bases the game decorates with: "required" if the fallback must exist
  base rock       required
  base wall       required
  base floor      required
  base corridor   required
  base doorway    required
  base hole       required
  base floor_lip
  # Other looks the game names
  look door_shut    required
  look door_open    required
  look stairs_up    required
  look stairs_down  required
  ```

  A base listed without `required` may be drawn or not: its looks are
  generated when it has some. Adding a base or a look the game names is a
  line here and its use in the code.
- **Names:** each slice's rectangle must lie within one canvas cell of the
  tilemap layer's cel (R22); the tile there is the look. Names are parsed
  (5.2), checked against the list, and grouped by base.
- **Checks (R22-R25):** every slice is checked and every problem
  collected; the list's required names are checked last. Then errors and
  warnings are printed, errors first, each with the slice and its cell;
  with any error the importer writes nothing and exits with status 1.
- **Output, added to today's (`src/data/`):**
  - `tile_data.h`: `LOOK_<NAME>` and `LOOK_<NAME>_BANK` for every name;
    `LOOK_BASE_<BASE>` for every base, its number in the rule tables; the
    tables' types and declarations.
  - `tile_rules_data.c`: the rule tables, constants in page 103 with the
    level's data (`#pragma constseg PAGE_103`, as the generators' tables
    are in their data pages):

    ```c
    const tile_look_t tile_looks[];   /* tile, bank attribute, weight */
    const tile_rule_t tile_rules[];   /* care, want (8 neighbour bits), first look, looks */
    const tile_base_t tile_bases[];   /* first rule, rules, fallback's first look, its looks */
    ```

    A base's rules are in order: those with a diagonal first, then the
    other codes, so a corner is found before the wall it is part of.
    Codes without a diagonal care about the four sides only, so two of
    them never match the same cell.
- **The build:** `tile_rules_data.c` is a fourth output of the tile rule
  (`make tiles`), `assets/tile_names.txt` one more input, and the
  Makefile builds `src/data/*.c` too. Warnings show in the build's output
  and don't stop it. The PC tests
  link it with the level module.

### 5.5 The decorator (`game/level/level_deco.c`, page 102)

```c
typedef struct deco_base {
    uint8_t base;               /* LOOK_BASE_* */
    terrain_set_t joins;        /* the neighbours that join, for its codes */
} deco_base_t;

#define DECO_BASES_MAX  2

typedef struct deco_terrain {
    nge_block_t block;                   /* R4 */
    uint8_t count;                       /* bases, tried in order */
    deco_base_t bases[DECO_BASES_MAX];
} deco_terrain_t;

static const deco_terrain_t deco_terrains[TERRAIN_COUNT] = {
    [TERRAIN_ROCK]     = { NGE_BLOCK_SOLID, 1, { { LOOK_BASE_ROCK, 0 } } },
    [TERRAIN_WALL]     = { NGE_BLOCK_SOLID, 1, { { LOOK_BASE_WALL,
                             TERRAIN_BIT(TERRAIN_WALL) | TERRAIN_BIT(TERRAIN_ROCK) } } },
    [TERRAIN_FLOOR]    = { NGE_BLOCK_NONE, 1, { { LOOK_BASE_FLOOR, 0 } } },
    [TERRAIN_CORRIDOR] = { NGE_BLOCK_NONE, 1, { { LOOK_BASE_CORRIDOR, 0 } } },
    [TERRAIN_DOORWAY]  = { NGE_BLOCK_NONE, 1, { { LOOK_BASE_DOORWAY, 0 } } },
    [TERRAIN_HOLE]     = { NGE_BLOCK_NO_FLOOR, 1, { { LOOK_BASE_HOLE, TERRAIN_BIT(TERRAIN_HOLE) } } },
};
```

Step 2 adds `{ LOOK_BASE_FLOOR_LIP, TERRAIN_BIT(TERRAIN_HOLE) }` before the
floor's base, when the art has it. A base the art doesn't have is a
compile error where it is named.

```c
/* Choose cell (x, y)'s look by its terrain's bases: writes the map tile */
void level_decorate(uint8_t x, uint8_t y);

/* Decorate every cell: once the terrain is complete (R7) */
void level_decorate_all(void);

/* A cell's terrain changed: decorate it and its 8 neighbours (R11) */
void level_decorate_around(uint8_t x, uint8_t y);

/* Every look's block value into the map's block table (R4, R12) */
void level_deco_blocks(void);
```

### 5.6 Matching and picking

- **The neighbours:** the 8 neighbours' terrains, read once (rock off
  the map, R9).
- **The signature, per base:** a bit for each neighbour (N, NE, E, SE, S,
  SW, W, NW) set if its terrain is in the base's join set.
- **The match:** each base in turn, each of its rules in order: the first
  with `(signature & care) == want`. A code sets care for the four sides,
  want for its `x`s; a diagonal adds its bit to care, clear in want.
  If no rule matches in any base, the last base's fallback.
- **The pick:** `h = hash(x, y, level_deco_seed)`, then the look at
  `h % total weight` along the matched name's variants. The hash, 8-bit,
  cheap on the Z80:

  ```c
  h = (uint8_t)(x * 151 + y * 59) ^ seed;
  h ^= h >> 3;
  h *= 109;   /* Z80N mul */
  h ^= h >> 4;
  ```

  Its spread is tested on the PC (5.9); if a level shows stripes, the
  constants change.
- **Cost:** about 500 T-states a cell, 2,400 cells: about 1.2M T-states,
  45 ms at 28 MHz, once a level.

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

1. A `TERRAIN_*` constant before `TERRAIN_COUNT` (up to 16).
2. Its entry in `deco_terrains`: block value, bases and join sets. Any
   other terrain whose looks depend on it adds a base whose join set
   names it (as `floor_lip` names the hole).
3. Its bases in `assets/tile_names.txt`, the last one `required`.
4. Where the generator places it, in terrain only.
5. Its looks, drawn and named in the tileset.

A look for an existing base: draw it, place it on the canvas, put a slice
on it with its name, `make`. Nothing in the code changes. If `make` warns
that the name is unrecognised, its base isn't in the list: a misspelt
base, or a new one to add (step 2 above).

Limits: 16 terrains (`terrain_set_t`), 255 rules and 255 looks in the
tables (`uint8_t` indices), and the tileset's 64 metatiles (R14).

### 5.9 Memory

- **Page 103:** `level_terrain`, 2,400 bytes, the seed, and the rule
  tables: 3 bytes a look, 4 a rule, 4 a base; step 2's walls,
  floors and holes about 40 looks and 30 rules, under 0.3K.
- **Page 102:** the decorator, about 0.5K, and `deco_terrains`.
- **Common memory:** unchanged.

### 5.10 Tests

- **The importer** (`tests/scripts/test_ase2tiles.py`, files built by
  `ase_writer.py`, which learns slices and tilemap cels):
  - slices read, and the tile under each;
  - names parsed: base, code, diagonal, variant, weight;
  - each error of R22 (a required fallback or look missing, a slice
    larger than a tile, one crossing two tiles, one off the canvas; a
    slice outside the cel names tile 0; a mistyped code of a known base, a diagonal
    without a code, a weight of 0 or 256, a name on two tiles), each
    named with its slice and cell;
  - each warning of R23 (an unknown base, a repeated slice, an unnamed
    slice): the output is still written, without them;
  - several problems in one file are all reported, errors before
    warnings; with an error nothing is written;
  - the list's syntax: an unknown keyword or a repeated name is an error
    of the list, with its line number;
  - a tile with two names; a name on two tiles is an error;
  - rules in order (diagonals first), care and want for a code and a
    diagonal; variants' weights;
  - the generated header and tables for a small tileset;
  - the real tileset reads, and every slice in it parses.
- **`test_level`:** its checks of the layout (rooms walled, corridors
  through rock, doorways, every room reachable) read `level_terrain`, not
  the map: they test the generator, whatever the art. Reachability is a
  walker's: holes are not open (R3a). With holes (step 2): none on or
  next to a doorway, only in room interiors. New: every map tile is a
  look of its cell's terrain's bases (or a feature's look); every cell's
  block value is its terrain's.
- **`test_deco`** (new, the level module and the tables linked):
  - a cell's signature, with neighbours off the map as rock;
  - a top wall, each corner, a side wall and a wall end match their names
    (with the real tables, where the art has them);
  - a terrain's bases in order: a floor with a hole south is a lip, one
    without is a floor variant;
  - every terrain's last base has a fallback (R8, R21), and every base in
    `deco_terrains` is in `assets/tile_names.txt`, the last of each
    terrain marked required;
  - no look is used by two terrains with different block values (R5);
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

Out, for later:

- step 2: the art's wall pieces, floor variations and holes, named; the
  generator's holes (R3a);
- falling into holes (R15);
- doors and stairs by their place (R13);
- more than 64 metatiles (R14);
- prefab rooms: hand-made rooms placed by the generator, in terrain, so
  they are decorated like the rest.

## 7. As built: differences from the design

- **The tileset already has step 2's wall pieces.** Its names (5.2) give
  the four corners, top, bottom and side walls, a pillar (`wall_oooo`) and
  walls one cell thick (`wall_oxox`, `wall_xoxo`), so step 1 draws walls
  by their place already; the cells beside a doorway still take the
  fallback (`wall`, a top wall's look).
- **`level_put_floor()` is `level_redraw()`:** it puts back the cell's
  look by its terrain, which isn't always floor (a doorway).
- **A slice outside the tilemap layer's cel names tile 0** (R22 as
  corrected): rock is the empty tile, and `rock` was named where the
  canvas has no cel.
- **A name without a variant number is a variant too.** `floor` (weight 1)
  with `floor_1_w7` and `floor_2` is three variants, two on the same tile:
  `floor_2` shows 1 time in 9. Without the plain `floor` slice it would be
  1 in 8, as before (`floor_1` is then the required name's tile).
- **Variants have no names of their own:** `LOOK_FLOOR` is the first
  variant's tile; the others are only in the tables.
- **`look` and `base` lines in the names file are treated alike:** a
  feature's look is a base with only a fallback, and its tables are
  generated but unused.
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

