# Raise VW PQ Output Driver Spec

## Overview

The Raise output driver (`src/output/raise.c`) implements the Raise VW PQ UART protocol at 38400 baud 8N1 for the ATOTO S8 MS head unit. It reads vehicle state exclusively via the `car_get_*` API and produces nine packet types.

Reference implementation: [smartgauges/canbox](https://github.com/smartgauges/canbox) `canbox.c`.

## Packet Format

```
[0x2E] [CMD] [LEN] [DATA × LEN bytes] [CSUM]
```

`CSUM = (CMD + LEN + sum(DATA)) ^ 0xFF`  (8-bit, wraps naturally)

---

## Requirements

### Requirement: Raise VW PQ packet format is implemented exactly
All packets SHALL use the framing above. Checksum SHALL be computed over CMD, LEN, and all DATA bytes, then XOR'd with 0xFF.

#### Scenario: Checksum is correct
- **WHEN** any Raise packet is assembled
- **THEN** the final byte equals `(CMD + LEN + sum(DATA)) ^ 0xFF`

---

### Requirement: All nine Raise packet types are implemented

#### 0x20 — SWC button event
- DATA: `[button_id, pressed]` (2 bytes)
- `button_id`: Raise button codes — Vol+=0x01, Vol-=0x02, Next=0x03, Prev=0x04, Mode=0x05, Mute=0x06, Answer=0x07, Hangup=0x08
- `pressed`: 0x01=press, 0x00=release

#### 0x21 — AC / climate status (5 bytes)
| Byte | Signal |
|------|--------|
| 0 | Flags: bit0=ac_on, bit1=recirculation, bit2=dual_zone |
| 1 | Fan speed (0–7) |
| 2 | Driver setpoint (°C × 2) |
| 3 | Passenger setpoint (°C × 2) |
| 4 | Airflow: bit0=windscreen, bit1=middle, bit2=floor |

#### 0x22 — Rear PDC distances (4 bytes)
- DATA: `[RL, RLM, RRM, RR]`
- Distance encoding: `raw ≥ 99 → 0 (clear)`; otherwise `11 − (raw × 10 / 98)`

#### 0x23 — Front PDC distances (4 bytes)
- DATA: `[FL, FLM, FRM, FR]`
- Same distance encoding as 0x22

#### 0x24 — Status flags (1 byte)
- bit0 = reverse active (`e_selector_r`)
- bit1 = park brake engaged
- bit2 = near lights on

#### 0x25 — Parking active (1 byte)
- `0x02` = sensors active; `0x00` = inactive

#### 0x26 — Steering angle (2 bytes)
- Signed int16 big-endian, degrees in range -540 to +540
- Derived from `car_get_wheel() / 10`

#### 0x41/0x01 — Door status (2 bytes)
- byte 0 = subcmd `0x01`
- byte 1 = door bitfield: FL=b0, FR=b1, RL=b2, RR=b3, tailgate=b4, bonnet=b5

#### 0x41/0x02 — Vehicle info (13 bytes)
| Bytes | Signal |
|-------|--------|
| 0 | subcmd `0x02` |
| 1–2 | speed km/h BE uint16 |
| 3–4 | RPM BE uint16 |
| 5 | coolant temp + 40 (unsigned) |
| 6 | fuel % (0x00 — not available) |
| 7–8 | battery 0.1 V BE uint16 |
| 9–12 | odometer km BE uint32 |

#### 0x41/0x03 — Warning flags (2 bytes)
- byte 0 = subcmd `0x03`
- byte 1 = flags: bit7=low_fuel (always 0 for Focus — no fuel signal on MS-CAN)

---

### Requirement: Raise driver responds to head-unit packets with 0xFF ACK
The driver SHALL parse incoming 0x2E-framed packets from the ATOTO head unit and send a single 0xFF ACK byte after each complete valid packet. Invalid checksum → no ACK, reset parser.

#### Scenario: Valid packet → ACK
- **WHEN** a complete `0x2E CMD LEN DATA... CSUM` packet is received with correct checksum
- **THEN** `0xFF` is transmitted within one UART poll cycle

#### Scenario: Wrong checksum → no ACK, parser reset
- **WHEN** a packet is received with an incorrect CSUM byte
- **THEN** no ACK is sent and the parser resets to wait-for-SOF state

---

### Requirement: Packet transmission is periodic and event-driven

| Packet | Trigger |
|--------|---------|
| 0x41/0x02 vehicle info | Every 500 ms |
| 0x41/0x01 doors | Every 500 ms |
| 0x41/0x03 warnings | Every 500 ms |
| 0x24 status flags | Every 500 ms |
| 0x21 AC status | Every 500 ms |
| 0x26 steering | Every 100 ms |
| 0x22 / 0x23 radar | Every 100 ms when PDC active |
| 0x25 parking active | On PDC active state change |
| 0x20 SWC | Immediately on `car_swc_dequeue()` returning an event |
