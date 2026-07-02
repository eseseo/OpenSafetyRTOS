# ADR-004: Boot Sequence — SafetyFunction Pre-Initialization

**Document ID:** ADR-004
**Version:** 1.1
**Status:** Accepted
**Date:** 2026-04-18 (v1.1: 2026-07-02)
**Author:** Agent-Safety
**Deciders:** Safety Manager, Architecture Lead
**Reference Standard:** ISO 26262 Part 4 Cl.8, Part 6 Cl.8 (Software Unit Design)
**Related:** ADR-002 (MPU), ADR-005 (Privilege Model), safety/doc/AOU.md (OSR-AOU-001)

---

## 1. Status

**Accepted** — Mandatory prerequisite for Phase 1 implementation. Reversing this decision requires a new ADR and Safety Manager approval.

---

## 2. Context

### 2.1 Problem Definition

OpenSafetyRTOS uses the ASIL Decomposition architecture (ADR-001): FreeRTOS as QM(D) partition, SafetyFunction as ASIL-D(D) partition. FFI between the two partitions is enforced via the ARM MPU per the ADR-002/ADR-005 model.

**The critical problem:** If FreeRTOS initializes before — or simultaneously with — SafetyFunction, QM code executes during a time window where the MPU has not yet been configured. During this window, QM code can access ASIL-D memory regions without restriction.

```
[Dangerous scenario — FreeRTOS executing before MPU is configured]

Reset Vector
    │
    ├─ FreeRTOS_Init() begins        ← QM code starts executing
    │       ├─ Heap initialization
    │       └─ Task creation         ← No MPU! ASIL-D memory is accessible
    │
    └─ safety_boot()                 ← MPU configured only now
            └─ MPU setup             ← Too late
```

This scenario is an **ASIL-D FFI violation** and invalidates the ISO 26262 Part 9 ASIL Decomposition argument.

### 2.2 Scope of Impact

- All ARM Cortex-M4/M7 target platforms
- Startup code (startup_stmXXXX.s or equivalent reset handler)
- Linker script (SafetyFunction stack placement in ASIL-D region)
- Integrator BSP (Board Support Package) modification requirements

---

## 3. Decision

**SafetyFunction must initialize before FreeRTOS — without exception.**

The first function called from the Reset Vector is `safety_boot()`. Only after it returns successfully may FreeRTOS initialization proceed. This guarantees the MPU is active and enforced before FreeRTOS executes its first instruction (AoU-01).

---

## 4. Boot Sequence Definition

### 4.1 Complete Sequence

```
Reset Vector
    │
    ▼
safety_boot()                                   [ASIL-D]
    │
    ├─ Step 0: RAM test (from startup assembly — see §4.2 note)
    │           March-C over the ASIL-D SRAM region, executed from the
    │           reset-handler assembly BEFORE the C stack is established
    │           (the test is destructive; it must not run on a live stack)
    │
    ├─ Step 1: Stack pointer initialization
    │           Set SafetyFunction stack in the (now-verified) ASIL-D region
    │           (Linker script places .sf_stack section in ASIL-D region)
    │
    ├─ Step 2: MPU configuration — ASIL-D protection first
    │           ├─ Build/verify CRC-protected shadow config (ADR-005 §D1(d))
    │           ├─ Program regions per ADR-002 table (deny-all background,
    │           │   ASIL-D privileged-only, QM code/data, mailbox, guards)
    │           └─ MPU_CTRL.ENABLE = 1 (MPU activated)
    │           [From this point: any unprivileged access to ASIL-D
    │            → MemManage fault; privileged tampering is detected
    │            per cycle by safety_mpu_integrity_check()]
    │
    ├─ Step 3: Hardware Watchdog initialization
    │           ├─ Independent Watchdog activated (AoU-02)
    │           ├─ Timeout = SAFETY_WDG_TIMEOUT_MS (default: 100 ms)
    │           └─ From this point: missed kick → MCU reset
    │
    ├─ Step 4: Boot-time Self-Test (POST)
    │           ├─ 4a. ROM CRC check: CRC-32 (IEEE 802.3) over Flash;
    │           │       compare to stored reference value
    │           ├─ 4b. Remaining RAM regions: March-C region-by-region
    │           │       (regions not tested in Step 0; never the active stack)
    │           ├─ 4c. CPU register test: pattern write-read
    │           │       (0x00000000 / 0xFFFFFFFF / 0xAAAAAAAA / 0x55555555)
    │           ├─ 4d. MPU register verification: read back all regions,
    │           │       compare to shadow config
    │           └─ [Any self-test failure] → Safe State Level 3
    │
    └─ Step 5: Declare safety_boot() complete
                └─ g_safety_init_complete = SAFETY_INIT_MAGIC (0x5AFEA55A)
                   [Atomic write to ASIL-D region variable]

    ↓ [safety_boot() returns; safety timer ISR (AoU-04) begins the
       periodic SafetyFunction cycle — SafetyFunction is NOT a FreeRTOS
       task (ADR-005 §D1(c))]

FreeRTOS_Init()                                 [QM]
    │
    ├─ Step 6: Heap initialization
    │           └─ Heap allocated within QM Region only
    │              (MPU active; QM tasks created unprivileged, AoU-10)
    │
    ├─ Step 7: FreeRTOS task creation (ARM_CM4_MPU port, unprivileged)
    │           └─ All task stacks placed within QM Region
    │
    └─ Step 8: Start scheduler (vTaskStartScheduler())

    ↓ [Both partitions running]

Normal Operation
    ├─ SafetyFunction periodic cycle in dedicated timer ISR
    │   (MPU/VTOR integrity checks, mailbox validation, checkpoint-gated
    │    watchdog kick)
    └─ FreeRTOS QM tasks (normal operation, unprivileged, under MPU)
```

### 4.2 Boot Self-Test Requirements

| Self-Test | Algorithm | Pass Criterion | Failure Action |
|-----------|-----------|----------------|----------------|
| ROM CRC check | CRC-32 (IEEE 802.3, polynomial 0x04C11DB7) | Computed == stored reference CRC | Level 3 |
| RAM March test | March-C algorithm (IEC 61508 recommended) | All cell read/write patterns pass | Level 3 |
| CPU register test | 0x00000000 / 0xFFFFFFFF / 0xAAAAAAAA / 0x55555555 pattern write-read | Written value == read back value | Level 3 |
| MPU register verify | Read back all MPU regions, compare to CRC-protected shadow | All regions match shadow | Level 3 |
| Stack pointer check | SP register within ASIL-D stack region | STACK_BASE ≤ SP ≤ STACK_TOP | Level 3 |

**RAM test destructiveness (normative):** March-C destroys memory contents. The test of the ASIL-D stack/data region therefore runs **from the startup assembly, before the C environment (stack, .data/.bss) is established** (Step 0). Any RAM region tested later (Step 4b) must be tested region-by-region and must never include the currently active stack; if a later full-coverage test is required, the stack is temporarily relocated to an already-verified region first.

**Note:** Self-tests run before the FreeRTOS scheduler starts. The Watchdog is active from Step 3 onward — each self-test step must kick the Watchdog to ensure total self-test time stays within `SAFETY_WDG_TIMEOUT_MS`.

### 4.3 Reset Cause Tracking

On any reboot, the reset cause must be classified, logged, and reported to the integrator:

```c
typedef enum {
    RESET_CAUSE_POWER_ON      = 0x01,  /* Normal power-on reset */
    RESET_CAUSE_SW_RESET      = 0x02,  /* AIRCR.SYSRESETREQ — Level 3 (primary path) */
    RESET_CAUSE_WATCHDOG_HW   = 0x03,  /* HW Watchdog expiry — Level 3 backstop /
                                          SafetyFunction hang */
    RESET_CAUSE_POWER_FAULT   = 0x04,  /* Power anomaly / Brown-out */
    RESET_CAUSE_BOOT_SELFTEST = 0x05,  /* Boot self-test failure */
    RESET_CAUSE_EXTERNAL      = 0x06,  /* External reset pin */
    RESET_CAUSE_UNKNOWN       = 0xFF,  /* Unknown cause */
} reset_cause_t;

reset_cause_t safety_get_reset_cause(void);  /* Reads and classifies RCC_CSR at boot */
```

### 4.4 Safe State Level 3 Mechanism (D2)

**Level 3 (Emergency Reset) is an immediate software reset via `AIRCR.SYSRESETREQ`.** Watchdog-kick starvation is retained **only as the backstop** (e.g., if the reset write itself is ineffective, or if SafetyFunction hangs before reaching the reset call).

**FTTI remark:** the previous design ("stop kicking, wait for watchdog expiry") added up to `SAFETY_WDG_TIMEOUT_MS` (100 ms default) of reaction latency to every Level 3 event, consuming FTTI budget with no benefit. With SYSRESETREQ the Level 3 reaction time is microseconds; the reset cause distinguishes the primary path (`RESET_CAUSE_SW_RESET`) from the backstop (`RESET_CAUSE_WATCHDOG_HW`), preserving field diagnosability. Integrators budget: detection latency (≤ one safety cycle) + Level 3 reaction (≈ immediate) ≤ FTTI (see AoU-04 verification duty in OSR-AOU-001).

Implementation: `kernel/safety/src/safe_state.c` (Level 3 escalation, injected reset hook — `AIRCR.SYSRESETREQ` on target).

---

## 5. Rejected Alternatives

### Alternative A: Simultaneous Initialization

**Description:** Initialize FreeRTOS and SafetyFunction in parallel, each within their own region.

**Rejected because:** QM code still executes before MPU configuration is complete. Race conditions in initialization order exist — unacceptable under ASIL-D. Static analysis cannot exhaustively exclude `pvPortMalloc()` from touching ASIL-D memory ranges before the MPU is active.

### Alternative B: FreeRTOS First, SafetyFunction as a Task

**Description:** Start FreeRTOS first; initialize SafetyFunction as one of its tasks.

**Rejected because:** There is no MPU before FreeRTOS's first task runs — violating ADR-002's foundational premise. Running SafetyFunction as a FreeRTOS task also creates a scheduler dependency that breaks FFI independence. *(This rejection is now embodied architecturally: SafetyFunction never runs as a FreeRTOS task at all — it executes in the boot context and in the dedicated safety timer ISR, ADR-005 §D1(c).)*

### Alternative C: SafetyFunction on a Separate MCU

**Description:** Run SafetyFunction on a dedicated secondary MCU.

**Rejected because:** Added cost and complexity. OpenSafetyRTOS targets single-MCU Cortex-M via decomposition. Multi-MCU architectures may be addressed in a future ADR variant.

---

## 6. Assumption of Use Impact

The boot-order obligation is **AoU-01** in the canonical registry (`safety/doc/AOU.md`, OSR-AOU-001). *(An earlier draft of this ADR introduced it as "AoU-09 (New)"; that numbering is superseded by the registry — AoU-09 now denotes the Level 2 recovery obligation.)*

### AoU-01: `safety_boot()` before any QM code

**Requirement:**
The integrator's Reset Handler must call `safety_boot()` before `FreeRTOS_Init()` or `vTaskStartScheduler()`. No exceptions.

**Prohibited actions:**
- Calling any FreeRTOS API before `safety_boot()` returns
- Calling `safety_boot()` from within a FreeRTOS task
- Calling MPU configuration functions outside SafetyFunction to override its MPU setup
- Using any compile option that skips boot self-tests (even in debug builds)

**Verification:** Linker map analysis + boot sequence code review by an independent reviewer + boot trace (logic analyzer or ITM).

**Rationale:** Violating this AoU invalidates the FFI guarantee established by ADR-001/ADR-002/ADR-005, making the safety claim unsupportable.

---

## 7. Implementation File References

| File Path | Content | Status |
|-----------|---------|--------|
| `arch/arm-cortex-m/startup_safety.s` | Reset Handler — Step 0 RAM test, safety_boot() call order | planned |
| `kernel/safety/src/safety_init.c` | safety_boot() implementation | planned |
| `kernel/safety/src/boot_selftest.c` | Boot self-test algorithms | planned |
| `kernel/safety/src/watchdog.c` | Kick-policy management (checkpoint bitmask) | implemented |
| `kernel/safety/src/safe_state.c` | Level 1–3 state machine, SYSRESETREQ hook | implemented |
| `arch/arm-cortex-m/src/mpu.c` | MPU region configuration + shadow integrity | implemented |
| `arch/arm-cortex-m/src/fault_handlers.c` | Fault record + Level 3 entry | implemented |
| `docs/ADR-002-mpu-partition-strategy.md` | MPU region assignment details | — |
| `docs/ADR-005-privilege-and-mpu-integrity.md` | Privilege model, integrity checks | — |
| `safety/doc/SAFE_STATE_DEFINITION.md` | Safe State Level 3 reset details | — |

---

## 8. Consequences

**(+)** MPU is active before FreeRTOS executes its first instruction → FFI temporal completeness at boot

**(+)** Watchdog is active before FreeRTOS initialization → initialization-phase hangs are detected

**(+)** Boot self-tests pass before any QM code runs → code and memory integrity verified before execution

**(+)** Level 3 via SYSRESETREQ removes up to 100 ms of reaction latency from the FTTI budget (watchdog retained as backstop)

**(+)** Reset cause tracking enables field fault analysis (primary vs. backstop reset paths distinguishable)

**(−)** Boot time increases: self-tests add tens to hundreds of milliseconds (algorithm optimization required)

**(−)** Integrators must modify their startup code to satisfy AoU-01 — existing BSP users need a porting step (including the pre-C-stack RAM test in assembly)

**(−)** Self-test code itself must be developed under ASIL-D requirements (MC/DC coverage, MISRA-C)

---

## 9. Revision History

| Date | Version | Author | Changes |
|------|---------|--------|---------|
| 2026-04-18 | 1.0 | Agent-Safety | Initial draft and approval |
| 2026-07-02 | 1.1 | Safety Team | D2: Level 3 = immediate AIRCR.SYSRESETREQ, watchdog as backstop (+FTTI remark). Fixed invalid magic literal (0xSAFEA55A → 0x5AFEA55A) and CRC naming ("CRC32 CCITT" → CRC-32 IEEE 802.3, poly 0x04C11DB7). March-C moved to startup assembly before C stack (destructive-test rule). Entry point renamed SafetyFunction_PreInit() → safety_boot(). AoU numbering mapped to canonical registry OSR-AOU-001 (boot order = AoU-01). SafetyFunction-as-task wording resolved per ADR-005 §D1(c). |

---

*This ADR documents a design decision per ISO 26262 Part 4 Cl.8 (Safety-related software architectural design). Independent review by Safety Manager and V&V engineer required before implementation.*
