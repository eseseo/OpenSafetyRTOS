/* SPDX-License-Identifier: Apache-2.0 */
/* SSR-017-MBX / SSR-019-MBX: recovery, consecutive failures and stop latch. */
#include "unit_test.h"
#include "safety/channel_monitor.h"
#include "safety/crc32.h"

typedef struct
{
    uint32_t stops;
    safety_ss_level_t notified;
    bool notification_preceded_stop;
} fixture_t;

static bool check(void *ctx, const uint8_t *data, uint32_t len, uint32_t schema)
{
    (void)ctx;
    return schema == 1U && len == 1U && data[0] <= 100U;
}

static void notify(void *ctx, safety_ss_level_t level)
{
    ((fixture_t *)ctx)->notified = level;
}

static void suspend_qm(void *ctx)
{
    fixture_t *test = ctx;
    test->notification_preceded_stop = test->notified == SAFETY_SS_LEVEL2_CONTROLLED_STOP;
    test->stops++;
}

static void reset(void *ctx)
{
    (void)ctx;
    UT_CHECK(false);
}

int main(void)
{
    fixture_t test = { 0 };
    const safety_ss_ops_t ops = { .suspend_qm = suspend_qm, .app_notify = notify,
                                  .system_reset = reset, .ctx = &test };
    safety_channel_monitor_t monitor = { 0 };
    safety_mailbox_t message = { .data_len = 1U, .schema_id = 1U, .data = { 42U } };
    uint8_t out = 0U;
    uint32_t i;
    safety_ss_ops_t incomplete = ops;
    UT_CHECK(safety_channel_cycle(NULL, 0U, &message, &out, 1U, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_channel_cycle(&monitor, 0U, &message, &out, 1U, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_channel_init(NULL, &ops, check) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_channel_init(&monitor, NULL, check) == SAFETY_ERR_PARAM);
    incomplete.app_notify = NULL;
    UT_CHECK(safety_channel_init(&monitor, &incomplete, check) == SAFETY_ERR_PARAM);
    incomplete = ops;
    incomplete.suspend_qm = NULL;
    UT_CHECK(safety_channel_init(&monitor, &incomplete, check) == SAFETY_ERR_PARAM);
    incomplete = ops;
    incomplete.system_reset = NULL;
    UT_CHECK(safety_channel_init(&monitor, &incomplete, check) == SAFETY_ERR_PARAM);
    message.crc = safety_crc32(message.data, message.data_len);
    UT_CHECK(safety_channel_init(&monitor, &ops, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_channel_init(&monitor, &ops, check) == SAFETY_OK);
    UT_CHECK(safety_channel_cycle(&monitor, 0U, &message, &out, 0U, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(monitor.state.level == SAFETY_SS_LEVEL0_NORMAL && out == 0U);
    UT_CHECK(safety_channel_cycle(&monitor, SAFETY_MAILBOX_MAX_AGE_MS + 1U,
                                  &message, &out, 1U, NULL) == SAFETY_ERR_STALE);
    UT_CHECK(monitor.state.level == SAFETY_SS_LEVEL1_DEGRADED && out == 0U);
    message.timestamp_ms = SAFETY_MAILBOX_MAX_AGE_MS + 1U;
    UT_CHECK(safety_channel_cycle(&monitor, message.timestamp_ms, &message, &out, 1U, NULL)
             == SAFETY_OK);
    UT_CHECK(monitor.state.level == SAFETY_SS_LEVEL0_NORMAL && out == 42U);
    message.schema_id = 2U;
    UT_CHECK(safety_channel_cycle(&monitor, message.timestamp_ms, &message, &out, 1U, NULL) == SAFETY_ERR_RANGE);
    UT_CHECK(test.notified == SAFETY_SS_LEVEL1_DEGRADED);
    message.schema_id = 1U;
    UT_CHECK(safety_channel_cycle(&monitor, message.timestamp_ms, &message, &out, 1U, NULL) == SAFETY_OK);
    UT_CHECK(out == 42U && monitor.state.level == SAFETY_SS_LEVEL0_NORMAL);
    message.crc ^= 1U;
    for (i = 0U; i < SAFETY_MAILBOX_FAILURE_LIMIT; i++)
    {
        UT_CHECK(test.stops == 0U);
        UT_CHECK(safety_channel_cycle(&monitor, message.timestamp_ms, &message, &out, 1U, NULL)
                 == SAFETY_ERR_CRC_FAIL);
    }
    UT_CHECK(test.stops == 1U && test.notification_preceded_stop);
    message.crc ^= 1U;
    UT_CHECK(safety_channel_cycle(&monitor, message.timestamp_ms, &message, &out, 1U, NULL) == SAFETY_ERR_STATE);
    UT_CHECK(test.stops == 1U && monitor.state.level == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    return ut_summary("channel_monitor");
}
