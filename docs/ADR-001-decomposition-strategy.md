# ADR-001: Adopt ISO 26262 Part 9 ASIL Decomposition Strategy

**Date:** 2026-04-18 (revised 2026-07-02)
**Status:** Accepted
**Deciders:** Project Architecture Team
**Related:** ADR-002 (MPU Partition Strategy), ADR-005 (Privilege Model), ARCHITECTURE.md §2

---

## Context

To build an open-source RTOS that satisfies ISO 26262 ASIL-D, we need a strategy that achieves the certification goal without the prohibitive cost of rewriting an entire kernel from scratch at ASIL-D. The FreeRTOS ecosystem — including its drivers, ports, and community — is a proven asset that should be leveraged rather than discarded.

---

## Decision

**Adopt the ISO 26262 Part 9 ASIL Decomposition strategy.**

Per ISO 26262-9:2018 Cl.5, the system-level ASIL-D safety requirement is decomposed into two redundant requirements allocated to sufficiently independent elements:

```
Safety requirement (ASIL D)
 ├── Monitoring requirement          → SafetyFunction   ASIL D(D)
 │     detect unsafe behaviour of the intended function and enforce
 │     the safe state (watchdog, MPU integrity, fault handling)
 └── Intended-function requirement   → FreeRTOS         QM(D)
       provide standard RTOS services (scheduling, IPC, drivers)
─────────────────────────────────────────────────────────────────────
 OpenSafetyRTOS system-level safety goal: ASIL D (fail-safe)
```

- **FreeRTOS** carries the intended-function requirement at QM(D). It is used as a COTS component under ISO 26262 Part 8 Cl.12 qualification with pre-existing use evidence.
- **SafetyFunction** carries the monitoring requirement at ASIL D(D). It monitors the system, owns the hardware memory protection configuration, manages the watchdog, and handles all fault responses. The safe-state enforcement (detect → controlled stop / reset) must be achievable by SafetyFunction **alone**, independent of FreeRTOS behaving correctly.
- The decomposition is valid **only if** sufficient independence — Freedom From Interference per ISO 26262-9:2018 Cl.6 — is demonstrated between the two elements (spatial: ADR-002/ADR-005; temporal: dedicated safety timer ISR, ADR-005 §D1(c); communication: mailbox validation).

**Scope of the resulting claim (important):** this architecture yields a *fail-safe monitor* system (E-Gas-style monitoring architecture). ASIL-D-rated behaviour is the detection of faults and enforcement of the safe state. FreeRTOS's RTOS services themselves remain QM: ASIL-D application logic must not rely on FreeRTOS scheduling/IPC as a safety mechanism, and fail-operational goals are out of scope. This differs from a fully ASIL-D-certified services RTOS (e.g., SafeRTOS), which provides ASIL-rated kernel services.

### Cross-Partition Communication Rules

1. QM → ASIL-D direct write: **prohibited** — enforced in hardware by the ARM MPU against unprivileged QM tasks (AoU-10), and detected within one safety cycle for privileged-kernel faults (ADR-005)
2. When QM must pass data to SafetyFunction: **Mailbox pattern only** (AoU-03)
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
