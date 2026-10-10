#ifndef ZXN_UART_STREAM_H
#define ZXN_UART_STREAM_H

/**
 * @file zxn_uart_stream.h
 * @brief Output streams on the UART. Bytes are sent unchanged.
 */

#include "zxn_stream.h"

/**
 * @brief Open an output stream on the UART.
 * @return The stream, or NULL if no stream slot is free.
 */
ZXN_FILE *zxn_open_uart(void);

#endif
