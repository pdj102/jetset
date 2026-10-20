/*
 * Minimal test framework for the nge library PC tests.
 *
 * Each tests/test_<name>.c is a separate program:
 *
 *     int main(void)
 *     {
 *         test_init();
 *         ... CHECK(...) ...
 *         return test_summary("name");
 *     }
 */
#ifndef TEST_H
#define TEST_H

#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "io/nge_stream.h"
#include "tilemap/nge_tilemap.h"

/* ---- Checks ---- */

/** Record a check; prints the expression and location when it fails. */
#define CHECK(cond)  test_check((cond) != 0, #cond, __FILE__, __LINE__)

/** Check two NUL-terminated strings are equal; prints both when they differ. */
#define CHECK_STR(got, exp)  test_check_str((got), (exp), #got, __FILE__, __LINE__)

void test_check(int ok, const char *expr, const char *file, int line);
void test_check_str(const char *got, const char *exp, const char *expr, const char *file, int line);

/** Reset all fakes; sends nge_util debug output to stdout. Call first in main. */
void test_init(void);

/** Print a summary; returns the process exit status (0 if every check passed). */
int test_summary(const char *name);

/* ---- Halts from nge_util_assert / nge_util_abort ---- */

extern jmp_buf test_halt_jmp;
extern int test_halt_armed;
extern int test_halt_count;

/** Run stmt, expecting it to halt; afterwards test_halt_count says how often it did. */
#define TEST_EXPECT_HALT(stmt) \
    do { \
        test_halt_count = 0; \
        test_halt_armed = 1; \
        if (!setjmp(test_halt_jmp)) { \
            stmt; \
        } \
        test_halt_armed = 0; \
    } while (0)

/* ---- Capture stream: a stream device that appends to a string ---- */

/** Open a capture stream; its output is read with test_capture_text(). */
NGE_FILE *test_capture_open(void);
const char *test_capture_text(void);
void test_capture_reset(void);

/* ---- Next registers ---- */

#define TEST_REG_LOG_MAX 4096

extern uint8_t test_regs[256];          /**< Last value written to each register. */
extern uint8_t test_reg_log_reg[TEST_REG_LOG_MAX];
extern uint8_t test_reg_log_val[TEST_REG_LOG_MAX];
extern int test_reg_log_count;          /**< Number of logged writes, in order. */

/** If set, register reads call this instead of returning test_regs[reg]. */
extern uint8_t (*test_reg_read_hook)(uint8_t reg);

void test_reg_reset(void);

/* ---- IO ports logged by support/arch/zxn.h ---- */

#define TEST_PORT_LOG_MAX 20000

extern uint16_t test_port_log_port[TEST_PORT_LOG_MAX];
extern uint8_t test_port_log_val[TEST_PORT_LOG_MAX];
extern int test_port_log_count;         /**< Number of logged writes, in order. */

/** If set, logged port reads call this; otherwise they return 0. */
extern uint8_t (*test_port_read_hook)(uint16_t port);

void test_port_reset(void);

/* ---- Tilemap memory (replaces 0x4000) ---- */

/** The tilemap as tiles: test_tilemap[y * NGE_TILEMAP_WIDTH + x]. */
#define test_tilemap  ((nge_tile_t *) test_tilemap_mem)

/** Copy tile ids of row y, columns x1..x2, into out as a string. */
void test_tilemap_row(uint8_t y, uint8_t x1, uint8_t x2, char *out);

/* ---- Layer 2: the fake drawing (support/test_layer2.c) records a cell at a time ---- */

/** A Layer 2 cell as the fake draws it: a fill is a space in its colour (ink and paper both). */
typedef struct test_l2_cell {
    uint8_t c;       /**< The character drawn, ' ' after a fill. */
    uint8_t ink;     /**< Its colour. */
    uint8_t paper;   /**< Its background. */
} test_l2_cell_t;

extern test_l2_cell_t test_layer2[32][40];   /**< By row, then column (NGE_LAYER2_ROWS x NGE_LAYER2_COLS). */
extern uint8_t test_layer2_shown;           /**< The last nge_layer2_show(). */

/** Copy the characters of Layer 2 row y, columns x1..x2, into out as a string. */
void test_layer2_row(uint8_t y, uint8_t x1, uint8_t x2, char *out);

/* ---- CSpect DebugOut: the fake nge_cspect_trap() records each call ---- */

#define TEST_CSPECT_MAX_LINES 16

extern char test_cspect_lines[TEST_CSPECT_MAX_LINES][128];
extern uint8_t test_cspect_slot0[TEST_CSPECT_MAX_LINES];   /**< Page in slot 0 at each trap. */
extern int test_cspect_count;

/** If set, called from the fake trap after recording (e.g. to inject faults). */
extern void (*test_cspect_hook)(void);

#endif
