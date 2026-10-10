Z88DK_ROOT ?= /home/paul/src/z88dk
ZCC := $(Z88DK_ROOT)/bin/zcc
ZCCCFG ?= $(Z88DK_ROOT)/lib/config
TARGET := main
OBJDIR := obj
BINDIR := bin

SRCS := \
	src/main.c \
	src/zxn/zxn_hw.c \
	src/zxn/zxn_frame.c \
	src/zxn/zxn_palette.c \
	src/zxn/zxn_putc.c \
	src/zxn/zxn_tile_map.c \
	src/zxn/zxn_uart.c \
	src/zxn/zxn_util.c \
	src/tile_defs_data.asm

OBJS := \
	$(OBJDIR)/main.o \
	$(OBJDIR)/zxn/zxn_hw.o \
	$(OBJDIR)/zxn/zxn_frame.o \
	$(OBJDIR)/zxn/zxn_palette.o \
	$(OBJDIR)/zxn/zxn_putc.o \
	$(OBJDIR)/zxn/zxn_tile_map.o \
	$(OBJDIR)/zxn/zxn_uart.o \
	$(OBJDIR)/zxn/zxn_util.o \
	$(OBJDIR)/tile_defs_data.o

BIN_TARGET := $(BINDIR)/$(TARGET)

export PATH := $(Z88DK_ROOT)/bin:$(PATH)
export ZCCCFG

.PHONY: all build check clean

all: build

build: $(BIN_TARGET).nex

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn -clib=sdcc_iy $< -o $@

$(OBJDIR)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(ZCC) -c +zxn $< -o $@

$(BIN_TARGET).nex: $(OBJS)
	@mkdir -p $(BINDIR)
	$(ZCC) +zxn -subtype=nex -clib=sdcc_iy $(OBJS) -o $(BIN_TARGET) -create-app

check:
	./scripts/check-toolchain.sh

clean:
	rm -rf $(OBJDIR) $(BINDIR)
