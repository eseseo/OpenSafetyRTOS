# ADR-003: Distribution Strategy — SEooC (Safety Element out of Context)

**Date:** 2026-04-18
**Status:** Accepted
**Deciders:** PM/PO
**Reference Standard:** ISO 26262 Part 8 Cl.13

---

## Context

OpenSafetyRTOS is an open-source project. Its distribution model must simultaneously satisfy the open-source philosophy and ISO 26262 certification requirements for integrators.

Three options were evaluated:

1. Source code distribution only (FreeRTOS model)
2. **Source code + SEooC certification package** ← Adopted
3. Hypervisor-based distribution (TTTech MotionWise model)

---

## Decision

**Develop and distribute the SafetyFunction layer as a SEooC (Safety Element out of Context) per ISO 26262 Part 8 Cl.13.**

```
OpenSafetyRTOS distribution package
├── kernel/freertos/        Source (MIT License)          — FreeRTOS as-is
├── kernel/safety/          Source (Apache 2.0)           — SafetyFunction SEooC
├── safety/doc/             Certification evidence (CC BY 4.0) — HARA, FSC, SSRS, etc.
├── docs/ASSUMPTION_OF_USE.md   SEooC Assumptions of Use
└── docs/INTEGRATION_GUIDE.md   Integrator guide
```

---

## SEooC Concept

Per ISO 26262 Part 8 Cl.13, SafetyFunction is a safety element developed and validated independently of any specific end-system context. Integrators can embed it in their own systems provided they satisfy the Assumptions of Use (AoU).

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

Integrators must satisfy all AoU items. Full details: `docs/ASSUMPTION_OF_USE.md`.

| ID | Assumption | Description |
|----|-----------|-------------|
| AoU-01 | MPU initialization | Call `safety_mpu_init()` first during system boot — before any other initialization |
| AoU-02 | Task priority | SafetyFunction task must hold the highest priority in the system |
| AoU-03 | Mailbox exclusive use | All QM→SafetyFunction data transfers must use the Mailbox API only |
| AoU-04 | Clock / Timer | Provide a dedicated independent hardware timer source to SafetyFunction |
| AoU-05 | Watchdog | Connect an external hardware Watchdog to SafetyFunction |
| AoU-06 | Compiler | Use a MISRA-C:2012 compliant compiler at the optimization level specified in the integration guide |

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

**(+)** ISO 26262 SEooC path is a recognized certification route (Part 8 Cl.13) with existing precedent

**(+)** Optimized for Cortex-M embedded automotive ECUs — no hypervisor overhead

**(−)** Integrators are responsible for satisfying AoU — integration guide quality is critical

**(−)** System-level certification remains the integrator's responsibility — OpenSafetyRTOS covers the SEooC element only

---

*Next ADR: ADR-004 — Boot Sequence and Power-On Self-Test*
