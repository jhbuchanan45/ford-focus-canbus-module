## Context

The existing firmware (Blue Pill 1) reads MS-CAN and transmits Raise VW PQ packets to the ATOTO head unit. The sniffer (Blue Pill 2) does the mirror-image job: it sits on the same UART line as the ATOTO, accepts packets from the FD60, ACKs them, and logs everything to USB CDC for analysis on a PC.

The two Blue Pills are never connected to the same UART simultaneously. During discovery sessions the FD60 is wired to Blue Pill 2 (sniffer); during development the FD60 is replaced by Blue Pill 1 (custom decoder).

## Goals / Non-Goals

**Goals:**
- Receive all Raise VW PQ packets from the FD60 on PA3 (USART2 RX, 38400 baud, 8N1)
- Send 0xFF ACK on PA2 (USART2 TX) after each valid packet to keep FD60 transmitting
- Log every packet (valid and bad checksum) to USB CDC in a timestamped hex format
- Build as a separate CMake target (`canmod-sniffer.bin`) alongside the existing firmware

**Non-Goals:**
- Host (x86) build
- Decoding packet payloads
- Bidirectional sniffing (FD60 → sniffer only; not injecting toward FD60)

## Decisions

### D1: Separate CMake target, not a new CANMOD_OUTPUT mode

**Decision:** Build as `canmod-sniffer` — a separate `add_executable()` in CMakeLists.txt with its own source list — rather than adding `CANMOD_OUTPUT=sniffer` that gates the existing `main.c`.

**Rationale:** The sniffer has fundamentally different application logic from the existing firmware: no CAN bus, no car state machine, no periodic send timers. Forcing it through `main.c` with `#ifdef` blocks would require gating CAN init, `car_process_frame`, all periodic send calls, and the SWC/PDC event handlers. A separate `sniffer_main.c` is ~50 lines with no conditionals and no dead code. The HAL is shared by including the same `HAL_SOURCES` in both targets.

**Alternative considered:** `CANMOD_OUTPUT=sniffer` with heavy `#ifdef` in main.c. Rejected: adds complexity to a file that is already well-structured; the sniffer is a distinct application not an output variant.

### D2: Log format — timestamped hex with pipe-separated sections

**Decision:** Each packet is logged as one ASCII line:

```
T+000123 2E 41 0D | 02 00 32 00 FA 5A 00 04 EE 00 00 30 39 | C=A7 OK\n
T+001456 2E 24 01 | 04 | C=DA OK\n
T+001789 2E FF 01 | 00 | C=FE BAD\n
```

Format: `T+<6-digit ms>  <SOF CMD LEN hex>  |  <DATA bytes hex>  |  C=<checksum byte hex>  OK|BAD\n`

**Rationale:**
- Timestamp gives relative timing between packets (useful for spotting rate anomalies)
- SOF/CMD/LEN separated from DATA makes CMD identification instant without a spec lookup
- Checksum field shown even for BAD packets so the actual vs. expected can be computed offline
- Plain ASCII over USB CDC — readable in any serial terminal, trivially grep-able
- 6-digit ms (`T+000000`–`T+999999`) covers ~16 minutes before wrapping; acceptable for discovery sessions. Wrapping is visible (counter resets to 000000) and does not corrupt the data.

**Alternative considered:** Raw binary passthrough (forward bytes to USB as-is). Rejected: not human-readable; requires a separate decoder tool on the PC.

**Alternative considered:** JSON (`{"ts":123,"cmd":"0x41","data":[...]}`). Rejected: verbose, harder to read at a glance in a terminal, no benefit until CMD values are known.

### D3: Buffer all DATA bytes in the RX state machine

**Decision:** The sniffer's RX state machine buffers up to 16 DATA bytes as they arrive, so the complete payload is available when the CSUM byte is processed and the line is formatted.

**Rationale:** The existing `raise_rx_poll()` discards DATA bytes (only accumulates checksum). The sniffer needs to log them, so a static 16-byte buffer is added to the state machine. Max known Raise payload is 13 bytes (0x41/0x02 vehicle info); 16 bytes provides a safe margin. Buffer overflow (LEN > 16) logs a truncated packet marked `TRUNC`.

### D4: Log BAD checksum packets, don't discard them

**Decision:** Packets with invalid checksums are logged with `BAD` suffix and still forwarded to USB CDC. No ACK is sent (matching existing behaviour).

**Rationale:** For discovery, a bad checksum is data — it reveals framing, timing, or noise issues that would be invisible if silently discarded. The FD60 may occasionally produce malformed frames; knowing this matters.

### D5: No host target for the sniffer

**Decision:** `canmod-sniffer` is only added to the `CANMOD_TARGET=stm32f1` branch of CMakeLists.txt. No host build.

**Rationale:** The sniffer's value is exclusively in reading bytes from real FD60 hardware over USART2. A host build would require a PTY-backed UART mock and a packet injector — infrastructure with no practical use, since you cannot run the FD60 on a PC.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Blue Pill 2 (canmod-sniffer)                 │
│                                                                 │
│  sniffer_main.c                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  init: timer_hal, gpio_hal, usb_hal, uart_hal            │   │
│  │  loop:                                                   │   │
│  │    sniffer_rx_poll()   ← drain UART RX, parse, log       │   │
│  │    usb_hal_poll()                                        │   │
│  │    heartbeat LED (1 Hz)                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  raise_sniffer.c                                                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  State machine: WAIT_SOF→CMD→LEN→DATA→CSUM               │   │
│  │  On CSUM:                                                │   │
│  │    format log line → usb_hal_write()                     │   │
│  │    if OK: uart_hal_tx(0xFF)  ← ACK to FD60               │   │
│  │  Static buffer: uint8_t rx_data[16]                      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
│  HAL (shared, unchanged)                                        │
│  uart_hal: PA3 RX ← FD60 TX    PA2 TX → FD60 RX (ACKs only)   │
│  usb_hal:  PA11/PA12 USB CDC → PC                              │
│  gpio_hal: PC13 heartbeat LED                                  │
│  timer_hal: hw_tick_get() for timestamps                       │
└─────────────────────────────────────────────────────────────────┘
         │ PA3 RX                      │ USB CDC
         │                             │
    ┌────▼────┐                   ┌────▼────┐
    │  FD60   │                   │   PC    │
    │ Raise   │                   │terminal │
    │  box    │                   │ / log   │
    └─────────┘                   └─────────┘
```

## Risks / Trade-offs

**[Risk] FD60 stops transmitting if ACKs arrive too late** → The ACK is sent immediately in the same poll cycle as packet completion. USART2 TX is blocking (`usart_send_blocking`), so the ACK is on the wire within one byte-time (~260 µs at 38400). USB CDC logging is non-blocking (bytes are dropped if host not reading). ACK latency is not affected by USB.

**[Risk] Log line overruns USB CDC buffer** → Longest possible log line: `T+999999 2E FF 10 | XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX | C=FF BAD\n` = 67 bytes. USB CDC write is a single call; no fragmentation issues.

**[Risk] LEN byte > 16 (unknown future packet)** → Handled: state machine caps capture at 16 bytes and marks the line `TRUNC`. Still ACKs if checksum over full (uncaptured) bytes would be valid — but since DATA bytes beyond 16 are not accumulated into the checksum, a `TRUNC` packet will always show `BAD`. Acceptable for discovery: the CMD byte and first 16 bytes are still logged.
