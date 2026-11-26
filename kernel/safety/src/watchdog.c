/**
 * @file watchdog.c
 * @brief Watchdog kick-policy implementation (checkpoint bitmask).
 *
 * @safety_req SSR-002-WDG
 */
#include "safety/watchdog.h"

safety_status_t safety_wdg_init(safety_wdg_t *wdg,
                                const safety_wdg_ops_t *ops,
                                uint32_t required_mask)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((wdg != NULL) && (ops != NULL) && (ops->hw_kick != NULL) &&
        (required_mask != 0U))
    {
        wdg->ops = ops;
        wdg->required_mask = required_mask;
        wdg->reached_mask = 0U;
        wdg->missed_cycles = 0U;
        status = SAFETY_OK;
    }

    return status;
}

safety_status_t safety_wdg_checkpoint(safety_wdg_t *wdg,
                                      uint32_t checkpoint_bit)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((wdg != NULL) && (wdg->ops != NULL) && (checkpoint_bit != 0U) &&
        ((checkpoint_bit & (checkpoint_bit - 1U)) == 0U) &&
        ((checkpoint_bit & ~wdg->required_mask) == 0U))
    {
        wdg->reached_mask |= checkpoint_bit;
        status = SAFETY_OK;
    }

    return status;
}

safety_status_t safety_wdg_cycle_end(safety_wdg_t *wdg)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((wdg != NULL) && (wdg->ops != NULL) && (wdg->ops->hw_kick != NULL) &&
        (wdg->required_mask != 0U))
    {
        if (wdg->reached_mask == wdg->required_mask)
        {
            /* All monitored checks passed this cycle: kick (SSR-002-WDG). */
            wdg->ops->hw_kick(wdg->ops->ctx);
            wdg->missed_cycles = 0U;
            status = SAFETY_OK;
        }
        else
        {
            /* Withhold the kick: hardware watchdog expiry -> Level 3. */
            if (wdg->missed_cycles < UINT32_MAX)
            {
                wdg->missed_cycles++;
            }
            status = SAFETY_ERR_STATE;
        }
        wdg->reached_mask = 0U;
    }

    return status;
}
