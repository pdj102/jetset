Z88DK_ROOT ?= /home/paul/src/z88dk
ZCC := $(Z88DK_ROOT)/bin/zcc
ZCCCFG ?= $(Z88DK_ROOT)/lib/config
TARGET := main
OBJDIR := obj
BINDIR := bin

# Sources are found automatically: a new module, folder or data file needs no
# change here.
#   src/main.c        entry point
#   src/nge/*/        the nge game engine library (see src/nge/README.md)
#   src/game/         Jetset game code
#   src/data/         graphics and other data (.asm)
SRCS := \
	src/main.c \
	$(sort $(wildcard src/nge/*/*.c)) \
	$(sort $(wildcard src/game/*.c)) \
	$(sort $(wildcard src/data/*.asm))

OBJS := $(patsubst src/%,$(OBJDIR)/%.o,$(basename $(SRCS)))

# Includes are written from these roots: "hw/nge_hw.h", "data/sprite_data.h"
INCLUDES := -Isrc/nge -Isrc

BIN_TARGET := $(BINDIR)/$(TARGET)

# Build modes:
#   make                     CSpect: asserts and debug output go to the DebugOut console
#   make CFLAGS=-DNDEBUG     hardware: asserts and debug output compiled out, no RST $18
CFLAGS ?= -DNGE_CSPECT

export PATH := $(Z88DK_ROOT)/bin:$(PATH)
export ZCCCFG

.PHONY: all build check clean test

all: build

build: $(BIN_TARGET).nex

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn -clib=sdcc_iy $(INCLUDES) $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn $< -o $@

# Memory layout (see docs/memory_map.md):
#   stack: 512 bytes below $C000, at the top of slot 5, which is never banked
#   heaps: none; the engine doesn't use malloc or fopen, and z88dk's default
#          heap would fill all memory up to the stack
LDFLAGS := \
	-pragma-define:REGISTER_SP=0xC000 \
	-pragma-define:CRT_STACK_SIZE=512 \
	-pragma-define:CLIB_MALLOC_HEAP_SIZE=0 \
	-pragma-define:CLIB_STDIO_HEAP_SIZE=0

$(BIN_TARGET).nex: $(OBJS)
	@mkdir -p $(BINDIR)
	$(ZCC) +zxn -subtype=nex -clib=sdcc_iy $(LDFLAGS) $(OBJS) -o $(BIN_TARGET) -create-app

check:
	./scripts/check-toolchain.sh

# PC tests for the nge library; see tests/Makefile
test:
	$(MAKE) -C tests Z88DK_ROOT=$(Z88DK_ROOT)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	$(MAKE) -C tests clean
