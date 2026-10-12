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
| `core/` | Basic types (`nge_types.h`), asserts and debug output (`nge_util`) |
| `hw/` | Hardware setup, frame sync, MMU paging, palettes |
| `tilemap/` | Tilemap layer |
| `sprite/` | Hardware sprites |
| `devices/` | Raw device drivers (CSpect DebugOut, UART), no streams |
| `output/` | Streams, print and printf, and the stream devices for the tilemap, CSpect and UART |

Planned: `input/` (keyboard, joystick), `sound/` (AY), `map/` (rooms, tile
collision), `event/` (event queue), `ecs/` (entities and components).

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

## Building and testing

The game Makefile and `tests/Makefile` find `src/nge/*/*.c` automatically. A
new module needs only its files and a `tests/test_<module>.c`. See
`tests/Makefile` for faking Next-only code on the PC.
