#include "zxn_uart_stream.h"

#include "zxn_stream_dev.h"
#include "zxn_uart.h"

/* UART device output function. */
static int uart_put(uint8_t c, ZXN_FILE *stream)
{
    (void)stream;
    zxn_uart_write_byte(c);
    return c;
}

ZXN_FILE *zxn_open_uart(void)
{
    return zxn_stream_alloc(uart_put);
}
