# OpenSafetyRTOS — Architecture Design Document

**Version:** 0.3
**Status:** In Progress
**Target Standard:** ISO 26262 ASIL-D (target) / IEC 61508 SIL-3 (target)

---

## 1. Project Philosophy

OpenSafetyRTOS is built on an open-source philosophy: create a production-ready, automotive safety-critical RTOS that any engineer can inspect, use, and contribute to — including its safety evidence.

Rather than rewriting FreeRTOS from scratch at ASIL-D, OpenSafetyRTOS adopts an **ISO 26262 Part 9 ASIL Decomposition** strategy. FreeRTOS runs as a QM(D) partition providing all standard RTOS services. An independent SafetyFunction layer, developed to ASIL-D(D), enforces safety monitoring, hardware memory protection, and fault handling above it.

**What this yields (scope of claim):** a *fail-safe monitor architecture* (E-Gas style). The ASIL-D-rated behaviour is fault detection and safe-state enforcement. FreeRTOS's own services (scheduling, IPC, drivers) remain QM and must not be used as safety mechanisms by ASIL-rated application logic. This differs from a fully ASIL-D-certified services RTOS (e.g., SafeRTOS); see ADR-001.

---

## 2. Core Design Strategy: ISO 26262 ASIL Decomposition

Per ISO 26262-9:2018 Cl.5, the system-level safety requirement is decomposed into redundant requirements allocated to sufficiently independent elements:

```
Safety requirement (ASIL D)
 ├── Monitoring requirement        → SafetyFunction  ASIL D(D)
 │      detect + enforce safe state (watchdog, MPU integrity, fault handling)
 └── Intended-function requirement → FreeRTOS        QM(D)
        standard RTOS services (scheduling, IPC, drivers)
──────────────────────────────────────────────────────────────────────────
 OpenSafetyRTOS system-level safety goal: ASIL D (fail-safe)
```

**Independence condition (ISO 26262-9:2018 Cl.6):** Freedom From Interference (FFI) between the QM and ASIL-D partitions must be demonstrated — spatially (ARM MPU + detective integrity checks, ADR-002/ADR-005), temporally (dedicated safety timer ISR, ADR-005 §D1(c)), and on the communication path (Mailbox validation). This is the foundational requirement that makes the decomposition valid.

---

## 3. Memory Partitioning, Privilege Model, and FFI

FFI is the **core design requirement** of this project. The QM partition must be demonstrably unable to interfere with the ASIL-D partition's data, execution, or timing — or, where hardware prevention is architecturally impossible, interference must be *detected within a bounded, FTTI-budgeted latency*.

### 3.1 Privilege Model (summary of ADR-005)

The ARMv7-M MPU distinguishes only **privileged vs. unprivileged** execution and has **no lock** — any privileged code can rewrite it. The protection model is therefore explicitly split:

| Interference source | Control type | Mechanism |
|---|---|---|
| Unprivileged QM tasks (FreeRTOS ARM_CM4_MPU port, AoU-10) | **Preventive** (hardware) | MPU access permissions; violation → MemManage fault |
| Privileged FreeRTOS kernel fault | **Detective** (bounded latency) | Per-cycle `safety_mpu_integrity_check()` against a CRC-protected shadow + VTOR/IVT CRC check; watchdog kick gated on the checks |
| Non-CPU bus masters (DMA, ...) | Configuration ownership + integrator constraint | SafetyFunction owns DMA configuration; AoU-07 |

SafetyFunction itself is **not a FreeRTOS task**: it runs in the boot context (`safety_boot()`, ADR-004) and thereafter in the **ISR of a dedicated independent hardware timer** (AoU-04) at higher NVIC priority than all QM interrupts — its periodic execution does not depend on the QM scheduler. Residual risks (e.g., the window between integrity checks) are stated openly in ADR-005 §4.

### 3.2 Memory Region Definitions

Access rights are expressed per privilege level (the only distinction the hardware offers):

```
┌────────────────────────────────────────────────────────────────────────┐
│ Area               │ Privileged (SafetyFunction  │ Unprivileged        │
│                    │ + FreeRTOS kernel)          │ (QM tasks)          │
├────────────────────────────────────────────────────────────────────────┤
│ ASIL-D Area        │ Code RO+X / Data RW         │ NO ACCESS           │
│ QM Area            │ R/W (Safety reads only,     │ R/W (+X for code)   │
│                    │  by convention)             │                     │
│ QM→Safety Mailbox  │ R/W (Safety validates       │ R/W                 │
│                    │  before use)                │                     │
└────────────────────────────────────────────────────────────────────────┘
```

**Invariant rules:**
- Unprivileged QM tasks **cannot access** the ASIL-D area — enforced in hardware by the ARM MPU. A violation raises a **MemManage fault** (escalating to HardFault if the MemManage exception is disabled), handled by SafetyFunction → Safe State Level 3.
- The privileged FreeRTOS kernel *architecturally could* write ASIL-D memory or the MPU; this is covered detectively per §3.1 (see ADR-005 for the full argument and residual risk).
- ARMv7-M AP encodings cannot give unprivileged code more rights than privileged code, so "QM writes / Safety read-only" mailbox semantics are a **software convention** (mailbox API + validation), not a hardware property.
- All QM→SafetyFunction data transfer uses the dedicated Mailbox region exclusively (AoU-03).
- The full 8-region MPU allocation (deny-all background, separate QM code/data regions — flash and SRAM cannot share one region — guard regions, peripheral SRD carve-outs) is specified in ADR-002.

### 3.3 Cross-Partition Communication: Mailbox Pattern

When the QM partition must pass data to SafetyFunction, it uses the **Mailbox** mechanism — the only permitted cross-partition channel.

```
 ┌──────────────┐         ┌─────────────────┐         ┌──────────────────┐
 │  FreeRTOS    │  Write  │  QM→Safety      │  Read   │  SafetyFunction  │
 │  (QM)        │────────▶│  Mailbox Region │────────▶│  (ASIL-D)        │
 └──────────────┘         └─────────────────┘         └──────────────────┘
                                                              │
                                              ┌───────────────┤
                                              ▼               ▼
                                        Validation OK    Validation FAIL
                                        Use data         Fault handler
```

**SafetyFunction Mailbox receive algorithm (3-stage validation), simplified illustration** — the actual implementation is `safety_mailbox_receive()` in `kernel/safety/src/mailbox.c`, which additionally snapshots the QM-writable mailbox into ASIL-D memory before validating (TOCTOU defence) and counts consecutive failures:

```c
/* Illustrative only — see kernel/safety/src/mailbox.c for the real code */
status = SAFETY_OK;
if (crc32(snap.data, snap.data_len) != snap.crc) {
    status = SAFETY_ERR_CRC_FAIL;      /* (1) CRC integrity */
} else if ((safety_get_tick() - snap.timestamp) > MAILBOX_MAX_AGE_MS) {
    status = SAFETY_ERR_STALE;         /* (2) Freshness check */
} else if (!safety_range_check(snap.data, snap.data_len, snap.schema)) {
    status = SAFETY_ERR_RANGE;         /* (3) Value range check */
} else {
    copy_to_asil_d(out, snap.data, snap.data_len);  /* safe to use */
}
```

SafetyFunction **never implicitly trusts** data from the QM partition. Three consecutive validation failures escalate to Safe State Level 2 (`SAFETY_MAILBOX_FAILURE_LIMIT`, SSR-017).

---

## 4. System Layer Structure

```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                     │
├───────────────────────┬─────────────────────────────────┤
│   FreeRTOS (QM)       │   SafetyFunction (ASIL-D)       │
│  - Task Scheduling    │   [runs in dedicated safety     │
│  - IPC (Queue/Sem)    │    timer ISR — not a task]      │
│  - Device Drivers     │  - Watchdog Kick Policy         │
│  - Memory Mgmt        │  - MPU Config + Integrity Check │
│  [kernel privileged;  │  - VTOR / IVT CRC Check         │
│   tasks unprivileged, │  - Task Deadline Monitor        │
│   ARM_CM4_MPU port]   │  - Fault Detection & Isolation  │
│                       │  - Safe State Manager (FSM)     │
│                       │  - Mailbox Validation           │
│                       │  - Clock Monitor                │
│                       │  - Boot Self-Test               │
├───────────────────────┴─────────────────────────────────┤
│              HAL (Hardware Abstraction Layer)            │
├─────────────────────────────────────────────────────────┤
│          ARM Cortex-M (Primary Target: M4/M7 + MPU)     │
└─────────────────────────────────────────────────────────┘
```

---

## 5. FFI Threat Analysis — Memory Threats

| Threat Scenario | Mitigation Mechanism |
|----------------|---------------------|
| Unprivileged QM task overwrites ASIL-D memory | ARM MPU — hardware block (violation → MemManage fault → Level 3) |
| Privileged QM kernel fault overwrites ASIL-D memory / MPU registers | Detective: per-cycle MPU integrity check vs. CRC shadow + RAM guard-word checks; detection ≤ 1 safety cycle, FTTI-budgeted (ADR-005) |
| QM writes corrupt data to Mailbox | CRC + range validation — software block |
| QM sends stale Mailbox data | Timestamp freshness check |
| QM starves SafetyFunction of CPU time | SafetyFunction runs in dedicated timer ISR above all QM interrupt priorities (AoU-04) — no scheduler dependency |
| QM stack overflow corrupts Safety stack | MPU stack guard regions (32 B no-access zones) + deny-all background gap |

---

## 5.2 Extended FFI Analysis — Non-Memory Shared Resources

Beyond memory, QM and ASIL-D partitions share hardware resources on ARM Cortex-M that require separate FFI analysis:

| Shared Resource | Threat Scenario | Mitigation |
|----------------|----------------|------------|
| Interrupts | QM ISR preempts / delays the SafetyFunction cycle | Safety timer ISR at NVIC priority above all QM interrupts and above `configMAX_SYSCALL_INTERRUPT_PRIORITY` (AoU-04); unprivileged code cannot touch the NVIC |
| NVIC / VTOR / vector table | Privileged QM code retargets VTOR or overwrites vectors | **The SCS (NVIC/SCB/VTOR) cannot be MPU-protected from privileged code.** Mitigation is detective: per-cycle VTOR readback + CRC-32 over the vector table (in ASIL-D flash); mismatch → Level 3 (ADR-005 §D1(e)) |
| DMA | A DMA channel writes into ASIL-D memory | **The MPU does NOT gate DMA (or any non-CPU bus master).** Mitigation: SafetyFunction owns the DMA controller configuration and validates every QM transfer request against the ASIL-D address ranges; DMA register window privileged-only; integrator obligation AoU-07 for all other bus masters; detective RAM pattern (guard-word) checks each cycle (ADR-005 §D1(f)) |
| Timer / Clock | QM modifies SafetyFunction clock source | SafetyFunction uses a dedicated independent hardware timer (AoU-04) on an independent clock source with hardware clock supervision (AoU-05); timer register window privileged-only |
| Power | Power anomaly causes MPU register loss | Power monitor (BOD/PVD, AoU-08) → Safe State Level 3; MPU integrity check as detective backup |
| Stack pointer | QM stack overflow → SafetyFunction stack corruption | MPU stack guard + dedicated per-partition stack regions + deny-all background |
| SafetyFunction internal fault | SafetyFunction deadlock / crash | Independent hardware Watchdog (AoU-02) — missed kick → MCU reset (Level 3 backstop) |

> **SafetyFunction Single Point of Failure mitigation:**
> The independent hardware Watchdog is the backstop against SafetyFunction itself becoming a single point of failure. The kick policy is checkpoint-gated (`kernel/safety/src/watchdog.c`): the watchdog is kicked only if *all* monitored checks (mailbox, MPU integrity, clock, ...) completed successfully in the cycle. Any deadlock, suppressed safety ISR, or corrupted check chain results in a missed kick and MCU-level reset — independent of all software state.

---

## 6. Safe State Architecture

Three escalating Safe State levels are defined (see `safety/doc/SAFE_STATE_DEFINITION.md`; implementation: `kernel/safety/src/safe_state.c`):

| Level | Name | Trigger Examples | System Action |
|-------|------|-----------------|---------------|
| Level 1 | Degraded Operation | Single Mailbox validation failure | Retry; increment fault counter (3 consecutive → Level 2) |
| Level 2 | Controlled Stop | 3 consecutive Mailbox failures; clock anomaly detected; Level-1 fault limit | Suspend QM tasks; invoke application callback (AoU-11); recovery only via integrator procedure (AoU-09) |
| Level 3 | Emergency Reset | ASIL-D memory violation; MPU/VTOR integrity mismatch; SafetyFunction internal fault; power anomaly | **Immediate software reset via AIRCR.SYSRESETREQ**; watchdog-kick starvation retained as backstop only (ADR-004 §4.4 — removes up to 100 ms from the FTTI budget) |

The safe-state level is monotonic at runtime (no silent de-escalation); a repeated Level-2 trigger while already stopped escalates to Level 3.

---

## 7. Target Platforms

| Platform | Status | Notes |
|----------|--------|-------|
| ARM Cortex-M4 | Primary | MPU 8 regions, FPU |
| ARM Cortex-M7 | Primary | MPU 16 regions, Cache |
| RISC-V | Future | PMP (Physical Memory Protection) |

---

## 8. Certification Strategy

| Component | Approach |
|-----------|---------|
| SafetyFunction layer | ASIL-D(D) development: MISRA-C:2012, MC/DC 100%, FMEA, independent V&V (Agent-VnV) and QA review (Agent-QA per ISO 26262 Part 2 Cl.8) — see docs/TEAM.md for the documented independence limitations of AI-agent review |
| FreeRTOS | ISO 26262 Part 8 Cl.12 COTS qualification + pre-existing use evidence |
| FFI argument | This document + ADR-005 (preventive/detective model, residual risks) + MPU configuration correctness + interrupt boundary + stack separation argument |
| Mailbox validation | 100% unit test + MC/DC coverage (Agent-VnV, independent of Agent-Safety) |
| Distribution | SEooC per ISO 26262-10:2018 Cl.9 — integrators validate the Assumptions of Use (canonical registry: safety/doc/AOU.md, OSR-AOU-001) |

---

## 9. Open-Source License Strategy

| Component | License |
|-----------|---------|
| FreeRTOS | MIT License (preserved as-is) |
| OpenSafetyRTOS SafetyFunction layer | Apache 2.0 |
| Safety evidence documents | CC BY 4.0 (freely shareable and adaptable with attribution) |

---

*This document is updated as design progresses. All changes are version-controlled in git. ASIL-D relevant sections require Safety Manager approval.*
