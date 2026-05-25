# Hardware Setup Guide

Covers the physical build for connecting an STM32F103C8T6 (Blue Pill) to the Ford Focus Mk3 MS-CAN bus and the ATOTO S8 MS head unit.

---

## Parts List

| Component | Notes |
|-----------|-------|
| STM32F103C8T6 (Blue Pill) | £2–3 aliexpress/amazon; confirm `C8T6` not counterfeit |
| TJA1042T/3 CAN transceiver | 3.3 V tolerant, SPI-free; NXP or clone |
| 120 Ω resistor (0.25 W) | Only if bus termination test shows ~120 Ω (see §Bus Termination) |
| 3.3 V LDO (e.g. AMS1117-3.3) | Only needed if ATOTO 8-pin power pin reads 12 V |
| CP2102 / CH340 USB-UART adapter | For USART1 debug output (115200 baud) |
| T-tap / insulation piercing connectors × 4 | Tap CAN H/L, ACC, GND from harness without cutting |
| Breadboard + jumper wires | 400pt breadboard recommended |

---

## Quad-Lock Connector Pinout

The quad-lock is the rectangular multi-pin connector behind the OEM head unit. Only the A-block is needed.

```
Quad-lock A-block (viewed from front, pins right-to-left per row):

  A1  A2  A3  A4  A5  A6  A7  A8  A9  A10  A11  A12
  ·   ·   ·   ·   ·   ·   ·   ·  [H] [L]   ·    ·
                                   │    │
                             MS-CAN H  MS-CAN L  (125 kbps)

  A3 = +12 V ACC (switched)
  A4 = GND
  A9  = MS-CAN H
  A10 = MS-CAN L
```

> ⚠ **Pin numbering varies by Ford part number.** Verify against your loom with a multimeter before connecting power.

---

## Bus Termination Test

Before first power-on, measure the termination resistance across MS-CAN H/L **with the OEM head unit removed and the car ignition off**.

| Reading | Meaning | Action |
|---------|---------|--------|
| ~60 Ω | Both 120 Ω stubs in loom | No resistor needed on canbox |
| ~120 Ω | One stub in head unit (now removed) | **Fit 120 Ω across TJA1042 CANH/CANL pins** |
| >200 Ω | Termination elsewhere / no termination | Add 120 Ω, flag for investigation |

---

## Wiring Diagram — Option A: RZ-FD09 harness tap (recommended)

Use this if you have the existing RZ-FD09 / quad-lock harness adapter already fitted.
The Blue Pill **replaces** the RZ-FD09 box — tap 4 wires from the 10-pin harness
connector, and connect UART output to the same ATOTO 8-pin socket the RZ-FD09 used.

```
 RZ-FD09 10-pin harness connector (loom side — T-tap, do not cut)
 ┌──────────────────────────────────┐
 │ Teal  (MS-CAN H) ────────────────┼──── CANH ──────────────────────┐
 │ White (MS-CAN L) ────────────────┼──── CANL ───────────────────┐  │
 │ Black (GND)      ────────────────┼────────────────────────┐    │  │
 │ Pink  (Reverse)  ─── (optional) ─┼──► Blue Pill GPIO      │    │  │
 │ (remaining 6 wires: leave alone) │                         │    │  │
 └──────────────────────────────────┘                         │    │  │
                                         ┌───────────────────┼────┼──┼──────┐
                                         │  TJA1042          │    │  │      │
                                         │  ┌─────────────┐  │    │  │      │
                                         │  │ CANH (p7)  ◄┼──┼────┘  │      │
                                         │  │ CANL (p6)  ◄┼──┼───────┘      │
                                         │  │ RXD  (p4) ──┼──┼──────────► PB8│
                                         │  │ TXD  (p1)  ◄┼──┼────────────  PB9│
                                         │  │ VCC  (p3)  ◄┼──┼────── 3V3       │
                                         │  │ STB  (p8) ──┼──┼────── GND       │
                                         │  │ GND  (p2)  ◄┼──┤                 │
                                         │  └─────────────┘  │    STM32F103    │
                                         └───────────────────┘                 │
                                                              │                 │
 ATOTO spare CarPlay USB port                                 │                 │
 ┌───────────────────────────┐                               │                 │
 │ USB 5V ────────────────── ┼───────────────────────────────┼──► USB port    │
 │ USB GND ───────────────── ┼───────────────────────────────┤   (onboard LDO │
 └───────────────────────────┘                               │   → 3.3V)      │
   Phase 1: use laptop USB instead for JSON log              │                 │
                                                             │                 │
 ATOTO 8-pin CAN socket (replaces RZ-FD09 plug)             │                 │
 ┌─────────────────┐                                         │                 │
 │ UART-RX ────────┼─────────────────────────────────────────┼── PA2 (TX)     │
 │ UART-TX ────────┼─────────────────────────────────────────┼── PA3 (RX)     │
 │ GND     ────────┼─────────────────────────────────────────┘                │
 └─────────────────┘                                                           │
                                                                               │
 USB-UART debug adapter (optional, bench use)                                  │
 ┌──────────────────┐                                                          │
 │ RX ◄─────────────┼──────────────────────────────────────────── PA9  (TX)  │
 │ TX ──────────────┼──────────────────────────────────────────── PA10 (RX)  │
 │ GND ─────────────┼──────────────────────────────────────────── GND        │
 └──────────────────┘                                                          │
                                                                               │
 PC13 (onboard LED) ── heartbeat 1 Hz blink ───────────────────────────────┘
```

> See [`wiring-diagram.svg`](./wiring-diagram.svg) for the visual schematic.

### Power via ATOTO CarPlay USB (no LDO required)

The spare CarPlay USB port on the ATOTO provides 5 V. Connect a USB cable from that
port to the Blue Pill's USB connector. The Blue Pill's onboard AMS1117-3.3 regulates
5 V → 3.3 V for the MCU and TJA1042.

| Phase | USB connected to | Purpose |
|-------|-----------------|---------|
| Phase 1 (development) | Laptop / PC | Power + JSON serial log |
| Phase 2 (installed) | ATOTO CarPlay USB port | Power only (data unused) |

**Only 3 wires needed from the harness: CAN H, CAN L, GND.**

---

## Wiring Diagram — Option B: Direct quad-lock (no existing harness)

Use this if you are wiring directly to the quad-lock connector without the RZ-FD09 harness.

```
 Quad-lock A-block
 ┌────────────────┐
 │ A3 (+12V ACC)  │──────┬─────────────────────────────────────────────┐
 │ A4 (GND)       │──────┼──────────────────────────────────┐          │
 │ A9 (MS-CAN H)  │──────┼──── CANH ──► TJA1042 ──► RXD ───►  PB8    │
 │ A10 (MS-CAN L) │──────┼──── CANL ──► TJA1042 ──► TXD ◄── PB9     │
 └────────────────┘      │                                   STM32F103 │
                         │    +12V ──► AMS1117-3.3 ──► 3V3 ──► VCC   │
                         └───────────────────────────────── GND ──► GND│
                                                                        │
 ATOTO 8-pin harness                                                    │
 ┌──────────────────┐                                                   │
 │ CAN-RX           │◄──────────────────────────────── PA2 (USART2 TX)│
 │ CAN-TX           │──────────────────────────────── PA3 (USART2 RX) │
 │ GND              │──────────────────────────────── GND              │
 └──────────────────┘                                        (Blue Pill)│
                                                                        │
 USB-UART debug (optional)                                              │
 ┌──────────────────┐                                                   │
 │ RX               │◄──────────────────────────────── PA9 (USART1 TX)│
 │ TX               │──────────────────────────────── PA10 (USART1 RX) │
 │ GND              │──────────────────────────────── GND              │
 └──────────────────┘                                                   │
                                                                        │
 USB-CDC (Phase 1 JSON log)                                             │
 └────────────────────────────────────────────────────── PA11/PA12 (USB)┘
```

### TJA1042 Connections

| TJA1042 Pin | Signal | Connect to |
|-------------|--------|-----------|
| 1 — TXD | MCU → CAN | Blue Pill PB9 (CAN1_TX, remapped) |
| 2 — GND | Ground | GND |
| 3 — VCC | Power | 3.3 V (Blue Pill 3V3 pin) |
| 4 — RXD | CAN → MCU | Blue Pill PB8 (CAN1_RX, remapped) |
| 5 — VREF | Ref (unused) | Leave unconnected |
| 6 — CANL | CAN bus L | MS-CAN L (teal wire, harness) |
| 7 — CANH | CAN bus H | MS-CAN H (white wire, harness) |
| 8 — STB | Standby (active high) | GND (always-on normal mode) |

> The STM32F103 CAN1 peripheral is remapped to PB8/PB9 in firmware (`AFIO_MAPR_CAN1_REMAP_PORTB`). PA11/PA12 are used for USB CDC and must not be used for CAN.

> ⚠ **CAN H/L wire colours assumed from RZ-FD09 harness convention — verify with multimeter before first power-on** (CAN H ≈ 2.5–3.5 V, CAN L ≈ 1.5–2.5 V, measured to GND with ignition on).

---

## Power Supply

**Option A — from ATOTO 8-pin (preferred, no extra parts):**
Measure the power pin on the ATOTO CAN box 8-pin connector while the head unit is on.
- **5 V measured** → connect to Blue Pill `5V` pin. Onboard AMS1117-3.3 regulates to 3.3 V.
- **3.3 V measured** → connect to Blue Pill `3V3` pin directly. No LDO needed.
- **12 V measured** → use Option B below.

**Option B — from ACC wire (fallback):**
- **Input:** +12 V ACC (red wire on RZ-FD09 harness / quad-lock A3)
- **Regulator:** AMS1117-3.3 LDO. Add 10 µF + 100 nF decoupling capacitors on the output.
- Connect LDO output to Blue Pill `3V3` pin.

**Current draw:** ~80 mA typical (Blue Pill + TJA1042). Both ATOTO supply and ACC LDO handle this easily.

---

## ATOTO S8 MS UART Harness

The ATOTO S8 MS exposes a CAN box header (8-pin). The relevant signals:

| ATOTO pin | Signal | Blue Pill pin |
|-----------|--------|---------------|
| CAN-RX | Head unit receives Raise data | PA2 (USART2 TX) |
| CAN-TX | Head unit sends commands | PA3 (USART2 RX) |
| GND | Common ground | GND |

**Protocol:** Raise VW PQ, 38400 baud 8N1. Set ATOTO: *Settings → Factory → CAN Box → [Raise protocol option — confirm exact name from ATOTO menu]*.

---

## First Power-On Checklist

### Before wiring
1. ☐ Measure ATOTO 8-pin power pin voltage — note 5 V or 3.3 V (determines power path)
2. ☐ Back-probe RZ-FD09 harness 10-pin: confirm teal ≈ 2.5–3.5 V (CAN H), white ≈ 1.5–2.5 V (CAN L) with ignition on
3. ☐ Bus termination measured across teal/white with ignition off (see §Bus Termination)

### Bench test (before fitting in car)
4. ☐ Power Blue Pill from USB — PC13 LED blinks 1 Hz (heartbeat confirms firmware)
5. ☐ USB CDC enumerated on laptop — serial terminal shows JSON lines when CAN frames injected via vcan0

### In car — Phase 1 (JSON)
6. ☐ 3.3 V present on Blue Pill VCC rail with ignition on (measure at `3V3` pin)
7. ☐ JSON lines appear in USB terminal within 2 seconds of ignition-on
8. ☐ Speed, RPM, door status values match real car state

### In car — Phase 2 (Raise)
9. ☐ ATOTO set to **Raise VW PQ** protocol in CAN box settings
10. ☐ USART2 TX (PA2) wired to ATOTO 8-pin UART-RX pin
11. ☐ Steering wheel Vol+/Vol− controls ATOTO volume

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No LED blink | No power / bad firmware | Check 3.3 V rail; re-flash |
| No CAN frames in JSON log | Wrong bus termination / wrong pins | Re-check A9/A10; measure 60 Ω |
| ATOTO shows no vehicle info | UART wired backwards or wrong baud | Swap PA2/PA3; verify 38400 |
| SWC presses missed | Poor soldering on USART2 RX | Re-flow PA3 joint |
| JSON values wrong | CAN IDs unverified | Run FORScan capture (task 1.2) |
