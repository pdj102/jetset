#ifndef NGE_UART_STREAM_H
#define NGE_UART_STREAM_H

/**
 * @file nge_uart_stream.h
 * @brief Output streams on the UART. Bytes are sent unchanged.
 */

#include "io/nge_stream.h"

/**
 * @brief Open an output stream on the UART.
 * @return The stream, or NULL if no stream slot is free.
 */
NGE_FILE *nge_open_uart(void);

#endif
