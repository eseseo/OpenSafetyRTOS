/**
 * @file test_safe_state.c
 * @brief Safe-state escalation matrix and monotonicity tests.
 *
 * Traceability: SSR-020 (safe state management), SSR-021 (escalation rules).
 */
#include "unit_test.h"
#include "safety/safe_state.h"

typedef struct
{
    uint32_t suspend_calls;
    uint32_t notify_calls;
    safety_ss_level_t last_notified;
    uint32_t reset_calls;
} tb_t;

static void tb_suspend(void *ctx)
{
    ((tb_t *)ctx)->suspend_calls++;
}

static void tb_notify(void *ctx, safety_ss_level_t level)
{
    tb_t *tb = (tb_t *)ctx;
    tb->notify_calls++;
    tb->last_notified = level;
}

static void tb_reset(void *ctx)
{
    /* Host stand-in for AIRCR.SYSRESETREQ: records the call and returns. */
    ((tb_t *)ctx)->reset_calls++;
}

static void tb_clear(tb_t *tb)
{
    tb->suspend_calls = 0U;
    tb->notify_calls = 0U;
    tb->last_notified = SAFETY_SS_LEVEL0_NORMAL;
    tb->reset_calls = 0U;
}

int main(void)
{
    tb_t tb;
    safety_ss_ops_t ops;
    safety_ss_t ss;

    ops.suspend_qm = tb_suspend;
    ops.app_notify = tb_notify;
    ops.system_reset = tb_reset;
    ops.ctx = &tb;

    /* init validation: system_reset is mandatory */
    UT_CHECK(safety_ss_init(NULL, &ops) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_ss_init(&ss, NULL) == SAFETY_ERR_PARAM);
    {
        safety_ss_ops_t bad = ops;
        bad.system_reset = NULL;
        UT_CHECK(safety_ss_init(&ss, &bad) == SAFETY_ERR_PARAM);
    }
    UT_CHECK(safety_ss_init(&ss, &ops) == SAFETY_OK);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL0_NORMAL);

    /* --- Level 1: counted; below the limit stays at Level 1 ------------ */
    tb_clear(&tb);
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL1_DEGRADED) == SAFETY_OK);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL1_DEGRADED);
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL1_DEGRADED) == SAFETY_OK);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL1_DEGRADED);
    UT_CHECK_EQ_U32(tb.suspend_calls, 0U);

    /* cycle-ok resets the counter and recovers to Level 0 */
    safety_ss_report_cycle_ok(&ss);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL0_NORMAL);

    /* --- Level 1 x limit escalates to Level 2 -------------------------- */
    tb_clear(&tb);
    {
        uint32_t i;
        for (i = 0U; i < SAFETY_SS_LEVEL1_FAULT_LIMIT; i++)
        {
            UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL1_DEGRADED) ==
                     SAFETY_OK);
        }
    }
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    UT_CHECK_EQ_U32(tb.suspend_calls, 1U);
    UT_CHECK(tb.last_notified == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    UT_CHECK_EQ_U32(tb.reset_calls, 0U);

    /* --- monotonic: de-escalation requests rejected --------------------- */
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL1_DEGRADED) ==
             SAFETY_ERR_STATE);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL2_CONTROLLED_STOP);

    /* cycle-ok must NOT de-escalate at Level >= 2 */
    safety_ss_report_cycle_ok(&ss);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL2_CONTROLLED_STOP);

    /* --- Level 2 re-entry escalates to Level 3 -------------------------- */
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL2_CONTROLLED_STOP) ==
             SAFETY_OK);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK_EQ_U32(tb.reset_calls, 1U);

    /* --- direct Level 2 from normal ------------------------------------- */
    UT_CHECK(safety_ss_init(&ss, &ops) == SAFETY_OK);
    tb_clear(&tb);
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL2_CONTROLLED_STOP) ==
             SAFETY_OK);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL2_CONTROLLED_STOP);
    UT_CHECK_EQ_U32(tb.suspend_calls, 1U);
    UT_CHECK_EQ_U32(tb.reset_calls, 0U);

    /* --- direct Level 3 from any level ----------------------------------- */
    UT_CHECK(safety_ss_init(&ss, &ops) == SAFETY_OK);
    tb_clear(&tb);
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL3_EMERGENCY_RESET) ==
             SAFETY_OK);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK_EQ_U32(tb.reset_calls, 1U);
    UT_CHECK(tb.last_notified == SAFETY_SS_LEVEL3_EMERGENCY_RESET);

    /* --- parameter validation -------------------------------------------- */
    UT_CHECK(safety_ss_enter(NULL, SAFETY_SS_LEVEL1_DEGRADED) ==
             SAFETY_ERR_PARAM);
    UT_CHECK(safety_ss_enter(&ss, SAFETY_SS_LEVEL0_NORMAL) ==
             SAFETY_ERR_PARAM);
    UT_CHECK(safety_ss_enter(&ss, (safety_ss_level_t)7) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_ss_get_level(NULL) == SAFETY_SS_LEVEL3_EMERGENCY_RESET);

    safety_ss_t uninitialized = { 0 };
    UT_CHECK(safety_ss_enter(&uninitialized, SAFETY_SS_LEVEL1_DEGRADED) == SAFETY_ERR_PARAM);
    safety_ss_report_cycle_ok(NULL);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL3_EMERGENCY_RESET);

    return ut_summary("safe_state");
}
