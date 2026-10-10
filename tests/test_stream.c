/* Stream core (zxn_stream.c) and the UART stream device. */
#include "test.h"

#include "zxn_stream_dev.h"
#include "zxn_uart_stream.h"

static int flush_calls;

static int dummy_put(uint8_t c, ZXN_FILE *stream)
{
    (void)stream;
    return c;
}

static void count_flush(ZXN_FILE *stream)
{
    (void)stream;
    flush_calls++;
}

static void test_pool(void)
{
    ZXN_FILE *s[ZXN_FILE_MAX + 1];
    int n = 0;
    int i;

    /* Open until the pool is full; test_init() already used one slot */
    while (n <= ZXN_FILE_MAX && (s[n] = zxn_stream_alloc(dummy_put, NULL)) != NULL) {
        n++;
    }
    CHECK(n == ZXN_FILE_MAX - 1);

    /* Closing a slot lets the next open reuse it */
    zxn_close(s[2]);
    CHECK(zxn_stream_alloc(dummy_put, NULL) == s[2]);
    CHECK(zxn_stream_alloc(dummy_put, NULL) == NULL);

    for (i = 0; i < n; i++) {
        zxn_close(s[i]);
    }
}

static void test_putc_and_close(void)
{
    ZXN_FILE *s = zxn_stream_alloc(dummy_put, NULL);

    CHECK(zxn_putc('A', s) == 'A');
    zxn_close(s);
    CHECK(zxn_putc('A', s) == ZXN_EOF);
    CHECK(zxn_putc('A', NULL) == ZXN_EOF);

    /* NULL is accepted, so streams that failed to open need no check */
    zxn_close(NULL);
    zxn_flush(NULL);
}

static void test_flush_hook(void)
{
    ZXN_FILE *s = zxn_stream_alloc(dummy_put, count_flush);

    flush_calls = 0;
    zxn_flush(s);
    CHECK(flush_calls == 1);
    zxn_close(s);
    CHECK(flush_calls == 2);
    zxn_flush(s);               /* closed: no further flush */
    CHECK(flush_calls == 2);

    /* Devices without a flush hook */
    s = zxn_stream_alloc(dummy_put, NULL);
    zxn_flush(s);
    zxn_close(s);
}

static void test_uart_stream(void)
{
    ZXN_FILE *u = zxn_open_uart();

    CHECK(u != NULL);
    CHECK(zxn_putc('x', u) == 'x');
    zxn_close(u);
}

int main(void)
{
    test_init();
    test_pool();
    test_putc_and_close();
    test_flush_hook();
    test_uart_stream();
    return test_summary("stream");
}
