/**
 * @file test_fault.c
 * @brief Fault-handler core logic tests (host).
 *
 * Traceability: SSR-006. Verifies fault recording and Level-3 escalation.
 */
#include "unit_test.h"
#include "arch/fault.h"

typedef struct
{
    uint32_t reset_calls;
} tb_t;

static void tb_reset(void *ctx)
{
    ((tb_t *)ctx)->reset_calls++;
}

int main(void)
{
    tb_t tb = {0U};
    safety_ss_ops_t ops;
    safety_ss_t ss;
    const safety_fault_record_t *rec;

    ops.suspend_qm = NULL;
    ops.app_notify = NULL;
    ops.system_reset = tb_reset;
    ops.ctx = &tb;

    UT_CHECK(safety_fault_init(NULL) == SAFETY_ERR_PARAM);

    UT_CHECK(safety_ss_init(&ss, &ops) == SAFETY_OK);
    UT_CHECK(safety_fault_init(&ss) == SAFETY_OK);

    /* Record starts invalid after init. */
    rec = safety_fault_get_record();
    UT_CHECK(rec != NULL);
    UT_CHECK(rec->magic != SAFETY_FAULT_RECORD_MAGIC);

    /* A fault records all registers and escalates to Level 3. */
    safety_fault_handle(0x00000082U, 0x40000000U, 0x2000C123U, 0U,
                        0x08001234U, 0x0800FFFFU);

    rec = safety_fault_get_record();
    UT_CHECK_EQ_U32(rec->magic, SAFETY_FAULT_RECORD_MAGIC);
    UT_CHECK_EQ_U32(rec->cfsr, 0x00000082U);
    UT_CHECK_EQ_U32(rec->hfsr, 0x40000000U);
    UT_CHECK_EQ_U32(rec->mmfar, 0x2000C123U);
    UT_CHECK_EQ_U32(rec->stacked_pc, 0x08001234U);
    UT_CHECK_EQ_U32(rec->stacked_lr, 0x0800FFFFU);
    UT_CHECK(safety_ss_get_level(&ss) == SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    UT_CHECK_EQ_U32(tb.reset_calls, 1U);

    return ut_summary("fault");
}
