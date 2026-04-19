# ADR-001: Adopt ISO 26262 Part 9 ASIL Decomposition Strategy

**Date:** 2026-04-18
**Status:** Accepted
**Deciders:** Project Architecture Team
**Related:** ADR-002 (MPU Partition Strategy), ARCHITECTURE.md §2

---

## Context

To build an open-source RTOS that satisfies ISO 26262 ASIL-D, we need a strategy that achieves the certification goal without the prohibitive cost of rewriting an entire kernel from scratch at ASIL-D. The FreeRTOS ecosystem — including its drivers, ports, and community — is a proven asset that should be leveraged rather than discarded.

---

## Decision

**Adopt the ISO 26262 Part 9 ASIL Decomposition strategy.**

```
FreeRTOS        QM(D)      — Standard RTOS functions: scheduling, IPC, drivers
SafetyFunction  ASIL-D(D)  — Safety monitoring, MPU enforcement, fault handling
──────────────────────────────────────────────────────────────────────────────
OpenSafetyRTOS  ASIL-D     — System-level safety goal
```

- **FreeRTOS** operates as the QM(D) partition. It is used as a COTS component under ISO 26262 Part 8 Cl.12 qualification with pre-existing use evidence.
- **SafetyFunction** operates as the ASIL-D(D) partition. It monitors the system, enforces hardware memory protection, manages the watchdog, and handles all fault responses.
- The combination satisfies the system-level ASIL-D requirement through demonstrated FFI.

### Cross-Partition Communication Rules

1. QM → ASIL-D direct write: **prohibited** — enforced in hardware by the ARM MPU
2. When QM must pass data to SafetyFunction: **Mailbox pattern only**
3. On every Mailbox receive, SafetyFunction performs mandatory 3-stage validation: CRC-32 → timestamp freshness → value range check

---

## Rationale

- The FreeRTOS ecosystem (drivers, BSPs, community, toolchain support) is reused without modification.
- Only the SafetyFunction layer requires ASIL-D development — significantly reducing certification scope and cost.
- The same architecture is used in production by TTTech (MotionWise: ASIL-D hypervisor + QM guest OS) and ETAS (RTA-OS over QM application layer). Certification body precedent exists.
- Publishing SafetyFunction as open-source with full certification evidence enables industry-wide contribution and reuse — a unique position no commercial RTOS occupies.

---

## Rejected Alternatives

| Alternative | Reason for Rejection |
|-------------|---------------------|
| Rewrite entire kernel at ASIL-D | Enormous cost and timeline; abandons the FreeRTOS ecosystem |
| Purchase SafeRTOS (commercial) | Violates open-source philosophy; closed source; licensing cost |
| Adopt AUTOSAR OS | Excessive complexity; no open-source ecosystem for embedded M4/M7 targets |

---

## Consequences

**(+)** FreeRTOS ecosystem fully preserved — drivers, community, tooling all carry over

**(+)** Certification precedent from TTTech and ETAS increases acceptance by certification bodies (TÜV SÜD, etc.)

**(+)** SafetyFunction scope is well-bounded — reviewable, testable, certifiable independently

**(−)** FFI documentation and proof is the critical challenge — must be tackled from project inception (see ARCHITECTURE.md §5, §5.2)

**(−)** Mailbox validation logic requires ASIL-D development (MISRA-C, MC/DC 100%) — adds implementation burden but is well-scoped

---

*Next ADR: ADR-002 — MPU Configuration and Partition Strategy*
