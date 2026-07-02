# ADR-005: Privilege Model, MPU Integrity Checking, and Non-CPU Master Policy

**Document ID:** ADR-005
**Version:** 1.0
**Status:** Accepted
**Date:** 2026-07-02
**Deciders:** Safety Team, Architecture Team
**Related:** ADR-001 (Decomposition), ADR-002 (MPU Partition Strategy), ADR-004 (Boot Sequence), ARCHITECTURE.md §3, §5.2, safety/doc/AOU.md (OSR-AOU-001)
**Reference Standard:** ISO 26262-9:2018 Cl.6 (FFI), ISO 26262-6:2018 Cl.7

---

## 1. Context

ADR-002 established the ARM Cortex-M MPU as the partition-boundary mechanism between the QM (FreeRTOS) and ASIL-D (SafetyFunction) partitions. Earlier drafts of ADR-002 stated that the MPU is "locked after initialization and cannot be reconfigured by QM partition code." That claim is **not supported by the ARMv7-M architecture**, and this ADR replaces it with an honest, architecturally accurate protection model.

Three hardware facts drive this ADR:

1. **There is no MPU lock on ARMv7-M.** The MPU registers (`MPU_CTRL`, `MPU_RNR`, `MPU_RBAR`, `MPU_RASR` at 0xE000ED90) are writable by *any privileged code* at any time. No configuration bit prevents this.
2. **The MPU distinguishes only privileged vs. unprivileged execution** — it has no concept of "partition A" vs. "partition B". Different access rights for QM and SafetyFunction code can only be expressed through the privilege level of the executing code.
3. **The System Control Space (SCS, 0xE000E000–0xE000EFFF: NVIC, SCB, VTOR, MPU registers) cannot be protected by the MPU against privileged accesses**, and **the MPU does not gate non-CPU bus masters (DMA, Ethernet MAC, USB, ...) at all.**

A stock FreeRTOS kernel executes in privileged mode (SVC handling, context switching, `BASEPRI` manipulation, SysTick/NVIC access). The FreeRTOS `ARM_CM4_MPU` port runs *tasks* unprivileged but the *kernel* remains privileged — and reprograms task MPU regions on every context switch. Any protection model must be consistent with these realities.

---

## 2. Decision

Adopt the following five-part protection model. Preventive and detective controls are explicitly distinguished, per the FFI argumentation needs of ISO 26262-9 Cl.6.

### D1(a) — QM application tasks run unprivileged (AoU-10)

All QM application tasks execute in **unprivileged Thread mode** using the FreeRTOS **ARM_CM4_MPU** port. Unprivileged code:

- cannot access the ASIL-D memory regions (MPU AP encoding: privileged-only),
- cannot write MPU, NVIC, VTOR, or any SCS register,
- cannot raise its own privilege except through the SVC gate controlled by the kernel.

For QM tasks, MPU protection is therefore **preventive**: interference attempts fault synchronously (MemManage fault) before any ASIL-D state is corrupted.

### D1(b) — The FreeRTOS kernel remains privileged; protection against it is detective

The FreeRTOS kernel (scheduler, context switch, ISR dispatch) executes privileged and *architecturally can* rewrite the MPU, the vector table, and ASIL-D memory. The kernel is qualified QM(D) software with bounded, reviewed privileged surface — but a *faulty* privileged kernel cannot be prevented by the MPU.

Against privileged-kernel faults the protection is **detective**, provided by D1(d) and D1(e) below, with detection latency bounded by the safety cycle and budgeted within the FTTI (see §4).

This preventive/detective split is the honest FFI claim of OpenSafetyRTOS:

| Interference source | Control type | Mechanism |
|---|---|---|
| Unprivileged QM task | Preventive (hardware) | MPU access permissions (AoU-10) |
| Privileged QM kernel fault | Detective (bounded latency) | Per-cycle MPU/VTOR/IVT integrity checks + watchdog kick policy |
| Non-CPU bus master (DMA, ...) | Preventive by configuration ownership + integrator constraint | D1(f), AoU-07 |

### D1(c) — SafetyFunction executes in a dedicated timer ISR, not as a FreeRTOS task

SafetyFunction is **not** a FreeRTOS task. It executes:

1. in the **boot context before the scheduler starts** (`safety_boot()`, ADR-004), and
2. periodically in the **ISR of a dedicated, independent hardware timer** (AoU-04), whose NVIC priority is higher than every QM interrupt and above `configMAX_SYSCALL_INTERRUPT_PRIORITY`, so FreeRTOS critical sections cannot mask it.

Consequences:

- SafetyFunction's periodic execution does **not** depend on the QM scheduler; a hung, corrupted, or livelocked FreeRTOS cannot starve it. This resolves the contradiction between ADR-004 (which rejected "SafetyFunction as a FreeRTOS task") and the former AoU wording ("SafetyFunction task must hold the highest priority").
- SafetyFunction executes privileged (Handler mode), which it requires to program the MPU and read the fault status registers.
- The residual dependency is the NVIC configuration itself, which is covered by the per-cycle integrity checks (D1(e)) and, ultimately, the hardware watchdog (AoU-02): if the safety ISR stops running for any reason, watchdog kicks stop and the MCU resets (Safe State Level 3).

### D1(d) — Per-cycle MPU integrity check against a CRC-protected shadow

Every safety cycle, `safety_mpu_integrity_check()` (implemented in `arch/arm-cortex-m/src/mpu.c`):

1. verifies the CRC-32 of the **shadow configuration** (the intended MPU state, stored in ASIL-D memory — detects corruption of the reference itself),
2. reads back all 8 live MPU regions (`RBAR`/`RASR`) and `MPU_CTRL`,
3. compares them word-for-word against the shadow,
4. on any mismatch: **re-initializes the MPU from the shadow immediately**, then reports `SAFETY_ERR_INTEGRITY` so the caller raises a safe-state event and the event is logged.

Detection latency for a privileged-code MPU manipulation is bounded by one safety cycle period (`SAFETY_TASK_PERIOD_MS`) and is budgeted in the FTTI analysis (AoU-04 verification duty). The MPU integrity check is a required watchdog checkpoint (kick policy, `kernel/safety/src/watchdog.c`): if the check cannot run or complete, the watchdog is not kicked.

### D1(e) — Per-cycle VTOR and vector-table CRC check

Because the SCS cannot be MPU-protected against privileged writes, the vector table is protected **detectively**:

- `VTOR` is read back each cycle and compared against the expected value;
- a CRC-32 over the interrupt vector table (which resides in the ASIL-D flash region) is recomputed each cycle and compared against the build-time reference.

A mismatch is treated as ASIL-D integrity violation: Safe State Level 3.

### D1(f) — The MPU does not gate DMA: SafetyFunction owns DMA configuration; AoU-07 covers other masters

Non-CPU bus masters bypass the MPU entirely. Therefore:

- **SafetyFunction owns the DMA controller configuration.** QM requests DMA transfers through a validation API; SafetyFunction verifies that source/destination windows lie entirely outside the ASIL-D regions before programming the channel. The DMA controller's register window is assigned to a privileged-only MPU region, so unprivileged QM tasks cannot program channels directly.
- **AoU-07** obliges the integrator to ensure that *no* bus master in the final system (DMA, Ethernet MAC, USB, external masters, ...) can write the ASIL-D address ranges — by routing all master configuration through SafetyFunction, by hardware means where available (bus firewalls, resource domain controllers), and by fault-injection testing.
- Detective backup: periodic RAM pattern checks over ASIL-D guard words detect a bus-master write that slipped through (bounded by the safety cycle).

---

## 3. Rejected Alternatives

### Alternative 1: Run the entire FreeRTOS kernel unprivileged

All QM code, including the kernel, in unprivileged mode — making MPU protection fully preventive.

**Rejected because:** stock FreeRTOS cannot run its kernel unprivileged. The kernel requires privileged access for context switching (PSP/CONTROL manipulation, exception return), `BASEPRI` masking, SysTick and NVIC programming, and SVC handling. Restructuring FreeRTOS so the kernel itself runs unprivileged (with a privileged microkernel beneath it) would amount to writing a new kernel/hypervisor — abandoning the ecosystem-reuse rationale of ADR-001. The ARM_CM4_MPU port (unprivileged tasks, privileged kernel) is the maximum privilege reduction available without forking FreeRTOS.

### Alternative 2: Hypervisor / hardware virtualization

Put FreeRTOS in a guest under an ASIL-D hypervisor so even a privileged guest kernel cannot touch safety state.

**Rejected because:** ARMv7-M has no virtualization extensions (no second privilege stage, no two-stage MMU/MPU). Hypervisor-based isolation requires Cortex-R/A class hardware, which contradicts the project's Cortex-M4/M7 target (see ADR-003). TrustZone-M (ARMv8-M) offers a secure/non-secure split that would materially strengthen this model; it is a candidate for a future ARMv8-M port ADR, not for the ARMv7-M primary target.

### Alternative 3: Accept the "locked MPU" claim and argue purely preventive FFI

**Rejected because:** the claim is factually wrong on ARMv7-M and would not survive assessment. An FFI argument built on a nonexistent hardware lock is a systematic fault in the safety case itself.

---

## 4. Residual Risk (honest statement)

This model does **not** claim complete prevention of privileged-kernel interference. The residual risks are:

1. **Window between integrity checks.** A faulty privileged QM kernel could rewrite the MPU (or ASIL-D RAM) and cause harm *within one safety cycle* before detection. Mitigations: the QM kernel's privileged surface is small, static, and qualified (ISO 26262-8 Cl.12); the safety cycle is short relative to the FTTI (integrator-verified, AoU-04); Level 3 reaction after detection is immediate (AIRCR.SYSRESETREQ, ADR-004).
2. **Detection depends on SafetyFunction execution.** If the safety timer ISR itself is suppressed (NVIC corruption by privileged code), integrity checks stop — and so do watchdog kicks. The independent hardware watchdog (AoU-02) then resets the MCU within `SAFETY_WDG_TIMEOUT_MS`. The watchdog is the anchor of the detective chain and must be external/independent, which is why AoU-02 is non-negotiable.
3. **Common-cause failure of check and checked state.** The shadow configuration could be corrupted consistently with the live registers by the same fault. Mitigation: the shadow is CRC-protected and lives in the ASIL-D region; a matching double corruption of shadow + CRC + live registers is argued as sufficiently improbable (residual, documented in the FMEA).
4. **Bus masters not under SafetyFunction control** remain the integrator's responsibility (AoU-07). On silicon without bus-level protection hardware, the assurance is analytic (address-path analysis) plus fault-injection testing, not hardware enforcement.

These residuals are carried into the safety case (GSN) as explicitly argued items, not hidden behind a "hardware-enforced" blanket claim.

---

## 5. Consequences

**(+)** The FFI argument matches the actual ARMv7-M architecture — assessable and defensible

**(+)** Preventive protection against the dominant interference source (application-level QM tasks) is retained in hardware

**(+)** SafetyFunction timing independence from the QM scheduler is by construction (dedicated timer ISR), not by scheduler-priority convention

**(+)** Detection latency is quantified (one safety cycle) and enters the FTTI budget explicitly

**(−)** Integrators must use the FreeRTOS ARM_CM4_MPU port and create QM tasks unprivileged (AoU-10) — a porting constraint for existing BSPs

**(−)** Per-cycle integrity checks (MPU readback, VTOR check, IVT CRC) consume safety-cycle WCET budget

**(−)** DMA-capable peripherals require routing through the SafetyFunction validation API — throughput/latency overhead for QM drivers

---

## 6. Implementation File References

| File Path | Content |
|-----------|---------|
| `arch/arm-cortex-m/src/mpu.c` | Shadow-config CRC, `safety_mpu_init()`, `safety_mpu_integrity_check()` |
| `arch/arm-cortex-m/include/arch/mpu.h` | ARMv7-M MPU register model, region encoding, ops interface |
| `arch/arm-cortex-m/src/fault_handlers.c` | MemManage/BusFault/UsageFault/HardFault -> Level 3 |
| `kernel/safety/src/watchdog.c` | Checkpoint-gated kick policy (integrity checks are checkpoints) |
| `safety/doc/AOU.md` | AoU-02, AoU-04, AoU-07, AoU-10 (canonical registry OSR-AOU-001) |
| `docs/ADR-002-mpu-partition-strategy.md` | Region allocation consistent with this model |

---

## 7. Revision History

| Date | Version | Author | Changes |
|------|---------|--------|---------|
| 2026-07-02 | 1.0 | Safety Team / Architecture Team | Initial version. Replaces the unsupported "MPU locked" claim of ADR-002 v1 with the privileged/unprivileged model, per-cycle integrity checking, and non-CPU master policy. |

---

*This ADR is a controlled document. Changes require review by @safety-team and @arch-team and must be reflected in ARCHITECTURE.md, ADR-002, and the FFI section of the safety case simultaneously.*
