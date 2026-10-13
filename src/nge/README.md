# nge: Next Game Engine

A lightweight game library for the ZX Spectrum Next, built with z88dk
(`+zxn`, `-clib=sdcc_iy`).

## Naming

| Kind | Form | Example |
|---|---|---|
| Functions | `nge_<module>_<verb>` | `nge_sprite_set`, `nge_tilemap_fill` |
| Types | `nge_<thing>_t` | `nge_rect_t`, `nge_tile_t` |
| Stream type | stdio style | `NGE_FILE`, `nge_printf` |
| Macros and constants | `NGE_<MODULE>_<NAME>` | `NGE_SPRITE_MAX` |
| Files | `<folder>/nge_<module>.[ch]` | `sprite/nge_sprite.c` |
| Banked functions | `NGE_BANKED` after the parameters, in header and definition | `int nge_puts(const char *s, NGE_FILE *stream) NGE_BANKED;` |

File names keep the `nge_` prefix inside their folders: z88dk object and
symbol names are global and the PC tests link everything together, so base
names must be unique.

Next register and port names come from z88dk (`REG_*`, `IO_*`, and the
`ZXN_READ_REG`/`ZXN_WRITE_REG` macros); they are not renamed. z88dk also has
`zxn_*` functions of its own, which is why the library doesn't use `zxn`.

Includes are written from `src/nge`, with the folder:

```c
#include "sprite/nge_sprite.h"
```

## Folders and layers

| Folder | Contents |
|---|---|
| `core/` | Basic types (`nge_types.h`), banked-function marker (`nge_bank.h`), asserts and debug output (`nge_util`) |
| `hw/` | Hardware setup, frame sync, MMU paging, palettes |
| `tilemap/` | Tilemap layer |
| `sprite/` | Hardware sprites |
| `map/` | A tile map larger than the screen, shown through a tilemap window |
| `devices/` | Raw device drivers (CSpect DebugOut, UART), no streams |
| `output/` | Streams, print and printf, and the stream devices for the tilemap, CSpect and UART |
| `ecs/` | Entity component system: `nge_entity` (IDs, lifecycle, flags, component masks) |

Planned: `input/` (keyboard, joystick), `sound/` (AY), `event/` (event
queue), more of `ecs/` (components, systems, deferred clean-up).

A folder may include only the folders before it in this chain:

```
core  <-  hw  <-  tilemap, sprite, devices, input, sound  <-  output  <-  map, event, ecs  <-  game
```

- Stream devices live in `output/`, not with the hardware they drive, so
  `tilemap/` and `devices/` work without streams.
- `map/`, `event/` and `ecs/` don't touch hardware registers, so they can be
  tested entirely on the PC.
- One exception: `core/nge_util` prints asserts and debug output to a
  stream, so it includes `output/nge_printf.h` and `output/nge_stream.h`.

## Banked code

Code that isn't needed all the time can live in a banked page (see "Banked
code" in [docs/memory_map.md](../../docs/memory_map.md)):

- Declare its functions `NGE_BANKED` (from `core/nge_bank.h`) in the header
  and the definitions, and start the `.c` file with guarded
  `#pragma codeseg PAGE_n` and `#pragma constseg PAGE_n` (n even), so its
  string literals and constants are in its page too.
- List the section in [banks.inc](banks.inc). The build fails if it is
  missing (`scripts/check-banks.sh`).
- Bank whole modules whose functions are called occasionally, such as
  the sprite setup functions (`sprite/nge_sprite_setup.c`, page 42). Each
  module has its own pair of pages from the allocation in
  [banks.inc](banks.inc): the even page for code, literals and constants,
  the odd page for large private data sets.
- Large private variables and tables go in the module's odd page, in its
  page data file `nge_<module>_pdata.c`, which starts with the marker
  `/* nge-dataseg: PAGE_n */` (uninitialised variables only) and
  `#pragma constseg PAGE_n` (tables). See "Page data" in
  `docs/memory_map.md`.
- The output path (`core/nge_util`, `output/`, `devices/nge_cspect`) stays
  in common memory, so banked code can print its own literals. Banked
  literals and data may be passed only to common code or code in the same
  page. A module with both
  per-frame and setup functions is split into two files, as `sprite/` is
  (`nge_sprite.c` common, `nge_sprite_setup.c` banked).
- A banked function that needs data from another page takes the page as an
  argument and maps it into slot 7 itself (see the sprite loaders); its own
  code must be in an 8K `PAGE_n` section so slot 7 holds no code. Keep per-frame code,
  callbacks (stream `put`/`flush`), fastcall functions, `hw/nge_mmu`,
  `nge_hw_init` and interrupt code in common memory.
- Being banked is part of a function's interface: a caller in common code
  must not call it while it has its own page mapped in slot 6 or 7.

On the PC tests `NGE_BANKED` is empty and the pragma is skipped, so banked
modules are tested like any other.

## Building and testing

The game Makefile and `tests/Makefile` find `src/nge/*/*.c` automatically. A
new module needs only its files and a `tests/test_<module>.c`. See
`tests/Makefile` for faking Next-only code on the PC.
