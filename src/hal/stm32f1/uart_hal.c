/**
 * uart_hal.c — STM32F103 UART HAL
 *
 * USART2 (PA2 TX / PA3 RX) — head-unit Raise protocol at 38400 baud, 8N1
 * USART1 (PA9 TX / PA10 RX) — debug console at 115200 baud (not exposed
 *   via this API; use usb_hal for JSON log output)
 *
 * Verification:
 *   Connect a USB-UART adapter to PA2/PA3, open at 38400 8N1.
 *   Build with CANMOD_OUTPUT=raise and replay a driving capture.
 *   You should see "2E 41 ..." packets at 500 ms intervals.
 */

#include "../uart_hal.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

void uart_hal_init(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_USART2);

    /* PA2 = USART2_TX (alt function push-pull) */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO2);
    /* PA3 = USART2_RX (input floating) */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO3);

    usart_set_baudrate(USART2, 38400);
    usart_set_databits(USART2, 8);
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    usart_set_parity(USART2, USART_PARITY_NONE);
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART2, USART_MODE_TX_RX);

    usart_enable(USART2);
}

void uart_hal_tx(uint8_t byte)
{
    usart_send_blocking(USART2, byte);
}

int uart_hal_rx(uint8_t *byte)
{
    if ((USART_SR(USART2) & USART_SR_RXNE) == 0) {
        return 0;
    }
    *byte = (uint8_t)usart_recv(USART2);
    return 1;
}
