/* SPDX-License-Identifier: Apache-2.0 */
#include "unit_test.h"
#include "safety/reset_history.h"

int main(void)
{
    safety_reset_history_t record = {0U, 0U, 0U};
    UT_CHECK(safety_boot_check_reset_cause(NULL, SAFETY_RESET_POWER_ON)
             == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_WATCHDOG)
             == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_POWER_ON)
             == SAFETY_SS_LEVEL0_NORMAL);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_BROWNOUT)
             == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_SOFTWARE)
             == SAFETY_SS_LEVEL0_NORMAL);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_WATCHDOG)
             == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_BROWNOUT)
             == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_WATCHDOG)
             == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK(record.consecutive == 3U && record.last_reset_cause == SAFETY_RESET_WATCHDOG);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_SOFTWARE)
             == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK(safety_boot_check_reset_cause(&record, SAFETY_RESET_POWER_ON)
             == SAFETY_SS_LEVEL0_NORMAL);
    UT_CHECK(safety_boot_check_reset_cause(&record, (safety_reset_cause_t)99)
             == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    return ut_summary("reset_history");
}
