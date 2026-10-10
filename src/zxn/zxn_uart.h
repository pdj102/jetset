#ifndef ZXN_UART_H
#define ZXN_UART_H

/**
 * @file zxn_uart.h
 * @brief ZX Next UART driver.
 */

#include <stdint.h>

/**
 * @brief Write a byte to the UART.
 * @param c Byte to transmit.
 */
void zxn_uart_write_byte(uint8_t c);

#endif
