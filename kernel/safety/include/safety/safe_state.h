/**
 * @file safe_state.h
 * @brief Three-level Safe State manager (see safety/doc/SAFE_STATE_DEFINITION.md).
 *
 * Level 1 — Degraded Operation : retry, fault counter increment
 * Level 2 — Controlled Stop    : suspend QM tasks, notify application
 * Level 3 — Emergency Reset    : immediate software reset (AIRCR.SYSRESETREQ
 *                                on target); watchdog starvation as backstop
 *
 * Properties:
 *  - The level is monotonic: it never decreases at runtime (no silent
 *    de-escalation). De-escalation only happens via reset.
 *  - Level 1 entries are counted; SAFETY_SS_LEVEL1_FAULT_LIMIT consecutive
 *    Level-1 faults escalate to Level 2.
 *  - A Level-2 request while already at Level 2 escalates to Level 3.
 *
 * Hardware/RTOS actions are injected through safety_ss_ops_t so the state
 * machine is host-testable (ISO 26262-6 Table 10 MC/DC evidence).
 *
 * @safety_req SSR-020 (safe state management), SSR-021 (escalation rules)
 */
#ifndef SAFETY_SAFE_STATE_H
#define SAFETY_SAFE_STATE_H

#include "safety/safety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Consecutive Level-1 faults that trigger escalation to Level 2. */
#ifndef SAFETY_SS_LEVEL1_FAULT_LIMIT
#define SAFETY_SS_LEVEL1_FAULT_LIMIT  (3U)
#endif

typedef enum
{
    SAFETY_SS_LEVEL0_NORMAL          = 0,
    SAFETY_SS_LEVEL1_DEGRADED        = 1,
    SAFETY_SS_LEVEL2_CONTROLLED_STOP = 2,
    SAFETY_SS_LEVEL3_EMERGENCY_RESET = 3
} safety_ss_level_t;

/**
 * Injected platform operations. All pointers must be valid for the lifetime
 * of the state manager. @p ctx is passed back verbatim.
 */
typedef struct
{
    /** Suspend all QM partition tasks (Level 2 action). */
    void (*suspend_qm)(void *ctx);
    /** Notify the integrator application of a level entry (AoU-09). */
    void (*app_notify)(void *ctx, safety_ss_level_t level);
    /**
     * Perform an immediate MCU reset (Level 3 action). On target this writes
     * AIRCR.SYSRESETREQ and does not return; on host test builds it records
     * the call and returns.
     */
    void (*system_reset)(void *ctx);
    void *ctx;
} safety_ss_ops_t;

typedef struct
{
    const safety_ss_ops_t *ops;
    safety_ss_level_t level;
    uint32_t level1_fault_count;
} safety_ss_t;

/**
 * @brief Initialize the safe-state manager at Level 0 (normal operation).
 * @return SAFETY_OK, or SAFETY_ERR_PARAM on NULL arguments / missing ops.
 */
safety_status_t safety_ss_init(safety_ss_t *ss, const safety_ss_ops_t *ops);

/**
 * @brief Request entry into a safe-state level.
 *
 * Escalation rules (SSR-021):
 *  - requested < current level  -> rejected with SAFETY_ERR_STATE (monotonic)
 *  - Level 1: fault counter incremented; on reaching
 *    SAFETY_SS_LEVEL1_FAULT_LIMIT the manager escalates to Level 2
 *  - Level 2 requested while already at Level 2 -> escalates to Level 3
 *  - Level 3: app_notify then system_reset (no return on target)
 *
 * @return SAFETY_OK on accepted transition, SAFETY_ERR_STATE on rejected
 *         de-escalation, SAFETY_ERR_PARAM on invalid arguments.
 */
safety_status_t safety_ss_enter(safety_ss_t *ss, safety_ss_level_t requested);

/**
 * @brief Report a successful monitoring cycle: resets the Level-1
 *        consecutive-fault counter. Has no effect at Level >= 2.
 */
void safety_ss_report_cycle_ok(safety_ss_t *ss);

/** @brief Current level (SAFETY_SS_LEVEL3_EMERGENCY_RESET if ss is NULL). */
safety_ss_level_t safety_ss_get_level(const safety_ss_t *ss);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_SAFE_STATE_H */
