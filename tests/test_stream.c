/* Stream core (nge_stream.c) and the UART stream device. */
#include "test.h"

#include "io/nge_stream_dev.h"
#include "io/nge_uart_stream.h"

static int flush_calls;

static int dummy_put(uint8_t c, NGE_FILE *stream)
{
    (void)stream;
    return c;
}

static void count_flush(NGE_FILE *stream)
{
    (void)stream;
    flush_calls++;
}

static void test_pool(void)
{
    NGE_FILE *s[NGE_FILE_MAX + 1];
    int n = 0;
    int i;

    /* Open until the pool is full; test_init() already used one slot */
    while (n <= NGE_FILE_MAX && (s[n] = nge_stream_alloc(dummy_put, NULL)) != NULL) {
        n++;
    }
    CHECK(n == NGE_FILE_MAX - 1);

    /* Closing a slot lets the next open reuse it */
    nge_close(s[2]);
    CHECK(nge_stream_alloc(dummy_put, NULL) == s[2]);
    CHECK(nge_stream_alloc(dummy_put, NULL) == NULL);

    for (i = 0; i < n; i++) {
        nge_close(s[i]);
    }
}

static void test_putc_and_close(void)
{
    NGE_FILE *s = nge_stream_alloc(dummy_put, NULL);

    CHECK(nge_putc('A', s) == 'A');
    nge_close(s);
    CHECK(nge_putc('A', s) == NGE_EOF);
    CHECK(nge_putc('A', NULL) == NGE_EOF);

    /* NULL is accepted, so streams that failed to open need no check */
    nge_close(NULL);
    nge_flush(NULL);
}

static void test_flush_hook(void)
{
    NGE_FILE *s = nge_stream_alloc(dummy_put, count_flush);

    flush_calls = 0;
    nge_flush(s);
    CHECK(flush_calls == 1);
    nge_close(s);
    CHECK(flush_calls == 2);
    nge_flush(s);               /* closed: no further flush */
    CHECK(flush_calls == 2);

    /* Devices without a flush hook */
    s = nge_stream_alloc(dummy_put, NULL);
    nge_flush(s);
    nge_close(s);
}

static void test_uart_stream(void)
{
    NGE_FILE *u = nge_open_uart();

    CHECK(u != NULL);
    CHECK(nge_putc('x', u) == 'x');
    nge_close(u);
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
