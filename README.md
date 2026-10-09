# Jetset

A Spectrum Next game project using C, z88dk, WSL, and VS Code.

## Requirements

- WSL
- z88dk with the `+zxn` target available on `PATH`
- the host packages `m4`, `bison`, `flex`, and Boost Graph (`sudo apt install m4 bison flex libboost-graph-dev` on Debian/Ubuntu WSL)
- VS Code opened in the WSL environment

## Build

Use **Terminal: Run Build Task** in VS Code, or run:

```sh
zcc +zxn -subtype=nex -clib=sdcc_iy -I/home/paul/src/z88dk/include src/main.c src/zxn/zxn_hw.c src/zxn/zxn_frame.c -o main -create-app
```

This creates `main.nex`, which can be loaded in an emulator or copied to a Spectrum Next SD card.

If the build reports a missing host dependency, run `./scripts/check-toolchain.sh`. On Debian/Ubuntu WSL, install the prerequisites with `sudo apt install m4 bison flex libboost-graph-dev`.

## Claude Code

Project-specific guidance is in [`CLAUDE.md`](CLAUDE.md). Claude Code will read it when started from this project directory. Keep the build command and Spectrum Next target in mind when asking Claude to modify the game.