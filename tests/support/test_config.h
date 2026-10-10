/*
 * Included before every library and test source in the PC test build
 * (gcc -include). Replaces z88dk-only keywords and wires library hooks to the
 * test support code.
 */
#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

/* SDCC / z88dk keywords with no gcc meaning */
#define __z88dk_fastcall
#define __z88dk_callee
#define __naked

/* Tilemap memory is a byte array in test.c instead of 0x4000 */
extern unsigned char test_tilemap_mem[];
#define ZXN_TILEMAP_MEM  ((zxn_tile_t *) test_tilemap_mem)

/* A fatal zxn_util error returns to the test instead of halting */
void test_halt(void);
#define ZXN_UTIL_HALT()  test_halt()

#endif
