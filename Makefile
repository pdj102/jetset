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
	$(sort $(wildcard src/nge/*/*.asm)) \
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

# A module's page data file (<module>/nge_<module>_pdata.c) starts with the
# marker /* nge-dataseg: PAGE_n */: its uninitialised variables go in page n
# (--dataseg) instead of common memory. See "Page data" in docs/memory_map.md.
dataseg = $(shell sed -n '1,5s|^/\* *nge-dataseg: *\(PAGE_[0-9]*\) *\*/.*|-Cs--dataseg\1|p' $(1))

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn -clib=sdcc_iy $(INCLUDES) $(CFLAGS) $(call dataseg,$<) $< -o $@

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
#   banks: origins of banks holding banked code are in src/nge/banks.inc;
#          scripts/check-banks.sh fails the link if one is missing
CODE_ORG := 0x6A00
STACK_BOTTOM := 0xBE00
BANKS_INC := src/nge/banks.inc

LDFLAGS := \
	-startup=31 \
	-pragma-define:CRT_ORG_CODE=$(CODE_ORG) \
	-pragma-define:REGISTER_SP=0xC000 \
	-pragma-define:CRT_STACK_SIZE=512 \
	-pragma-define:CLIB_MALLOC_HEAP_SIZE=0 \
	-pragma-define:CLIB_STDIO_HEAP_SIZE=0 \
	-pragma-define:CLIB_BANKING_STACK_SIZE=32 \
	-pragma-include:$(BANKS_INC)

$(BIN_TARGET).nex: $(OBJS) $(BANKS_INC)
	@mkdir -p $(BINDIR)
	$(ZCC) +zxn -subtype=nex -clib=sdcc_iy $(LDFLAGS) $(OBJS) -o $(BIN_TARGET) -m -create-app
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
test:
	$(MAKE) -C tests Z88DK_ROOT=$(Z88DK_ROOT)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	$(MAKE) -C tests clean
