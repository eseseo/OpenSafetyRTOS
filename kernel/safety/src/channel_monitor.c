/* SPDX-License-Identifier: Apache-2.0 */
#include "safety/channel_monitor.h"

static uint32_t current_ms(void *ctx)
{
    const safety_channel_monitor_t *monitor = ctx;
    return monitor->now_ms;
}

static bool check_payload(void *ctx, const uint8_t *data, uint32_t len, uint32_t schema)
{
    const safety_channel_monitor_t *monitor = ctx;
    return monitor->check(monitor->state.ops->ctx, data, len, schema);
}

static void controlled_stop(void *ctx)
{
    safety_channel_monitor_t *monitor = ctx;
    (void)safety_ss_enter(&monitor->state, SAFETY_SS_LEVEL2_CONTROLLED_STOP);
}

safety_status_t safety_channel_init(safety_channel_monitor_t *monitor,
                                    const safety_ss_ops_t *state_ops,
                                    safety_channel_check_t check)
{
    safety_status_t result = SAFETY_ERR_PARAM;
    if ((monitor != NULL) && (check != NULL) && (state_ops != NULL) &&
        (state_ops->suspend_qm != NULL) && (state_ops->app_notify != NULL) &&
        (safety_ss_init(&monitor->state, state_ops) == SAFETY_OK))
    {
        monitor->check = check;
        monitor->rx_ops.get_tick_ms = current_ms;
        monitor->rx_ops.range_check = check_payload;
        monitor->rx_ops.escalate_level2 = controlled_stop;
        monitor->rx_ops.ctx = monitor;
        result = safety_mailbox_rx_init(&monitor->rx, &monitor->rx_ops);
    }
    return result;
}

safety_status_t safety_channel_cycle(safety_channel_monitor_t *monitor, uint32_t now_ms,
                                     const volatile safety_mailbox_t *mailbox,
                                     uint8_t *out, uint32_t capacity, uint32_t *len)
{
    safety_status_t result = SAFETY_ERR_PARAM;
    if ((monitor != NULL) && (monitor->rx.ops == &monitor->rx_ops))
    {
        result = SAFETY_ERR_STATE;
        if (monitor->state.level < SAFETY_SS_LEVEL2_CONTROLLED_STOP)
        {
            monitor->now_ms = now_ms;
            result = safety_mailbox_receive(&monitor->rx, mailbox, out, capacity, len);
            if (result == SAFETY_OK)
            {
                safety_ss_report_cycle_ok(&monitor->state);
            }
            else if (((result == SAFETY_ERR_STALE) || (result == SAFETY_ERR_RANGE) ||
                      (result == SAFETY_ERR_CRC_FAIL)) &&
                     (monitor->state.level == SAFETY_SS_LEVEL0_NORMAL))
            {
                /* The receiver owns the consecutive-failure threshold. */
                (void)safety_ss_enter(&monitor->state, SAFETY_SS_LEVEL1_DEGRADED);
            }
        }
    }
    return result;
}
