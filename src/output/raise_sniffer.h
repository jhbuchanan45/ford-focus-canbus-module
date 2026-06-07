#ifndef CANMOD_RAISE_SNIFFER_H
#define CANMOD_RAISE_SNIFFER_H

/**
 * Poll the UART RX buffer for Raise VW PQ packets from the FD60 decoder.
 *
 * For each complete packet:
 *   - Logs a hex dump line to USB CDC:
 *       T+NNNNNN 2E CC LL | DD DD ... | C=XX OK|BAD|TRUNC\n
 *   - Sends a 0xFF ACK on UART TX if the checksum is valid.
 *
 * Call once per main-loop iteration.
 */
void sniffer_rx_poll(void);

#endif /* CANMOD_RAISE_SNIFFER_H */
