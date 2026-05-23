# ford-focus-canbus-module

Firmware playground for CAN bus decoding on embedded microcontrollers, targeting the **STM32** family. Focused on the Ford Focus HS-CAN and MS-CAN networks, but structured to be adaptable to other vehicles and platforms.

> **Status:** Early stage — hardware target (exact STM32 variant) TBD.

---

## 🎯 Goals

- Decode raw CAN frames from Ford Focus HS-CAN (500 kbps) and MS-CAN (125 kbps)
- Layered, chip-agnostic firmware: only the HAL touches MCU-specific code
- Spec-driven development via [OpenSpec](https://github.com/Fission-AI/OpenSpec) — all features start as a spec before any code is written

---

## 📁 Structure

```
ford-focus-canbus-module/
├── openspec/                  # Spec-driven development (OpenSpec)
│   ├── config.yaml            # Project context and conventions
│   ├── specs/
│   │   ├── firmware/spec.md   # Firmware architecture spec
│   │   └── canbus/spec.md     # CAN message & signal definitions
│   └── changes/               # Proposed changes (proposals, design, tasks)
├── src/                       # Firmware source (C)
│   ├── main.c
│   ├── can_decoder.c/.h
│   └── hal/                   # MCU-specific drivers
├── tests/                     # Host-runnable unit tests
└── tools/                     # PC-side utilities (log parsing, simulation)
```

---

## 🗂 Spec-Driven Development

This project uses **[OpenSpec](https://github.com/Fission-AI/OpenSpec)** — all changes start with a proposal and spec before implementation.

| Slash command | What it does |
|---------------|-------------|
| `/opsx:propose` | Draft a new change proposal |
| `/opsx:explore` | Explore existing specs and changes |
| `/opsx:apply` | Implement an approved change |
| `/opsx:archive` | Archive a completed change |

Specs live in [`openspec/specs/`](./openspec/specs/) and are the authoritative reference for what the firmware should do.

See **[`docs/development-guide.md`](./docs/development-guide.md)** for the full workflow — small features, large features, and how to correct specs.

---

## 🔧 Hardware Target

| Parameter | Value |
|-----------|-------|
| MCU Family | STM32 (TBD — likely F103 or G4) |
| CAN Controller | bxCAN or FDCAN |
| HS-CAN Speed | 500 kbps |
| MS-CAN Speed | 125 kbps |
| Transceiver | SN65HVD230 or TJA1050 |

---

## 🚗 Ford Focus CAN Networks

| Bus | Speed | Scope |
|-----|-------|-------|
| HS-CAN | 500 kbps | Powertrain, ABS, engine, transmission |
| MS-CAN | 125 kbps | Body, HVAC, lighting, instrument cluster |
| LIN | — | Out of scope |

---

## 🚀 Getting Started

> Toolchain and build system will be added once the MCU is confirmed.

**Install OpenSpec (for spec-driven development):**

```bash
npm install -g @fission-ai/openspec@latest
```

**Propose a new feature or change:**

```
/opsx:propose "add steering angle signal decoding"
```

---

## 📝 License

TBD — likely MIT or Apache 2.0.
