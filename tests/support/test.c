#include "test.h"

#include <stdio.h>
#include <stdlib.h>

#include "zxn_stream_dev.h"
#include "zxn_util.h"

/* ---- Checks ---- */

static int checks;
static int failures;

void test_check(int ok, const char *expr, const char *file, int line)
{
    checks++;
    if (!ok) {
        failures++;
        printf("  FAIL %s:%d: %s\n", file, line, expr);
    }
}

void test_check_str(const char *got, const char *exp, const char *expr, const char *file, int line)
{
    checks++;
    if (strcmp(got, exp) != 0) {
        failures++;
        printf("  FAIL %s:%d: %s\n       got      \"%s\"\n       expected \"%s\"\n",
               file, line, expr, got, exp);
    }
}

int test_summary(const char *name)
{
    printf("%-4s %-14s %d checks, %d failed\n", failures ? "FAIL" : "ok", name, checks, failures);
    return failures ? 1 : 0;
}

/* ---- Halts ---- */

jmp_buf test_halt_jmp;
int test_halt_armed;
int test_halt_count;

void test_halt(void)
{
    test_halt_count++;
    if (test_halt_armed) {
        longjmp(test_halt_jmp, 1);
    }
    /* An assert fired that no test expected; its message is on stdout */
    failures++;
    printf("  FAIL unexpected halt (zxn_util_assert or zxn_util_abort)\n");
    exit(test_summary("(aborted)") | 1);
}

/* ---- Capture stream ---- */

static char capture[4096];
static size_t capture_len;

static int capture_put(uint8_t c, ZXN_FILE *stream)
{
    (void)stream;
    if (capture_len < sizeof(capture) - 1) {
        capture[capture_len++] = (char)c;
        capture[capture_len] = 0;
    }
    return c;
}

ZXN_FILE *test_capture_open(void)
{
    return zxn_stream_alloc(capture_put, NULL);
}

const char *test_capture_text(void)
{
    return capture;
}

void test_capture_reset(void)
{
    capture_len = 0;
    capture[0] = 0;
}

/* ---- Debug output to stdout, so unexpected asserts are visible ---- */

static int stdout_put(uint8_t c, ZXN_FILE *stream)
{
    (void)stream;
    /* Messages from asserts a test triggers on purpose are not shown */
    if (!test_halt_armed) {
        putchar(c);
    }
    return c;
}

/* ---- Next registers ---- */

uint8_t test_regs[256];
uint8_t test_reg_log_reg[TEST_REG_LOG_MAX];
uint8_t test_reg_log_val[TEST_REG_LOG_MAX];
int test_reg_log_count;
uint8_t (*test_reg_read_hook)(uint8_t reg);

void test_reg_write(uint8_t reg, uint8_t val)
{
    test_regs[reg] = val;
    if (test_reg_log_count < TEST_REG_LOG_MAX) {
        test_reg_log_reg[test_reg_log_count] = reg;
        test_reg_log_val[test_reg_log_count] = val;
        test_reg_log_count++;
    }
}

uint8_t test_reg_read(uint8_t reg)
{
    return test_reg_read_hook ? test_reg_read_hook(reg) : test_regs[reg];
}

void test_reg_reset(void)
{
    memset(test_regs, 0, sizeof(test_regs));
    test_reg_log_count = 0;
    test_reg_read_hook = NULL;
}

/* ---- Tilemap memory ---- */

unsigned char test_tilemap_mem[ZXN_TILEMAP_WIDTH * ZXN_TILEMAP_HEIGHT * sizeof(zxn_tile_t)];

void test_tilemap_row(uint8_t y, uint8_t x1, uint8_t x2, char *out)
{
    uint8_t x;

    for (x = x1; x <= x2; x++) {
        *out++ = (char)test_tilemap[y * ZXN_TILEMAP_WIDTH + x].tile_id;
    }
    *out = 0;
}

/* ---- CSpect DebugOut ---- */

char test_cspect_lines[TEST_CSPECT_MAX_LINES][128];
int test_cspect_count;
void (*test_cspect_hook)(void);

void zxn_cspect_print(const char *s)
{
    if (test_cspect_count < TEST_CSPECT_MAX_LINES) {
        strncpy(test_cspect_lines[test_cspect_count], s, sizeof(test_cspect_lines[0]) - 1);
        test_cspect_count++;
    }
    if (test_cspect_hook) {
        test_cspect_hook();
    }
}

/* ---- Setup ---- */

void test_init(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    test_reg_reset();
    test_capture_reset();
    memset(test_tilemap_mem, 0, sizeof(test_tilemap_mem));
    test_cspect_count = 0;
    test_cspect_hook = NULL;

    /* Unexpected assert messages appear in the test output */
    zxn_util_set_debug_stream(zxn_stream_alloc(stdout_put, NULL));
}
