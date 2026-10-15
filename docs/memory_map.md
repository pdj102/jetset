# Jetset memory map

The Z80 sees 64K as eight 8K **slots**. Each slot shows one 8K **page** of the
Next's RAM (or the ROM), chosen with the MMU registers `0x50`-`0x57`
(`REG_MMU0`-`REG_MMU7`). A 16K **bank** n is the page pair 2n and 2n+1, so
bank 5 is pages 10 and 11.

Changing a slot's page is a single `nextreg` write and takes effect
immediately. The hardware (tilemap, sprites, palettes) does not use the MMU:
it reads physical memory or its own on-chip RAM, whatever the slots show.

## Slot layout

| Slot | Address | Default page | Use |
|---|---|---|---|
| 0 | `$0000`-`$1FFF` | ROM | ECS data, page 50, mapped by `nge_ecs_init()`: guard area `$0000`-`$00FF`, tables from `$0100` (see [ECS data in slot 0](#ecs-data-in-slot-0)) |
| 1 | `$2000`-`$3FFF` | ROM | The map's data, page 51, mapped by `nge_map_init()`: block table, state, tiles (see [Always-mapped data pages](#always-mapped-data-pages)) |
| 2 | `$4000`-`$5FFF` | 10 | Tilemap and tile definitions. Free space: none |
| 3 | `$6000`-`$7FFF` | 11 | End of the tile definitions, then the **start of the program** at `$6A00` |
| 4 | `$8000`-`$9FFF` | 4 | Program (code and data) |
| 5 | `$A000`-`$BFFF` | 5 | Program, free space, **stack at the top** |
| 6 | `$C000`-`$DFFF` | 0 | Banked game data: mapped in, used, mapped out |
| 7 | `$E000`-`$FFFF` | 1 | Banked game data: mapped in, used, mapped out |

The defaults are what the NEX loader leaves when the game starts.

### Slots 2 and 3: video memory (bank 5)

The tilemap hardware always reads bank 5, pages 10 and 11. (Bank 7 can be
selected instead with bit 7 of registers `0x6E`/`0x6F`; Jetset doesn't.) The
library writes to fixed CPU addresses, so **pages 10 and 11 must stay mapped
in slots 2 and 3** whenever the game draws to the tilemap. Treat these two
slots as permanently mapped rather than as general data slots.

| Address | Page | Contents | Defined in |
|---|---|---|---|
| `$4000`-`$49FF` | 10 | Tilemap: 40x32 cells, 2 bytes each | `NGE_TILEMAP_BASE`, [nge_tilemap.h](../src/nge/tilemap/nge_tilemap.h) |
| `$4A00`-`$5FFF` | 10 | Tile definitions, tiles 0-175 | [tile_defs_data.asm](../src/data/tile_defs_data.asm) (`SECTION BANK_5`, `org 0x4A00`) |
| `$6000`-`$69FF` | 11 | Tile definitions, tiles 176-255 | |
| `$6A00`-`$7FFF` | 11 | Start of the program (see below) | `CODE_ORG` in the [Makefile](../Makefile) |

The tile definitions cross from page 10 into page 11, so the tiles depend on
both pages, not just page 10.

The ULA screen is switched off (register `0x68` bit 7), so the ULA screen and
attributes at `$4000`-`$5AFF` no longer matter, and neither do the ROM system
variables at `$5C00`-`$5CB5` once the ROM is unused. They overlap tiles 112-135
and 144-149. Never call ROM routines or enable the ROM's IM1 interrupt
handler: both write to the system variables and would corrupt those tiles.

### The program: `$6A00` to the stack

The program starts straight after the tile definitions, at `$6A00`, and runs
through slots 3, 4 and 5 up to the stack. None of these slots ever changes, so
anything that has to be reachable at any time belongs in the program:

- code, constant data, initialised data and BSS
- the code that changes the MMU (it must not page itself out)
- interrupt handlers and the IM2 vector table, when added
- `banked_call`, and everything else that must stay in common memory (see
  [Banked code](#banked-code))
- data used every frame that isn't ECS data: a sprite table if the game
  defines one, input state, the stream pool, game state (the ECS tables read
  every frame are in slot 0, see [ECS data in slot 0](#ecs-data-in-slot-0);
  the render system's sprite records are its page data in page 53)

Starting at `$6A00` rather than z88dk's default `$8000` adds 5.5K. The tile
definitions can therefore no longer grow past `$69FF`.

The current CSpect build (`bin/main.map`) uses:

| Address | Contents |
|---|---|
| `$6A00`-`$AD73` | Code and constant data (`CODE`) |
| `$AD74`-`$AD89` | Initialised data (`DATA`) |
| `$AD8A`-`$AEB3` | Uninitialised data (`BSS`) |
| `$AEB4`-`$BDFF` | Free, about 3.8K (about 9.2K in the hardware build, without asserts and their messages) |
| `$BE00`-`$BFFF` | Stack, 512 bytes |

The link step checks `__BSS_END_tail` in the map file and fails the build if
the program reaches the stack; otherwise it prints how much space is left.

The program is linked with z88dk's startup 31, which creates no `stdin` or
`stdout`. The default startup links z88dk's console drivers (about 3.3K),
which nothing uses and which would print to the ULA screen memory under the
tilemap. Don't use the C library's `stdio` functions; use `io/`.

## The stack

SP is set to `$C000`. The stack grows down from the top of slot 5, so its
512 bytes are `$BE00`-`$BFFF`, inside the slots that never change.

At startup `banked_call` takes the top 32 bytes, `$BFE0`-`$BFFF`, as its own
stack of saved banks and return addresses (4 bytes per nested banked call,
`CLIB_BANKING_STACK_SIZE`). The program's stack starts below it, at `$BFE0`.

The link command in the [Makefile](../Makefile) sets this (`LDFLAGS`):

```make
-pragma-define:CRT_ORG_CODE=0x6A00 \
-pragma-define:REGISTER_SP=0xC000 -pragma-define:CRT_STACK_SIZE=512 \
-pragma-define:CLIB_MALLOC_HEAP_SIZE=0 -pragma-define:CLIB_STDIO_HEAP_SIZE=0 \
-pragma-define:CLIB_BANKING_STACK_SIZE=32
```

z88dk's own defaults would put the stack at `$FF58`, in slot 7 ("under UDGs",
for Spectrum BASIC), where remapping slot 7 would remove the stack, including
the return address of the function doing the remapping. Its default
`CLIB_MALLOC_HEAP_SIZE` of -1 would also make the heap fill all memory from
the end of BSS to the bottom of the stack. The engine doesn't use `malloc` or
`fopen`, so both heaps are zero.

Nothing may hold a pointer to a stack variable in a banked slot, and nothing
in a banked slot may call back into code that changes the same slot.

## Slots 6 and 7: banked data and code

Slots 6 and 7 are shared by two users: game code that maps a data page in by
hand and maps it out again, and banked functions, whose bank `banked_call`
maps in for each call. The two must not overlap; see
[Banked code](#banked-code).

z88dk already supports this layout. Assembly or C data placed in
`SECTION PAGE_n` is assembled for `$C000` when n is even and for `$E000` when
n is odd (`CRT_ORG_PAGE_n` in `zxn_rules.inc`), and is loaded into page n by
the NEX file. A 16K set uses an even/odd pair, mapped together into slots 6
and 7.

### Page allocation

The target is a 2MB Next: pages 0-223. Every module that has banked code,
nge's or the game's, gets a pair of pages: the even page for its code, string literals and
constants (8K: `codeseg` and `constseg PAGE_n`), the odd page for large data
sets private to it. The allocation is kept in
[src/nge/banks.inc](../src/nge/banks.inc), which also sets the origins of the
code pages.

| Pages | Owner |
|---|---|
| 0-31 | System: program (4-5), video (10-11), bank 7 (14-15), Layer 2 (16-31) |
| 32-39 | Spare |
| 40/41 | nge `io` (reserved; the output path stays in common memory) |
| 42/43 | nge `sprite` (setup functions and loaders) |
| 44/45 | nge `map`: set-up code (init, block table, sprite clip) / unused. Its per-frame code is in common memory, its data in page 51 |
| 46/47 | nge `event` (reserved; the event queue and its processing run every turn and are in common memory) |
| 48-87 | nge `ecs` (20 pairs): 48/49 `entity` and `owner` (lifecycle and ownership code / lifecycle bookkeeping); 50 ECS data, mapped permanently into slot 0 (see [ECS data in slot 0](#ecs-data-in-slot-0)); 51 map data, mapped permanently into slot 1 (see [Always-mapped data pages](#always-mapped-data-pages)); 52/53 `render` system |
| 88-99 | Further nge modules (6 pairs): 88/89 `input` (setup, ASCII tables and waiting / unused) |
| 100 | Game: free (assets) |
| 101 | Game: sprite patterns and palette (slot 7, through the loaders) |
| 102-131 | Game code pairs (15), allocated from 102 up: 102/103 `level` (setting up and building a level / unused); 104/105 `title` (the title state / unused) |
| 132-223 | Game assets: maps, graphics, level data |

- **Module code, string literals and constants:** the even page. A banked
  `.c` file sets both `#pragma codeseg PAGE_n` and `#pragma constseg
  PAGE_n`, so its literals (assert messages included) and constants are in
  its own page, not in common memory. 8K per module: if it grows past 8K,
  the linker stops the build ("Section PAGE_n overlaps section PAGE_n+1");
  split the module across another pair from its range.
- **Large private data sets:** the odd page, mapped into slot 7 whenever the
  module's code runs, so the module reads it directly. Not every constant
  goes here, only data sets big enough to matter. They go in the module's
  page data file (see [Page data](#page-data-a-modules-variables-and-tables-in-its-odd-page)).
  Only the module's own code uses them.
- **Banked literals and data are passed only to common code** (the output
  path, `nge_util`) **or code in the same page,** and not kept after the
  call returns. A function in another bank would be called through
  `banked_call`, which maps its pages over them. The whole output path stays
  in common memory for this reason: `core/nge_util`, `io/` (streams,
  `nge_print`, `nge_printf`) and `devices/nge_cspect`. The streams use each
  character at once (the CSpect line buffer copies it; the tilemap stream
  draws it), so no stream keeps a pointer to a string.
- **Game assets** (sprite sets, maps, level data) go in the game range, not
  in nge module pages. The sprite loaders accept any odd page.

### Banked code

Functions that don't need to be in memory all the time can live in a banked
page and be called through z88dk's `banked_call`. On each call it maps the
function's page into slot 6 and the next page into slot 7, calls it, and maps
the previous pages back. The same mechanism and rules apply to nge modules
and to game modules (see [Banked game modules](#banked-game-modules)).
Banked so far:

| Pages | Module | Code (even page) | Page data (odd page) |
|---|---|---|---|
| 42/43 | `sprite` | hardware set-up functions and loaders, with literals (`nge_sprite_setup.c`, about 1.1K) | none |
| 44/45 | `map` | set-up: `nge_map_init()` (maps page 51 into slot 1), the block table functions, the sprite clip (`nge_map_setup.c`, about 0.7K) | none |
| 48/49 | `ecs` entity, owner | lifecycle functions (`nge_entity_life.c`) and the ownership functions that change or search the owner table (`nge_owner_set.c`), with literals, about 0.6K | lifecycle bookkeeping: free and dying stacks, alive positions (`nge_entity_pdata.c`, 302 bytes) |
| 52/53 | `ecs` render system | the system (`ecs/systems/nge_render.c`), with literals, about 2.4K | the sprite component (10 bytes per entity), the sprite records for hardware sprites 0 to `NGE_RENDER_SPRITES`-1, and its working data (`nge_render_pdata.c`, about 1.8K) |
| 88/89 | `input` | setup, the key-to-ASCII tables and waiting (`input/nge_input_setup.c`, about 0.5K) | none |
| 102/103 | game `level` | setting up and building a level, with literals (`game/level/level_bank.c`, about 0.7K) | none yet |
| 104/105 | game `title` | the title state and the name prompt, with literals (`game/title/title_bank.c`, about 0.4K; no common part) | none |

How a module is banked:

1. Its functions are declared `NGE_BANKED` (from
   [core/nge_bank.h](../src/nge/core/nge_bank.h)) in its header and
   definitions. Callers must see the declaration, so being banked is part of
   a function's interface.
2. Its `.c` file starts with the page its code, literals and constants go
   in:

   ```c
   #ifdef __SDCC
   #pragma codeseg PAGE_42
   #pragma constseg PAGE_42
   #endif
   ```

3. The page is listed in [src/nge/banks.inc](../src/nge/banks.inc) with its
   origin: `#pragma define CRT_ORG_PAGE_42 = 0x2AC000`.

nge uses 8K `PAGE_n` sections rather than 16K `BANK_n` sections, so the odd
page is free for data. If a page's code grows past 8K, the linker stops the
build ("Section PAGE_42 overlaps section PAGE_43").

**Why the origin carries the page.** After each `call banked_call`, SDCC
stores the 32-bit address of the function, and `banked_call` uses bits 16-23
as the page to map. z88dk's default origin for the Next is a plain `$C000`, so
every call would map page 0 and jump into the wrong code. The origin
`(page << 16) | $C000` gives the right page; the CPU uses only the low 16 bits
and the NEX file is built from the section name, so nothing else changes.
[scripts/check-banks.sh](../scripts/check-banks.sh) runs after every link and
fails the build if banked C code or its literals don't carry their page,
or if a `#pragma codeseg` names an odd page.

**Rules:**

- **Banked calls never enable interrupts.** z88dk's `banked_call` ends
  with `ei`, which would let an interrupt jump to `$0038`, game data once
  slot 0 holds RAM. nge defines its own `banked_call`
  ([core/nge_banked_call.asm](../src/nge/core/nge_banked_call.asm), a
  modified copy of z88dk's): it keeps the interrupt state it was called
  with, both for the called function and on return. The `di` around its
  stack switch stays, so it is also correct once an interrupt handler
  enables interrupts. `scripts/check-banks.sh` fails the build if the map
  shows z88dk's version (module `zxn_banked_call`).
- **Don't mix banked calls with pages mapped by hand.** `banked_call` maps
  back the pages it last mapped itself, not whatever was in slots 6 and 7
  before. While game code has its own page mapped there, it must not call a
  banked function. Pass the page to a banked loader instead (below).
- **Banked code may map slot 7, but never slot 6.** Banked code in an 8K
  `PAGE_n` section runs from slot 6, so slot 7 is free: the sprite loaders
  map their data page into slot 7, upload, and map the previous page back.
  While it is mapped they call no banked function. Code in a 16K `BANK_n`
  section may extend into slot 7, so it must not do this.
- **Split a module when only part of it can be banked.** A `.c` file is in
  one section, so `sprite/` is two files sharing `nge_sprite_internal.h`:
  `nge_sprite.c` (common: per-frame functions) and `nge_sprite_setup.c`
  (page 42: setup and loading). `ecs/nge_entity` is four, sharing
  `nge_entity_internal.h`: `nge_entity.c` (common: flag and component
  functions), `nge_entity_cdata.c` (slot 0: the tables used every frame),
  `nge_entity_life.c` (page 48: lifecycle) and `nge_entity_pdata.c` (page
  49: the lifecycle's page data). `ecs/nge_owner` is three: `nge_owner.c`
  (common: `nge_owner_get`), `nge_owner_cdata.c` (slot 0: the owner table)
  and `nge_owner_set.c` (page 48, beside the lifecycle that uses it).
- **Use even `PAGE_n` sections, or 16K `BANK_n` sections.** `banked_call`
  maps the page into slot 6, so code in an odd page (assembled for `$E000`)
  would be in the wrong slot.
- **Keep these in common memory:**
  - code called many times a frame (the sprite record functions,
    `nge_sprite_upload`, `nge_map_update`, the tilemap stream, `nge_putc`):
    each banked call costs about 150-250 T-states. A system that does a
    whole frame's work in one call, such as `nge_render_update`, can be
    banked;
  - functions that read data the caller has mapped into slot 6 or 7, such
    as `nge_write_palette9()`: a banked function would map its own page over
    that data (banked loaders take the data's page instead, as the sprite
    loaders do);
  - functions called through a pointer, such as stream `put` and `flush`
    functions: a pointer to a banked function doesn't switch banks;
  - `__z88dk_fastcall` functions: `banked_call` overwrites HL, which holds
    their argument;
  - the output path (`core/nge_util`, `io/`, `devices/nge_cspect`), so
    banked code can print its own literals;
  - `nge_mmu`, `nge_hw_init` and interrupt code.
- **Literals and constants live in the module's page** (`constseg`, above).
  Without `constseg`, SDCC would put them in common memory: assert messages
  alone cost about 45 bytes per assert site in CSpect builds.
- Return values in HL and DE (up to 32 bits) and stack arguments, including
  variadic ones, work as normal.

### Banked game modules

Game code is banked the same way as nge's, with a module's pair from the
game code range (102-131) and its origin in
[banks.inc](../src/nge/banks.inc). `level` is the first. A banked game
module lives in its own folder, `src/game/<module>/`, split by where each
part runs:

| File | Memory | Contents |
|---|---|---|
| `<module>.h` | | the public interface; banked functions declared `NGE_BANKED` |
| `<module>_internal.h` | | shared between the module's files only |
| `<module>.c` | common | code run every frame or every turn, and data others read |
| `<module>_bank.c` | even page | rarely run code (set-up, generation, menus), with `#pragma codeseg` and `constseg PAGE_n` |
| `<module>_pdata.c` | odd page | large private variables and tables, with the `nge-dataseg` marker; only when needed |

```text
src/game/level/
  level.h, level_internal.h
  level.c          common: level_can_enter()   (every turn)
  level_bank.c     page 102: level_init() and the layout builder   (once per level)
```

- **What stays common:** anything called every frame or many times a turn
  (`level_can_enter()` is called for every move, and will be for every
  monster move), and data read by common code every frame.
- **A game function that receives literals or data from a banked module
  stays common,** or is in the same page. `screen_message()` is common, so
  `level_bank.c` can pass it its own literals; a banked `screen` module
  would map its own page over them.
- **Base names stay unique across the project** (`level.c`,
  `level_bank.c`): z88dk object and symbol names are global.
- The Makefile builds `src/game/*/*.c` automatically, and
  [scripts/check-banks.sh](../scripts/check-banks.sh) checks game modules as
  it checks nge's. Game modules are not covered by the PC tests; check them
  in CSpect.
- Game modules not yet banked stay as single files in `src/game/` and move
  into a folder when they are banked. A module that is all rarely run code
  (`title`) has no `<module>.c`.
- **Banked functions are called directly, never through a function pointer**
  (a pointer to a banked function doesn't switch banks): `game.c` calls each
  state's entry action and update from a `switch`, not a table of pointers.

### Page data: a module's variables and tables in its odd page

A module's large private variables and tables go in its odd page, in one
**page data file**, `<module>/nge_<module>_pdata.c`:

```c
/* nge-dataseg: PAGE_43 */
#ifdef __SDCC
#pragma constseg PAGE_43
#endif
#include "sprite/nge_sprite_internal.h"

nge_sprite_cache_t nge_sprite_cache;            /* variables: page 43 */
const uint8_t nge_sprite_lookup[256] = { ... };  /* tables: page 43 */
```

- **Variables:** SDCC can't move variables with a `#pragma`, but its
  `--dataseg` option moves a file's uninitialised variables. The Makefile
  reads the `nge-dataseg` marker in the first lines of a file and passes
  `--dataseg PAGE_n` for that file only.
- **Tables:** `#pragma constseg` moves the file's `const` data.
- **No initialised variables.** z88dk keeps the initial values of
  initialised variables in common memory, so they can't be moved. The NEX
  file loads the page as zeros; the module's banked `init` function sets
  every value it needs, which also covers restarting a game.
- **Declare the variables `extern`** in the module's internal header. Only
  the module's own banked code uses them, while its page pair is mapped;
  common code would read whatever page is in slot 7. This rule is not
  checked automatically.
- **On the PC tests** the marker and pragma have no effect: the variables
  are ordinary globals.

[scripts/check-banks.sh](../scripts/check-banks.sh) checks after every link
that:

- every symbol of a `*_pdata.c` file is in an odd page: an initialised
  variable, a missing marker or a missing `constseg` would otherwise put it
  in common memory without any warning;
- every `PAGE_n` section fits in 8K: the linker reports an even page growing
  into a non-empty odd page, but not a page growing into the next bank.

Other ways were considered and rejected: `__at(0xE000)` variables are
invisible to the linker, so nothing stops them overlapping other data in the
page or overflowing it; data in an `.asm` file duplicates the C layout by
hand (use it only for byte buffers or generated data, which can also have
initial values loaded from the NEX file); writing to `const` data through a
cast is undefined behaviour that SDCC's optimiser can break.

### Sprite and palette data: loaders that map their own page

Sprite patterns and palettes are copied into the FPGA's own memory, not read
from RAM by the video hardware. The source RAM is only needed while the upload
runs, which makes them ideal banked data.

Put them in an odd page (`SECTION PAGE_n`, n odd), which z88dk assembles for
slot 7 (`$E000`), and pass that page to the loaders:

```c
nge_sprite_load_patterns(DEMO_SPRITE_PAGE, 0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
nge_sprite_load_palette(DEMO_SPRITE_PAGE, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);
```

The loaders (`nge_sprite_load_patterns()`, `nge_sprite_load_patterns4()`,
`nge_sprite_load_palette()`) are banked in page 42. Each maps the data page
into slot 7, uploads, and maps the previous page back; nothing is mapped by
the caller. Pass `NGE_SPRITE_DATA_COMMON` for data in common memory.

- One call reads at most 8K from one page: 32 8-bit patterns. The pattern
  memory is 16K (64 patterns of 16x16 at 8 bits per pixel, or 128 at 4
  bits), so a full set takes two pages and two calls.
- The game's sprite graphics are in page 101, in the game range
  ([sprite_data.asm](../src/data/sprite_data.asm), `SECTION PAGE_101`). A
  game with several sprite sets puts each in its own odd page.
- Hide sprites, or load during vblank, so patterns don't change while they
  are being drawn.
- Tile definitions can be copied into `$4A00` the same way: slots 2 and 3
  stay mapped, so a copy goes directly from slot 7 into video memory.

Sprite attributes are sent to port `$57` every frame from records
(`nge_sprite_attr_t`) that their callers own, so the sprite module itself
holds no 640-byte table. The record functions and `nge_sprite_upload()` are
in common memory and take a pointer, so the records can be anywhere their
caller can see them:

- the render system's records are page data in page 53, for hardware
  sprites 0 to `NGE_RENDER_SPRITES`-1: its banked code has page 53 in slot 7
  when it calls the record functions and the upload;
- a game's own records for the other sprites (a HUD, for example) are an
  array in common memory, or in its own page if its code is banked. The
  game keeps the ranges apart.

Keep in non-banked memory anything used after the upload:

- a palette copy used for fades or palette cycling;
- pointers into banked data are valid only while their page is mapped, so
  copy any values needed later before unmapping.

### Other banked data

Text and level tables can use the same pattern: map the page, copy or decode
what is needed, then unmap. The active area's map is the exception: it is
read constantly (drawing and collision checks), so it stays mapped in slot 1
instead.

## Always-mapped data pages

Two pages hold common data at fixed addresses outside the program, each
mapped once at start-up and left in its slot:

| Page | Slot | Addresses | Contents | Mapped by |
|---|---|---|---|---|
| 50 | 0 | `$0100`-`$1FFF` (guard area below) | ECS tables read every frame (below) | `nge_ecs_init()` |
| 51 | 1 | `$2000`-`$3FFF` | the map: block table (`$2000`, 256 bytes), state (`nge_map`), tiles (80 x 48, 7.5K) | `nge_map_init()` |

- **Common data files:** a module's tables for one of these pages are in its
  `nge_<module>_cdata.c`, with the marker `/* nge-dataseg: PAGE_50 */` or
  `PAGE_51`: uninitialised variables only, set up by the module's init.
- **Origins without a page** (`CRT_ORG_PAGE_50 = 0x0100`,
  `CRT_ORG_PAGE_51 = 0x2000` in [banks.inc](../src/nge/banks.inc)): the
  data is addressed in its slot, never through `banked_call`.
- [scripts/check-banks.sh](../scripts/check-banks.sh) checks that every
  symbol of a `*_cdata.c` file is inside one of these pages and its range,
  that each page's section stays in its range (the linker also stops a page
  overflowing into the next), and that no source puts code or constants
  there (`#pragma codeseg`/`constseg PAGE_50` or `PAGE_51`).
- **One map:** with its data at fixed addresses, no map function takes a
  pointer (`nge_map_set_camera(x, y)`, `NGE_MAP_TILE(x, y)`). A later level
  system can keep each dungeon level's whole map (state, tiles, blocks) in a
  page with the same layout and switch level with one MMU write.

## ECS data in slot 0

Slot 0 holds page 50, the **ECS data page**, for the whole game:
`nge_ecs_init()` ([nge_ecs.h](../src/nge/ecs/nge_ecs.h)) maps it after
`nge_hw_init()`. It holds the ECS tables that several systems read every
frame, out of the program's common memory, which is left for code and other
data.

| Address | Contents |
|---|---|
| `$0000`-`$00FF` | Guard area against null pointers |
| `$0100`-`$1FFF` | ECS tables (`SECTION PAGE_50`, `CRT_ORG_PAGE_50 = 0x0100` in [banks.inc](../src/nge/banks.inc)): entity flags, components and alive list, owners, locations, block flags (about 1.1K at `NGE_ENTITY_MAX` 100) |

### Where component data goes

- **Hot and shared:** in slot 0. Hot means read or written per entity, per
  frame, by more than one system, or by per-frame code in common memory
  (the `NGE_ENTITY_*`, `NGE_OWNER_*` and `NGE_LOCATION_*` macros). Examples:
  the alive list, owner, location.
- **Used by one system:** in that system's odd page, as page data. The
  system's update function is banked, so calling it maps its code into slot 6
  and its data into slot 7, and it runs a tight loop over its own data and
  the shared data in slot 0 (and the map in slot 1). Example: the sprite
  component, page data of the render system
  ([nge_render.h](../src/nge/ecs/systems/nge_render.h)), which builds every
  sprite in one call per frame and uploads them in another.
- **Shared but not hot** (used only when particular game events happen):
  in the page of the system that owns it, with banked `get` and `set`
  helpers for the others.
- **Never call another system's helpers per entity per frame.** Each is a
  banked call (two MMU writes each way plus the banking stack, a few hundred
  T-states). Data that others need every frame is hot: it belongs in slot 0,
  or the owning system provides one call that does the whole batch.
- **One table per field, indexed by entity ID** (`nge_location_x[]`,
  `nge_location_y[]`), not an array of structs: indexing a byte table is the
  cheapest access on the Z80.

### ECS data files

A module's slot 0 tables are in its **ECS data file**,
`<module>/nge_<module>_cdata.c`, which starts with the marker
`/* nge-dataseg: PAGE_50 */` (the same mechanism as
[page data](#page-data-a-modules-variables-and-tables-in-its-odd-page)).
Game code can put its own hot shared components in slot 0 the same way.

- **Uninitialised variables only:** the owning module's init or create
  function sets them. No code and no `const` data in page 50.
- **On the PC tests** the marker has no effect: the tables are ordinary
  globals.
- [scripts/check-banks.sh](../scripts/check-banks.sh) checks that every
  symbol of a `*_cdata.c` file is in `PAGE_50` between `$0100` and `$1FFF`
  (an initialised variable or a missing marker would otherwise put it in
  common memory), that the section stays in `$0100`-`$1FFF`, and that no
  source sets `#pragma codeseg` or `constseg PAGE_50`.

### The guard area

With RAM in slot 0, a write through a null pointer (or a small offset from
one) no longer crashes: it would silently change ECS data. The tables
therefore start at `$0100`. `nge_ecs_init()` fills `$0000`-`$00FF` with a
pattern that varies by address, and `nge_ecs_check_guard()` (compiled out
with `NDEBUG`) aborts with a message if it has changed; the game calls it
once a frame. It catches writes only: a read through a null pointer returns
the pattern.

### When slot 0 is mapped out

ECS data is unavailable, and must not be read, while the ROM is in slot 0:

- **CSpect debug output:** `nge_cspect_print()` maps the ROM in around each
  `rst $18` (see below). Values passed to `nge_printf` are copied first, so
  printing them is fine; a string printed with `%s` must not be in slot 0.
- **NextZXOS and esxDOS calls** (file loading): the loader must not touch
  ECS data during a call.
- **A future IM2 interrupt handler** must not read slot 0, or debug output
  must disable interrupts around the ROM swap.

**Layer 2 must be accessed through the MMU only,** never by mapping it with
port `$123B`: that overlays `$0000`-`$3FFF` (or more), hiding the ECS data
and the map.

## Slots 0 and 1

Writing a RAM page number to registers `0x50`/`0x51` replaces that half of
the ROM with writable RAM; writing `$FF` restores the ROM.

**Slot 0 holds RAM** (the ECS data page, from `nge_ecs_init()`), which is
safe because:

- **No maskable interrupt can happen.** `nge_hw_init()` runs `di` and clears
  the interrupt enable registers `$C4`-`$C6` (expansion bus `/INT`, line and
  ULA interrupts, CTC, UARTs), and nge's `banked_call` never enables
  interrupts (see [Banked code](#banked-code)). Nothing reaches `$0038`.
- **CSpect debug output maps the ROM in around each print.** CSpect only
  prints `rst $18` with the ROM in slot 0; with RAM there it jumps to
  `$0018` like a real Z80, so a `RET` stub does not help (tested with
  `rst18test`). `nge_cspect_print()` maps the ROM into slot 0 for the
  `rst $18` and maps the previous page back, so strings printed must not be
  in slot 0.
- **NMI is not handled, by choice.** It cannot be masked and jumps to
  `$0066`, which is game data, so pressing the NMI button crashes the game.

Slot 0 holds the ROM until `nge_ecs_init()` maps the ECS data page there.

**Slot 1 holds the map.** Map data is read constantly, by drawing and by
collision checks, so it can't be mapped in and out per access. All of it is
in page 51 ([Always-mapped data pages](#always-mapped-data-pages)):
`nge_map_blocks` (what each tile ID blocks: walls, pits; see "blocking
flags" in [nge_map.h](../src/nge/map/nge_map.h)) at `$2000`, the state
`nge_map`, and the tiles `nge_map_tiles` (a map of up to 80 x 48, see
`NGE_MAP_STRIDE` and `NGE_MAP_MAX_HEIGHT`), 7,947 bytes. `nge_map_init()`
maps the page into slot 1; the game then fills in the tiles and the block
table. The game's level ([level_bank.c](../src/game/level/level_bank.c))
does this at start-up:

```c
nge_map_init(LEVEL_WIDTH, LEVEL_HEIGHT, &screen_map_view);   /* page 51 into slot 1 */
set_up_blocks();          /* nge_map_clear_blocks(), nge_map_set_block() */
build_test_layout();      /* NGE_MAP_TILE(x, y) = ... */
nge_map_clip_sprites();
```

Consequences:

- **NextZXOS and esxDOS calls** (`rst $08`, file loading) need the whole ROM.
  Map `$FF` back into slot 1 for the duration of a call, for example while
  loading the next area.
- **Interrupts** are disabled (above). A future interrupt module will use
  IM2 with a vector table in the program, never the ROM's IM1 handler.
- **Map data from the NEX file:** a `PAGE_n` section with `CRT_ORG_PAGE_n`
  set to `0x2000` (with `-pragma-define`) is assembled for slot 1.
  Alternatively, an area copies or decompresses its map into the slot-1 page
  when it starts, as the level does.
