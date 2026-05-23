## ADDED Requirements

### Requirement: Raise VW PQ packet format is implemented exactly
The Raise output driver SHALL implement the Raise VW PQ UART protocol exactly as documented in [smartgauges/canbox](https://github.com/smartgauges/canbox) `canbox.c`. Packet structure: `[0x2E][CMD][LEN][DATA...][CHECKSUM]`. Checksum: `sum(CMD, LEN, DATA...) XOR 0xFF`. Baud rate: 38400, 8N1.

#### Scenario: Checksum is correct
- **WHEN** any Raise packet is assembled
- **THEN** the final byte equals `(sum of CMD + LEN + all DATA bytes) XOR 0xFF`

#### Scenario: Packet is transmitted over UART
- **WHEN** `canmod_output_raise_send(cmd, data, len)` is called
- **THEN** the complete packet including start byte, checksum, and all data bytes is written to the head unit UART within one main loop tick (≤ 1 ms at 38400 baud)

---

### Requirement: All Raise packet types are implemented
The driver SHALL implement every Raise VW PQ packet type needed for full ATOTO integration.

#### Scenario: SWC button press packet sent (0x20)
- **WHEN** a SWC event with state PRESSED is dequeued
- **THEN** a 0x20 packet is sent with `[btn_id, 0x01]`, followed immediately by a 0x20 release packet with `[btn_id, 0x00]`
- **AND** btn_id maps Ford SWC codes to Raise button IDs: Vol+=0x01, Vol-=0x02, Prev=0x03, Next=0x04, Mode=0x0A, Mute=0x0C

#### Scenario: AC status packet sent (0x21)
- **WHEN** the AC state changes or the 500 ms periodic timer fires
- **THEN** a 0x21 packet is sent with the 5-byte AC payload encoding ac_on, fan_speed, l_temp, r_temp, recycling, airflow direction, dual zone, and seat heat levels per the canbox.c bit layout

#### Scenario: Rear radar packet sent (0x22)
- **WHEN** `car_get_radar().state == e_radar_on`
- **THEN** a 0x22 packet is sent with 4 bytes `[RL, RLM, RRM, RR]` where each value is `(rmax + 1) - scale(raw, 0, 99, 0, rmax)` with rmax=10

#### Scenario: Front radar packet sent (0x23)
- **WHEN** `car_get_radar().state == e_radar_on`
- **THEN** a 0x23 packet is sent with 4 bytes `[FR, FRM, FLM, FL]` using the same inversion formula with fmax=10

#### Scenario: Status flags packet sent (0x24)
- **WHEN** reverse, park brake, or near-lights state changes or the 200 ms timer fires
- **THEN** a 0x24 packet is sent with 1 byte: `bit0=reverse, bit1=park_brake, bit2=near_lights`

#### Scenario: Parking active packet sent (0x25)
- **WHEN** PDC active state changes (on→off or off→on)
- **THEN** a 0x25 packet is sent with `0x02` (active) or `0x00` (inactive)

#### Scenario: Steering angle packet sent (0x26)
- **WHEN** steering angle changes or the 100 ms timer fires and reverse is active
- **THEN** a 0x26 packet is sent with a signed int16_t angle in range -540 to +540 (scaled from car_get_wheel() -100 to +100)

#### Scenario: Door status sub-packet sent (0x41/0x01)
- **WHEN** any door, tailgate, or bonnet state changes
- **THEN** a 0x41 packet is sent with sub-type byte 0x01 and door bitfield: `FL=b0, FR=b1, RL=b2, RR=b3, tailgate=b4, bonnet=b5`

#### Scenario: Vehicle data sub-packet sent (0x41/0x02)
- **WHEN** the 500 ms periodic timer fires
- **THEN** a 0x41 packet is sent with sub-type byte 0x02 and 12 data bytes: RPM (2B), speed×100 (2B), voltage×100 (2B), temp×10 (2B), odometer (3B), fuel (1B)

#### Scenario: Warning flags sub-packet sent (0x41/0x03)
- **WHEN** low_fuel or low_voltage flag changes
- **THEN** a 0x41 packet is sent with sub-type 0x03 and flags byte: `low_fuel=b7, low_voltage=b6`

---

### Requirement: Raise driver responds to head unit commands
The Raise output driver SHALL process incoming bytes from the head unit UART and respond appropriately.

#### Scenario: 0xFF ACK sent after each received packet
- **WHEN** a complete packet is received from the head unit (start 0x2E, valid checksum)
- **THEN** a single 0xFF byte is transmitted as acknowledgement

#### Scenario: Known head unit commands handled
- **WHEN** command byte 0x81 (start/stop), 0x90 (request ID), 0xA0 (amplifier), or 0xA6 (set time) is received
- **THEN** the command is parsed without error; responses are no-ops in Phase 2 (extend later)

---

### Requirement: Packet transmission is periodic and event-driven
Door, speed, and AC packets SHALL be transmitted on a periodic timer. SWC, radar state change, and warning packets SHALL be sent immediately on state change.

#### Scenario: No change — vehicle data still sent periodically
- **WHEN** 500 ms elapses with no signal change
- **THEN** the 0x41/0x02 vehicle data packet is transmitted regardless (head unit keepalive)
