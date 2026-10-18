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
| `map/` | The map: one tile map larger than the screen, shown through a tilemap window, with a block table by tile ID. Its data is in page 51, always in slot 1 (no map pointers); set-up banked in page 44 |
| `input/` | Keyboard: a per-frame snapshot of the keys (held, pressed, released), key events with auto-repeat, ASCII (`nge_input`; setup and tables banked in page 88) |
| `devices/` | Raw device drivers (CSpect DebugOut, UART), no streams |
| `io/` | Streams (output and input), print and printf, the stream devices for the tilemap, CSpect, UART and keyboard, and the line editor (`nge_line`) |
| `event/` | Events: a queue in common memory of things that have happened (target, event, source, two payloads), handled in order through the game's dispatch function (`nge_event`) |
| `ecs/` | Entity component system core, data every entity has: `nge_entity` (IDs, lifecycle, flags, component masks; lifecycle banked in pages 48/49), `nge_owner` (the owner of every entity: an entity, the world or an area; changes banked in page 48), `nge_ecs` (ECS start-up: the ECS data page in slot 0 and its guard area) |
| `ecs/components/` | What a component is, and every component number, nge's and the game's range (`nge_components.h`); components shared by several systems, with their data in slot 0: `nge_location` (tile and pixel position; entities at a tile), `nge_block` (what an entity blocks and can enter; whether a tile can be entered, map and entities together) |
| `ecs/systems/` | Systems, each with its private components as page data: `nge_render` (draws the sprite component as hardware sprites; pages 52/53), `nge_timer` (the game clock: timers on entities fire every period, for actors' turns and effects, and the clock can be stopped for the player; page 54, data in slot 0) |

Planned: more of `input/` (joysticks, actions), `sound/` (AY), more of
`ecs/` (components, systems, deferred clean-up).

Where an ECS file goes follows where its data goes: a component shared by
several systems is in `components/` (slot 0), a component used by one system
is part of that system in `systems/` (its page data). Within `ecs/`,
`systems/` may include `components/` and the core, `components/` only the
core, and the core neither.

A folder may include only the folders before it in this chain:

```
core  <-  hw  <-  tilemap, sprite, devices, input, sound  <-  io  <-  map, event  <-  ecs  <-  game
```

- Stream devices live in `io/`, not with the hardware they drive, so
  `tilemap/` and `devices/` work without streams.
- `map/`, `event/` and `ecs/` don't touch hardware registers themselves (the
  render system drives sprites through `sprite/`), so they can be tested
  entirely on the PC.
- One exception: `core/nge_util` prints asserts and debug output to a
  stream, so it includes `io/nge_printf.h` and `io/nge_stream.h`.

## Module initialisation

Every `<module>_init()` (nge and game alike) must:

- **Reset the module completely** to a valid initial state: every variable,
  table and piece of hardware it owns, whatever state it was left in. Don't
  rely on the NEX file's zeros or on an earlier call (page data is
  uninitialised, see "Page data" in docs/memory_map.md).
- **Be safe to call more than once.** A new game calls the inits again
  (`GAME_NEW_GAME` in src/game/game.c), so an init must not leak resources
  (close streams it opened before) and must undo what the last use left
  behind (`nge_render_init()` hides the sprites it drew; `nge_map_init()`
  clears the tiles and the block table).

Add a check to the module's PC test that a second init gives the same
state as the first.

## Banked code

Code that isn't needed all the time can live in a banked page (see "Banked
code" in [docs/memory_map.md](../../docs/memory_map.md)):

- Declare its functions `NGE_BANKED` (from `core/nge_bank.h`) in the header
  and the definitions, and start the `.c` file with guarded
  `#pragma codeseg PAGE_n` and `#pragma constseg PAGE_n` (n even), so its
  string literals and constants are in its page too.
- List the section in [src/banks.inc](../banks.inc). The build fails if it is
  missing (`scripts/check-banks.sh`).
- Bank whole modules whose functions are called occasionally, such as
  the sprite setup functions (`sprite/nge_sprite_setup.c`, page 42). Each
  module has its own pair of pages from the allocation in
  [src/banks.inc](../banks.inc): the even page for code, literals and constants,
  the odd page for large private data sets.
- Large private variables and tables go in the module's odd page, in its
  page data file `nge_<module>_pdata.c`, which starts with the marker
  `/* nge-dataseg: PAGE_n */` (uninitialised variables only) and
  `#pragma constseg PAGE_n` (tables). See "Page data" in
  `docs/memory_map.md`.
- ECS tables that several systems read every frame go in slot 0, in the
  module's ECS data file `nge_<module>_cdata.c` with the marker
  `/* nge-dataseg: PAGE_50 */` (uninitialised variables only). See "ECS
  data in slot 0" in `docs/memory_map.md` for which data goes where.
- The output path (`core/nge_util`, `io/`, `devices/nge_cspect`) stays
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
