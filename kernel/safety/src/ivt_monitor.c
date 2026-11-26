/* SPDX-License-Identifier: Apache-2.0 */
#include "safety/ivt_monitor.h"
#include "safety/crc32.h"

safety_status_t safety_ivt_check(uintptr_t active_vtor, const uint8_t *expected_table,
                                 size_t length, uint32_t reference_crc)
{
    safety_status_t status = SAFETY_ERR_PARAM;
    if ((expected_table != NULL) && (length != 0U))
    {
        status = SAFETY_ERR_INTEGRITY;
        if ((active_vtor == (uintptr_t)expected_table) &&
            (safety_crc32(expected_table, length) == reference_crc))
        {
            status = SAFETY_OK;
        }
    }
    return status;
}
