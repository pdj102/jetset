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
- data used every frame: the sprite attribute table, input state, the
  stream pool, game state

Starting at `$6A00` rather than z88dk's default `$8000` adds 5.5K. The tile
definitions can therefore no longer grow past `$69FF`.

The current CSpect build (`bin/main.map`) uses:

| Address | Contents |
|---|---|
| `$6A00`-`$A9B2` | Code and constant data (`CODE`) |
| `$A9B3`-`$AAA3` | Initialised data (`DATA`) |
| `$AAA4`-`$ADCC` | Uninitialised data (`BSS`), including the 640-byte sprite attribute table |
| `$ADCD`-`$BDFF` | Free, about 4K (about 7.7K in the hardware build, without asserts) |
| `$BE00`-`$BFFF` | Stack, 512 bytes |

The link step checks `__BSS_END_tail` in the map file and fails the build if
the program reaches the stack; otherwise it prints how much space is left.

## The stack

SP is set to `$C000`. The stack grows down from the top of slot 5, so its
512 bytes are `$BE00`-`$BFFF`, inside the slots that never change. The first
push writes `$BFFE`-`$BFFF`.

The link command in the [Makefile](../Makefile) sets this (`LDFLAGS`):

```make
-pragma-define:CRT_ORG_CODE=0x6A00 \
-pragma-define:REGISTER_SP=0xC000 -pragma-define:CRT_STACK_SIZE=512 \
-pragma-define:CLIB_MALLOC_HEAP_SIZE=0 -pragma-define:CLIB_STDIO_HEAP_SIZE=0
```

z88dk's own defaults would put the stack at `$FF58`, in slot 7 ("under UDGs",
for Spectrum BASIC), where remapping slot 7 would remove the stack, including
the return address of the function doing the remapping. Its default
`CLIB_MALLOC_HEAP_SIZE` of -1 would also make the heap fill all memory from
the end of BSS to the bottom of the stack. The engine doesn't use `malloc` or
`fopen`, so both heaps are zero.

Nothing may hold a pointer to a stack variable in a banked slot, and nothing
in a banked slot may call back into code that changes the same slot.

## Slots 6 and 7: banked data

z88dk already supports this layout. Assembly or C data placed in
`SECTION PAGE_n` is assembled for `$C000` when n is even and for `$E000` when
n is odd (`CRT_ORG_PAGE_n` in `zxn_rules.inc`), and is loaded into page n by
the NEX file. A 16K set uses an even/odd pair, mapped together into slots 6
and 7.

Choosing page numbers:

- Avoid pages 10, 11 (video and the start of the program) and 4, 5 (the
  program).
- Page 34 holds the demo map (see [Slots 0 and 1](#slots-0-and-1)).
- Avoid the Layer 2 banks if Layer 2 is used.
- Starting game data at page 32 (bank 16) leaves room below for those.

### Sprite and palette data: map, upload, unmap

Sprite patterns and palettes are copied into the FPGA's own memory, not read
from RAM by the video hardware. The source RAM is only needed while the upload
runs. That makes them ideal banked data:

1. Keep a level's graphics together: sprite patterns, the sprite palette, and
   (if they change per level) tile definitions and the tilemap palette, in one
   page pair.
2. Hide sprites, or wait for vblank, so patterns don't change while they are
   being drawn.
3. Map the graphics pages in with `nge_mmu_map()`, which returns the page
   that was there before.
4. Upload:
   - Sprite patterns: `nge_sprite_load_patterns()` or
     `nge_sprite_load_patterns4()` select the first pattern with port `$303B`,
     then write the bytes to port `$5B` (zxnDMA could do this faster later).
     The pattern RAM is 16K: 64 patterns of 16x16 at 8 bits per pixel, or 128
     at 4 bits.
   - Palettes: `nge_write_palette9()` reads from any pointer, so it works on
     banked data while the page is mapped. A full 256-entry 9-bit palette is
     512 bytes.
   - Tile definitions: copy into `$4A00` (slot 2 and 3 stay mapped, so the
     copy goes directly from slots 6/7 into video memory).
5. Map the saved pages back.

The demo in [main.c](../src/main.c) does this with its graphics in
[sprite_data.asm](../src/data/sprite_data.asm) (`SECTION PAGE_32`):

```c
uint8_t old = nge_mmu_map(DEMO_SPRITE_SLOT, DEMO_SPRITE_PAGE);

nge_sprite_load_patterns(0, demo_sprite_patterns, DEMO_SPRITE_PATTERNS);
nge_write_palette9(RPC_SELECT_SPRITES_PALETTE_0, 0, demo_sprite_palette, DEMO_SPRITE_COLOURS);

nge_mmu_map(DEMO_SPRITE_SLOT, old);
```

The upload code, its stack and its local variables are all in the program and stack (slots 3-5),
so they stay mapped throughout. Saving and restoring the previous pages,
rather than restoring fixed defaults, lets these calls nest.

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

**Slot 0 keeps the ROM.** CSpect's debug output (`rst $18`) stops working
when slot 0 holds RAM (tested); it keeps working with RAM in slot 1. Slot 0
also holds the RST and interrupt entry points (`$0038` IM1, `$0066` NMI).

**Slot 1 holds the active area's map.** Map data is read constantly, by
drawing and by collision checks, so it can't be mapped in and out per access.
The area maps its map's page into slot 1 when it starts, and the page stays
there while the area is active. A map of up to 80 x 48 tiles (7.5K, see
`NGE_MAP_STRIDE` and `NGE_MAP_MAX_HEIGHT` in
[nge_map.h](../src/nge/map/nge_map.h)) fits in the slot. The demo in
[main.c](../src/main.c) maps page 34 there and builds its map at startup:

```c
nge_mmu_map(DEMO_MAP_SLOT, DEMO_MAP_PAGE);          /* slot 1, page 34 */
build_demo_map((nge_tile_t *)NGE_MMU_SLOT_ADDR(DEMO_MAP_SLOT));
nge_map_init(&map, (nge_tile_t *)NGE_MMU_SLOT_ADDR(DEMO_MAP_SLOT),
             DEMO_MAP_W, DEMO_MAP_H, &game_area);
```

Consequences:

- **NextZXOS and esxDOS calls** (`rst $08`, file loading) need the whole ROM.
  Map `$FF` back into slot 1 for the duration of a call, for example while
  loading the next area.
- **Interrupts:** keep them disabled (the z88dk startup code runs `di`, and
  `nge_video_init()` turns off the ULA interrupt) or use IM2 with a vector
  table in the program; never the ROM's IM1 handler.
- **Map data from the NEX file:** a `PAGE_n` section with `CRT_ORG_PAGE_n`
  set to `0x2000` (with `-pragma-define`) is assembled for slot 1.
  Alternatively, an area copies or decompresses its map into the slot-1 page
  when it starts, as the demo does.
