/**
 * @file safe_state.c
 * @brief Three-level Safe State manager implementation.
 *
 * @safety_req SSR-020, SSR-021
 */
#include "safety/safe_state.h"

static void ss_do_level2(safety_ss_t *ss)
{
    ss->level = SAFETY_SS_LEVEL2_CONTROLLED_STOP;
    if (ss->ops->app_notify != NULL)
    {
        ss->ops->app_notify(ss->ops->ctx, SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    }
    if (ss->ops->suspend_qm != NULL)
    {
        ss->ops->suspend_qm(ss->ops->ctx);
    }
}

static void ss_do_level3(safety_ss_t *ss)
{
    ss->level = SAFETY_SS_LEVEL3_EMERGENCY_RESET;
    if (ss->ops->app_notify != NULL)
    {
        ss->ops->app_notify(ss->ops->ctx, SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    }
    /* Immediate software reset (AIRCR.SYSRESETREQ on target, ADR-004 §D2).
     * The independent hardware watchdog (AoU-02) remains the backstop if
     * this call fails to reset the MCU. Does not return on target. */
    ss->ops->system_reset(ss->ops->ctx);
}

safety_status_t safety_ss_init(safety_ss_t *ss, const safety_ss_ops_t *ops)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((ss != NULL) && (ops != NULL) && (ops->system_reset != NULL))
    {
        ss->ops = ops;
        ss->level = SAFETY_SS_LEVEL0_NORMAL;
        ss->level1_fault_count = 0U;
        status = SAFETY_OK;
    }

    return status;
}

safety_status_t safety_ss_enter(safety_ss_t *ss, safety_ss_level_t requested)
{
    safety_status_t status;

    if ((ss == NULL) || (ss->ops == NULL) ||
        (requested < SAFETY_SS_LEVEL1_DEGRADED) ||
        (requested > SAFETY_SS_LEVEL3_EMERGENCY_RESET))
    {
        status = SAFETY_ERR_PARAM;
    }
    else if (requested < ss->level)
    {
        /* Monotonic level property (SSR-021): no silent de-escalation. */
        status = SAFETY_ERR_STATE;
    }
    else if (requested == SAFETY_SS_LEVEL3_EMERGENCY_RESET)
    {
        ss_do_level3(ss);
        status = SAFETY_OK;
    }
    else if (requested == SAFETY_SS_LEVEL2_CONTROLLED_STOP)
    {
        if (ss->level == SAFETY_SS_LEVEL2_CONTROLLED_STOP)
        {
            /* Repeated critical fault while already stopped: escalate. */
            ss_do_level3(ss);
        }
        else
        {
            ss_do_level2(ss);
        }
        status = SAFETY_OK;
    }
    else /* SAFETY_SS_LEVEL1_DEGRADED */
    {
        ss->level = SAFETY_SS_LEVEL1_DEGRADED;
        ss->level1_fault_count++;
        if (ss->level1_fault_count >= SAFETY_SS_LEVEL1_FAULT_LIMIT)
        {
            ss_do_level2(ss);
        }
        else if (ss->ops->app_notify != NULL)
        {
            ss->ops->app_notify(ss->ops->ctx, SAFETY_SS_LEVEL1_DEGRADED);
        }
        else
        {
            /* No notification hook registered — counting is sufficient. */
        }
        status = SAFETY_OK;
    }

    return status;
}

void safety_ss_report_cycle_ok(safety_ss_t *ss)
{
    if ((ss != NULL) && (ss->level <= SAFETY_SS_LEVEL1_DEGRADED))
    {
        ss->level1_fault_count = 0U;
        ss->level = SAFETY_SS_LEVEL0_NORMAL;
    }
}

safety_ss_level_t safety_ss_get_level(const safety_ss_t *ss)
{
    safety_ss_level_t level = SAFETY_SS_LEVEL3_EMERGENCY_RESET;

    if (ss != NULL)
    {
        level = ss->level;
    }

    return level;
}
