/**
 * main.c — Application layer
 *
 * Responsibilities:
 *   1. Initialise all HAL modules and the output driver.
 *   2. Drive the main loop:
 *      a. Drain CAN Rx → car_process_frame() → output driver emit
 *      b. Call periodic output functions at correct intervals
 *      c. Poll USB CDC stack (STM32 only)
 *      d. Heartbeat LED at 1 Hz
 *
 * Timing (from firmware spec):
 *   Vehicle data + AC + doors + status flags  : 500 ms
 *   Steering angle                            : 100 ms
 *   Radar packets (when PDC active)           : 100 ms
 *   SWC events                               : immediately on dequeue
 *   Heartbeat LED toggle                      : 500 ms → 1 Hz blink
 */

#include "car.h"
#include "hal/can_hal.h"
#include "hal/uart_hal.h"
#include "hal/usb_hal.h"
#include "hal/gpio_hal.h"
#include "hal/timer_hal.h"

#ifdef CANMOD_OUTPUT_JSON
#include "output/json_log.h"
#endif

#ifdef CANMOD_OUTPUT_RAISE
#include "output/raise.h"
#endif

#ifdef CANMOD_TARGET_HOST
#include <stdio.h>
#include <string.h>

/* can_hal_set_interface() is declared here to avoid polluting can_hal.h
   with host-only symbols */
void can_hal_set_interface(const char *ifname);
#endif

#ifdef CANMOD_TARGET_STM32F1
#include <libopencm3/stm32/rcc.h>
#endif

/* -------------------------------------------------------------------------
 * MS-CAN acceptance filter list
 * Update CAN IDs here after FORScan verification (task 1.2).
 * ---------------------------------------------------------------------- */

static const uint32_t MS_CAN_IDS[] = {
    0x080u,   /* ⚠ EPAS steering angle        */
    0x072u,   /* ⚠ Odometer                   */
    0x1A9u,   /* ⚠ Steering wheel controls    */
    0x217u,   /* ⚠ GEM-mirrored speed + RPM   */
    0x230u,   /* ⚠ Battery voltage            */
    0x165u,   /* ⚠ Gear selector              */
    0x3B5u,   /* ⚠ HVAC                       */
    0x420u,   /* ⚠ Coolant temperature        */
    0x4B0u,   /* ⚠ Lighting status            */
    0x540u,   /* ⚠ BCM doors                  */
    0x5C0u,   /* ⚠ PDC                        */
};
#define MS_CAN_ID_COUNT ((uint8_t)(sizeof(MS_CAN_IDS) / sizeof(MS_CAN_IDS[0])))

/* -------------------------------------------------------------------------
 * Elapsed-time helper (handles 32-bit wrap correctly)
 * ---------------------------------------------------------------------- */

static inline uint32_t elapsed(uint32_t since)
{
    return hw_tick_get() - since;
}

/* -------------------------------------------------------------------------
 * STM32 clock setup
 * ---------------------------------------------------------------------- */

#ifdef CANMOD_TARGET_STM32F1
static void clock_setup(void)
{
    /* 8 MHz HSE crystal → 72 MHz system clock via PLL */
    rcc_clock_setup_in_hse_8mhz_out_72mhz();
}
#endif

/* -------------------------------------------------------------------------
 * main()
 * ---------------------------------------------------------------------- */

#ifdef CANMOD_TARGET_HOST
int main(int argc, char *argv[])
{
    /* Parse --interface <name> */
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--interface") == 0) {
            can_hal_set_interface(argv[i + 1]);
            i++;
        }
    }
#else
int main(void)
{
    clock_setup();
#endif

    /* --- Initialise HAL ------------------------------------------------- */
    timer_hal_init();
    gpio_hal_init();
    usb_hal_init();
    uart_hal_init();
    can_hal_init(MS_CAN_IDS, MS_CAN_ID_COUNT);

    /* --- Periodic timer last-fire timestamps ---------------------------- */
    uint32_t t_slow      = hw_tick_get();   /* 500 ms: veh info, doors, AC, flags */
    uint32_t t_fast      = hw_tick_get();   /* 100 ms: steering, radar            */
    uint32_t t_heartbeat = hw_tick_get();   /* 500 ms: LED toggle                 */

#ifdef CANMOD_OUTPUT_RAISE
    e_radar_state_t prev_radar_state = e_radar_undef;
#endif

    /* --- Main loop ------------------------------------------------------- */
    while (1) {
        uint32_t now = hw_tick_get();

        /* ---- CAN Rx ---------------------------------------------------- */
        {
            uint32_t id;
            uint8_t  data[8];
            uint8_t  dlc;

            while (can_hal_rx(&id, data, &dlc)) {
                car_process_frame(id, data, dlc);

#ifdef CANMOD_OUTPUT_JSON
                json_log_emit(hw_tick_get(), id);
#endif
            }
        }

        /* ---- SWC events (immediate) ------------------------------------ */
#ifdef CANMOD_OUTPUT_RAISE
        {
            canmod_swc_event_t evt;
            while (car_swc_dequeue(&evt)) {
                raise_send_swc(evt.button_id, evt.pressed);
            }
        }
#endif

        /* ---- PDC active change (event-driven) -------------------------- */
#ifdef CANMOD_OUTPUT_RAISE
        {
            e_radar_state_t cur = car_get_radar().state;
            if (cur != prev_radar_state) {
                raise_send_parking_active();
                prev_radar_state = cur;
            }
        }
#endif

        /* ---- 100 ms fast periodic (steering + radar) ------------------- */
        if (elapsed(t_fast) >= 100u) {
            t_fast = now;
#ifdef CANMOD_OUTPUT_RAISE
            raise_send_steering();
            if (car_get_radar().state == e_radar_on) {
                raise_send_rear_radar();
                raise_send_front_radar();
            }
#endif
        }

        /* ---- 500 ms slow periodic (vehicle data, doors, AC, flags) ----- */
        if (elapsed(t_slow) >= 500u) {
            t_slow = now;
#ifdef CANMOD_OUTPUT_RAISE
            raise_send_vehicle_info();
            raise_send_doors();
            raise_send_warnings();
            raise_send_status_flags();
            raise_send_ac();
#endif
        }

        /* ---- Head-unit RX (Raise only) --------------------------------- */
#ifdef CANMOD_OUTPUT_RAISE
        raise_rx_poll();
#endif

        /* ---- USB CDC poll (STM32 only) --------------------------------- */
        usb_hal_poll();

        /* ---- Heartbeat LED (1 Hz) -------------------------------------- */
        if (elapsed(t_heartbeat) >= 500u) {
            t_heartbeat = now;
            gpio_hal_led_toggle();
        }
    }

    return 0; /* unreachable */
}
