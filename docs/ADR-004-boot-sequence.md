# ADR-004: Boot Sequence — SafetyFunction Pre-Initialization

**Document ID:** ADR-004
**Version:** 1.0
**Status:** Accepted
**Date:** 2026-04-18
**Author:** Agent-Safety
**Deciders:** Safety Manager, Architecture Lead
**Reference Standard:** ISO 26262 Part 4 Cl.8, Part 6 Cl.8 (Software Unit Design)

---

## 1. Status

**Accepted** — Mandatory prerequisite for Phase 1 implementation. Reversing this decision requires a new ADR and Safety Manager approval.

---

## 2. Context

### 2.1 Problem Definition

OpenSafetyRTOS uses the ASIL Decomposition architecture (ADR-001): FreeRTOS as QM(D) partition, SafetyFunction as ASIL-D(D) partition. FFI between the two partitions is enforced in hardware via the ARM MPU (ADR-002).

**The critical problem:** If FreeRTOS initializes before — or simultaneously with — SafetyFunction, QM code executes during a time window where the MPU has not yet been configured. During this window, QM code can access ASIL-D memory regions without restriction.

```
[Dangerous scenario — FreeRTOS executing before MPU is configured]

Reset Vector
    │
    ├─ FreeRTOS_Init() begins        ← QM code starts executing
    │       ├─ Heap initialization
    │       └─ Task creation         ← No MPU! ASIL-D memory is accessible
    │
    └─ SafetyFunction_PreInit()      ← MPU configured only now
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

The first function called from the Reset Vector is `SafetyFunction_PreInit()`. Only after it returns successfully may FreeRTOS initialization proceed. This guarantees the MPU is active and enforced before FreeRTOS executes its first instruction.

---

## 4. Boot Sequence Definition

### 4.1 Complete Sequence

```
Reset Vector
    │
    ▼
SafetyFunction_PreInit()                        [ASIL-D]
    │
    ├─ Step 1: Stack pointer initialization
    │           Set SafetyFunction stack in ASIL-D memory region
    │           (Linker script places .sf_stack section in ASIL-D region)
    │
    ├─ Step 2: MPU configuration — ASIL-D protection first
    │           ├─ ASIL-D Region: SafetyFunction R/W, QM NO ACCESS
    │           ├─ QM Region: QM R/W, SafetyFunction R/O
    │           ├─ Mailbox Region: QM W, SafetyFunction R/O
    │           └─ MPU_CTRL.ENABLE = 1 (MPU activated)
    │           [From this point: any QM access to ASIL-D → HardFault]
    │
    ├─ Step 3: Hardware Watchdog initialization
    │           ├─ IWDG (Independent Watchdog) activated
    │           ├─ Timeout = WATCHDOG_TIMEOUT_MS (default: 100 ms)
    │           └─ From this point: missed kick → MCU reset
    │
    ├─ Step 4: Boot-time Self-Test (POST)
    │           ├─ 4a. ROM CRC check: CRC32 over Flash; compare to reference value
    │           ├─ 4b. RAM March test: March-C algorithm on ASIL-D stack/data regions
    │           ├─ 4c. CPU register test: 0x00000000 / 0xFFFFFFFF pattern write-read
    │           ├─ 4d. MPU register verification: read back all MPU regions, compare to expected
    │           └─ [Any self-test failure] → stop Watchdog kick → Level 3 reset
    │
    └─ Step 5: Declare SafetyFunction_PreInit() complete
                └─ g_safety_init_complete = SAFETY_INIT_MAGIC (0xSAFEA55A)
                   [Atomic write to ASIL-D region variable]

    ↓ [SafetyFunction_PreInit() returns]

FreeRTOS_Init()                                 [QM]
    │
    ├─ Step 6: Heap initialization
    │           └─ Heap allocated within QM Region only
    │              (MPU already active → any ASIL-D access → HardFault)
    │
    ├─ Step 7: FreeRTOS task creation
    │           └─ All task stacks placed within QM Region
    │
    └─ Step 8: Start scheduler (vTaskStartScheduler())

    ↓ [Both partitions running]

Normal Operation
    ├─ SafetyFunction monitor task (highest priority, periodic Watchdog kick)
    └─ FreeRTOS QM tasks (normal operation, under MPU protection)
```

### 4.2 Boot Self-Test Requirements

| Self-Test | Algorithm | Pass Criterion | Failure Action |
|-----------|-----------|----------------|----------------|
| ROM CRC check | CRC32 (CCITT polynomial) | Computed == stored reference CRC | Level 3 (Watchdog reset) |
| RAM March test | March-C algorithm (IEC 61508 recommended) | All cell read/write patterns pass | Level 3 (Watchdog reset) |
| CPU register test | 0x00000000 / 0xFFFFFFFF pattern write-read | Written value == read back value | Level 3 (Watchdog reset) |
| MPU register verify | Read back all MPU regions, compare to expected config | All regions match expected | Level 3 (Watchdog reset) |
| Stack pointer check | SP register within ASIL-D stack region | STACK_BASE ≤ SP ≤ STACK_TOP | Level 3 (Watchdog reset) |

**Note:** Self-tests run before the FreeRTOS scheduler starts. The Watchdog is active throughout — each self-test step must kick the Watchdog to ensure total self-test time stays within `WATCHDOG_TIMEOUT_MS`.

### 4.3 Reset Cause Tracking

On any reboot, the reset cause must be classified, logged, and reported to the integrator:

```c
typedef enum {
    RESET_CAUSE_POWER_ON      = 0x01,  /* Normal power-on reset */
    RESET_CAUSE_WATCHDOG_HW   = 0x02,  /* HW Watchdog — Level 3 (SafetyFunction fault) */
    RESET_CAUSE_WATCHDOG_SW   = 0x03,  /* SW-triggered Watchdog — Level 3 (intentional) */
    RESET_CAUSE_POWER_FAULT   = 0x04,  /* Power anomaly / Brown-out */
    RESET_CAUSE_BOOT_SELFTEST = 0x05,  /* Boot self-test failure */
    RESET_CAUSE_EXTERNAL      = 0x06,  /* External reset pin */
    RESET_CAUSE_UNKNOWN       = 0xFF,  /* Unknown cause */
} reset_cause_t;

reset_cause_t safety_get_reset_cause(void);  /* Reads and classifies RCC_CSR at boot */
```

---

## 5. Rejected Alternatives

### Alternative A: Simultaneous Initialization

**Description:** Initialize FreeRTOS and SafetyFunction in parallel, each within their own region.

**Rejected because:** QM code still executes before MPU configuration is complete. Race conditions in initialization order exist — unacceptable under ASIL-D. Static analysis cannot exhaustively exclude `pvPortMalloc()` from touching ASIL-D memory ranges before the MPU is active.

### Alternative B: FreeRTOS First, SafetyFunction as a Task

**Description:** Start FreeRTOS first; initialize SafetyFunction as one of its tasks.

**Rejected because:** There is no MPU before FreeRTOS's first task runs — violating ADR-002's foundational premise. Running SafetyFunction as a FreeRTOS task also creates a scheduler dependency that breaks FFI independence.

### Alternative C: SafetyFunction on a Separate MCU

**Description:** Run SafetyFunction on a dedicated secondary MCU.

**Rejected because:** Added cost and complexity. OpenSafetyRTOS targets single-MCU Cortex-M ASIL-D via decomposition. Multi-MCU architectures may be addressed in a future ADR variant.

---

## 6. Assumption of Use Impact

### AoU-09: No FreeRTOS Call Before SafetyFunction_PreInit() (New)

**ID:** AoU-09
**Title:** Integrator must ensure `SafetyFunction_PreInit()` is called before any FreeRTOS initialization

**Requirement:**
The integrator's Reset Handler must call `SafetyFunction_PreInit()` before `FreeRTOS_Init()` or `vTaskStartScheduler()`. No exceptions.

**Prohibited actions:**
- Calling any FreeRTOS API before `SafetyFunction_PreInit()` returns
- Calling `SafetyFunction_PreInit()` from within a FreeRTOS task
- Calling MPU configuration functions outside SafetyFunction to override its MPU setup
- Using any compile option that skips boot self-tests (even in debug builds)

**Verification:** Linker map analysis + boot sequence code review by an independent reviewer.

**Rationale:** Violating this AoU invalidates the FFI guarantee established by ADR-001 and ADR-002, making ASIL-D certification impossible.

---

## 7. Implementation File References

| File Path | Content |
|-----------|---------|
| `arch/arm-cortex-m/startup_safety.s` | Reset Handler — SafetyFunction_PreInit() call order |
| `kernel/safety/src/safety_init.c` | SafetyFunction_PreInit() implementation |
| `kernel/safety/src/boot_selftest.c` | Boot self-test algorithms |
| `kernel/safety/src/watchdog.c` | IWDG initialization and kick management |
| `arch/arm-cortex-m/src/mpu.c` | MPU region configuration |
| `docs/ADR-002-mpu-partition-strategy.md` | MPU region assignment details |
| `safety/doc/SAFE_STATE_DEFINITION.md` | Safe State Level 3 reset details |

---

## 8. Consequences

**(+)** MPU is active before FreeRTOS executes its first instruction → FFI temporal completeness achieved

**(+)** Watchdog is active before FreeRTOS initialization → initialization-phase hangs are detected

**(+)** Boot self-tests pass before any QM code runs → code and memory integrity verified before execution

**(+)** Reset cause tracking enables field fault analysis

**(−)** Boot time increases: self-tests add tens to hundreds of milliseconds (algorithm optimization required)

**(−)** Integrators must modify their startup code to satisfy AoU-09 — existing BSP users need a porting step

**(−)** Self-test code itself must be developed under ASIL-D requirements (MC/DC coverage, MISRA-C)

---

## 9. Revision History

| Date | Version | Author | Changes |
|------|---------|--------|---------|
| 2026-04-18 | 1.0 | Agent-Safety | Initial draft and approval |

---

*This ADR documents a design decision per ISO 26262 Part 4 Cl.8 (Safety-related software architectural design). Independent review by Safety Manager and V&V engineer required before implementation.*
