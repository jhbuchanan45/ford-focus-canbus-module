/**
 * can_hal.c — Host (SocketCAN) CAN HAL implementation
 *
 * Opens a PF_CAN socket on the interface specified at startup (default:
 * vcan0) with O_NONBLOCK so can_hal_rx() never blocks the main loop.
 *
 * Quick start:
 *   sudo modprobe vcan
 *   sudo ip link add dev vcan0 type vcan
 *   sudo ip link set up vcan0
 *   ./canmod-host --interface vcan0
 *   cansend vcan0 3B5#0300002B00000000
 */

#include "../can_hal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

static int s_sock = -1;

/* Set from main() via can_hal_set_interface() before can_hal_init() */
static char s_ifname[IFNAMSIZ] = "vcan0";

void can_hal_set_interface(const char *ifname)
{
    strncpy(s_ifname, ifname, IFNAMSIZ - 1);
    s_ifname[IFNAMSIZ - 1] = '\0';
}

void can_hal_init(const uint32_t *acceptance_ids, uint8_t count)
{
    struct sockaddr_can addr;
    struct ifreq ifr;

    s_sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s_sock < 0) {
        perror("can_hal: socket()");
        exit(EXIT_FAILURE);
    }

    /* Apply hardware-filter list if provided */
    if (acceptance_ids != NULL && count > 0) {
        struct can_filter filters[count];
        for (uint8_t i = 0; i < count; i++) {
            filters[i].can_id   = acceptance_ids[i] & CAN_SFF_MASK;
            filters[i].can_mask = CAN_SFF_MASK;
        }
        if (setsockopt(s_sock, SOL_CAN_RAW, CAN_RAW_FILTER,
                       filters, sizeof(filters)) < 0) {
            perror("can_hal: setsockopt(CAN_RAW_FILTER)");
        }
    }

    /* Non-blocking receive */
    int flags = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, flags | O_NONBLOCK);

    strncpy(ifr.ifr_name, s_ifname, IFNAMSIZ - 1);
    if (ioctl(s_sock, SIOCGIFINDEX, &ifr) < 0) {
        fprintf(stderr, "can_hal: interface '%s' not found: %s\n",
                s_ifname, strerror(errno));
        fprintf(stderr, "  Hint: sudo ip link add dev %s type vcan"
                        " && sudo ip link set up %s\n",
                s_ifname, s_ifname);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s_sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("can_hal: bind()");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "[can_hal] Listening on %s\n", s_ifname);
}

int can_hal_rx(uint32_t *id, uint8_t *data, uint8_t *dlc)
{
    struct can_frame frame;
    ssize_t n = read(s_sock, &frame, sizeof(frame));
    if (n < 0) {
        /* EAGAIN / EWOULDBLOCK is normal (no frame available) */
        return 0;
    }
    if (n < (ssize_t)sizeof(frame)) {
        return 0;
    }

    *id  = frame.can_id & CAN_SFF_MASK;
    *dlc = frame.can_dlc;
    memcpy(data, frame.data, frame.can_dlc);
    return 1;
}
