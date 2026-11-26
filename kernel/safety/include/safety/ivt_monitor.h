/* SPDX-License-Identifier: Apache-2.0 */
#ifndef SAFETY_IVT_MONITOR_H
#define SAFETY_IVT_MONITOR_H

#include "safety/safety_types.h"

/** SSR-006-MPU. Compare the active VTOR and the complete vector table with
 * the linker-selected table and CRC recorded at trusted startup. */
safety_status_t safety_ivt_check(uintptr_t active_vtor, const uint8_t *expected_table,
                                 size_t length, uint32_t reference_crc);

#endif
