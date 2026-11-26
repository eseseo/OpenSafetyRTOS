/* SPDX-License-Identifier: Apache-2.0 */
#ifndef SAFETY_CHANNEL_MONITOR_H
#define SAFETY_CHANNEL_MONITOR_H

#include "safety/mailbox.h"
#include "safety/safe_state.h"

typedef bool (*safety_channel_check_t)(void *, const uint8_t *, uint32_t, uint32_t);

/** One monitored QM channel with its own state manager. Initialize before
 * enabling its periodic interrupt. Keep this object at a fixed address.
 * State callbacks execute in ISR context and must be bounded and ISR-safe.
 * @safety_req SSR-010, SSR-017-MBX, SSR-019-MBX
 */
typedef struct
{
    safety_mailbox_rx_t rx;
    safety_ss_t state;
    safety_mailbox_ops_t rx_ops;
    safety_channel_check_t check;
    uint32_t now_ms;
} safety_channel_monitor_t;

/** Context for the schema check is state_ops->ctx. Keep state_ops alive.
 * @safety_req SSR-017-MBX
 */
safety_status_t safety_channel_init(safety_channel_monitor_t *monitor,
                                    const safety_ss_ops_t *state_ops,
                                    safety_channel_check_t check);

/** Call from one periodic ISR. Consume the payload only on SAFETY_OK.
 * @safety_req SSR-018-MBX, SSR-019-MBX
 */
safety_status_t safety_channel_cycle(safety_channel_monitor_t *monitor, uint32_t now_ms,
                                     const volatile safety_mailbox_t *mailbox,
                                     uint8_t *out, uint32_t capacity, uint32_t *len);
#endif
