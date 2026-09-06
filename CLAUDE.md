# Jetset

Jetset is a Spectrum Next game written in C and built with z88dk from WSL.

## Build

Run the default VS Code build task, or execute this command from the project root:

```sh
zcc +zxn -subtype=nex -clib=sdcc_iy main.c -o main -create-app
```

The build produces `main.nex`. Generated compiler and emulator files are ignored by Git.

## Development conventions

- Keep the program entry point in `main.c` until the build layout is deliberately changed.
- Put reusable game code and headers under `src/`.
- Target Spectrum Next hardware and the z88dk `+zxn` target; do not substitute a desktop C runtime.
- Keep changes small and build after changes to C code.
- Use the `-clib=sdcc_iy` flag to select the SDCC-based compiler with the IY register reserved.
- Prefer z88dk and Spectrum Next APIs already available in the local toolchain over adding dependencies.
- Before claiming a feature is complete, run the build and report any emulator or hardware testing separately.

## Working with this repository

Read the nearby source and existing task configuration before changing the project structure. Do not commit generated `.nex`, `.o`, `.map`, or other build artifacts.