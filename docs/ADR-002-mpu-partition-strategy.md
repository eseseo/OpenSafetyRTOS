# ADR-002: Use ARM Cortex-M MPU to Enforce Partition Boundaries

**Date:** 2026-04-18 (revised 2026-07-02)
**Status:** Accepted (amended by ADR-005)
**Deciders:** Safety Team, Architecture Team
**Supersedes:** N/A
**Amended by:** ADR-005 (privilege model and MPU integrity checking)
**Related:** ADR-001 (Decomposition Strategy), ADR-005, ARCHITECTURE.md §3

---

## Context

OpenSafetyRTOS achieves its safety goal via ISO 26262 Part 9 ASIL Decomposition: FreeRTOS operates as QM(D) and SafetyFunction operates as ASIL-D(D). The core requirement linking these two partitions is **Freedom From Interference (FFI)**.

FFI requires that a fault or erroneous behaviour in the QM partition cannot corrupt, suppress, or delay the safety functions provided by the SafetyFunction partition. A software-only boundary is insufficient because software in the QM partition (or a bug crossing into it) could bypass any software guard.

The ARM Cortex-M Memory Protection Unit (MPU) provides hardware-enforced memory access control **for unprivileged CPU accesses**. Any violation triggers a synchronous fault (MemManage fault, escalating to HardFault if disabled) that cannot be silently ignored by the offending code.

**Architectural limits (see ADR-005 for the full model):** the ARMv7-M MPU has no lock — any privileged code can rewrite it — and it distinguishes only privileged vs. unprivileged execution, not partitions. It also does not gate non-CPU bus masters (DMA). The protection model is therefore:

- **Preventive** against unprivileged QM tasks (AoU-10: FreeRTOS ARM_CM4_MPU port, QM tasks unprivileged);
- **Detective** against privileged-kernel faults: every safety cycle, `safety_mpu_integrity_check()` compares the live MPU registers against a CRC-protected shadow configuration and re-initializes on mismatch (detection latency = one safety cycle, budgeted in the FTTI);
- **Configuration-ownership + integrator constraint (AoU-07)** against bus masters.

---

## Decision

**Use the ARM Cortex-M MPU to enforce the memory partition boundary against unprivileged QM code, combined with the per-cycle MPU integrity check of ADR-005 to detect privileged-code interference.**

MPU configuration is owned exclusively by SafetyFunction: it is initialized before any QM code runs (ADR-004), its intended state is held in a CRC-protected shadow in ASIL-D memory, and any divergence of the live registers from the shadow is detected within one safety cycle and repaired, with safe-state escalation. *(An earlier revision of this ADR claimed the MPU is "locked after initialization"; ARMv7-M provides no such lock. That claim is withdrawn — see ADR-005 §1.)*

---

## Memory Region Definitions

Three logical memory areas are defined. Their exact base addresses and sizes are fixed in the linker script (`arch/arm-cortex-m/memory.ld`, per target) and must match the MPU shadow configuration exactly.

```
┌──────────────────────────────────────────────────────────────────────┐
│ Area                 │ Privileged access     │ Unprivileged (QM task)│
│                      │ (SafetyFunction +     │ access                │
│                      │  FreeRTOS kernel)     │                       │
├──────────────────────────────────────────────────────────────────────┤
│ ASIL-D area          │ per region below      │ NO ACCESS             │
│  (Safety code, data, │ (code RO+X, data RW)  │ (MemManage fault on   │
│   stack, IVT)        │                       │  any attempt)         │
├──────────────────────────────────────────────────────────────────────┤
│ QM area              │ R/W (kernel)          │ R/W (+X for code)     │
│  (FreeRTOS code,     │ SafetyFunction reads  │                       │
│   data, stacks)      │ for monitoring only   │                       │
├──────────────────────────────────────────────────────────────────────┤
│ QM→Safety Mailbox    │ R/W                   │ R/W                   │
│  (inter-partition    │ SafetyFunction reads  │ QM writes messages    │
│   communication)     │ and validates only    │                       │
└──────────────────────────────────────────────────────────────────────┘
```

**Notes on honesty of this table:**

- The MPU expresses rights per privilege level, not per partition. "SafetyFunction R/W, kernel no-access" is not encodable; the ASIL-D area is privileged-access, and the privileged QM kernel is constrained detectively (ADR-005 §D1(b)).
- ARMv7-M AP encodings cannot grant unprivileged code *more* access than privileged code, so a "QM writes / Safety read-only" mailbox is not encodable in hardware either. The mailbox is RW for both; the *write-only from QM, validated-read by Safety* discipline is a software convention enforced by the mailbox API and 3-stage validation (ADR-001), not by the MPU.

**Invariant:** unprivileged QM tasks have zero access to the ASIL-D area (hardware-enforced), and the live MPU configuration equals the shadow at every integrity check (verified per cycle; violations escalate). This is verified at boot by `safety_mpu_selftest()` and per cycle by `safety_mpu_integrity_check()`.

---

## MPU Region Assignment Strategy (ARM Cortex-M4, 8 Regions)

The ARM Cortex-M4 MPU provides 8 programmable regions (numbered 0–7). Higher region numbers take precedence when regions overlap. Every region size must be a power of 2 (>= 32 bytes) and its base naturally aligned to its size — this is why the QM area cannot be covered by a single region spanning flash *and* SRAM (non-contiguous, at 0x08xxxxxx and 0x20xxxxxx respectively): QM code and QM data need separate regions.

The assignment below is the authoritative scheme. Example addresses are for an STM32F4-class part (1 MB flash at 0x08000000, SRAM at 0x20000000); exact values come from the linker script and are validated at build time by `safety_mpu_shadow_build()` (power-of-2 size, natural alignment).

| MPU Region | Name | Example Base | Example Size | Privileged Access | Unprivileged (QM task) | Notes |
|---|---|---|---|---|---|---|
| 0 | Deny-all background | 0x00000000 | 4 GB | No access | No access | Fail-secure default (SSR-004): any access not matching regions 1–7 faults. `PRIVDEFENA=0`. |
| 1 | ASIL-D Code (flash) | 0x08000000 | 64 KB | RO + Execute | No access | SafetyFunction code + vector table |
| 2 | ASIL-D Data + Stack (SRAM) | 0x20000000 | 16 KB | RW, XN | No access | SafetyFunction data, BSS, stack, MPU shadow |
| 3 | QM Code (flash) | 0x08040000 | 256 KB | RO + Execute | RO + Execute | FreeRTOS kernel + QM application code |
| 4 | QM Data (SRAM) | 0x20008000 | 16 KB | RW, XN | RW, XN | FreeRTOS heap, task stacks |
| 5 | QM→Safety Mailbox | 0x2000C000 | 1 KB (>= 256 B) | RW, XN | RW, XN | Software-disciplined channel (see note above) |
| 6 | Safety Stack Guard | 0x20004000 | 32 B (min, aligned) | No access | No access | Guard between QM area and ASIL-D SRAM; faults on overflow |
| 7 | Peripherals | 0x40000000 | 512 MB | RW, XN, Device | RW, XN (with SRD carve-outs) | Subregion-disable (SRD) bits carve out safety-critical peripheral windows (IWDG, safety timer, DMA controller) as privileged-only where alignment permits |

Any SRAM between the explicitly mapped regions is intentionally unmapped: the deny-all background makes it fault on access (additional guard space beyond Region 6).

**Peripheral note:** the System Control Space (NVIC/SCB/VTOR/MPU registers at 0xE000E000) is *not* in Region 7 and **cannot be MPU-protected from privileged code** — its integrity is covered detectively by the per-cycle VTOR + vector-table CRC check (ADR-005 §D1(e)). Unprivileged code cannot access the SCS at all (architectural rule, no MPU region needed).

**Note for ARM Cortex-M7:** the M7 MPU has 16 regions. The additional 8 regions are used for TCM, cache-attribute control, and finer-grained peripheral isolation (fewer SRD compromises). Region numbering follows the same convention; assignments will be specified in `arch/arm-cortex-m/mpu_config_m7.c` when the M7 port lands.

**Alignment requirement:** all MPU regions must be naturally aligned to a power-of-2 boundary equal to their size (ARMv7-M requirement). The linker script enforces this with `ALIGN()` directives, and `safety_mpu_shadow_build()` (`arch/arm-cortex-m/src/mpu.c`) rejects any region violating the encoding at initialization time.

---

## Stack Guard Regions

Both partitions have MPU stack guards to detect stack overflow before it corrupts adjacent memory.

### SafetyFunction Stack Guard

- Placed immediately below the SafetyFunction stack (growing toward lower addresses)
- Size: 32 bytes minimum
- MPU attributes: no access for all (including privileged)
- On overflow: MemManage fault → SafetyFunction fault handler → Safe State Level 3

### QM Partition Stack Guard

- Placed between the top of QM stack space and the ASIL-D SRAM region (Region 6, plus the unmapped deny-all gap)
- Size: 32 bytes minimum
- MPU attributes: no access for all
- On overflow: MemManage fault → **SafetyFunction's fault handler takes control** → safe state

The guard regions are defence-in-depth for *privileged* stack overflows (unprivileged QM tasks are already excluded from the ASIL-D area by Regions 1–2). A privileged-kernel stack overflow into the guard faults immediately instead of corrupting ASIL-D data silently.

---

## MemManage / HardFault Handler

**The MemManage, BusFault, UsageFault, and HardFault handlers are exclusively owned by SafetyFunction** (implemented in `arch/arm-cortex-m/src/fault_handlers.c`). Unprivileged QM code cannot modify the vector table (SCS is privileged-only); privileged modification of the vector table is detected by the per-cycle VTOR/IVT CRC check (ADR-005 §D1(e)).

Fault handler behaviour:

1. Save fault status registers (CFSR, HFSR, MMFAR, BFAR) and the stacked PC/LR to a fault record in ASIL-D memory (`safety_fault_record_t`)
2. Enter Safe State Level 3: immediate software reset via AIRCR.SYSRESETREQ (ADR-004); the independent hardware watchdog (AoU-02) is the backstop if the reset write itself fails
3. Reset cause and fault record are reported to the integrator at next boot (ADR-004 §4.3)

Fault handlers are MISRA-C:2012 compliant and covered by fault injection tests.

---

## Rejected Alternatives

### Alternative 1: Software-Only Isolation

Use software checks (guard values, pointer range validation, canary patterns) to detect QM interference with ASIL-D memory.

**Rejected because:** software-only isolation cannot support the FFI claim. If QM code is corrupted or misbehaving, it could overwrite the guard values themselves before they are checked. Hardware exclusion of unprivileged QM tasks by the MPU — combined with the bounded-latency detective controls of ADR-005 for privileged faults — is the strongest mechanism available on a single-core ARMv7-M device.

### Alternative 2: Separate MCUs (Dual-Core / Multi-MCU)

Run FreeRTOS on one MCU/core and SafetyFunction on a physically separate MCU/core, with communication over a hardware bus (SPI, CAN, etc.).

**Rejected because:**
- Cost and PCB complexity: requires two MCUs, additional routing, power regulation
- Latency: inter-MCU communication adds latency to safety monitoring functions
- Synchronization complexity: clock domain alignment, bus error handling become additional failure modes to analyse
- For OpenSafetyRTOS's target use case (embedded Cortex-M4/M7), single-MCU MPU-based isolation is the industry-standard approach
- This option remains valid for higher-complexity safety architectures and may be reconsidered if a multi-core variant is developed in a future ADR

*(A third alternative — a fully preventive privilege model or hypervisor — is analysed and rejected in ADR-005 §3.)*

---

## Consequences

**Positive:**
- Hardware-enforced exclusion of unprivileged QM tasks from ASIL-D memory — the dominant interference source is blocked preventively
- Any unprivileged violation generates an immediate, synchronous, detectable fault — no silent data corruption
- MPU configuration is auditable: region registers are read back and verified at boot (`safety_mpu_selftest()`) and every cycle (`safety_mpu_integrity_check()`)
- Single-MCU solution keeps cost and complexity within embedded automotive targets

**Negative / Risks:**
- Privileged-kernel interference is detected (bounded by one safety cycle), not prevented — residual risk analysis in ADR-005 §4
- MPU region count is limited (8 on M4, 16 on M7): careful allocation required; subregion disable bits used to maximise coverage within the region budget
- Linker script and MPU shadow configuration must be kept perfectly synchronized — a mismatch creates a silent safety gap. Mitigated by `safety_mpu_shadow_build()` encoding validation, `safety_mpu_selftest()` boot verification, and CI checks that parse both artefacts
- Deny-all background (Region 0, `PRIVDEFENA=0`) means any access not covered by regions 1–7 faults — peripheral and stack addresses must all be explicitly covered. Intentional (fail-secure, SSR-004) but requires careful initial bring-up

---

## Implementation Checklist

- [ ] Define ASIL-D and QM memory regions in `arch/arm-cortex-m/memory.ld`
- [x] Implement MPU region encoding + shadow build/verify in `arch/arm-cortex-m/src/mpu.c` (MISRA-C leaning, host-tested)
- [x] Implement `safety_mpu_init()` and `safety_mpu_integrity_check()` (`arch/arm-cortex-m/src/mpu.c`)
- [ ] Implement `safety_mpu_selftest()` — probe each boundary from unprivileged context and verify expected fault
- [x] Implement MemManage/BusFault/UsageFault/HardFault handlers in `arch/arm-cortex-m/src/fault_handlers.c`
- [x] Unit tests for shadow build/verify, integrity check (detect + repair), fault record path (`tests/unit/test_mpu.c`, `tests/unit/test_fault.c`); MC/DC coverage measurement pending
- [ ] Integration test: attempt unprivileged QM write to ASIL-D region, verify MemManage fault and safe state entry
- [ ] Integration test: privileged MPU register tamper, verify detection + repair within one safety cycle
- [ ] CI lint check: verify linker script region addresses match the MPU shadow constants
- [ ] Update FMEA-002 and FMEA-005 with implementation references and test evidence

---

## References

- ARM Architecture Reference Manual — ARMv7-M, Section B3.5 (MPU), B3.2 (SCS)
- ISO 26262-9:2018 Section 5 (ASIL Decomposition), Section 6 (FFI)
- `docs/ADR-005-privilege-and-mpu-integrity.md` — privilege model, integrity checking, DMA policy
- `docs/ARCHITECTURE.md` — system architecture and FFI analysis
- `safety/doc/FMEA_TEMPLATE.md` — FMEA-002 (MPU misconfiguration), FMEA-005 (stack overflow)
- `safety/doc/AOU.md` (OSR-AOU-001) — AoU-07, AoU-10
- ADR-001 — decomposition strategy decision

---

*This ADR is a controlled document. Changes require review by @safety-team and @arch-team and must be reflected in ARCHITECTURE.md and the linker script simultaneously.*
