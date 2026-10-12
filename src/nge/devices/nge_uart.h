#ifndef NGE_UART_H
#define NGE_UART_H

/**
 * @file nge_uart.h
 * @brief ZX Next UART driver.
 */

#include <stdint.h>

/**
 * @brief Write a byte to the UART.
 * @param c Byte to transmit.
 */
void nge_uart_write_byte(uint8_t c);

#endif
