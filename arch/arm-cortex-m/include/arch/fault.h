/**
 * @file fault.h
 * @brief Fault handling: record fault state, enter Safe State Level 3.
 *
 * The MemManage / BusFault / UsageFault / HardFault handlers are owned by
 * SafetyFunction (ADR-002). On any fault the handler records the fault
 * status registers plus the stacked return context into ASIL-D memory and
 * enters Safe State Level 3 (immediate AIRCR.SYSRESETREQ reset, ADR-004).
 *
 * The recording/escalation core (safety_fault_handle) is host-testable;
 * only the vector-table entry points are target-specific.
 *
 * @safety_req SSR-006 (fault detection and reaction)
 */
#ifndef ARCH_ARM_CORTEX_M_FAULT_H
#define ARCH_ARM_CORTEX_M_FAULT_H

#include "safety/safety_types.h"
#include "safety/safe_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Magic marking a valid (non-stale) fault record. */
#define SAFETY_FAULT_RECORD_MAGIC  (0x5AFEFA17U)

/** Snapshot of fault state, persisted in ASIL-D memory for field analysis. */
typedef struct
{
    uint32_t magic;      /**< SAFETY_FAULT_RECORD_MAGIC when valid */
    uint32_t cfsr;       /**< Configurable Fault Status Register */
    uint32_t hfsr;       /**< HardFault Status Register */
    uint32_t mmfar;      /**< MemManage Fault Address Register */
    uint32_t bfar;       /**< BusFault Address Register */
    uint32_t stacked_pc; /**< PC at fault (from exception stack frame) */
    uint32_t stacked_lr; /**< LR at fault */
} safety_fault_record_t;

/**
 * @brief Register the safe-state manager used for fault escalation.
 * Must be called during safety boot, before the MPU is enabled.
 * @return SAFETY_OK or SAFETY_ERR_PARAM.
 */
safety_status_t safety_fault_init(safety_ss_t *ss);

/**
 * @brief Record a fault and enter Safe State Level 3.
 *
 * Core handler logic, called by the target exception entry points (and
 * directly by unit tests). Does not return on target (Level 3 resets).
 */
void safety_fault_handle(uint32_t cfsr, uint32_t hfsr, uint32_t mmfar,
                         uint32_t bfar, uint32_t stacked_pc,
                         uint32_t stacked_lr);

/** @brief Last fault record (check .magic for validity). */
const safety_fault_record_t *safety_fault_get_record(void);

#ifdef __cplusplus
}
#endif

#endif /* ARCH_ARM_CORTEX_M_FAULT_H */
