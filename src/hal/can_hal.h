#ifndef CANMOD_CAN_HAL_H
#define CANMOD_CAN_HAL_H

#include <stdint.h>

/**
 * Initialise the CAN peripheral at 125 kbps (MS-CAN).
 *
 * @param acceptance_ids  Array of 11-bit CAN IDs to accept via hardware
 *                        filters.  Pass NULL / count=0 to accept all frames.
 * @param count           Number of IDs in acceptance_ids (max 14 on bxCAN).
 *
 * On the STM32F103 target this configures bxCAN1 (remapped to PB8/PB9) and
 * enables the Rx0 interrupt.  On the host target this opens a PF_CAN socket
 * on the interface specified at startup (default: vcan0).
 */
void can_hal_init(const uint32_t *acceptance_ids, uint8_t count);

/**
 * Non-blocking CAN frame receive.
 *
 * @param id   Receives the 11-bit CAN identifier.
 * @param data Receives the payload (caller must supply ≥8 bytes).
 * @param dlc  Receives the data length code (0–8).
 * @return     1 if a frame was read, 0 if no frame available.
 *
 * On STM32: drains one frame from the bxCAN Rx FIFO.
 * On host:  reads from the SocketCAN socket (O_NONBLOCK).
 */
int can_hal_rx(uint32_t *id, uint8_t *data, uint8_t *dlc);

#endif /* CANMOD_CAN_HAL_H */
