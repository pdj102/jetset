#include "io/nge_uart_stream.h"

#include <stddef.h>

#include "io/nge_stream_dev.h"
#include "devices/nge_uart.h"

/* UART device output function. */
static int uart_put(uint8_t c, NGE_FILE *stream)
{
    (void)stream;
    nge_uart_write_byte(c);
    return c;
}

NGE_FILE *nge_open_uart(void)
{
    return nge_stream_alloc(uart_put, NULL, NULL);
}
