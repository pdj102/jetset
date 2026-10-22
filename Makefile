Z88DK_ROOT ?= /home/paul/src/z88dk
ZCC := $(Z88DK_ROOT)/bin/zcc
ZCCCFG ?= $(Z88DK_ROOT)/lib/config
TARGET := main
OBJDIR := obj
BINDIR := bin

# Sources are found automatically: a new module, folder or data file needs no
# change here.
#   src/main.c        entry point
#   src/nge/*/        the nge game engine library (see src/nge/README.md),
#                     and one level of subfolders (ecs/components, ecs/systems)
#   src/game/         game code, and one level of subfolders (banked game modules)
#   src/data/         graphics and other data (.asm)
NGE_SRCS := \
	$(sort $(wildcard src/nge/*/*.c src/nge/*/*/*.c)) \
	$(sort $(wildcard src/nge/*/*.asm src/nge/*/*/*.asm))
GAME_SRCS := \
	src/main.c \
	$(sort $(wildcard src/game/*.c src/game/*/*.c)) \
	$(sort $(wildcard src/data/*.asm))
SRCS := $(NGE_SRCS) $(GAME_SRCS)

NGE_OBJS := $(patsubst src/%,$(OBJDIR)/%.o,$(basename $(NGE_SRCS)))
GAME_OBJS := $(patsubst src/%,$(OBJDIR)/%.o,$(basename $(GAME_SRCS)))
OBJS := $(NGE_OBJS) $(GAME_OBJS)

# nge is linked as a library, so only the modules something uses are linked:
# the linker takes every object file it is given, used or not, and common
# memory has no room for unused ones (the tilemap stream, when the UI moved
# to Layer 2). The game's objects are all linked
NGE_LIB := $(OBJDIR)/nge.lib
Z80ASM := $(Z88DK_ROOT)/bin/z88dk-z80asm

# Includes are written from these roots: "hw/nge_hw.h", "data/sprite_data.h"
INCLUDES := -Isrc/nge -Isrc

BIN_TARGET := $(BINDIR)/$(TARGET)

# Build modes:
#   make                     CSpect: asserts and debug output go to the DebugOut console
#   make CFLAGS=-DNDEBUG     hardware: asserts and debug output compiled out, no RST $18
CFLAGS ?= -DNGE_CSPECT

# Header dependencies: zcc can't write them, so the host compiler lists the
# headers each C file includes (obj/<file>.d), and a changed header
# rebuilds every file that includes it. -MG: z88dk's own headers aren't on
# the host's path, and are listed as they are; -MP keeps a deleted header
# from stopping the build.
HOSTCC ?= gcc
DEPFLAGS = -MM -MG -MP -MT $@ -MF $(@:.o=.d) -D__SDCC

export PATH := $(Z88DK_ROOT)/bin:$(PATH)
export ZCCCFG

.PHONY: all build check clean sprites test tiles

all: build

build: $(BIN_TARGET).nex

# The map's metatiles and their palette, generated from the Aseprite tileset
# (docs/tileset_import.md) whenever it or the scripts change. The output is
# committed, so it is a source like any other: src/data/*.asm are found by
# the wildcard above, and tile_data.h is included from src/. C files are
# compiled after it is up to date (order-only: their .d files rebuild those
# that include it)
TILESET := assets/Tilesheet.aseprite
TILE_SCRIPTS := scripts/ase2tiles.py scripts/aseprite.py scripts/next_colour.py
TILE_DATA := src/data/tile_defs_data.asm src/data/tile_palette_data.asm src/data/tile_data.h

$(TILE_DATA) &: $(TILESET) $(TILE_SCRIPTS)
	python3 -I scripts/ase2tiles.py $(TILESET) src/data

tiles: $(TILE_DATA)

# The game's sprites from the Aseprite sprite file (docs/sprite_import.md).
# Step 1: written into obj/sprites/ for trying on the art; the build doesn't
# use them yet
SPRITESET := assets/Sprites.aseprite

sprites:
	python3 -I scripts/ase2sprites.py $(SPRITESET) $(OBJDIR)/sprites

# A module's page data file (<module>/nge_<module>_pdata.c) starts with the
# marker /* nge-dataseg: PAGE_n */: its uninitialised variables go in page n
# (--dataseg) instead of common memory. See "Page data" in docs/memory_map.md.
dataseg = $(shell sed -n '1,5s|^/\* *nge-dataseg: *\(PAGE_[0-9]*\) *\*/.*|-Cs--dataseg\1|p' $(1))

$(OBJDIR)/%.o: src/%.c | $(TILE_DATA)
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn -clib=sdcc_iy $(INCLUDES) $(CFLAGS) $(call dataseg,$<) $< -o $@
	@$(HOSTCC) $(DEPFLAGS) $(INCLUDES) $(CFLAGS) $<

$(OBJDIR)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn $< -o $@

# Memory layout (see docs/memory_map.md):
#   code:  from $6A00, straight after the tile definitions in page 11, up to
#          the stack; the link fails if the program grows into the stack
#   stack: 512 bytes below $C000, at the top of slot 5, which is never banked
#   heaps: none; the engine doesn't use malloc or fopen, and z88dk's default
#          heap would fill all memory up to the stack
#   startup 31: no stdin/stdout. The default startup links z88dk's console
#          drivers (about 3.3K), which nothing uses and which would print to
#          the ULA screen memory under the tilemap
#   banks: origins of banks holding banked code are in src/banks.inc;
#          scripts/check-banks.sh fails the link if one is missing
CODE_ORG := 0x6A00
STACK_BOTTOM := 0xBE00
BANKS_INC := src/banks.inc

LDFLAGS := \
	-startup=31 \
	-pragma-define:CRT_ORG_CODE=$(CODE_ORG) \
	-pragma-define:REGISTER_SP=0xC000 \
	-pragma-define:CRT_STACK_SIZE=512 \
	-pragma-define:CLIB_MALLOC_HEAP_SIZE=0 \
	-pragma-define:CLIB_STDIO_HEAP_SIZE=0 \
	-pragma-define:CLIB_BANKING_STACK_SIZE=32 \
	-pragma-include:$(BANKS_INC)

$(NGE_LIB): $(NGE_OBJS)
	@rm -f $@
	$(Z80ASM) -mz80n -x$@ $(NGE_OBJS)

$(BIN_TARGET).nex: $(GAME_OBJS) $(NGE_LIB) $(BANKS_INC)
	@mkdir -p $(BINDIR)
	$(ZCC) +zxn -subtype=nex -clib=sdcc_iy $(LDFLAGS) $(GAME_OBJS) -L$(OBJDIR) -lnge -o $(BIN_TARGET) -m -create-app
	@end=$$(sed -n 's/^__BSS_END_tail *= \$$\([0-9A-Fa-f]*\).*/\1/p' $(BIN_TARGET).map); \
	if [ -z "$$end" ]; then echo "error: __BSS_END_tail not found in $(BIN_TARGET).map"; rm -f $@; exit 1; fi; \
	if [ $$((0x$$end)) -gt $$(($(STACK_BOTTOM))) ]; then \
		echo "error: the program ends at \$$$$end, past the bottom of the stack at $(STACK_BOTTOM)"; \
		rm -f $@; exit 1; \
	fi; \
	echo "program ends at \$$$$end; $$(( $(STACK_BOTTOM) - 0x$$end )) bytes free below the stack"
	@./scripts/check-banks.sh $(BIN_TARGET).map || { rm -f $@; exit 1; }

check:
	./scripts/check-toolchain.sh

# PC tests for the nge library; see tests/Makefile
test: $(TILE_DATA)
	$(MAKE) -C tests Z88DK_ROOT=$(Z88DK_ROOT)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	$(MAKE) -C tests clean

-include $(OBJS:.o=.d)
