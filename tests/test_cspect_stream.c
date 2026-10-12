/* nge_cspect_stream.c: CSpect DebugOut stream (the RST $18 trap is faked). */
#include "test.h"

#include "output/nge_cspect_stream.h"
#include "output/nge_printf.h"
#include "output/nge_print.h"

static NGE_FILE *d;

static void reset(void)
{
    test_cspect_count = 0;
}

static void test_lines(void)
{
    char x80[81], x79[80], expect[81];

    reset();
    nge_puts("ab\ncd\n", d);
    CHECK(test_cspect_count == 2);
    CHECK_STR(test_cspect_lines[0], "ab");
    CHECK_STR(test_cspect_lines[1], "cd");

    /* A long line is sent in NGE_CSPECT_LINE_MAX pieces */
    reset();
    memset(x80, 'x', 80);
    x80[80] = 0;
    nge_puts(x80, d);
    nge_puts("yyyyy\n", d);
    CHECK(test_cspect_count == 2);
    CHECK_STR(test_cspect_lines[0], x80);
    CHECK_STR(test_cspect_lines[1], "yyyyy");

    /* '%' is replaced, including in the last column */
    reset();
    nge_printf(d, "100%% ok\n");
    CHECK_STR(test_cspect_lines[0], "100# ok");
    reset();
    memset(x79, 'x', 79);
    x79[79] = 0;
    nge_puts(x79, d);
    nge_puts("%z\n", d);
    memcpy(expect, x79, 79);
    expect[79] = NGE_CSPECT_PERCENT;
    expect[80] = 0;
    CHECK(test_cspect_count == 2);
    CHECK_STR(test_cspect_lines[0], expect);
    CHECK_STR(test_cspect_lines[1], "z");

    /* Control codes and zero bytes are dropped */
    reset();
    nge_write("a\0\x10\x01\rb\n", 7, d);
    CHECK(test_cspect_count == 1);
    CHECK_STR(test_cspect_lines[0], "ab");
}

static void test_flush_and_close(void)
{
    reset();
    nge_puts("part", d);
    CHECK(test_cspect_count == 0);
    nge_flush(d);
    CHECK(test_cspect_count == 1);
    CHECK_STR(test_cspect_lines[0], "part");

    nge_flush(d);                       /* nothing buffered: nothing sent */
    CHECK(test_cspect_count == 1);

    nge_puts("tail", d);
    nge_close(d);
    CHECK(test_cspect_count == 2);
    CHECK_STR(test_cspect_lines[1], "tail");
    CHECK(nge_putc('q', d) == NGE_EOF);
}

int main(void)
{
    test_init();
    d = nge_open_cspect_debug();
    CHECK(d != NULL);
    test_lines();
    test_flush_and_close();
    return test_summary("cspect_stream");
}
