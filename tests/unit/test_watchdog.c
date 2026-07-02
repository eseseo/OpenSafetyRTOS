/**
 * @file test_watchdog.c
 * @brief Watchdog kick-policy tests (checkpoint bitmask).
 *
 * Traceability: SSR-030, SSR-031. Key property: NO kick when any required
 * checkpoint was missed in the cycle.
 */
#include "unit_test.h"
#include "safety/watchdog.h"

#define CP_MAILBOX  (1UL << 0)
#define CP_MPU      (1UL << 1)
#define CP_CLOCK    (1UL << 2)
#define CP_ALL      (CP_MAILBOX | CP_MPU | CP_CLOCK)

typedef struct
{
    uint32_t kicks;
} tb_t;

static void tb_kick(void *ctx)
{
    ((tb_t *)ctx)->kicks++;
}

int main(void)
{
    tb_t tb = {0U};
    safety_wdg_ops_t ops;
    safety_wdg_t wdg;

    ops.hw_kick = tb_kick;
    ops.ctx = &tb;

    /* init validation */
    UT_CHECK(safety_wdg_init(NULL, &ops, CP_ALL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_wdg_init(&wdg, NULL, CP_ALL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_wdg_init(&wdg, &ops, 0U) == SAFETY_ERR_PARAM);
    {
        safety_wdg_ops_t bad = ops;
        bad.hw_kick = NULL;
        UT_CHECK(safety_wdg_init(&wdg, &bad, CP_ALL) == SAFETY_ERR_PARAM);
    }
    UT_CHECK(safety_wdg_init(&wdg, &ops, CP_ALL) == SAFETY_OK);

    /* --- all checkpoints reached -> kick -------------------------------- */
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MAILBOX) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MPU) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_CLOCK) == SAFETY_OK);
    UT_CHECK(safety_wdg_cycle_end(&wdg) == SAFETY_OK);
    UT_CHECK_EQ_U32(tb.kicks, 1U);

    /* --- one checkpoint missed -> NO kick -------------------------------- */
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MAILBOX) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_CLOCK) == SAFETY_OK);
    UT_CHECK(safety_wdg_cycle_end(&wdg) == SAFETY_ERR_STATE);
    UT_CHECK_EQ_U32(tb.kicks, 1U);
    UT_CHECK_EQ_U32(wdg.missed_cycles, 1U);

    /* --- mask cleared per cycle: previous checkpoints don't carry over -- */
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MPU) == SAFETY_OK);
    UT_CHECK(safety_wdg_cycle_end(&wdg) == SAFETY_ERR_STATE);
    UT_CHECK_EQ_U32(tb.kicks, 1U);

    /* --- recovery: full set again -> kick, missed counter cleared ------- */
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MAILBOX) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MPU) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_CLOCK) == SAFETY_OK);
    UT_CHECK(safety_wdg_cycle_end(&wdg) == SAFETY_OK);
    UT_CHECK_EQ_U32(tb.kicks, 2U);
    UT_CHECK_EQ_U32(wdg.missed_cycles, 0U);

    /* --- empty cycle -> no kick ------------------------------------------ */
    UT_CHECK(safety_wdg_cycle_end(&wdg) == SAFETY_ERR_STATE);
    UT_CHECK_EQ_U32(tb.kicks, 2U);

    /* --- checkpoint validation ------------------------------------------- */
    UT_CHECK(safety_wdg_checkpoint(&wdg, 0U) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_wdg_checkpoint(&wdg, 1UL << 5) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_wdg_checkpoint(NULL, CP_MPU) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_wdg_cycle_end(NULL) == SAFETY_ERR_PARAM);

    /* A rejected (out-of-mask) checkpoint must not poison the cycle. */
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MAILBOX) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_MPU) == SAFETY_OK);
    UT_CHECK(safety_wdg_checkpoint(&wdg, CP_CLOCK) == SAFETY_OK);
    UT_CHECK(safety_wdg_cycle_end(&wdg) == SAFETY_OK);
    UT_CHECK_EQ_U32(tb.kicks, 3U);

    /* Compile-time constants sanity (mirrors the static asserts). */
    UT_CHECK(SAFETY_WDG_KICK_PERIOD_MS <= (SAFETY_TASK_PERIOD_MS / 2U));
    UT_CHECK((2U * SAFETY_TASK_PERIOD_MS) <= SAFETY_WDG_TIMEOUT_MS);

    return ut_summary("watchdog");
}
