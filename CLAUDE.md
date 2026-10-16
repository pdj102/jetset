# Jetset

Jetset is a Spectrum Next game written in C and built with z88dk from WSL.

## Build

Run make

The build produces `bin/main.nex`. Generated compiler and emulator files are ignored by Git.

## Development conventions

- Keep the program entry point in `src/main.c` until the build layout is deliberately changed.
- Put reusable engine code in the nge library under `src/nge/<folder>/` (prefix `nge_`/`NGE_`); follow the naming and layering rules in `src/nge/README.md`.
- Put game-specific code in `src/game/` and graphics and other data in `src/data/`. The Makefiles find new files automatically. A banked game module lives in `src/game/<module>/` (`<module>.c` common, `<module>_bank.c` banked, `<module>_pdata.c` page data) with a page pair from the game code range (102-131) in `src/banks.inc`; see "Banked game modules" in `docs/memory_map.md`.
- Add a PC test `tests/test_<module>.c` for new nge modules and run `make test`.
- Every `<module>_init()` (nge and game) must reset the module completely to a valid initial state and be safe to call more than once (a new game calls them again); see "Module initialisation" in `src/nge/README.md`.
- Banked code (`NGE_BANKED`) goes in its module's page pair from the allocation table in `src/banks.inc` (even page for code, string literals and constants via `codeseg` and `constseg`; odd page for large private variables and tables in the module's page data file `nge_<module>_pdata.c` with its `nge-dataseg` marker), must have its origin listed there, and follow the rules in "Banked code" in `docs/memory_map.md`: never call banked functions while a page is mapped by hand in slot 6 or 7, and pass banked literals or data only to common code (the output path stays common) or code in the same page. nge replaces z88dk's `banked_call` (`src/nge/core/nge_banked_call.asm`) so banked calls never enable interrupts.
- Common data in the always-mapped pages goes in the module's `nge_<module>_cdata.c` with an `nge-dataseg` marker: ECS component data read every frame by several systems in page 50 (slot 0, mapped by `nge_ecs_init()`), the map's data in page 51 (slot 1, mapped by `nge_map_init()`). Data used by one system goes in that system's page data. Follow "Always-mapped data pages" and "ECS data in slot 0" in `docs/memory_map.md`; never access Layer 2 through port `$123B`.
- Target Spectrum Next hardware and the z88dk `+zxn` target; do not substitute a desktop C runtime.
- Keep changes small and build after changes to C code.
- Use the `-clib=sdcc_iy` flag to select the SDCC-based compiler with the IY register reserved.
- Prefer z88dk and Spectrum Next APIs already available in the local toolchain over adding dependencies.
- Before claiming a feature is complete, run the build and report any emulator or hardware testing separately.

## Working with this repository

Read the nearby source and existing task configuration before changing the project structure. Do not commit generated `.nex`, `.o`, `.map`, or other build artifacts.