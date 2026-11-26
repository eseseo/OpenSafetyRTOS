/* SPDX-License-Identifier: Apache-2.0 */
#include "safety/reset_history.h"
#include "safety/crc32.h"

static uint32_t record_crc(uint32_t cause, uint32_t count)
{
    const uint32_t words[] = { 0x52535431U, cause, count };
    return safety_crc32((const uint8_t *)words, sizeof(words));
}

safety_ss_level_t safety_boot_check_reset_cause(volatile safety_reset_history_t *history,
                                               safety_reset_cause_t cause)
{
    safety_ss_level_t level = SAFETY_SS_LEVEL3_EMERGENCY_RESET;
    if (history != NULL)
    {
        uint32_t count = 0U;
        if (cause != SAFETY_RESET_POWER_ON)
        {
            const uint32_t previous = history->last_reset_cause;
            count = history->consecutive;
            if ((history->crc != record_crc(previous, count)) ||
                (count >= SAFETY_ABNORMAL_RESET_LIMIT))
            {
                count = SAFETY_ABNORMAL_RESET_LIMIT;
            }
            else if ((cause == SAFETY_RESET_WATCHDOG) || (cause == SAFETY_RESET_BROWNOUT))
            {
                count++;
            }
            else if (cause == SAFETY_RESET_SOFTWARE)
            {
                count = 0U;
            }
            else
            {
                count = SAFETY_ABNORMAL_RESET_LIMIT;
            }
        }
        history->last_reset_cause = (uint32_t)cause;
        history->consecutive = count;
        history->crc = record_crc((uint32_t)cause, count);
        if (count == 0U)
        {
            level = SAFETY_SS_LEVEL0_NORMAL;
        }
        else if (count < SAFETY_ABNORMAL_RESET_LIMIT)
        {
            level = SAFETY_SS_LEVEL2_CONTROLLED_STOP;
        }
    }
    return level;
}
