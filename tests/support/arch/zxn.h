/*
 * PC stand-in for z88dk's <arch/zxn.h>, used only by the tests.
 *
 * Register numbers and bit values come from z88dk itself: config_zxn.h holds
 * the __REG_* / __R* constants, and the build generates zxn_names.h (REG_* and
 * other public aliases) and zxn_io.h (IO_* port variables) from z88dk's real
 * arch/zxn.h. New library code that uses more Next registers or ports
 * therefore needs no changes here.
 *
 * Next register reads and writes go to the register file in test.c, which
 * logs writes and lets a test supply the values that reads return.
 *
 * Other IO ports are plain variables, which keep only the last value written.
 * Ports whose order of writes matters (data streamed to an auto-incrementing
 * port) are redefined below to go to the port log in test.c instead: add one
 * line per port. Reads of a logged port call test_port_read().
 */
#ifndef TEST_ARCH_ZXN_H
#define TEST_ARCH_ZXN_H

#include <stddef.h>
#include <stdint.h>

#include <config_zxn.h>
#include "zxn_names.h"
#include "zxn_io.h"

void test_reg_write(uint8_t reg, uint8_t val);
uint8_t test_reg_read(uint8_t reg);

#define ZXN_WRITE_REG(reg, val)  test_reg_write((uint8_t)(reg), (uint8_t)(val))
#define ZXN_NEXTREG(reg, val)    test_reg_write((uint8_t)(reg), (uint8_t)(val))
#define ZXN_READ_REG(reg)        test_reg_read((uint8_t)(reg))

uint8_t *test_port_write(uint16_t port);
uint8_t test_port_read(uint16_t port);

/* Logged ports: each use as an lvalue appends one entry to the port log */
#define IO_SPRITE_SLOT       (*test_port_write(__IO_SPRITE_SLOT))
#define IO_SPRITE_ATTRIBUTE  (*test_port_write(__IO_SPRITE_ATTRIBUTE))
#define IO_SPRITE_PATTERN    (*test_port_write(__IO_SPRITE_PATTERN))
#define IO_SPRITE_FLAGS      test_port_read(__IO_SPRITE_FLAGS)

#endif
