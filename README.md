# OpenSafetyRTOS

**The first fully open-source automotive RTOS with public ISO 26262 ASIL-D certification evidence.**

![Build Status](https://img.shields.io/badge/build-in%20progress-yellow)
![License](https://img.shields.io/badge/license-Apache%202.0%20%2F%20MIT%20%2F%20CC%20BY%204.0-blue)
![ASIL](https://img.shields.io/badge/ASIL-D-red)
![Standard](https://img.shields.io/badge/ISO%2026262-Part%202%2F6%2F8%2F9-orange)
![SEooC](https://img.shields.io/badge/SEooC-ISO%2026262%20Part%208%20Cl.13-purple)

> **Status:** Active development — Phase 1 (Safety Analysis) in progress.
> Safety evidence documents (HARA, FSC, SSRS) are being written and reviewed in public.

---

## What Is OpenSafetyRTOS?

OpenSafetyRTOS is an open-source real-time operating system for **automotive safety-critical embedded systems**, targeting ISO 26262 ASIL-D compliance on ARM Cortex-M4/M7 microcontrollers.

The key idea: instead of rewriting an entire RTOS from scratch under ASIL-D — an enormously expensive undertaking — OpenSafetyRTOS applies the **ISO 26262 Part 9 ASIL Decomposition** strategy:

```
FreeRTOS        QM(D)      — Standard RTOS functions: scheduling, IPC, drivers
SafetyFunction  ASIL-D(D)  — Safety monitoring, MPU enforcement, fault handling
──────────────────────────────────────────────────────────────────────────────
OpenSafetyRTOS  ASIL-D     — Combined system-level safety goal
```

FreeRTOS does what it has always done. SafetyFunction, developed independently to ASIL-D(D), enforces the safety shell around it. **Freedom From Interference (FFI)** between the two partitions — proven in hardware via ARM MPU and verified in software — is what makes the decomposition valid under ISO 26262.

---

## Why OpenSafetyRTOS?

Every existing ASIL-D RTOS or safety OS solution is either closed-source, commercially licensed, or ships without its certification evidence. OpenSafetyRTOS changes that.

### Comparison with Commercial Solutions

| | TTTech MotionWise | ETAS RTA-OS | Green Hills INTEGRITY | QNX Neutrino | **OpenSafetyRTOS** |
|---|---|---|---|---|---|
| **ASIL level** | ASIL-D | ASIL-D | ASIL-D | ASIL-D | **ASIL-D** |
| **ISO 26262 strategy** | Hypervisor (ASIL-D + Guest QM) | Full OS certification | Full OS certification | Full OS certification | **Decomposition (Part 9)** |
| **Source code** | Closed | Closed | Closed | Closed | **Open (Apache 2.0)** |
| **Certification evidence** | Private | Private | Private | Private | **Public (CC BY 4.0)** |
| **HARA / FSC / SSRS** | Not disclosed | Not disclosed | Not disclosed | Not disclosed | **Fully public** |
| **FreeRTOS compatible** | No | No | No | No | **Yes (MIT)** |
| **Target hardware** | Cortex-R/A (MMU) | Any | Any | Any | **Cortex-M4/M7 (MPU)** |
| **Cost** | Commercial license | Commercial license | Commercial license | Commercial license | **Free** |

### The Open Certification Evidence Advantage

The safety evidence documents — HARA, FSC, SSRS, FMEA, and the complete SafetyCase — are published under CC BY 4.0. This means:

- **Integrators** can review and understand exactly what was analyzed, what assumptions were made, and what risks remain.
- **Researchers and educators** can use real-world ISO 26262 documents as learning material.
- **Community contributors** can improve the safety arguments, not just the code.
- **Audit trails** are fully transparent — every review record and QA decision is on GitHub.

No other ASIL-D RTOS does this.

### vs. FreeRTOS / SafeRTOS

| | FreeRTOS | SafeRTOS | **OpenSafetyRTOS** |
|---|---|---|---|
| ASIL rating | QM | ASIL-D | **ASIL-D** |
| Open source | ✅ | ❌ | **✅** |
| FreeRTOS ecosystem | ✅ | ❌ (full rewrite) | **✅** |
| ISO 26262 strategy | N/A | Full re-certification | **Decomposition (Part 9)** |
| MPU-enforced FFI | ❌ | ✅ | **✅** |
| Certification evidence public | ❌ | ❌ | **✅** |
| Cost | Free | Commercial | **Free** |

SafeRTOS achieves ASIL-D by completely rewriting FreeRTOS — breaking ecosystem compatibility and adding significant licensing cost. OpenSafetyRTOS achieves the same system-level ASIL-D goal by isolating the safety-critical work into the SafetyFunction partition, keeping FreeRTOS as-is.

---

## Core Design: FFI via MPU + Mailbox

### Memory Partitioning

```
┌──────────────────────────────────────────────────────────────────┐
│ Region                │ SafetyFunction Access │ FreeRTOS(QM) Access│
├──────────────────────────────────────────────────────────────────┤
│ ASIL-D Region         │ Read / Write          │ NO ACCESS          │
│ QM Region             │ Read Only             │ Read / Write       │
│ QM→Safety Mailbox     │ Read Only             │ Read / Write       │
└──────────────────────────────────────────────────────────────────┘
```

QM cannot write to ASIL-D memory — this is enforced in hardware by the ARM MPU. Any violation triggers a MemManage Fault, which transitions the system to Safe State Level 3 (Emergency Reset).

### Mailbox: Controlled Cross-Partition Communication

When QM must pass data to SafetyFunction, it writes to a dedicated Mailbox region. SafetyFunction never blindly trusts that data:

```c
safety_status_t safety_mailbox_receive(mailbox_t *mb, void *out, size_t len)
{
    if (crc32(mb->data, mb->data_len) != mb->crc)
        return SAFETY_ERR_CRC_FAIL;         // ① CRC integrity

    if ((safety_get_tick() - mb->timestamp) > MAILBOX_MAX_AGE_MS)
        return SAFETY_ERR_STALE;            // ② Freshness check

    if (!safety_range_check(mb->data, mb->data_len, mb->schema))
        return SAFETY_ERR_RANGE;            // ③ Value range check

    memcpy(out, mb->data, len);
    return SAFETY_OK;
}
```

Three consecutive validation failures escalate to Safe State Level 2 (Controlled Stop).

---

## Safe State Architecture

| Level | Name | Action |
|-------|------|--------|
| Level 1 | Degraded Operation | Retry with fault counter increment |
| Level 2 | Controlled Stop | Suspend QM tasks, notify application |
| Level 3 | Emergency Reset | Disable interrupts, stop Watchdog kick → MCU reset |

The independent hardware Watchdog monitors SafetyFunction itself — if SafetyFunction hangs, the system resets regardless of software state.

---

## Distribution: SEooC (Safety Element out of Context)

OpenSafetyRTOS is developed as a **SEooC** per ISO 26262 Part 8 Cl.13. This means:

- SafetyFunction is developed and certified **independently** of any specific end-system.
- Integrators receive the full source code, all safety evidence documents, and a set of **Assumptions of Use (AoU)** that must be satisfied in the target system.
- Integrators are responsible for validating AoU compliance and performing system-level HARA for their specific application.

This is the same model used by TTTech MotionWise and ETAS RTA-OS — but with all evidence documents fully public.

### Assumptions of Use (Summary)

| AoU ID | Requirement |
|--------|-------------|
| AoU-01 | Call `safety_mpu_init()` first at system boot — before any other initialization |
| AoU-02 | SafetyFunction task must hold the highest priority in the system |
| AoU-03 | All QM→SafetyFunction data transfers must use the Mailbox API exclusively |
| AoU-04 | Provide a dedicated independent hardware timer source to SafetyFunction |
| AoU-05 | Connect an external hardware Watchdog to SafetyFunction |
| AoU-06 | Use a MISRA-C:2012 compliant compiler at the recommended optimization level |

---

## Repository Structure

```
opensafetyrtos/
├── kernel/
│   ├── freertos/           # FreeRTOS source — QM partition (MIT License)
│   └── safety/             # SafetyFunction kernel layer (ASIL-D, Apache 2.0)
│       └── src/            # watchdog.c, mailbox.c, safe_state.c, ...
├── arch/
│   └── arm-cortex-m/
│       └── src/            # mpu.c, fault_handlers.c (platform-specific)
├── safety/
│   └── doc/                # ISO 26262 certification evidence (CC BY 4.0)
│       ├── HARA.md         # Hazard Analysis and Risk Assessment
│       ├── FSC.md          # Functional Safety Concept (27 FSRs)
│       ├── SSRS.md         # Software Safety Requirements Spec (27 SSRs)
│       ├── SAFETY_CASE.md  # Top-level safety argument (GSN)
│       ├── SAFETY_PLAN.md  # ISO 26262 Part 2 safety management plan
│       └── reviews/        # QA Confirmation Review records (OSR-CR-*)
├── docs/
│   ├── ARCHITECTURE.md     # System architecture and FFI design
│   ├── TEAM.md             # Roles and independence matrix
│   ├── ADR-001-*.md        # ASIL Decomposition rationale
│   ├── ADR-002-*.md        # MPU partition strategy
│   ├── ADR-003-*.md        # SEooC distribution strategy
│   └── ADR-004-*.md        # Boot sequence
└── .github/
    ├── CODEOWNERS          # @safety-team mandatory for safety/* PRs
    └── branch-strategy.md
```

---

## Safety Evidence Status

All certification evidence is written in public and reviewed by an independent QA agent following ISO 26262 Part 2 Cl.8 Confirmation Measures.

| Document | ID | Version | Status |
|----------|----|---------|--------|
| Hazard Analysis (HARA) | OSR-HARA-001 | v1.2 | 🟢 Conditionally approved |
| Functional Safety Concept (FSC) | OSR-FSC-001 | v1.2 | 🟢 **Final approved** |
| SW Safety Requirements (SSRS) | OSR-SSRS-001 | v1.0 | 🟡 Draft — QA review pending |
| Safety Case | OSR-SC-001 | v0.4 | 🟡 In progress |
| Safety Plan | OSR-SP-001 | v1.0 | 🟢 Approved |
| Tool Qualification Plan | OSR-TQ-001 | v1.0 | 🟢 Approved |

QA review records: `safety/doc/reviews/OSR-CR-*.md`

---

## Quick Start

> **Note:** Build system and hardware bring-up are in active development.

```bash
git clone https://github.com/eseseo/opensafetyrtos.git
cd opensafetyrtos

# Configure for ARM Cortex-M4
cmake -B build \
  -DTARGET_ARCH=arm-cortex-m \
  -DSAFETY_ASIL_D=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-cortex-m4.cmake

cmake --build build
```

---

## License

| Component | License |
|-----------|---------|
| SafetyFunction layer (`safety/`, `kernel/safety/`) | Apache License 2.0 |
| FreeRTOS (`kernel/freertos/`) | MIT License |
| Certification evidence documents (`safety/doc/`) | CC BY 4.0 |

The CC BY 4.0 license on certification documents means you can use, adapt, and redistribute the safety evidence for your own products — as long as you credit OpenSafetyRTOS.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Two tracks:

- **QM track** — standard contributions to FreeRTOS integration, build system, HAL
- **ASIL-D track** — contributions to `safety/` or `kernel/safety/` require MISRA-C:2012 compliance and mandatory independent QA review

Safety-related PRs require `@safety-team` approval. All ASIL-D code changes must include an SSR traceability tag (e.g., `/* SSR-015-MBX */`).

---

## References

- [ISO 26262-9](https://www.iso.org/standard/68385.html) — ASIL Decomposition
- [ISO 26262-8 Cl.13](https://www.iso.org/standard/68384.html) — SEooC development
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — System architecture and FFI design
- [safety/doc/SAFETY_CASE.md](safety/doc/SAFETY_CASE.md) — Top-level safety argument
- [safety/doc/FSC.md](safety/doc/FSC.md) — Functional Safety Concept (all FSRs)
- [ADR-001](docs/ADR-001-decomposition-strategy.md) — Why ASIL Decomposition?
- [ADR-003](docs/ADR-003-distribution-strategy-seooc.md) — Why SEooC?
