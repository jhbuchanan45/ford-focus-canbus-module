#ifndef CANMOD_USB_HAL_H
#define CANMOD_USB_HAL_H

#include <stdint.h>

/**
 * Initialise the USB CDC interface.
 *
 * STM32: configures the USB peripheral (PA11/PA12) as a CDC-ACM device and
 *        starts the USB stack.  Must be called after gpio_hal_init().
 * Host:  no-op — usb_hal_write() maps to fwrite(stdout).
 */
void usb_hal_init(void);

/**
 * Write bytes to the USB CDC output (NDJSON log or debug text).
 *
 * @param buf  Data to send.
 * @param len  Number of bytes.
 *
 * On STM32 this is non-blocking; bytes are dropped if the host is not
 * reading.  On the host target this calls fwrite + fflush(stdout).
 */
void usb_hal_write(const uint8_t *buf, uint16_t len);

/**
 * Service the USB CDC stack.  Call once per main-loop iteration on STM32.
 * On the host target this is a no-op.
 */
void usb_hal_poll(void);

#endif /* CANMOD_USB_HAL_H */
