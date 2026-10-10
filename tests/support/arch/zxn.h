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

#endif
