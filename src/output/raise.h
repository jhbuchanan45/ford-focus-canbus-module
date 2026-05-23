#ifndef CANMOD_RAISE_H
#define CANMOD_RAISE_H

#include <stdint.h>

/**
 * Raise VW PQ UART output driver — Phase 2 (CANMOD_OUTPUT=raise)
 *
 * Packet format:  [0x2E][CMD][LEN][DATA...][CSUM]
 * CSUM = (CMD + LEN + sum(DATA)) ^ 0xFF
 *
 * All raise_send_*() functions read from car_get_*() and transmit via
 * uart_hal_tx().  They are called by the main loop at the intervals
 * specified below.
 */

/* ---- Periodic senders (call from main loop at indicated intervals) ----- */

/** Vehicle info (speed, RPM, temp, voltage, odometer) — every 500 ms */
void raise_send_vehicle_info(void);

/** Door status — every 500 ms or on change */
void raise_send_doors(void);

/** Warning flags — every 500 ms */
void raise_send_warnings(void);

/** Status flags (reverse, park brake, near lights) — every 500 ms */
void raise_send_status_flags(void);

/** AC status — every 500 ms */
void raise_send_ac(void);

/** Steering angle — every 100 ms */
void raise_send_steering(void);

/* ---- Event-driven senders --------------------------------------------- */

/** Parking active / inactive — call when pdc_active changes */
void raise_send_parking_active(void);

/** Rear radar distances — every 100 ms when PDC active */
void raise_send_rear_radar(void);

/** Front radar distances — every 100 ms when PDC active */
void raise_send_front_radar(void);

/** SWC button press or release — call immediately when event dequeued */
void raise_send_swc(uint8_t button_id, uint8_t pressed);

/* ---- Head-unit RX handler (0x2E packets from ATOTO → 0xFF ACK) -------- */

/** Call once per main loop; reads uart_hal_rx() and sends ACK as needed */
void raise_rx_poll(void);

#endif /* CANMOD_RAISE_H */
