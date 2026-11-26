/* SPDX-License-Identifier: Apache-2.0 */
#ifndef SAFETY_RESET_HISTORY_H
#define SAFETY_RESET_HISTORY_H

#include "safety/safe_state.h"

typedef enum
{
    SAFETY_RESET_POWER_ON = 1,
    SAFETY_RESET_WATCHDOG,
    SAFETY_RESET_BROWNOUT,
    SAFETY_RESET_SOFTWARE
} safety_reset_cause_t;

#define SAFETY_ABNORMAL_RESET_LIMIT 3U

typedef struct
{
    uint32_t last_reset_cause;
    uint32_t consecutive;
    uint32_t crc;
} safety_reset_history_t;

/** SSR-023-BST. Call once before enabling QM tasks, with a trusted platform
 * reset cause. Only confirmed power-on initializes arbitrary retained RAM.
 * A corrupt warm record or three consecutive abnormal resets latch Level 3
 * across warm boots. The caller enforces the returned level before startup. */
safety_ss_level_t safety_boot_check_reset_cause(volatile safety_reset_history_t *history,
                                               safety_reset_cause_t cause);
#endif
