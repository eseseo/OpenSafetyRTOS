/* SPDX-License-Identifier: Apache-2.0 */
/* SSR-006-MPU: reference table, relocated VTOR and changed vector contents. */
#include "unit_test.h"
#include "safety/ivt_monitor.h"
#include "safety/crc32.h"

int main(void)
{
    uint8_t table[192] = { 0U };
    uintptr_t base = (uintptr_t)table;
    uint32_t reference = safety_crc32(table, sizeof(table));
    UT_CHECK(safety_ivt_check(base, table, sizeof(table), reference) == SAFETY_OK);
    UT_CHECK(safety_ivt_check(base + 256U, table, sizeof(table), reference)
             == SAFETY_ERR_INTEGRITY);
    table[sizeof(table) - 1U] ^= 1U;
    UT_CHECK(safety_ivt_check(base, table, sizeof(table), reference) == SAFETY_ERR_INTEGRITY);
    UT_CHECK(safety_ivt_check(base, table, 0U, reference) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_ivt_check(base, NULL, sizeof(table), reference) == SAFETY_ERR_PARAM);
    return ut_summary("ivt");
}
