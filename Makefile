Z88DK_ROOT ?= /home/paul/src/z88dk
ZCC := $(Z88DK_ROOT)/bin/zcc
ZCCCFG ?= $(Z88DK_ROOT)/lib/config
TARGET := main
OBJDIR := obj
BINDIR := bin

SRCS := \
	src/main.c \
	src/zxn/zxn_cspect.c \
	src/zxn/zxn_cspect_stream.c \
	src/zxn/zxn_hw.c \
	src/zxn/zxn_mmu.c \
	src/zxn/zxn_frame.c \
	src/zxn/zxn_palette.c \
	src/zxn/zxn_print.c \
	src/zxn/zxn_printf.c \
	src/zxn/zxn_sprite.c \
	src/zxn/zxn_stream.c \
	src/zxn/zxn_tilemap.c \
	src/zxn/zxn_tm_stream.c \
	src/zxn/zxn_uart.c \
	src/zxn/zxn_uart_stream.c \
	src/zxn/zxn_util.c \
	src/tile_defs_data.asm \
	src/sprite_data.asm

OBJS := \
	$(OBJDIR)/main.o \
	$(OBJDIR)/zxn/zxn_cspect.o \
	$(OBJDIR)/zxn/zxn_cspect_stream.o \
	$(OBJDIR)/zxn/zxn_hw.o \
	$(OBJDIR)/zxn/zxn_mmu.o \
	$(OBJDIR)/zxn/zxn_frame.o \
	$(OBJDIR)/zxn/zxn_palette.o \
	$(OBJDIR)/zxn/zxn_print.o \
	$(OBJDIR)/zxn/zxn_printf.o \
	$(OBJDIR)/zxn/zxn_sprite.o \
	$(OBJDIR)/zxn/zxn_stream.o \
	$(OBJDIR)/zxn/zxn_tilemap.o \
	$(OBJDIR)/zxn/zxn_tm_stream.o \
	$(OBJDIR)/zxn/zxn_uart.o \
	$(OBJDIR)/zxn/zxn_uart_stream.o \
	$(OBJDIR)/zxn/zxn_util.o \
	$(OBJDIR)/tile_defs_data.o \
	$(OBJDIR)/sprite_data.o

BIN_TARGET := $(BINDIR)/$(TARGET)

# Build modes:
#   make                     CSpect: asserts and debug output go to the DebugOut console
#   make CFLAGS=-DNDEBUG     hardware: asserts and debug output compiled out, no RST $18
CFLAGS ?= -DZXN_CSPECT

export PATH := $(Z88DK_ROOT)/bin:$(PATH)
export ZCCCFG

.PHONY: all build check clean test

all: build

build: $(BIN_TARGET).nex

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn -clib=sdcc_iy $(CFLAGS) $< -o $@

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

# PC tests for the zxn library; see tests/Makefile
test:
	$(MAKE) -C tests Z88DK_ROOT=$(Z88DK_ROOT)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	$(MAKE) -C tests clean
