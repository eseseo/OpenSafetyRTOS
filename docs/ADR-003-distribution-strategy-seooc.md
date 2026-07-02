# ADR-003: Distribution Strategy — SEooC (Safety Element out of Context)

**Date:** 2026-04-18 (revised 2026-07-02)
**Status:** Accepted
**Deciders:** PM/PO
**Reference Standard:** ISO 26262-10:2018 Cl.9 (SEooC)

---

## Context

OpenSafetyRTOS is an open-source project. Its distribution model must simultaneously satisfy the open-source philosophy and ISO 26262 certification requirements for integrators.

Three options were evaluated:

1. Source code distribution only (FreeRTOS model)
2. **Source code + SEooC certification package** ← Adopted
3. Hypervisor-based distribution (TTTech MotionWise model)

---

## Decision

**Develop and distribute the SafetyFunction layer as a SEooC (Safety Element out of Context) per ISO 26262-10:2018 Cl.9.**

```
OpenSafetyRTOS distribution package
├── kernel/freertos/        Source (MIT License)          — FreeRTOS as-is (planned; not yet vendored)
├── kernel/safety/          Source (Apache 2.0)           — SafetyFunction SEooC
├── arch/arm-cortex-m/      Source (Apache 2.0)           — MPU + fault handling port layer
├── safety/doc/             Safety evidence (CC BY 4.0)   — HARA, FSC, SSRS, AOU, etc.
│   └── AOU.md              Canonical AoU registry (OSR-AOU-001)
└── docs/INTEGRATION_GUIDE.md   Integrator guide (planned — Phase 2)
```

---

## SEooC Concept

Per ISO 26262-10:2018 Cl.9, SafetyFunction is a safety element developed and validated independently of any specific end-system context, on the basis of *assumed* requirements and an assumed context. Integrators can embed it in their own systems provided they satisfy the Assumptions of Use (AoU).

```
OpenSafetyRTOS (SEooC development)          Integrator System
┌──────────────────────────────┐         ┌──────────────────────────────┐
│ SafetyFunction ASIL-D(D)     │         │ Integrator Application       │
│  - HARA performed            │  ────▶  │  + FreeRTOS                  │
│  - SSRS written              │         │  + SafetyFunction (SEooC)    │
│  - FMEA analyzed             │         │                              │
│  - MC/DC 100% tested         │         │ Integrator responsibilities: │
│  - Assumptions defined       │         │  - Verify AoU satisfied      │
│  - Evidence documents public │         │  - System-level HARA         │
└──────────────────────────────┘         │  - Integration testing       │
                                         └──────────────────────────────┘
```

---

## Why Not the Hypervisor Approach?

TTTech MotionWise and similar hypervisor-based products require hardware virtualization (MMU/SMMU), targeting Cortex-R/A class processors.

| Comparison | Hypervisor Approach | MPU Partitioning (Adopted) |
|------------|--------------------|-----------------------------|
| Required Hardware | Cortex-R/A (MMU) | Cortex-M4/M7 (MPU) |
| Complexity | Very high | Moderate |
| Open-source ecosystem | Limited | FreeRTOS — full ecosystem |
| Automotive MCU fit | Subset of ECUs | Most embedded automotive ECUs |

The primary target of OpenSafetyRTOS is ARM Cortex-M based automotive ECUs — the dominant embedded MCU class in production vehicles. MPU-based partitioning is the correct choice for this target. The hypervisor approach introduces unnecessary complexity and hardware requirements.

---

## Assumptions of Use (Summary)

Integrators must satisfy all AoU items. The single authoritative source of AoU IDs and definitions is the canonical registry **`safety/doc/AOU.md` (OSR-AOU-001)**; the summary below is informative only.

| ID | Assumption |
|----|-----------|
| AoU-01 | Boot order: `safety_boot()` executes before any QM/FreeRTOS code (ADR-004) |
| AoU-02 | External independent hardware watchdog connected to SafetyFunction |
| AoU-03 | All QM→SafetyFunction data transfers use the Mailbox API exclusively |
| AoU-04 | Dedicated independent hardware timer for SafetyFunction; its ISR above all QM interrupts (ADR-005 §D1(c)) |
| AoU-05 | Independent clock source for the safety timer + hardware clock supervision (CSS) enabled |
| AoU-06 | MISRA-C:2012 compliant toolchain at the qualified optimization level |
| AoU-07 | No bus master (DMA etc.) can write the ASIL-D region — the MPU does not gate DMA (ADR-005 §D1(f)) |
| AoU-08 | Power supervision (BOD/PVD) configured per SAFE_STATE_DEFINITION.md |
| AoU-09 | Integrator implements the Level 2 recovery procedure (no automatic recovery) |
| AoU-10 | QM tasks run unprivileged via the FreeRTOS ARM_CM4_MPU port (ADR-005 §D1(a)) |
| AoU-11 | Safe State callbacks registered at initialization (see OSR-AOU-001) |

---

## Distribution Methods

| Method | Status | Notes |
|--------|--------|-------|
| git submodule | Phase 1+ | Default integration method |
| CMake FetchContent | Phase 2+ | CMake project integration |
| Package manager (vcpkg/conan) | Post Phase 5 | After certification complete |

---

## Consequences

**(+)** Open-source philosophy fully preserved — source code and all certification evidence publicly available

**(+)** ISO 26262 SEooC path is a recognized development route (ISO 26262-10:2018 Cl.9) with existing precedent

**(+)** Optimized for Cortex-M embedded automotive ECUs — no hypervisor overhead

**(−)** Integrators are responsible for satisfying AoU — integration guide quality is critical

**(−)** System-level certification remains the integrator's responsibility — OpenSafetyRTOS covers the SEooC element only

---

*Next ADR: ADR-004 — Boot Sequence and Power-On Self-Test*
