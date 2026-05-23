#ifndef CANMOD_UART_HAL_H
#define CANMOD_UART_HAL_H

#include <stdint.h>

/**
 * Initialise the head-unit UART at 38400 baud, 8N1.
 *
 * STM32: USART2 on PA2 (TX) / PA3 (RX).
 * Host:  opens a PTY pair; the slave device name is printed to stderr at
 *        startup so the caller can attach with `cat` or a serial monitor.
 */
void uart_hal_init(void);

/**
 * Transmit one byte to the head-unit UART (blocking).
 * On the host target this writes to the master side of the PTY.
 */
void uart_hal_tx(uint8_t byte);

/**
 * Non-blocking receive from the head-unit UART.
 *
 * @param byte  Receives the byte if one is available.
 * @return      1 if a byte was received, 0 otherwise.
 */
int uart_hal_rx(uint8_t *byte);

#endif /* CANMOD_UART_HAL_H */
