# OpenSafetyRTOS — Architecture Design Document

**Version:** 0.2
**Status:** In Progress
**Target Standard:** ISO 26262 ASIL-D / IEC 61508 SIL-3

---

## 1. Project Philosophy

OpenSafetyRTOS is built on an open-source philosophy: create a production-ready, automotive safety-critical RTOS that any engineer can inspect, use, and contribute to — including its certification evidence.

Rather than rewriting FreeRTOS from scratch at ASIL-D, OpenSafetyRTOS adopts an **ISO 26262 Part 9 ASIL Decomposition** strategy. FreeRTOS runs as a QM(D) partition providing all standard RTOS services. An independent SafetyFunction layer, developed to ASIL-D(D), enforces safety monitoring, hardware memory protection, and fault handling above it.

This is the same structural approach used by TTTech (MotionWise: ASIL-D Hypervisor + QM Guest OS) and ETAS (RTA-OS over a QM application layer) — but fully open-source, with all certification evidence public under CC BY 4.0.

---

## 2. Core Design Strategy: ISO 26262 ASIL Decomposition

Per ISO 26262 Part 9, ASIL-D can be achieved by decomposing a system-level requirement into two independent components that together fulfill the requirement:

```
FreeRTOS        QM(D)          Standard RTOS functions (scheduling, IPC, drivers)
SafetyFunction  ASIL-D(D)      Safety monitoring, MPU enforcement, fault handling
─────────────────────────────────────────────────────────────────────────────────
OpenSafetyRTOS  ASIL-D         System-level safety goal
```

**Independence condition**: Freedom From Interference (FFI) between the QM and ASIL-D partitions must be demonstrated — in hardware (ARM MPU) and in software (Mailbox validation). This is the foundational requirement that makes the decomposition valid under ISO 26262.

Reference implementations: TTTech MotionWise (Hypervisor + Guest OS), ETAS RTA-OS (decomposed architecture).

---

## 3. Memory Partitioning and FFI

FFI is the **core design requirement** of this project. The QM partition must be demonstrably unable to interfere with the ASIL-D partition's data, execution, or timing.

### 3.1 Memory Region Definitions

```
┌──────────────────────────────────────────────────────────────────┐
│ Region                │ SafetyFunction Access │ FreeRTOS(QM) Access│
├──────────────────────────────────────────────────────────────────┤
│ ASIL-D Region         │ Read / Write          │ NO ACCESS          │
│ QM Region             │ Read Only             │ Read / Write       │
│ QM→Safety Mailbox     │ Read Only             │ Read / Write       │
└──────────────────────────────────────────────────────────────────┘
```

**Invariant rules:**
- FreeRTOS(QM) **cannot write** to the ASIL-D Region — enforced in hardware by the ARM MPU (violation → HardFault → Safe State Level 3)
- SafetyFunction reads the QM Region for monitoring purposes only; it does not write to QM memory
- All QM→SafetyFunction data transfer uses the dedicated Mailbox region exclusively

### 3.2 Cross-Partition Communication: Mailbox Pattern

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

**SafetyFunction Mailbox receive algorithm (3-stage validation):**

```c
safety_status_t safety_mailbox_receive(mailbox_t *mb, void *out, size_t len)
{
    /* ① CRC integrity check */
    if (crc32(mb->data, mb->data_len) != mb->crc) {
        return SAFETY_ERR_CRC_FAIL;
    }

    /* ② Timestamp freshness check (stale data detection) */
    if ((safety_get_tick() - mb->timestamp) > MAILBOX_MAX_AGE_MS) {
        return SAFETY_ERR_STALE;
    }

    /* ③ Value range check against expected schema */
    if (!safety_range_check(mb->data, mb->data_len, mb->schema)) {
        return SAFETY_ERR_RANGE;
    }

    /* All checks passed — safe to use */
    memcpy(out, mb->data, len);
    return SAFETY_OK;
}
```

SafetyFunction **never implicitly trusts** data from the QM partition. This is the core of software-level FFI achievement.

---

## 4. System Layer Structure

```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                     │
├───────────────────────┬─────────────────────────────────┤
│   FreeRTOS (QM)       │   SafetyFunction (ASIL-D)       │
│  - Task Scheduling    │  - Watchdog Management          │
│  - IPC (Queue/Sem)    │  - MPU Configuration            │
│  - Device Drivers     │  - Task Deadline Monitor        │
│  - Memory Mgmt        │  - Fault Detection & Isolation  │
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
| QM overwrites ASIL-D memory | ARM MPU — hardware block (violation → HardFault → Level 3 reset) |
| QM writes corrupt data to Mailbox | CRC + range validation — software block |
| QM sends stale Mailbox data | Timestamp freshness check |
| QM task starves SafetyFunction | SafetyFunction task holds highest priority (fixed) |
| QM stack overflow corrupts Safety stack | MPU stack guard regions (32 B no-access zones) |

---

## 5.2 Extended FFI Analysis — Non-Memory Shared Resources

Beyond memory, QM and ASIL-D partitions share hardware resources on ARM Cortex-M that require separate FFI analysis:

| Shared Resource | Threat Scenario | Mitigation |
|----------------|----------------|------------|
| Interrupts | QM ISR preempts SafetyFunction task | SafetyFunction ISR at highest priority; BASEPRI masking available |
| NVIC vector table | QM overwrites interrupt vector table | IVT placed in ASIL-D memory region (MPU protected) |
| DMA | QM DMA channel accesses ASIL-D memory | Dedicated ASIL-D DMA channels; QM DMA restricted to QM memory |
| Timer / Clock | QM modifies SafetyFunction clock source | SafetyFunction uses dedicated hardware timer (AoU-04); timer registers MPU-protected |
| Power | Power anomaly causes MPU register loss | Power monitor → Safe State Level 3 |
| Stack pointer | QM stack overflow → SafetyFunction stack corruption | MPU stack guard + dedicated per-partition stack regions |
| SafetyFunction internal fault | SafetyFunction deadlock / crash | Independent hardware Watchdog — missed kick → MCU reset (Level 3) |

> **SafetyFunction Single Point of Failure mitigation:**
> The independent hardware Watchdog is the backstop against SafetyFunction itself becoming a single point of failure. SafetyFunction kicks the Watchdog only during normal operation. Any deadlock, infinite loop, or crash results in a missed kick and MCU-level reset (Safe State Level 3) — independent of all software state.

---

## 6. Safe State Architecture

Three escalating Safe State levels are defined (see `safety/doc/SAFE_STATE_DEFINITION.md`):

| Level | Name | Trigger Examples | System Action |
|-------|------|-----------------|---------------|
| Level 1 | Degraded Operation | Single Mailbox validation failure | Retry; increment fault counter |
| Level 2 | Controlled Stop | N=3 consecutive Mailbox failures; clock anomaly detected | Suspend QM tasks; invoke application callback |
| Level 3 | Emergency Reset | ASIL-D memory violation; SafetyFunction internal fault; power anomaly | Disable interrupts; stop Watchdog kick → MCU reset |

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
| SafetyFunction layer | ASIL-D(D) development: MISRA-C:2012, MC/DC 100%, FMEA, independent V&V (Agent-VnV) and QA review (Agent-QA per ISO 26262 Part 2 Cl.8) |
| FreeRTOS | ISO 26262 Part 8 Cl.12 COTS qualification + pre-existing use evidence |
| FFI argument | This document + MPU configuration correctness + interrupt boundary + stack separation formal argument |
| Mailbox validation | 100% unit test + MC/DC coverage (Agent-VnV, independent of Agent-Safety) |
| Distribution | SEooC per ISO 26262 Part 8 Cl.13 — integrators validate Assumptions of Use |

---

## 9. Open-Source License Strategy

| Component | License |
|-----------|---------|
| FreeRTOS | MIT License (preserved as-is) |
| OpenSafetyRTOS SafetyFunction layer | Apache 2.0 |
| Certification evidence documents | CC BY 4.0 (freely shareable and adaptable with attribution) |

---

*This document is updated as design progresses. All changes are version-controlled in git. ASIL-D relevant sections require Safety Manager approval.*
