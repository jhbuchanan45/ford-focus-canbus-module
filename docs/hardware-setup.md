# Hardware Setup Guide

Covers the physical build for connecting an STM32F103C8T6 (Blue Pill) to the Ford Focus Mk3 MS-CAN bus and the ATOTO S8 MS head unit.

---

## Parts List

| Component | Notes |
|-----------|-------|
| STM32F103C8T6 (Blue Pill) | £2–3 aliexpress/amazon; confirm `C8T6` not counterfeit |
| TJA1042T/3 CAN transceiver | 3.3 V tolerant, SPI-free; NXP or clone |
| 120 Ω resistor (0.25 W) | Only if bus termination test shows ~120 Ω (see §Bus Termination) |
| 3.3 V LDO (e.g. AMS1117-3.3) | Or MP2307 buck if power dissipation matters |
| CP2102 / CH340 USB-UART adapter | For USART1 debug output (115200 baud) |
| Ford quad-lock breakout / patch cable | Expose pins without cutting OEM loom |
| Breadboard + jumper wires | 170pt mini-breadboard fits in dash cavity |

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

## Wiring Diagram

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
 ATOTO 8-pin harness                                                     │
 ┌──────────────────┐                                                    │
 │ CAN-RX           │◄──────────────────────────────── PA2 (USART2 TX) │
 │ CAN-TX           │──────────────────────────────── PA3 (USART2 RX)  │
 │ GND              │──────────────────────────────── GND               │
 └──────────────────┘                                         (Blue Pill)│
                                                                         │
 USB-UART debug (optional)                                               │
 ┌──────────────────┐                                                    │
 │ RX               │◄──────────────────────────────── PA9 (USART1 TX) │
 │ TX               │──────────────────────────────── PA10 (USART1 RX)  │
 │ GND              │──────────────────────────────── GND               │
 └──────────────────┘                                                    │
                                                                         │
 USB-CDC (Phase 1 JSON log)                                              │
 └─────────────────────────────────────────────────────── PA11/PA12 (USB)┘
```

### TJA1042 Connections

| TJA1042 Pin | Connect to |
|-------------|-----------|
| VCC (pin 3) | 3.3 V |
| GND (pin 2) | GND |
| TXD (pin 1) | STM32 PB9 (CAN1_TX, remapped) |
| RXD (pin 4) | STM32 PB8 (CAN1_RX, remapped) |
| CANH (pin 7) | MS-CAN H (quad-lock A9) |
| CANL (pin 6) | MS-CAN L (quad-lock A10) |
| STB (pin 8) | GND (normal mode; pull high for standby) |

> The STM32F103 CAN1 peripheral is remapped to PB8/PB9 in firmware (`AFIO_MAPR_CAN1_REMAP_PORTB`). PA11/PA12 are used for USB CDC and must not be used for CAN.

---

## Power Supply

- **Input:** +12 V ACC from quad-lock A3 (switched with ignition)
- **Regulator:** AMS1117-3.3 LDO (TO-92 or SOT-223). Add 10 µF + 100 nF decoupling on output.
- **Current draw:** ~80 mA typical (Blue Pill + TJA1042)
- **Alternative:** If fitting into a warm enclosure, consider a small buck converter (e.g. MP2307) to reduce heat.

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

1. ☐ Confirm +12 V ACC present on A3 with ignition on
2. ☐ Confirm GND continuity between A4 and chassis
3. ☐ Bus termination measured (see §Bus Termination)
4. ☐ 3.3 V present on Blue Pill VCC rail before connecting CAN/UART
5. ☐ PC13 LED blinks at 1 Hz (heartbeat) — confirms firmware running
6. ☐ USB CDC enumerated — open terminal at 115200 (Phase 1 only)
7. ☐ JSON lines appear in terminal on ignition-on
8. ☐ ATOTO set to correct Raise profile; USART2 wired to ATOTO harness

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No LED blink | No power / bad firmware | Check 3.3 V rail; re-flash |
| No CAN frames in JSON log | Wrong bus termination / wrong pins | Re-check A9/A10; measure 60 Ω |
| ATOTO shows no vehicle info | UART wired backwards or wrong baud | Swap PA2/PA3; verify 38400 |
| SWC presses missed | Poor soldering on USART2 RX | Re-flow PA3 joint |
| JSON values wrong | CAN IDs unverified | Run FORScan capture (task 1.2) |
