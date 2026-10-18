# Jetset

Jetset is a Spectrum Next game written in C and built with z88dk from WSL.

## Build

Run make

The build produces `bin/main.nex`. Generated compiler and emulator files are ignored by Git.

## Code guidelines

All C code follows the rules in `docs/guidelines/` (index: `docs/guidelines/README.md`). Before writing code, read the files that apply to the folder you are changing; before handing it over, check it against them. The `review` skill does both checks. The rules cover naming, headers, constant groups, comments, toolchain traps, the engine's layers, banking and data placement, module initialisation, the game's modules and architecture, and the design patterns (each written up in its owning header).

## Development conventions

- Keep the program entry point in `src/main.c` until the build layout is deliberately changed.
- Put reusable engine code in the nge library under `src/nge/<folder>/`, game-specific code in `src/game/`, and graphics and other data in `src/data/`. The Makefiles find new files automatically.
- Target Spectrum Next hardware and the z88dk `+zxn` target; do not substitute a desktop C runtime.
- Keep changes small and build after changes to C code.
- Use the `-clib=sdcc_iy` flag to select the SDCC-based compiler with the IY register reserved.
- Prefer z88dk and Spectrum Next APIs already available in the local toolchain over adding dependencies.
- Add or extend PC tests for new code and run `make test`.
- Before claiming a feature is complete, run the build and report any emulator or hardware testing separately.

## Working with this repository

Read the nearby source and existing task configuration before changing the project structure. Do not commit generated `.nex`, `.o`, `.map`, or other build artifacts.