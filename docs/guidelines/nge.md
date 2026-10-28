# nge: engine rules

Rules for the engine library in `src/nge/`. The banking and data rules apply
to game code too. The explanations are in [src/nge/README.md](../../src/nge/README.md)
and [docs/memory_map.md](../memory_map.md); each rule names its section. See
[README.md](README.md) for the format and strengths.

## Naming and files

### N-NAME-1 · should · Engine names carry the nge prefix and their module
Functions `nge_<module>_<verb>` (`nge_sprite_set`), types
`nge_<thing>_t`, macros and constants `NGE_<MODULE>_<NAME>`, globals
`nge_<module>_<name>`. Next register and port names from z88dk (`REG_*`,
`IO_*`) are used as they are. See "Naming" in `src/nge/README.md`.

### N-FILE-1 · should · A module's files are split by where they live
In `src/nge/<folder>/`:
- `nge_<module>.c`: common memory;
- `nge_<module>_<part>.c`: a banked part, named by what it does (`_setup`,
  `_life`, `_set`);
- `nge_<module>_cdata.c`: its tables in slot 0 or 1;
- `nge_<module>_pdata.c`: its page data, in the odd page;
- `nge_<module>_internal.h`: shared between those files only.

### N-FILE-2 · must · Base names are unique across the project
No two files in `src/` share a base name, in nge or the game.
Why: z88dk object and symbol names are global, and the PC tests link
everything together.

### N-INC-1 · must · Includes are written from the source roots
Engine headers are included from `src/nge` with their folder
(`#include "sprite/nge_sprite.h"`), and game headers from `src`
(`#include "game/level/level.h"`). Never use a relative path.

## Layers

### N-LAYER-1 · must · A folder includes only the folders below it
`core <- hw <- tilemap, layer2, sprite, devices, input, sound <- io <- map, event, ui <- ecs <- game`.
Within `ecs/`: `systems/` may include `components/` and the core,
`components/` only the core. The one exception is `core/nge_util`, which
includes `io/` for its output. See "Folders and layers" in `src/nge/README.md`.

### N-LAYER-2 · must · nge never includes game code
Nothing in `src/nge/` includes a header from `src/game/`. The game reaches
into the engine only through its headers, and the engine calls game code
only through dispatch functions the game passes in (`nge_timer_advance()`,
`nge_event_process()`).

### N-LAYER-3 · should · map, event and ecs touch no hardware
These folders don't write registers or ports themselves (the render system
drives sprites through `sprite/`), so they run entirely in the PC tests.

## Initialisation

### N-INIT-1 · must · An init resets its module completely and can be called again
Every `<module>_init()`, in nge and the game, sets every variable, table
and piece of hardware the module owns to a valid start state, whatever was
left there. It doesn't rely on zeroed memory, and a second call leaks
nothing: it closes the streams it opened and hides the sprites it drew. See
"Module initialisation" in `src/nge/README.md`.
Check: each module's state against its init. The module's PC test checks
that a second init gives the same state.

## Banking

### N-BANK-1 · must · A banked file starts with its page's pragmas
`#ifdef __SDCC` / `#pragma codeseg PAGE_n` / `#pragma constseg PAGE_n` /
`#endif`, with n even and the module's code page from the allocation in
`src/banks.inc`, so its literals and constants go in its page too.

### N-BANK-2 · must · Banked functions are declared NGE_BANKED in header and definition
`NGE_BANKED` goes after the parameter list in both. A static function in a
banked file is not marked: it is called directly within its page.

### N-BANK-3 · must · Keep these in common memory
Code called many times a frame; functions called through a pointer (stream
`put` and `flush`, the dispatch functions); `__z88dk_fastcall` functions;
functions that read data the caller has mapped into slot 6 or 7; the output
path (`core/nge_util`, `io/`, `devices/nge_cspect`); `nge_mmu`,
`nge_hw_init` and interrupt code. See "Banked code" in
`docs/memory_map.md`.

### N-BANK-4 · must · Never call a banked function while a page is mapped by hand
While code has its own page mapped in slot 6 or 7, it calls no
`NGE_BANKED` function. Banked code may map slot 7, never slot 6.

### N-BANK-5 · must · Banked literals and data go only to common code or the same page
A string literal or table in a banked page may be passed only to a common
function or a function in the same page. The other page's code would map
its own page over it.

### N-BANK-6 · must · Behaviour is chosen by number, not by pointer to a banked function
A pointer to a banked function doesn't switch banks. Choose banked
behaviour with a `switch` on a number in common code (`act_do()`,
`game_on_timer()`). Pointers are only to common functions.

### N-BANK-7 · should · Split a module when only part of it can be banked
A `.c` file is in one section. Per-frame functions stay in the common file,
and setup and rarely run functions go in the banked file (N-FILE-1).

## Data placement

### N-DATA-1 · must · ECS data goes where it is read
Data read or written per entity, per frame, by more than one system or by
common code goes in slot 0 (`*_cdata.c`, page 50). Data used by one system
goes in that system's page data (`*_pdata.c`). See "Where component data
goes" in `docs/memory_map.md`.

### N-DATA-2 · must · cdata and pdata files hold uninitialised variables only
A `*_cdata.c` or `*_pdata.c` file starts with `/* nge-dataseg: PAGE_n */`.
Its variables have no initialiser, and the module's init sets them.
`const` tables in an odd page use `#pragma constseg PAGE_n` instead.

### N-DATA-3 · should · One table per field, indexed by entity ID
Component data is a set of arrays `[NGE_ENTITY_MAX]`, one per field
(`nge_location_x[]`, `nge_location_y[]`), not an array of structs.
A component only one entity ever has (the player's, `game/components/pc.h`)
has one variable per field instead.

### N-DATA-4 · must · Layer 2 only through the MMU
Never map Layer 2 with port `$123B`: it would hide the ECS data in slot 0
and the map in slot 1.

### N-DATA-5 · must · Nothing reads slot 0 while the ROM is in it
Code that maps the ROM into slot 0 (CSpect output, NextZXOS calls) reads no
ECS data until it maps page 50 back. A string printed with `%s` is never in
slot 0. See "When slot 0 is mapped out" in `docs/memory_map.md`.

## Tests

### N-TEST-1 · should · Every nge module has a PC test
`tests/test_<module>.c` for each module, run by `make test`. Hardware-only
drivers (`devices/`) and the Layer 2 drawing (`layer2/`, which writes slot
7 by address) are exempt: they are left out of the test build
(`LIB_EXCLUDE`), and what uses them is tested through fakes in
`tests/support`.
