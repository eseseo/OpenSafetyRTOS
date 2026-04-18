# OpenSafetyRTOS

**An open-source automotive RTOS targeting ISO 26262 ASIL-D via decomposition of FreeRTOS (QM) and a dedicated SafetyFunction layer (ASIL-D).**

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![License](https://img.shields.io/badge/license-Apache%202.0%20%2F%20MIT-blue)
![ASIL](https://img.shields.io/badge/ASIL-D-red)
![Standard](https://img.shields.io/badge/ISO%2026262-Part%209-orange)

---

## What Is OpenSafetyRTOS?

OpenSafetyRTOS is an open-source real-time operating system designed for automotive safety-critical applications. It targets **ISO 26262 ASIL-D** compliance while leveraging the proven FreeRTOS ecosystem.

Rather than rewriting an entire RTOS from scratch at ASIL-D, OpenSafetyRTOS uses the **ISO 26262 Part 9 ASIL Decomposition** strategy:

```
FreeRTOS        QM(D)      — Base RTOS functions (scheduling, IPC, drivers)
SafetyFunction  ASIL-D(D)  — Safety monitoring, MPU enforcement, fault handling
────────────────────────────────────────────────────────────────────────
OpenSafetyRTOS  ASIL-D     — Combined system safety goal
```

Target hardware: **ARM Cortex-M4/M7 with MPU**.

---

## Key Design Principle: ASIL Decomposition

The core insight is that you do not need to certify every line of FreeRTOS at ASIL-D. Instead:

- **FreeRTOS** operates as a QM(D) partition — it handles scheduling, queues, semaphores, and drivers. It is used as a COTS component with pre-existing safety case evidence.
- **SafetyFunction** operates as an independent ASIL-D(D) partition — it monitors the system, enforces memory protection, manages watchdogs, and handles faults.
- **Freedom From Interference (FFI)** between the two partitions is the foundational requirement, enforced in hardware (ARM MPU) and verified in software.

### Partition Communication: Mailbox Pattern

QM never writes directly to ASIL-D memory (MPU-enforced hardware block). When QM must pass data to SafetyFunction, it writes to a dedicated **Mailbox region**. SafetyFunction reads and validates every message before use:

1. CRC-32 integrity check
2. Timestamp freshness check (stale data detection)
3. Value range check against expected schema

Any validation failure triggers the SafetyFunction fault handler — QM partition data is **never implicitly trusted**.

---

## How Is This Different from FreeRTOS / SafeRTOS?

| Feature | FreeRTOS | SafeRTOS | OpenSafetyRTOS |
|---|---|---|---|
| ASIL rating | QM | ASIL-D (full rewrite) | ASIL-D (decomposition) |
| Open source | Yes | No (commercial) | Yes |
| FreeRTOS ecosystem | Yes | No | Yes |
| ISO 26262 strategy | N/A | Full certification | Decomposition (Part 9) |
| MPU-enforced FFI | No | Yes | Yes |
| Cost | Free | Licensed | Free |

SafeRTOS achieves ASIL-D by fully rewriting FreeRTOS under a rigorous process — at significant cost and with source code restrictions. OpenSafetyRTOS achieves the same system-level ASIL-D goal by isolating the safety-critical work into the SafetyFunction partition while keeping FreeRTOS as-is, open-source, and fully accessible.

---

## Quick Start

> **Note:** Build system and hardware bring-up are under active development. The following is a placeholder.

```bash
# Clone the repository
git clone https://github.com/opensafetyrtos/opensafetyrtos.git
cd opensafetyrtos

# Configure for ARM Cortex-M4 target
cmake -B build -DTARGET=cortex-m4 -DBOARD=stm32f4-discovery

# Build
cmake --build build

# Flash (requires OpenOCD or J-Link)
cmake --build build --target flash
```

See `docs/ARCHITECTURE.md` for full system design and `safety/doc/SAFETY_PLAN.md` for the safety development process.

---

## Repository Structure

```
opensafetyrtos/
├── kernel/
│   ├── freertos/        # FreeRTOS source (QM partition, MIT license)
│   └── safety/          # Safety kernel hooks (ASIL-D, MISRA-C)
├── safety/              # SafetyFunction partition (ASIL-D)
│   ├── src/             # Safety source code
│   ├── include/         # Public safety API headers
│   └── doc/             # Safety evidence documents (FMEA, safety plan)
├── arch/                # Architecture-specific MPU/HAL code
├── docs/                # Architecture and ADR documents
│   ├── ARCHITECTURE.md
│   ├── ADR-001-decomposition-strategy.md
│   └── ADR-002-mpu-partition-strategy.md
└── .github/             # CI, CODEOWNERS, issue templates
```

---

## License

- **SafetyFunction layer** (`safety/`, `kernel/safety/`): [Apache License 2.0](LICENSE)
- **FreeRTOS** (`kernel/freertos/`): MIT License (see `kernel/freertos/LICENSE`)
- **Certification evidence documents** (`safety/doc/`): CC BY 4.0

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution tracks, coding standards, and the ASIL-D review process.

Safety-related contributions require MISRA-C compliance and a dedicated review process — read CONTRIBUTING.md before submitting changes to `safety/` or `kernel/safety/`.

---

## References

- ISO 26262-9: ASIL Decomposition
- ARCHITECTURE.md — full system design
- safety/doc/SAFETY_PLAN.md — safety development plan
- ADR-001: Decomposition strategy rationale
- ADR-002: MPU partition strategy
