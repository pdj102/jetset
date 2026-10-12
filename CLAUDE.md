# Jetset

Jetset is a Spectrum Next game written in C and built with z88dk from WSL.

## Build

Run make

The build produces `bin/main.nex`. Generated compiler and emulator files are ignored by Git.

## Development conventions

- Keep the program entry point in `src/main.c` until the build layout is deliberately changed.
- Put reusable engine code in the nge library under `src/nge/<folder>/` (prefix `nge_`/`NGE_`); follow the naming and layering rules in `src/nge/README.md`.
- Put Jetset-specific code in `src/game/` and graphics and other data in `src/data/`. The Makefiles find new files automatically.
- Add a PC test `tests/test_<module>.c` for new nge modules and run `make test`.
- Banked code (`NGE_BANKED`) must have its bank listed in `src/nge/banks.inc` and follow the rules in "Banked code" in `docs/memory_map.md`: call `nge_hw_init()` first, and never call banked functions while a page is mapped by hand in slot 6 or 7.
- Target Spectrum Next hardware and the z88dk `+zxn` target; do not substitute a desktop C runtime.
- Keep changes small and build after changes to C code.
- Use the `-clib=sdcc_iy` flag to select the SDCC-based compiler with the IY register reserved.
- Prefer z88dk and Spectrum Next APIs already available in the local toolchain over adding dependencies.
- Before claiming a feature is complete, run the build and report any emulator or hardware testing separately.

## Working with this repository

Read the nearby source and existing task configuration before changing the project structure. Do not commit generated `.nex`, `.o`, `.map`, or other build artifacts.