# OpenSafetyRTOS — Failure Mode and Effects Analysis (FMEA)

**Document ID:** OSR-FMEA-001
**Version:** 0.1 (Draft)
**Status:** In Progress
**Date:** 2026-04-18
**Standard:** ISO 26262-9, ISO 26262-6 Annex B
**Scope:** SafetyFunction partition (ASIL-D(D)) and QM/Safety partition boundary

---

## 1. Purpose

This FMEA identifies failure modes of OpenSafetyRTOS components, assesses their effects on system safety, and documents the detection and mitigation measures that justify the ASIL-D safety claim. It is a living document updated throughout development.

Each row is traceable to one or more Software Safety Requirements (SSR) in OSR-SSRS-001 and to test evidence in the unit and integration test reports.

---

## 2. Severity Classification

| Severity Level | Definition |
|---|---|
| S3 — Catastrophic | Loss of life or severe injury possible |
| S2 — Serious | Serious injury possible |
| S1 — Minor | Minor injury possible |
| S0 — No injury | No safety consequence |

---

## 3. FMEA Table

| ID | Component | Failure Mode | Effect on System | Severity | Cause | Detection Method | Mitigation | ASIL | Status |
|---|---|---|---|---|---|---|---|---|---|
| FMEA-001 | QM→Safety Mailbox | QM partition writes corrupt data to Mailbox (bit flip, overrun, or malicious write) | SafetyFunction receives incorrect data; safety decision based on bad input could fail to detect a real fault or issue false commands | S3 | Memory corruption in QM partition; software bug in QM mailbox writer; transient hardware fault (SEU) | CRC-32 check on every Mailbox read by SafetyFunction (software detection); CRC mismatch triggers fault handler | (1) SafetyFunction validates CRC before any use of Mailbox data — see `safety_mailbox_receive()` in ARCHITECTURE.md; (2) On CRC failure, SafetyFunction logs the event and enters fault handling; (3) Mailbox schema includes data length to prevent overrun; (4) MISRA-C compliant implementation with 100% MC/DC test coverage of validation path | ASIL-D | Open — implementation pending |
| FMEA-002 | ARM MPU Configuration | MPU misconfiguration allows QM partition to write to ASIL-D memory region | QM partition can directly overwrite SafetyFunction variables, stack, or code; FFI is broken; safety monitoring is silently corrupted | S3 | Software bug in MPU initialization sequence; incorrect region base/size/attribute encoding; MPU disabled accidentally during system init or after exception return | (1) MPU HardFault triggers immediately on any unauthorized access attempt — hardware detection; (2) SafetyFunction self-test at boot verifies MPU regions by intentionally probing boundaries and confirming faults; (3) MPU configuration locked after init (PRIVDEFENA=0, region attributes set before enabling) | (1) MPU region assignments defined in ADR-002 and verified against linker script at build time; (2) Boot-time MPU self-test (`safety_mpu_selftest()`) probes each partition boundary and confirms HardFault response; (3) HardFault handler owned by SafetyFunction and cannot be overridden by QM; (4) Static analysis verifies MPU init sequence; (5) Integration test exercises each region boundary | ASIL-D | Open — design in progress |
| FMEA-003 | FreeRTOS Task Scheduler | QM tasks monopolize CPU, causing SafetyFunction task starvation | SafetyFunction watchdog kick, deadline monitoring, and fault detection are delayed beyond fault-tolerant time interval; watchdog may expire without response; genuine faults go undetected | S3 | QM task assigned priority equal to or higher than SafetyFunction; runaway loop in QM task disables interrupts; scheduler bug in FreeRTOS | (1) FreeRTOS tick interrupt remains active — SafetyFunction runs on tick interrupt if configured; (2) Hardware watchdog expires if SafetyFunction cannot kick it, triggering MCU reset; (3) SafetyFunction tasks assigned highest fixed priority, never lowerable by QM | (1) SafetyFunction tasks assigned highest priority level in FreeRTOS priority table — this is a system invariant enforced by CODEOWNERS and code review; (2) Hardware watchdog with independent clock source (IWDG on STM32) as second line of defence; (3) SafetyFunction task deadline monitor checks its own execution period and escalates if missed; (4) QM tasks are forbidden from calling `taskENTER_CRITICAL()` for unbounded durations — enforced by code review policy | ASIL-D | Open — priority policy pending |
| FMEA-004 | Hardware Watchdog | Watchdog not kicked because SafetyFunction task is deadlocked or indefinitely blocked | MCU does not reset; system remains in a partially failed state; downstream actuators receive stale or no commands; safety goal violation if vehicle assumes RTOS is healthy | S3 | SafetyFunction task blocked on a semaphore or mutex held by a failed QM task; deadlock in inter-partition communication; stack overflow in SafetyFunction task causing control flow corruption | (1) Hardware watchdog reset triggers MCU reset — hardware detection with no software dependency; (2) SafetyFunction watchdog kick is in a dedicated highest-priority task with no blocking calls and no shared mutexes with QM | (1) SafetyFunction watchdog task uses only non-blocking operations; no `xSemaphoreTake()` with finite timeout on QM-owned resources; (2) Watchdog window (WWDG) configured: kick too late OR too early both trigger reset, preventing a stuck loop that happens to kick at the right time; (3) SafetyFunction stack protected by MPU stack guard region (FMEA-005); (4) Unit test verifies watchdog task has no blocking call path; (5) Fault injection test: block SafetyFunction task and verify MCU resets within watchdog timeout | ASIL-D | Open — implementation pending |
| FMEA-005 | FreeRTOS Stack Management | QM task stack overflow crosses partition boundary into SafetyFunction memory region | QM stack data overwrites SafetyFunction variables or return addresses; SafetyFunction behaviour becomes undefined; safety monitoring silently corrupted | S3 | QM task stack sized too small for actual call depth; stack growth direction not accounted for in linker layout; no stack guard between QM and SafetyFunction stacks | (1) MPU stack guard region between QM and SafetyFunction stack areas — hardware detection; any overflow into guard region triggers MemManage fault immediately; (2) FreeRTOS stack overflow hook (`vApplicationStackOverflowHook`) as software-level secondary detection | (1) Linker script places an MPU-protected stack guard region (minimum 32 bytes, aligned) between QM stack area and SafetyFunction memory region — enforced in `arch/` linker script and verified by ADR-002; (2) MPU region for guard area: no-access for all (neither QM nor Safety can read/write it); (3) MemManage fault handler owned by SafetyFunction and unconditionally escalates to safe state; (4) QM stack sizes reviewed with worst-case analysis; (5) Integration test intentionally overflows a QM task stack and verifies MemManage fault response and safe state entry | ASIL-D | Open — linker script pending |

---

## 4. Template: Adding New FMEA Entries

Copy and complete the following template when adding new failure modes.

```
| FMEA-NNN | <Component> | <Failure Mode description> | <Effect on system and safety goal> | S0/S1/S2/S3 | <Root cause(s)> | <How the failure is detected — hardware or software> | <What prevents or mitigates the failure, referencing design artifacts and tests> | ASIL-A/B/C/D | Open / In Progress / Closed |
```

**Guidelines for FMEA entries:**
- ID must be unique and monotonically increasing
- Failure Mode must describe what goes wrong, not why
- Effect must describe the safety consequence at system level
- Detection Method must be independently verifiable (test reference required before status can be set to Closed)
- Mitigation must reference at least one design artifact (ADR, ARCHITECTURE.md section, source file) and one test
- ASIL assignment follows from the safety goal; all items in this FMEA default to ASIL-D unless decomposition applies
- Status moves to Closed only after the detection method and mitigation are implemented and test evidence is archived

---

## 5. Coverage Matrix

| SSR ID | FMEA Item(s) | Design Reference | Test Reference | Status |
|---|---|---|---|---|
| SSR-010 (Mailbox integrity) | FMEA-001 | ARCHITECTURE.md §3.2 | TBD | Open |
| SSR-011 (MPU enforcement) | FMEA-002 | ADR-002 | TBD | Open |
| SSR-012 (Priority enforcement) | FMEA-003 | ARCHITECTURE.md §5 | TBD | Open |
| SSR-013 (Watchdog liveness) | FMEA-004 | TBD | TBD | Open |
| SSR-014 (Stack isolation) | FMEA-005 | ADR-002 §Stack Guards | TBD | Open |

---

## 6. Revision History

| Version | Date | Author | Changes |
|---|---|---|---|
| 0.1 | 2026-04-18 | Safety Engineer | Initial draft with 5 seed entries |

---

*All FMEA entries must be reviewed and approved by @safety-team before status can be set to Closed. The FMEA is part of the certification evidence package and is subject to ISA review.*
