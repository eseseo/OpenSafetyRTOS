/**
 * @file watchdog.h
 * @brief Watchdog kick policy: kick only when all monitored checks passed.
 *
 * SafetyFunction owns the (external, independent — AoU-02) hardware watchdog.
 * The kick policy is a checkpoint bitmask: each monitored check (mailbox
 * validation, MPU integrity, clock supervision, ...) reports a checkpoint
 * during the safety cycle. At cycle end the watchdog is kicked only if
 * every required checkpoint was reached. A missed checkpoint means the kick
 * is withheld and the hardware watchdog will reset the MCU — the ultimate
 * backstop for a hung or corrupted SafetyFunction (ARCHITECTURE.md §5.2).
 *
 * Hardware access is injected via safety_wdg_ops_t (host-testable logic).
 *
 * @safety_req SSR-030 (watchdog management), SSR-031 (kick policy)
 */
#ifndef SAFETY_WATCHDOG_H
#define SAFETY_WATCHDOG_H

#include "safety/safety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Period of the SafetyFunction monitoring cycle (safety timer ISR, AoU-04). */
#ifndef SAFETY_TASK_PERIOD_MS
#define SAFETY_TASK_PERIOD_MS  (10U)
#endif

/** Interval between watchdog kicks (one kick per completed cycle pair). */
#ifndef SAFETY_WDG_KICK_PERIOD_MS
#define SAFETY_WDG_KICK_PERIOD_MS  (SAFETY_TASK_PERIOD_MS / 2U)
#endif

/** Hardware watchdog timeout. */
#ifndef SAFETY_WDG_TIMEOUT_MS
#define SAFETY_WDG_TIMEOUT_MS  (100U)
#endif

/* Compile-time timing sanity (SSR-031):
 * the kick interval must not exceed half the safety cycle period, and
 * kicks must arrive with margin well inside the hardware timeout. */
SAFETY_STATIC_ASSERT(SAFETY_WDG_KICK_PERIOD_MS <= (SAFETY_TASK_PERIOD_MS / 2U),
                     wdg_kick_le_half_task_period);
SAFETY_STATIC_ASSERT((2U * SAFETY_TASK_PERIOD_MS) <= SAFETY_WDG_TIMEOUT_MS,
                     wdg_timeout_margin);

/** Injected hardware operations. */
typedef struct
{
    /** Kick (refresh) the hardware watchdog. Must not be NULL. */
    void (*hw_kick)(void *ctx);
    void *ctx;
} safety_wdg_ops_t;

typedef struct
{
    const safety_wdg_ops_t *ops;
    uint32_t required_mask;  /**< Checkpoints that must be reached per cycle */
    uint32_t reached_mask;   /**< Checkpoints reached in the current cycle */
    uint32_t missed_cycles;  /**< Cycles completed without kicking */
} safety_wdg_t;

/**
 * @brief Initialize the kick-policy state.
 * @param required_mask Non-zero bitmask of checkpoints required per cycle.
 * @return SAFETY_OK, or SAFETY_ERR_PARAM on invalid arguments.
 */
safety_status_t safety_wdg_init(safety_wdg_t *wdg,
                                const safety_wdg_ops_t *ops,
                                uint32_t required_mask);

/**
 * @brief Report that a monitored check passed in the current cycle.
 * @param checkpoint_bit One-hot bit within required_mask.
 * @return SAFETY_OK; SAFETY_ERR_PARAM if the bit is not a subset of
 *         required_mask or is zero.
 */
safety_status_t safety_wdg_checkpoint(safety_wdg_t *wdg,
                                      uint32_t checkpoint_bit);

/**
 * @brief End the current safety cycle.
 *
 * Kicks the hardware watchdog if and only if all required checkpoints were
 * reached; otherwise withholds the kick (the hardware watchdog will expire
 * — Safe State Level 3 backstop). The checkpoint mask is cleared for the
 * next cycle in both cases.
 *
 * @return SAFETY_OK if kicked; SAFETY_ERR_STATE if the kick was withheld;
 *         SAFETY_ERR_PARAM on invalid arguments.
 */
safety_status_t safety_wdg_cycle_end(safety_wdg_t *wdg);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_WATCHDOG_H */
