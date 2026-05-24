/**
 * uart_hal.c — Host UART HAL: writes Raise protocol bytes to a PTY
 *
 * On init, a PTY master/slave pair is opened.  The slave device name is
 * printed to stderr so you can monitor the output:
 *
 *   [uart_hal] Raise output PTY: /dev/pts/3
 *   cat /dev/pts/3 | xxd
 *
 * Incoming bytes from the head unit (simulated) can be written to the
 * slave from another terminal for testing the RX handler.
 */

#include "../uart_hal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef __linux__
#include <pty.h>
#else
#include <util.h>
#endif

static int s_master_fd = -1;

void uart_hal_init(void)
{
    int master, slave;
    char slave_name[64];

    if (openpty(&master, &slave, slave_name, NULL, NULL) < 0) {
        perror("uart_hal: openpty()");
        exit(EXIT_FAILURE);
    }

    /* Set master non-blocking for rx */
    int flags = fcntl(master, F_GETFL, 0);
    fcntl(master, F_SETFL, flags | O_NONBLOCK);

    /* Close our copy of the slave — it stays open because the slave fd is
     * also retained.  Keep it open so the PTY doesn't auto-close. */
    (void)slave; /* intentionally keep slave open until process exits */

    s_master_fd = master;
    fprintf(stderr, "[uart_hal] Raise output PTY: %s\n", slave_name);
    fprintf(stderr, "  Monitor: cat %s | xxd\n", slave_name);
}

void uart_hal_tx(uint8_t byte)
{
    if (s_master_fd < 0) return;
    ssize_t n;
    do {
        n = write(s_master_fd, &byte, 1);
    } while (n < 0 && errno == EINTR);
}

int uart_hal_rx(uint8_t *byte)
{
    if (s_master_fd < 0) return 0;
    ssize_t n = read(s_master_fd, byte, 1);
    return (n == 1) ? 1 : 0;
}
