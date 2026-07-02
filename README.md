# OpenSafetyRTOS

**An open-source automotive RTOS project developing toward ISO 26262 ASIL-D — with all safety evidence written and reviewed in public.**

![Build Status](https://img.shields.io/badge/build-in%20progress-yellow)
![License](https://img.shields.io/badge/license-Apache%202.0%20%2F%20MIT%20%2F%20CC%20BY%204.0-blue)
![ASIL](https://img.shields.io/badge/ASIL--D-target-orange)
![Standard](https://img.shields.io/badge/ISO%2026262-Part%202%2F6%2F8%2F9%2F10-orange)
![SEooC](https://img.shields.io/badge/SEooC-ISO%2026262--10%20Cl.9-purple)

> **Status:** Active development — Phase 1 (Safety Analysis) in progress.
> Safety evidence documents (HARA, FSC, SSRS) are being written and reviewed in public.
> The first SafetyFunction modules (CRC-32, mailbox validation, safe-state manager,
> watchdog kick policy, MPU shadow/integrity checking, fault handlers) are implemented
> and host-unit-tested. **Nothing here is certified**: no assessment by a certification
> body has taken place, and no hardware bring-up has been completed yet.

---

## What Is OpenSafetyRTOS?

OpenSafetyRTOS is an open-source real-time operating system project for **automotive safety-critical embedded systems**, targeting ISO 26262 ASIL-D compliance on ARM Cortex-M4/M7 microcontrollers.

The key idea: instead of rewriting an entire RTOS from scratch under ASIL-D — an enormously expensive undertaking — OpenSafetyRTOS applies the **ISO 26262 Part 9 ASIL Decomposition** strategy:

```
FreeRTOS        QM(D)      — Intended function: scheduling, IPC, drivers
SafetyFunction  ASIL-D(D)  — Monitor: safety checks, MPU integrity, fault handling
──────────────────────────────────────────────────────────────────────────────
OpenSafetyRTOS  ASIL-D (target) — Combined system-level safety goal (fail-safe)
```

FreeRTOS does what it has always done. SafetyFunction, developed independently to ASIL-D(D), enforces the safety shell around it. **Freedom From Interference (FFI)** between the two partitions — hardware-enforced against unprivileged QM tasks via the ARM MPU, and *detected within a bounded latency* for privileged-kernel faults (see [ADR-005](docs/ADR-005-privilege-and-mpu-integrity.md)) — is what makes the decomposition valid under ISO 26262.

**Scope of the claim — read this before comparing products:** this is a **safety-monitor architecture (E-Gas style)**. The ASIL-D-rated behaviour is *detecting faults and enforcing a safe state* (degraded operation → controlled stop → reset). FreeRTOS's RTOS services themselves remain QM: ASIL-rated application logic cannot treat FreeRTOS scheduling or IPC as a safety mechanism. That is a different (and more modest) claim than a fully ASIL-D-certified services RTOS such as SafeRTOS or INTEGRITY, which provide ASIL-rated kernel services. See [ADR-001](docs/ADR-001-decomposition-strategy.md).

---

## Why OpenSafetyRTOS?

Existing ASIL-D RTOS and safety-OS solutions are closed-source, commercially licensed, or ship without their certification evidence. OpenSafetyRTOS aims to change that by developing the element *and its safety evidence* in the open.

### Comparison with Commercial Solutions

> **Honesty note:** every OpenSafetyRTOS value below is a **target**, not an achieved
> certification. The commercial products listed hold actual certificates today.
> Architecturally, OpenSafetyRTOS is a *safety monitor over a QM kernel*, not an
> ASIL-D-services OS — the "ASIL level" rows are not like-for-like (see note above).

| | TTTech MotionWise | ETAS RTA-OS | Green Hills INTEGRITY | QNX Neutrino | **OpenSafetyRTOS** |
|---|---|---|---|---|---|
| **ASIL level** | ASIL-D (certified) | ASIL-D (certified) | ASIL-D (certified) | ASIL-D (certified) | **ASIL-D (target, uncertified)** |
| **Architecture class** | Hypervisor platform | ASIL OS services | ASIL OS services | ASIL OS services | **Safety monitor over QM kernel (E-Gas style)** |
| **ISO 26262 strategy** | Hypervisor (ASIL-D + Guest QM) | Full OS certification | Full OS certification | Full OS certification | **Decomposition (Part 9)** |
| **Source code** | Closed | Closed | Closed | Closed | **Open (Apache 2.0)** |
| **Safety evidence** | Private | Private | Private | Private | **Public (CC BY 4.0), in progress** |
| **HARA / FSC / SSRS** | Not disclosed | Not disclosed | Not disclosed | Not disclosed | **Fully public** |
| **FreeRTOS compatible** | No | No | No | No | **Yes — planned (MIT)** |
| **Target hardware** | Cortex-R/A (MMU) | Any | Any | Any | **Cortex-M4/M7 (MPU)** |
| **Cost** | Commercial license | Commercial license | Commercial license | Commercial license | **Free** |

### The Open Safety Evidence Approach

The safety evidence documents — HARA, FSC, SSRS, FMEA, and the Safety Case — are published under CC BY 4.0 as they are written. This means:

- **Integrators** can review exactly what was analyzed, what assumptions were made, and what risks remain.
- **Researchers and educators** can use real-world ISO 26262 work products as learning material.
- **Community contributors** can improve the safety arguments, not just the code.
- **Audit trails** are transparent — every review record and QA decision is on GitHub.

**Review-independence disclosure:** the confirmation reviews in this repository are performed by an **AI QA agent** (Agent-QA), with authorship/verification separation between AI agents (Agent-Safety / Agent-VnV / Agent-QA). ISO 26262-2 confirmation measures require independence of *persons/organizations*; AI-agent review does not satisfy the I1–I3 independence classes by itself. The limitations and the plan for independent human review are documented in [docs/TEAM.md](docs/TEAM.md).

### vs. FreeRTOS / SafeRTOS

| | FreeRTOS | SafeRTOS | **OpenSafetyRTOS** |
|---|---|---|---|
| ASIL rating | QM | ASIL-D (certified) | **ASIL-D (target, uncertified)** |
| What is ASIL-rated | — | Kernel services | **Safety monitor + safe-state enforcement only** |
| Open source | ✅ | ❌ | **✅** |
| FreeRTOS ecosystem | ✅ | ❌ (full rewrite) | **✅ (planned integration)** |
| ISO 26262 strategy | N/A | Full re-certification | **Decomposition (Part 9)** |
| MPU-enforced FFI | ❌ | ✅ | **✅ against unprivileged tasks; detective for privileged faults (ADR-005)** |
| Safety evidence public | ❌ | ❌ | **✅** |
| Cost | Free | Commercial | **Free** |

SafeRTOS achieves ASIL-D by completely rewriting FreeRTOS and certifying its kernel services. OpenSafetyRTOS pursues a different, narrower goal: keep FreeRTOS as-is and add an independently developed ASIL-D(D) *monitor* that guarantees the system reaches a safe state when anything goes wrong.

---

## Core Design: FFI via MPU + Mailbox

### Memory Partitioning and Privilege Model

The ARMv7-M MPU distinguishes privileged vs. unprivileged execution (and has no configuration lock), so the protection model is explicitly two-tier ([ADR-005](docs/ADR-005-privilege-and-mpu-integrity.md)):

```
┌────────────────────────────────────────────────────────────────────┐
│ Region             │ Privileged (Safety + kernel) │ Unpriv. QM task│
├────────────────────────────────────────────────────────────────────┤
│ ASIL-D Region      │ Code RO+X / Data RW          │ NO ACCESS      │
│ QM Region          │ R/W (Safety reads only)      │ R/W (+X code)  │
│ QM→Safety Mailbox  │ R/W (validated before use)   │ R/W            │
└────────────────────────────────────────────────────────────────────┘
```

- **Preventive:** unprivileged QM tasks (FreeRTOS ARM_CM4_MPU port, AoU-10) cannot touch ASIL-D memory — a violation raises a MemManage fault and the system enters Safe State Level 3.
- **Detective:** the privileged FreeRTOS kernel could architecturally rewrite the MPU; every safety cycle `safety_mpu_integrity_check()` compares the live MPU registers to a CRC-protected shadow and re-initializes on mismatch. Detection latency is bounded by the safety cycle and budgeted in the FTTI.
- **DMA is not gated by the MPU:** SafetyFunction owns DMA configuration; integrators must ensure no bus master can write ASIL-D memory (AoU-07).

### Mailbox: Controlled Cross-Partition Communication

When QM must pass data to SafetyFunction, it writes to a dedicated Mailbox region. SafetyFunction never blindly trusts that data — three-stage validation (CRC-32 → freshness → range), implemented in [`kernel/safety/src/mailbox.c`](kernel/safety/src/mailbox.c):

```c
/* Illustrative sketch — the real implementation additionally snapshots the
 * QM-writable mailbox into ASIL-D memory before validating (TOCTOU defence)
 * and tracks consecutive failures. See kernel/safety/src/mailbox.c. */
if (crc32(mb->data, mb->data_len) != mb->crc)
    return SAFETY_ERR_CRC_FAIL;         /* (1) CRC integrity   */
if ((safety_get_tick() - mb->timestamp) > MAILBOX_MAX_AGE_MS)
    return SAFETY_ERR_STALE;            /* (2) Freshness check */
if (!safety_range_check(mb->data, mb->data_len, mb->schema))
    return SAFETY_ERR_RANGE;            /* (3) Range check     */
```

Three consecutive validation failures escalate to Safe State Level 2 (Controlled Stop).

---

## Safe State Architecture

| Level | Name | Action |
|-------|------|--------|
| Level 1 | Degraded Operation | Retry with fault counter increment (3 consecutive → Level 2) |
| Level 2 | Controlled Stop | Suspend QM tasks, notify application (recovery only via integrator procedure, AoU-09) |
| Level 3 | Emergency Reset | **Immediate software reset via AIRCR.SYSRESETREQ**; watchdog-kick starvation retained as backstop only |

Level 3 uses an immediate software reset rather than waiting for watchdog expiry — this removes up to 100 ms of reaction latency from the FTTI budget ([ADR-004 §4.4](docs/ADR-004-boot-sequence.md)). The independent hardware watchdog (AoU-02) still monitors SafetyFunction itself: kicks are checkpoint-gated (kick only when all monitored checks passed in the cycle), so if SafetyFunction hangs, the system resets regardless of software state.

---

## Distribution: SEooC (Safety Element out of Context)

OpenSafetyRTOS is developed as a **SEooC** per ISO 26262-10:2018 Cl.9. This means:

- SafetyFunction is developed **independently** of any specific end-system, against assumed requirements.
- Integrators receive the full source code, all safety evidence documents, and a set of **Assumptions of Use (AoU)** that must be satisfied in the target system.
- Integrators are responsible for validating AoU compliance and performing system-level HARA for their specific application.

### Assumptions of Use (Summary)

Canonical registry (authoritative): [`safety/doc/AOU.md`](safety/doc/AOU.md) (OSR-AOU-001).

| AoU ID | Requirement |
|--------|-------------|
| AoU-01 | Boot order: `safety_boot()` runs before any QM/FreeRTOS code |
| AoU-02 | External independent hardware watchdog connected to SafetyFunction |
| AoU-03 | All QM→SafetyFunction data transfers use the Mailbox API exclusively |
| AoU-04 | Dedicated independent hardware timer for SafetyFunction; ISR above all QM interrupts |
| AoU-05 | Independent clock source + hardware clock supervision enabled |
| AoU-06 | MISRA-C:2012 compliant toolchain at the qualified optimization level |
| AoU-07 | No bus master (DMA etc.) can write the ASIL-D region (the MPU does not gate DMA) |
| AoU-08 | Power supervision (BOD/PVD) configured |
| AoU-09 | Integrator implements the Level 2 recovery procedure |
| AoU-10 | QM tasks run unprivileged (FreeRTOS ARM_CM4_MPU port) |
| AoU-11 | Safe State callbacks registered at initialization |

---

## Repository Structure (actual, as of Phase 1)

```
OpenSafetyRTOS/
├── kernel/
│   ├── CMakeLists.txt          # kernel umbrella target
│   ├── freertos/               # PLANNED — FreeRTOS vendoring (MIT) not yet imported
│   └── safety/                 # SafetyFunction (ASIL-D track, Apache 2.0)
│       ├── include/safety/     # safety_types.h, crc32.h, mailbox.h,
│       │                       #   safe_state.h, watchdog.h
│       └── src/                # crc32.c, mailbox.c, safe_state.c, watchdog.c
├── arch/
│   ├── arm-cortex-m/
│   │   ├── include/arch/       # mpu.h, fault.h
│   │   └── src/                # mpu.c (shadow + integrity check),
│   │                           #   fault_handlers.c
│   └── riscv/                  # placeholder (future)
├── hal/                        # HAL skeleton (stm32/ stub)
├── cmake/
│   └── toolchain-arm-cortex-m4.cmake
├── tests/
│   ├── unit/                   # STANDALONE host-compiled unit tests (CTest)
│   └── integration/            # stub — QEMU/HIL planned
├── safety/
│   └── doc/                    # ISO 26262 safety evidence (CC BY 4.0)
│       ├── HARA.md             # Hazard Analysis and Risk Assessment
│       ├── FSC.md              # Functional Safety Concept
│       ├── SSRS.md             # Software Safety Requirements Spec
│       ├── AOU.md              # Canonical Assumptions-of-Use registry
│       ├── SAFETY_CASE.md      # Top-level safety argument (GSN)
│       ├── SAFETY_PLAN.md      # ISO 26262 Part 2 safety management plan
│       └── reviews/            # QA Confirmation Review records (OSR-CR-*)
├── docs/
│   ├── ARCHITECTURE.md         # System architecture and FFI design
│   ├── TEAM.md                 # Roles and independence (incl. AI-agent limits)
│   ├── ADR-001..005            # Decomposition, MPU, SEooC, Boot, Privilege model
│   └── ...
└── .github/
    ├── CODEOWNERS              # @safety-team mandatory for safety/* PRs
    └── branch-strategy.md
```

Still missing (planned): FreeRTOS import, linker scripts (`memory.ld`), startup assembly, `safety_boot()`/POST implementation, board bring-up, CI workflows, integration tests.

---

## Safety Evidence Status

All safety evidence is written in public and reviewed following ISO 26262 Part 2 Cl.8 confirmation measures — currently by an AI QA agent, with the independence limitations documented in [docs/TEAM.md](docs/TEAM.md).

| Document | ID | Version | Status |
|----------|----|---------|--------|
| Hazard Analysis (HARA) | OSR-HARA-001 | v1.2 | 🟢 Conditionally approved |
| Functional Safety Concept (FSC) | OSR-FSC-001 | v1.2 | 🟢 **Final approved** |
| SW Safety Requirements (SSRS) | OSR-SSRS-001 | v1.0 | 🟡 Draft — QA review pending |
| Assumptions of Use registry | OSR-AOU-001 | v1.0 | 🟢 Released |
| Safety Case | OSR-SC-001 | v0.4 | 🟡 In progress |
| Safety Plan | OSR-SP-001 | v1.0 | 🟢 Approved |
| Tool Qualification Plan | OSR-TQ-001 | v1.0 | 🟢 Approved |

QA review records: `safety/doc/reviews/OSR-CR-*.md`

---

## Quick Start

> **Note:** hardware bring-up is still in development — the cross build currently
> produces the SafetyFunction and arch static libraries only (no bootable image yet).

### Cross build (ARM Cortex-M4; requires arm-none-eabi-gcc)

```bash
git clone https://github.com/eseseo/opensafetyrtos.git
cd opensafetyrtos

cmake -B build \
  -DTARGET_ARCH=arm-cortex-m \
  -DSAFETY_ASIL_D=ON \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-cortex-m4.cmake

cmake --build build
```

### Host unit tests (no cross toolchain required)

The unit tests are a standalone host-compiled project (same sources as the target build; hardware access is injected via ops interfaces):

```bash
cmake -S tests/unit -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

---

## License

| Component | License |
|-----------|---------|
| SafetyFunction layer (`safety/`, `kernel/safety/`, `arch/`) | Apache License 2.0 |
| FreeRTOS (`kernel/freertos/`, once vendored) | MIT License |
| Safety evidence documents (`safety/doc/`) | CC BY 4.0 |

The CC BY 4.0 license on the evidence documents means you can use, adapt, and redistribute the safety evidence for your own products — as long as you credit OpenSafetyRTOS.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Two tracks:

- **QM track** — standard contributions to FreeRTOS integration, build system, HAL
- **ASIL-D track** — contributions to `safety/` or `kernel/safety/` require MISRA-C:2012 compliance and mandatory independent QA review

Safety-related PRs require `@safety-team` approval. All ASIL-D code changes must include an SSR traceability tag (e.g., `/* SSR-015-MBX */`).

---

## References

- [ISO 26262-9](https://www.iso.org/standard/68385.html) — ASIL Decomposition (Cl.5) and FFI (Cl.6)
- [ISO 26262-10](https://www.iso.org/standard/68392.html) — Cl.9: SEooC development
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — System architecture and FFI design
- [docs/ADR-001](docs/ADR-001-decomposition-strategy.md) — Why ASIL Decomposition (and what it does *not* claim)
- [docs/ADR-002](docs/ADR-002-mpu-partition-strategy.md) — MPU partition strategy
- [docs/ADR-003](docs/ADR-003-distribution-strategy-seooc.md) — Why SEooC
- [docs/ADR-004](docs/ADR-004-boot-sequence.md) — Boot sequence and Safe State Level 3 mechanism
- [docs/ADR-005](docs/ADR-005-privilege-and-mpu-integrity.md) — Privilege model, MPU integrity, DMA policy
- [safety/doc/SAFETY_CASE.md](safety/doc/SAFETY_CASE.md) — Top-level safety argument
- [safety/doc/FSC.md](safety/doc/FSC.md) — Functional Safety Concept (all FSRs)
- [safety/doc/AOU.md](safety/doc/AOU.md) — Canonical Assumptions-of-Use registry
