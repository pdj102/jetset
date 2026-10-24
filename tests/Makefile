# PC tests for the nge library (src/nge), built with the host C compiler.
#
#   make test                  from the project root: build and run every test
#   make -C tests              the same, from anywhere
#   make -C tests build/test_print && tests/build/test_print   one test
#
# Adding tests for a new module (for example sprites):
#   1. Create tests/test_<name>.c with a main() that calls test_init(),
#      makes CHECK()s and returns test_summary("<name>") (see support/test.h).
#   2. That's all: every tests/test_*.c is found automatically and linked with
#      every library source in src/nge/*/ and src/nge/*/*/, so new modules
#      and folders are included too. Next register and IO port names come from z88dk's own
#      headers.
#   Game code (src/game) is linked only into the tests listed with it below
#   (GEN_GAME_SRCS, with the components and effects, for test_gen, LEVEL_GAME_SRCS and those for test_level,
#   ACT_GAME_SRCS and the level and gen ones for test_act, the gen ones for
#   test_inventory, UI_GAME_SRCS and the action ones for test_ui and test_target).
#   Library files that only work on the Next (inline asm traps, Layer 2
#   drawing by address) are listed in LIB_EXCLUDE and replaced by fakes in
#   support/ (test.c, test_layer2.c); the map's palette, assembly in
#   src/data/, by support/test_tile_data.c.

Z88DK_ROOT ?= /home/paul/src/z88dk
HOSTCC ?= gcc

SRCDIR := ../src/nge
BUILD := build
GEN := $(BUILD)/gen

Z88DK_ARCH_H := $(Z88DK_ROOT)/include/_DEVELOPMENT/common/arch/zxn.h
Z88DK_CONFIG_DIR := $(Z88DK_ROOT)/libsrc/newlib/target/zxn

# Library code that cannot run on the PC
LIB_EXCLUDE := devices/nge_cspect_trap.c layer2/nge_layer2_draw.c

LIB_SRCS := $(filter-out $(addprefix $(SRCDIR)/,$(LIB_EXCLUDE)),$(wildcard $(SRCDIR)/*/*.c $(SRCDIR)/*/*/*.c))
LIB_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILD)/lib/%.o,$(LIB_SRCS))
SUPPORT_OBJS := $(BUILD)/support/test.o $(BUILD)/support/test_layer2.o $(BUILD)/support/test_tile_data.o \
                $(BUILD)/support/zxn_io.o

# Game code tested on the PC, linked only into the tests that need it
GAMEDIR := ../src/game
GEN_GAME_SRCS := $(addprefix $(GAMEDIR)/,gen/gen.c gen/gen_shared_bank.c \
                 gen/gen_creature_bank.c gen/gen_creature_pdata.c \
                 gen/gen_item_bank.c gen/gen_item_pdata.c components/kind.c \
                 components/kind_cdata.c components/stats.c components/stats_cdata.c \
                 components/item.c components/item_cdata.c components/inventory.c \
                 components/inventory_cdata.c components/health.c components/health_cdata.c \
                 components/attack.c components/attack_cdata.c components/attrs_cdata.c \
                 components/feature.c components/feature_cdata.c \
                 gen/gen_feature_bank.c gen/gen_feature_pdata.c \
                 effects/effects_core.c effects/effects_game.c effects/effects_pdata.c)
GEN_GAME_OBJS := $(patsubst $(GAMEDIR)/%.c,$(BUILD)/game/%.o,$(GEN_GAME_SRCS))
LEVEL_GAME_SRCS := $(addprefix $(GAMEDIR)/,level/level.c level/level_bank.c level/level_gen.c level/level_deco.c \
                   level/level_travel.c level/level_pdata.c)
# The decorator's tables, generated into src/data (docs/dungeon_decoration.md)
DATADIR := ../src/data
LEVEL_ONLY_OBJS := $(patsubst $(GAMEDIR)/%.c,$(BUILD)/game/%.o,$(LEVEL_GAME_SRCS)) \
                   $(BUILD)/data/tile_rules_data.o
LEVEL_GAME_OBJS := $(LEVEL_ONLY_OBJS) $(GEN_GAME_OBJS)
# The stairs actions send entities between levels: the actions need the level module
ACT_GAME_SRCS := $(wildcard $(GAMEDIR)/actions/*.c)
ACT_GAME_OBJS := $(patsubst $(GAMEDIR)/%.c,$(BUILD)/game/%.o,$(ACT_GAME_SRCS)) $(LEVEL_GAME_OBJS)
UI_GAME_SRCS := $(addprefix $(GAMEDIR)/,ui/ui_bank.c player/player.c player/player_bank.c screen.c \
                messages/messages_bank.c target/target_bank.c)
UI_GAME_OBJS := $(patsubst $(GAMEDIR)/%.c,$(BUILD)/game/%.o,$(UI_GAME_SRCS)) $(ACT_GAME_OBJS)

ANIM_GAME_SRCS := $(addprefix $(GAMEDIR)/,anim/anim.c anim/anim_bank.c anim/anim_pdata.c)
ANIM_GAME_OBJS := $(patsubst $(GAMEDIR)/%.c,$(BUILD)/game/%.o,$(ANIM_GAME_SRCS)) $(LEVEL_GAME_OBJS)

TEST_SRCS := $(wildcard test_*.c)
TESTS := $(patsubst %.c,$(BUILD)/%,$(TEST_SRCS))

# NGE_CSPECT: build the DebugOut stream (its trap is faked).
# NGE_FILE_MAX: room for the stdout debug stream that test_init() opens.
CPPFLAGS := -DNGE_CSPECT -DNGE_FILE_MAX=8 \
            -include support/test_config.h \
            -Isupport -I$(GEN) -I$(SRCDIR) -I$(SRCDIR)/.. -I$(Z88DK_CONFIG_DIR)
CFLAGS := -std=gnu11 -O1 -g -Wall -MMD -MP

.PHONY: all run clean

# Keep objects between runs (they are intermediates of the link rule)
.SECONDARY:

all: run

# The effects core knows nothing of the game's components: it reaches them
# through its seam (src/game/effects/effects_core.h)
EFFECTS_CORE := $(GAMEDIR)/effects/effects_core.c $(GAMEDIR)/effects/effects_core.h
EFFECTS_CORE_BANNED := ^\#include "game/(components/(health|item|stats|inventory|attack|kind)|gen/|timers|actions/|actors|player/)

.PHONY: check-effects-core
check-effects-core:
	@if grep -nE '$(EFFECTS_CORE_BANNED)' $(EFFECTS_CORE); then \
	  echo "effects core includes game content: use the seam (effects_core.h)"; exit 1; fi

# Build everything, run every test, and fail if any test fails. The
# scripts' tests (tests/scripts/test_*.py, Python's unittest) run too
run: check-effects-core $(TESTS)
	@failed=0; \
	for t in $(TESTS); do ./$$t || failed=1; done; \
	python3 -I -m unittest discover -s scripts || failed=1; \
	if [ $$failed -ne 0 ]; then echo "TESTS FAILED"; exit 1; fi; \
	echo "all tests passed"

$(BUILD)/test_%: $(BUILD)/test_%.o $(LIB_OBJS) $(SUPPORT_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_level: $(BUILD)/test_level.o $(LIB_OBJS) $(SUPPORT_OBJS) $(LEVEL_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_deco: $(BUILD)/test_deco.o $(LIB_OBJS) $(SUPPORT_OBJS) $(LEVEL_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_gen: $(BUILD)/test_gen.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_act: $(BUILD)/test_act.o $(LIB_OBJS) $(SUPPORT_OBJS) $(ACT_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_inventory: $(BUILD)/test_inventory.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_ui: $(BUILD)/test_ui.o $(LIB_OBJS) $(SUPPORT_OBJS) $(UI_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_target: $(BUILD)/test_target.o $(LIB_OBJS) $(SUPPORT_OBJS) $(UI_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_anim: $(BUILD)/test_anim.o $(LIB_OBJS) $(SUPPORT_OBJS) $(ANIM_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_health: $(BUILD)/test_health.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_attack: $(BUILD)/test_attack.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_stats: $(BUILD)/test_stats.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_feature: $(BUILD)/test_feature.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_effects: $(BUILD)/test_effects.o $(LIB_OBJS) $(SUPPORT_OBJS) $(GEN_GAME_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/data/%.o: $(DATADIR)/%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/game/%.o: $(GAMEDIR)/%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/test_%.o: test_%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/lib/%.o: $(SRCDIR)/%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/support/%.o: support/%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/support/zxn_io.o: $(GEN)/zxn_io.c
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CFLAGS) -c $< -o $@

# REG_*, RPC_*, RSLS_* ... names: z88dk's arch/zxn.h defines each as an
# alias of a __ constant from config_zxn.h
$(GEN)/zxn_names.h: $(Z88DK_ARCH_H)
	@mkdir -p $(dir $@)
	{ echo '/* Generated from $< by tests/Makefile */'; \
	  grep -E '^#define [A-Za-z_][A-Za-z0-9_]*[[:space:]]+__[A-Za-z_][A-Za-z0-9_]*[[:space:]]*$$' $<; } > $@

# IO_* ports become plain variables, so library code can read and write them
$(GEN)/zxn_io.h: $(Z88DK_ARCH_H)
	@mkdir -p $(dir $@)
	{ echo '/* Generated from $< by tests/Makefile */'; \
	  echo '#include <stdint.h>'; \
	  grep -oE '\bIO_[A-Za-z0-9_]+[[:space:]]*;' $< | tr -d ' ;' | sort -u | sed 's/.*/extern uint8_t &;/'; } > $@

$(GEN)/zxn_io.c: $(Z88DK_ARCH_H)
	@mkdir -p $(dir $@)
	{ echo '/* Generated from $< by tests/Makefile */'; \
	  echo '#include <stdint.h>'; \
	  grep -oE '\bIO_[A-Za-z0-9_]+[[:space:]]*;' $< | tr -d ' ;' | sort -u | sed 's/.*/uint8_t &;/'; } > $@

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d $(BUILD)/lib/*/*.d $(BUILD)/lib/*/*/*.d $(BUILD)/support/*.d \
                    $(BUILD)/game/*/*.d $(BUILD)/data/*.d)
