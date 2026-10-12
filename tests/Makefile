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
#      every library source in src/nge/*/, so new modules and folders are
#      included too. Next register and IO port names come from z88dk's own
#      headers.
#   Library files that only work on the Next (inline asm traps) are listed in
#   LIB_EXCLUDE and replaced by fakes in support/test.c.

Z88DK_ROOT ?= /home/paul/src/z88dk
HOSTCC ?= gcc

SRCDIR := ../src/nge
BUILD := build
GEN := $(BUILD)/gen

Z88DK_ARCH_H := $(Z88DK_ROOT)/include/_DEVELOPMENT/common/arch/zxn.h
Z88DK_CONFIG_DIR := $(Z88DK_ROOT)/libsrc/newlib/target/zxn

# Library code that cannot run on the PC
LIB_EXCLUDE := devices/nge_cspect.c

LIB_SRCS := $(filter-out $(addprefix $(SRCDIR)/,$(LIB_EXCLUDE)),$(wildcard $(SRCDIR)/*/*.c))
LIB_OBJS := $(patsubst $(SRCDIR)/%.c,$(BUILD)/lib/%.o,$(LIB_SRCS))
SUPPORT_OBJS := $(BUILD)/support/test.o $(BUILD)/support/zxn_io.o

TEST_SRCS := $(wildcard test_*.c)
TESTS := $(patsubst %.c,$(BUILD)/%,$(TEST_SRCS))

# NGE_CSPECT: build the DebugOut stream (its trap is faked).
# NGE_FILE_MAX: room for the stdout debug stream that test_init() opens.
CPPFLAGS := -DNGE_CSPECT -DNGE_FILE_MAX=8 \
            -include support/test_config.h \
            -Isupport -I$(GEN) -I$(SRCDIR) -I$(Z88DK_CONFIG_DIR)
CFLAGS := -std=gnu11 -O1 -g -Wall -MMD -MP

.PHONY: all run clean

# Keep objects between runs (they are intermediates of the link rule)
.SECONDARY:

all: run

# Build everything, run every test, and fail if any test fails
run: $(TESTS)
	@failed=0; \
	for t in $(TESTS); do ./$$t || failed=1; done; \
	if [ $$failed -ne 0 ]; then echo "TESTS FAILED"; exit 1; fi; \
	echo "all tests passed"

$(BUILD)/test_%: $(BUILD)/test_%.o $(LIB_OBJS) $(SUPPORT_OBJS)
	$(HOSTCC) $(CFLAGS) $^ -o $@

$(BUILD)/test_%.o: test_%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/lib/%.o: $(SRCDIR)/%.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
	@mkdir -p $(dir $@)
	$(HOSTCC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/support/test.o: support/test.c $(GEN)/zxn_names.h $(GEN)/zxn_io.h
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

-include $(wildcard $(BUILD)/*.d $(BUILD)/lib/*/*.d $(BUILD)/support/*.d)
