/* nge_print.c: strings, raw writes and number formatting. */
#include "test.h"

#include "io/nge_print.h"

static NGE_FILE *cap;

#define OUT(call, expected) \
    do { test_capture_reset(); call; CHECK_STR(test_capture_text(), expected); } while (0)

static void test_numbers(void)
{
    OUT(nge_putu16(0, cap), "0");
    OUT(nge_putu16(9, cap), "9");
    OUT(nge_putu16(10, cap), "10");
    OUT(nge_putu16(100, cap), "100");
    OUT(nge_putu16(10000, cap), "10000");
    OUT(nge_putu16(65535, cap), "65535");

    OUT(nge_puti16(-32768, cap), "-32768");
    OUT(nge_puti16(-1, cap), "-1");
    OUT(nge_puti16(32767, cap), "32767");

    OUT(nge_putu16w(42, 5, '0', cap), "00042");
    OUT(nge_putu16w(42, 5, ' ', cap), "   42");
    OUT(nge_putu16w(57920, 3, '0', cap), "57920");   /* wider than width: not truncated */
    OUT(nge_putu16w(0, 3, '0', cap), "000");

    OUT(nge_puthex8(0x0A, cap), "0A");
    OUT(nge_puthex16(0xBEEF, cap), "BEEF");
    OUT(nge_puthex16(0x0000, cap), "0000");
}

static void test_strings(void)
{
    int r;

    test_capture_reset();
    r = nge_puts("hello", cap);
    CHECK(r == 5);
    CHECK_STR(test_capture_text(), "hello");

    /* nge_write is binary-safe: the zero byte is sent */
    test_capture_reset();
    r = nge_write("a\0b", 3, cap);
    CHECK(r == 3);
    CHECK(memcmp(test_capture_text(), "a\0b", 3) == 0);

    /* Errors on a closed stream */
    CHECK(nge_puts("x", NULL) == NGE_EOF);
    CHECK(nge_write("x", 1, NULL) == NGE_EOF);
}

int main(void)
{
    test_init();
    cap = test_capture_open();
    test_numbers();
    test_strings();
    return test_summary("print");
}
