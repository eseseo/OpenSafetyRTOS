# ADR-002: Use ARM Cortex-M MPU to Enforce Partition Boundaries

**Date:** 2026-04-18
**Status:** Accepted
**Deciders:** Safety Team, Architecture Team
**Supersedes:** N/A
**Related:** ADR-001 (Decomposition Strategy), ARCHITECTURE.md §3

---

## Context

OpenSafetyRTOS achieves ASIL-D via ISO 26262 Part 9 ASIL Decomposition: FreeRTOS operates as QM(D) and SafetyFunction operates as ASIL-D(D). The core requirement linking these two partitions is **Freedom From Interference (FFI)**.

FFI requires that a fault or erroneous behaviour in the QM partition cannot corrupt, suppress, or delay the safety functions provided by the SafetyFunction partition. For FFI to satisfy ASIL-D, the isolation mechanism must itself be at ASIL-D — a software-only boundary is insufficient because software in the QM partition (or a bug crossing into it) could bypass any software guard.

The ARM Cortex-M Memory Protection Unit (MPU) provides hardware-enforced memory access control independent of software executing in either partition. Any violation triggers a synchronous fault (MemManage fault or HardFault) that cannot be silently ignored by the offending partition.

---

## Decision

**Use the ARM Cortex-M MPU to enforce memory partition boundaries between the QM (FreeRTOS) and ASIL-D (SafetyFunction) partitions.**

MPU configuration is owned exclusively by SafetyFunction. It is initialized before any QM code runs, locked after initialization, and cannot be reconfigured by QM partition code.

---

## Memory Region Definitions

Three logical memory regions are defined. Their exact base addresses and sizes are fixed in the linker script (`arch/<target>/memory.ld`) and must match the MPU configuration exactly.

```
┌──────────────────────────────────────────────────────────────────────┐
│ Region               │ SafetyFunction Access │ FreeRTOS (QM) Access  │
├──────────────────────────────────────────────────────────────────────┤
│ ASIL-D Region        │ Read / Write / Execute│ NO ACCESS             │
│  (Safety code, data, │                       │ (MemManage fault on   │
│   stack, constants)  │                       │  any attempt)         │
├──────────────────────────────────────────────────────────────────────┤
│ QM Region            │ Read Only             │ Read / Write / Execute│
│  (FreeRTOS code,     │ (monitoring only)     │                       │
│   data, stack)       │                       │                       │
├──────────────────────────────────────────────────────────────────────┤
│ QM→Safety Mailbox    │ Read Only             │ Read / Write          │
│  (inter-partition    │ (validated before use)│ (write only channel)  │
│   communication)     │                       │                       │
└──────────────────────────────────────────────────────────────────────┘
```

**Invariant:** QM partition has zero write access to any ASIL-D memory. This is verified at boot by `safety_mpu_selftest()` and is a hard requirement — no deviation permitted.

---

## MPU Region Assignment Strategy (ARM Cortex-M4, 8 Regions)

The ARM Cortex-M4 MPU provides 8 programmable regions (numbered 0–7). Higher region numbers take precedence over lower when regions overlap. The assignments below are fixed and documented here as the authoritative reference.

| MPU Region | Name | Base Address | Size | SafetyFunction Access | QM Access | Notes |
|---|---|---|---|---|---|---|
| 0 | Default background | 0x00000000 | 4 GB | Privileged RW | None | Deny-all background; all accesses must match a higher-priority region |
| 1 | ASIL-D Code (Flash) | Per linker script | Per linker script | Execute + Read | No access | SafetyFunction executable code |
| 2 | ASIL-D Data (SRAM) | Per linker script | Per linker script | Read + Write | No access | SafetyFunction variables, BSS |
| 3 | ASIL-D Stack | Per linker script | Per linker script | Read + Write | No access | SafetyFunction task stacks |
| 4 | QM Region (Flash + SRAM) | Per linker script | Per linker script | Read only (privileged) | Read + Write + Execute | FreeRTOS code and data |
| 5 | QM→Safety Mailbox | Per linker script | 256 bytes (min) | Read only | Read + Write | Dedicated communication channel |
| 6 | Safety Stack Guard | Per linker script | 32 bytes (min, aligned) | No access | No access | MPU guard page between QM stack top and ASIL-D region; triggers MemManage on QM stack overflow |
| 7 | Peripheral / Device | Device-specific | Device-specific | Privileged access | Restricted per device | NVIC, SysTick, watchdog registers |

**Note for ARM Cortex-M7:** The M7 MPU has 16 regions. The additional 8 regions are used for cache maintenance, tightly coupled memory (TCM), and finer-grained peripheral isolation. Region numbering follows the same convention; assignments will be specified in `arch/cortex-m7/mpu_config.c`.

**Alignment requirement:** All MPU regions must be naturally aligned to a power-of-2 boundary equal to or greater than their size (ARM MPU requirement). The linker script enforces this with `ALIGN()` directives.

---

## Stack Guard Regions

Both partitions have dedicated MPU stack guard regions to detect stack overflow before it corrupts adjacent memory.

### SafetyFunction Stack Guard

- Placed immediately below the SafetyFunction stack (growing toward lower addresses)
- Size: 32 bytes minimum (one cache line)
- MPU attributes: No access for all (including privileged)
- On overflow: MemManage fault → HardFault handler → safe state escalation

### QM Partition Stack Guard

- Placed between the top of QM stack space and the bottom of the ASIL-D region
- Size: 32 bytes minimum
- MPU attributes: No access for all
- On overflow: MemManage fault → HardFault → **SafetyFunction's HardFault handler takes control** → safe state

The QM stack guard is the last line of software defence against a QM stack overflow threatening the ASIL-D region. The ASIL-D Region MPU entry (Region 2/3) provides additional hardware enforcement even if the guard region is somehow bypassed (which is not possible given correct MPU configuration, but is defence-in-depth).

---

## HardFault and MemManage Fault Handler

**The HardFault handler and MemManage fault handler are exclusively owned by SafetyFunction.** QM partition code cannot register, override, or intercept these handlers.

Fault handler behaviour:

1. Save fault status registers (CFSR, HFSR, MMFAR, BFAR) to a non-volatile fault log in ASIL-D memory
2. Identify the faulting partition (inspect stacked PC against partition address ranges)
3. If fault originated in QM: isolate QM partition (suspend all QM tasks), log event, enter safe state via `safety_safe_state_enter()`
4. If fault originated in SafetyFunction itself: this is a catastrophic internal failure — force hardware watchdog expiry immediately (do not attempt software recovery)
5. Safe state definition is application-defined but must be provided by the integrator via `safety_application_safe_state()` callback

Fault handlers are implemented in MISRA-C compliant code under `safety/src/fault_handler.c` and covered by fault injection tests.

---

## Rejected Alternatives

### Alternative 1: Software-Only Isolation

Use software checks (guard values, pointer range validation, canary patterns) to detect QM interference with ASIL-D memory.

**Rejected because:** Software-only isolation cannot satisfy FFI at ASIL-D. If QM code is corrupted or misbehaving, it could overwrite the guard values themselves before they are checked. ISO 26262 requires that the independence mechanism between decomposed partitions be at the higher ASIL level (ASIL-D). Hardware enforcement by the MPU is the only practical mechanism that satisfies this on a single-core Cortex-M. This rejection is consistent with the TTTech and ETAS reference implementations cited in ADR-001.

### Alternative 2: Separate MCUs (Dual-Core / Multi-MCU)

Run FreeRTOS on one MCU/core and SafetyFunction on a physically separate MCU/core, with communication over a hardware bus (SPI, CAN, etc.).

**Rejected because:** 
- Cost and PCB complexity: requires two MCUs, additional routing, power regulation
- Latency: inter-MCU communication adds latency to safety monitoring functions
- Synchronization complexity: clock domain alignment, bus error handling become additional failure modes to analyse
- For OpenSafetyRTOS's target use case (embedded Cortex-M4/M7), single-MCU MPU-based isolation is the industry-standard approach
- This option remains valid for higher-complexity safety architectures and may be reconsidered if a multi-core variant is developed in a future ADR

---

## Consequences

**Positive:**
- Hardware-enforced FFI satisfies ISO 26262 Part 9 independence requirement at ASIL-D without software dependency
- Any QM violation generates an immediate, synchronous, detectable fault — no silent data corruption
- MPU configuration is auditable: region registers can be read back and verified at runtime by SafetyFunction self-test
- Single-MCU solution keeps cost and complexity within embedded automotive targets

**Negative / Risks:**
- MPU region count is limited (8 on M4, 16 on M7): careful region allocation required; subregion disable bits used to maximise coverage within region budget
- Linker script and MPU configuration must be kept perfectly synchronized — a mismatch between linker layout and MPU registers creates a silent safety gap. This is mitigated by `safety_mpu_selftest()` boot verification and CI checks that parse both artefacts
- Background region (Region 0) deny-all means any memory access not covered by regions 1–7 will fault — peripheral and stack addresses must all be explicitly covered. This is intentional (fail-secure default) but requires careful initial bring-up

---

## Implementation Checklist

- [ ] Define ASIL-D and QM memory regions in `arch/cortex-m4/memory.ld`
- [ ] Implement `safety_mpu_init()` in `arch/cortex-m4/mpu_config.c` (MISRA-C compliant)
- [ ] Implement `safety_mpu_selftest()` — probe each boundary and verify expected fault
- [ ] Implement HardFault and MemManage handlers in `safety/src/fault_handler.c`
- [ ] Write unit tests with MC/DC coverage for `safety_mpu_selftest()` and fault handlers
- [ ] Write integration test: attempt QM write to ASIL-D region, verify MemManage fault and safe state entry
- [ ] CI lint check: verify linker script region addresses match MPU configuration constants
- [ ] Update FMEA-002 and FMEA-005 with implementation references and test evidence

---

## References

- ARM Architecture Reference Manual — ARMv7-M, Section B3.5 (MPU)
- ISO 26262-9:2018 Section 5 — ASIL Decomposition
- `docs/ARCHITECTURE.md` — System architecture and FFI analysis
- `safety/doc/FMEA_TEMPLATE.md` — FMEA-002 (MPU misconfiguration), FMEA-005 (stack overflow)
- ADR-001 — Decomposition strategy decision

---

*This ADR is a controlled document. Changes require review by @safety-team and @arch-team and must be reflected in ARCHITECTURE.md and the linker script simultaneously.*
