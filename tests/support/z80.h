/*
 * PC tests: replaces z88dk's <z80.h>. z80_inp() reads go to the port read
 * hook in test.c (test_port_read_hook), so tests can set the keyboard rows.
 */
#ifndef TEST_Z80_H
#define TEST_Z80_H

#include <stdint.h>

uint8_t test_port_read(uint16_t port);

#define z80_inp(port)  test_port_read(port)

#endif
