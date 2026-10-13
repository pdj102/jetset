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
| 0 | `$0000`-`$1FFF` | ROM | **Keeps the ROM** (see [Slots 0 and 1](#slots-0-and-1)) |
| 1 | `$2000`-`$3FFF` | ROM | The active area's map (`nge_map`), with the ROM's upper half swapped out |
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
- data used every frame: the sprite attribute table, the entity tables
  (`nge_entity`: 7 bytes per entity, `NGE_ENTITY_MAX` entities), input
  state, the stream pool, game state

Starting at `$6A00` rather than z88dk's default `$8000` adds 5.5K. The tile
definitions can therefore no longer grow past `$69FF`.

The current CSpect build (`bin/main.map`) uses:

| Address | Contents |
|---|---|
| `$6A00`-`$9EF5` | Code and constant data (`CODE`) |
| `$9EF6`-`$9F14` | Initialised data (`DATA`) |
| `$9F15`-`$A503` | Uninitialised data (`BSS`), including the sprite attribute table (640 bytes) and the entity tables (about 700 bytes) |
| `$A504`-`$BDFF` | Free, about 6.2K (about 10K in the hardware build, without asserts) |
| `$BE00`-`$BFFF` | Stack, 512 bytes |

The link step checks `__BSS_END_tail` in the map file and fails the build if
the program reaches the stack; otherwise it prints how much space is left.

The program is linked with z88dk's startup 31, which creates no `stdin` or
`stdout`. The default startup links z88dk's console drivers (about 3.3K),
which nothing uses and which would print to the ULA screen memory under the
tilemap. Don't use the C library's `stdio` functions; use `output/`.

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

The target is a 2MB Next: pages 0-223. Every module that has banked code
gets a pair of pages: the even page for its code, string literals and
constants (8K: `codeseg` and `constseg PAGE_n`), the odd page for large data
sets private to it. The allocation is kept in
[src/nge/banks.inc](../src/nge/banks.inc), which also sets the origins of the
code pages.

| Pages | Owner |
|---|---|
| 0-31 | System: program (4-5), video (10-11), bank 7 (14-15), Layer 2 (16-31) |
| 32-39 | Spare |
| 40/41 | nge `output` (reserved; the output path stays in common memory) |
| 42/43 | nge `sprite` (setup functions and loaders) |
| 44/45 | nge `map` (reserved; its code runs every frame and is in common memory) |
| 46/47 | nge `event` (reserved) |
| 48-67 | nge `ecs` (reserved: 10 pairs) |
| 68-87 | Further nge modules (reserved: 10 pairs) |
| 88-223 | Game code and assets; the demo uses 88 (map, slot 1) and 89 (sprites) |

- **Module code, string literals and constants:** the even page. A banked
  `.c` file sets both `#pragma codeseg PAGE_n` and `#pragma constseg
  PAGE_n`, so its literals (assert messages included) and constants are in
  its own page, not in common memory. 8K per module: if it grows past 8K,
  the linker stops the build ("Section PAGE_n overlaps section PAGE_n+1");
  split the module across another pair from its range.
- **Large private data sets:** the odd page, mapped into slot 7 whenever the
  module's code runs, so the module reads it directly. Not every constant
  goes here, only data sets big enough to matter. `constseg` applies to a
  whole file, so a data set gets its own file: a `.c` file with only
  `#pragma constseg PAGE_<odd>`, or an `.asm` file with
  `SECTION PAGE_<odd>`. Only the module's own code reads it.
- **Banked literals and data are passed only to common code** (the output
  path, `nge_util`) **or code in the same page,** and not kept after the
  call returns. A function in another bank would be called through
  `banked_call`, which maps its pages over them. The whole output path stays
  in common memory for this reason: `core/nge_util`, `output/` (streams,
  `nge_print`, `nge_printf`) and `devices/nge_cspect`. The streams use each
  character at once (the CSpect line buffer copies it; the tilemap stream
  draws it), so no stream keeps a pointer to a string.
- **Game assets** (sprite sets, maps, level data) go in the game range, not
  in nge module pages. The sprite loaders accept any odd page.

### Banked code

Functions that don't need to be in memory all the time can live in a banked
page and be called through z88dk's `banked_call`. On each call it maps the
function's page into slot 6 and the next page into slot 7, calls it, and maps
the previous pages back. Page 42 holds the sprite setup functions,
including the loaders, with their string literals
(`sprite/nge_sprite_setup.c`; about 1.3K of 8K).

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
  (page 42: setup and loading).
- **Use even `PAGE_n` sections, or 16K `BANK_n` sections.** `banked_call`
  maps the page into slot 6, so code in an odd page (assembled for `$E000`)
  would be in the wrong slot.
- **Keep these in common memory:**
  - code that runs every frame (`nge_sprite_update`, `nge_map_update`, the
    tilemap stream, `nge_putc`): each banked call costs about 150-250
    T-states;
  - functions that read data the caller has mapped into slot 6 or 7, such
    as `nge_write_palette9()`: a banked function would map its own page over
    that data (banked loaders take the data's page instead, as the sprite
    loaders do);
  - functions called through a pointer, such as stream `put` and `flush`
    functions: a pointer to a banked function doesn't switch banks;
  - `__z88dk_fastcall` functions: `banked_call` overwrites HL, which holds
    their argument;
  - the output path (`core/nge_util`, `output/`, `devices/nge_cspect`), so
    banked code can print its own literals;
  - `nge_mmu`, `nge_hw_init` and interrupt code.
- **Literals and constants live in the module's page** (`constseg`, above).
  Without `constseg`, SDCC would put them in common memory: assert messages
  alone cost about 45 bytes per assert site in CSpect builds.
- Return values in HL and DE (up to 32 bits) and stack arguments, including
  variadic ones, work as normal.

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
- The demo's graphics are in page 89, in the game range
  ([sprite_data.asm](../src/data/sprite_data.asm), `SECTION PAGE_89`). A
  game with several sprite sets puts each in its own odd page.
- Hide sprites, or load during vblank, so patterns don't change while they
  are being drawn.
- Tile definitions can be copied into `$4A00` the same way: slots 2 and 3
  stay mapped, so a copy goes directly from slot 7 into video memory.

Keep in non-banked memory anything used after the upload:

- the sprite attribute table (128 sprites x 5 bytes = 640 bytes, a static
  array in [nge_sprite.c](../src/nge/sprite/nge_sprite.c), so in BSS), sent to port
  `$57` by `nge_sprite_update()` every frame;
- a palette copy used for fades or palette cycling;
- pointers into banked data are valid only while their page is mapped, so
  copy any values needed later before unmapping.

### Other banked data

Text and level tables can use the same pattern: map the page, copy or decode
what is needed, then unmap. The active area's map is the exception: it is
read constantly (drawing and collision checks), so it stays mapped in slot 1
instead.

## Slots 0 and 1

Writing a RAM page number to registers `0x50`/`0x51` replaces that half of
the ROM with writable RAM; writing `$FF` restores the ROM.

**Slot 0 may hold RAM** (for common game data, from `$0000`), because:

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

Slot 0 still holds the ROM until the game maps a page there.

**Slot 1 holds the active area's map.** Map data is read constantly, by
drawing and by collision checks, so it can't be mapped in and out per access.
The area maps its map's page into slot 1 when it starts, and the page stays
there while the area is active. A map of up to 80 x 48 tiles (7.5K, see
`NGE_MAP_STRIDE` and `NGE_MAP_MAX_HEIGHT` in
[nge_map.h](../src/nge/map/nge_map.h)) fits in the slot. The demo in
[main.c](../src/main.c) maps page 88 there and builds its map at startup:

```c
nge_mmu_map(DEMO_MAP_SLOT, DEMO_MAP_PAGE);          /* slot 1, page 88 */
build_demo_map((nge_tile_t *)NGE_MMU_SLOT_ADDR(DEMO_MAP_SLOT));
nge_map_init(&map, (nge_tile_t *)NGE_MMU_SLOT_ADDR(DEMO_MAP_SLOT),
             DEMO_MAP_W, DEMO_MAP_H, &game_area);
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
  when it starts, as the demo does.
