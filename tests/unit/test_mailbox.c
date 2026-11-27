/**
 * @file test_mailbox.c
 * @brief Mailbox 3-stage validation and escalation tests.
 *
 * Traceability: SSR-015-MBX (CRC), SSR-016 (freshness), SSR-017 (escalation).
 * Covers: valid receive, CRC mismatch, stale timestamp (incl. boundary),
 * range failure, 3-consecutive-failure Level-2 escalation, counter reset.
 */
#include "unit_test.h"
#include "safety/mailbox.h"
#include "safety/crc32.h"

/* ---- test doubles ---------------------------------------------------- */

typedef struct
{
    uint32_t now_ms;
    bool range_ok;
    uint32_t range_calls;
    uint32_t escalations;
} tb_t;

static uint32_t tb_get_tick(void *ctx)
{
    return ((tb_t *)ctx)->now_ms;
}

static bool tb_range_check(void *ctx, const uint8_t *data, uint32_t len,
                           uint32_t schema_id)
{
    tb_t *tb = (tb_t *)ctx;
    (void)data;
    (void)len;
    (void)schema_id;
    tb->range_calls++;
    return tb->range_ok;
}

static void tb_escalate(void *ctx)
{
    ((tb_t *)ctx)->escalations++;
}

static void mb_fill(safety_mailbox_t *mb, uint32_t ts, const uint8_t *payload,
                    uint32_t len, uint32_t sequence)
{
    safety_mailbox_t frame = { .sequence = sequence };
    uint32_t i;

    frame.timestamp_ms = ts;
    frame.data_len = len;
    frame.schema_id = 1U;
    for (i = 0U; i < len; i++)
    {
        frame.data[i] = payload[i];
    }
    UT_CHECK(safety_mailbox_publish(mb, &frame) == SAFETY_OK);
}

int main(void)
{
    static const uint8_t payload[4] = {0x11U, 0x22U, 0x33U, 0x44U};
    tb_t tb;
    safety_mailbox_ops_t ops;
    safety_mailbox_rx_t rx;
    safety_mailbox_t mb;
    uint8_t out[SAFETY_MAILBOX_DATA_MAX];
    uint32_t rx_len;

    ops.get_tick_ms = tb_get_tick;
    ops.range_check = tb_range_check;
    ops.escalate_level2 = tb_escalate;
    ops.ctx = &tb;

    /* init parameter validation */
    UT_CHECK(safety_mailbox_rx_init(NULL, &ops) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mailbox_rx_init(&rx, NULL) == SAFETY_ERR_PARAM);
    {
        safety_mailbox_ops_t bad = ops;
        bad.range_check = NULL;
        UT_CHECK(safety_mailbox_rx_init(&rx, &bad) == SAFETY_ERR_PARAM);
        bad = ops;
        bad.get_tick_ms = NULL;
        UT_CHECK(safety_mailbox_rx_init(&rx, &bad) == SAFETY_ERR_PARAM);
        bad = ops;
        bad.escalate_level2 = NULL;
        UT_CHECK(safety_mailbox_rx_init(&rx, &bad) == SAFETY_ERR_PARAM);
    }

    UT_CHECK(safety_mailbox_rx_init(&rx, &ops) == SAFETY_OK);

    /* --- valid receive ------------------------------------------------ */
    tb.now_ms = 100U;
    tb.range_ok = true;
    tb.range_calls = 0U;
    tb.escalations = 0U;
    mb_fill(&mb, 95U, payload, 4U, rx.expected_sequence);
    rx_len = 0U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), &rx_len) ==
             SAFETY_OK);
    UT_CHECK_EQ_U32(rx_len, 4U);
    UT_CHECK(out[0] == 0x11U && out[3] == 0x44U);
    UT_CHECK_EQ_U32(tb.range_calls, 1U);

    /* --- CRC failure ---------------------------------------------------*/
    mb_fill(&mb, 100U, payload, 4U, rx.expected_sequence);
    mb.crc ^= 0x1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_CRC_FAIL);

    /* --- staleness: boundary (age == MAX_AGE is accepted) -------------- */
    tb.now_ms = 1000U;
    mb_fill(&mb, 1000U - SAFETY_MAILBOX_MAX_AGE_MS, payload, 4U, rx.expected_sequence);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_OK);

    /* --- staleness: age == MAX_AGE + 1 is rejected ---------------------- */
    mb_fill(&mb, 1000U - (SAFETY_MAILBOX_MAX_AGE_MS + 1U), payload, 4U, rx.expected_sequence);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_STALE);

    /* --- range check failure ------------------------------------------- */
    tb.range_ok = false;
    mb_fill(&mb, 1000U, payload, 4U, rx.expected_sequence);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_RANGE);
    tb.range_ok = true;

    /* --- oversize data_len is a RANGE failure --------------------------- */
    mb_fill(&mb, 1000U, payload, 4U, rx.expected_sequence);
    mb.data_len = SAFETY_MAILBOX_DATA_MAX + 1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_RANGE);

    /* --- undersized caller buffer: PARAM error, no escalation count ---- */
    mb_fill(&mb, 1000U, payload, 4U, rx.expected_sequence);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, 2U, NULL) ==
             SAFETY_ERR_PARAM);

    /* --- consecutive-failure escalation --------------------------------
     * Fresh receiver: exactly SAFETY_MAILBOX_FAILURE_LIMIT consecutive
     * validation failures trigger one Level-2 escalation. */
    UT_CHECK(safety_mailbox_rx_init(&rx, &ops) == SAFETY_OK);
    tb.escalations = 0U;
    {
        uint32_t i;
        for (i = 0U; i < SAFETY_MAILBOX_FAILURE_LIMIT; i++)
        {
            mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence);
            mb.crc ^= 0xFFU;
            UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out),
                                            NULL) == SAFETY_ERR_CRC_FAIL);
        }
    }
    UT_CHECK_EQ_U32(tb.escalations, 1U);

    /* --- a success in between resets the counter ------------------------ */
    UT_CHECK(safety_mailbox_rx_init(&rx, &ops) == SAFETY_OK);
    tb.escalations = 0U;
    mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence);
    mb.crc ^= 0xFFU;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_CRC_FAIL);
    mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence);
    mb.crc ^= 0xFFU;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_CRC_FAIL);
    mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence); /* valid */
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_OK);
    mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence);
    mb.crc ^= 0xFFU;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_CRC_FAIL);
    UT_CHECK_EQ_U32(tb.escalations, 0U);

    /* --- receive parameter validation ----------------------------------- */
    UT_CHECK(safety_mailbox_receive(NULL, &mb, out, sizeof(out), NULL) ==
             SAFETY_ERR_PARAM);
    UT_CHECK(safety_mailbox_receive(&rx, NULL, out, sizeof(out), NULL) ==
             SAFETY_ERR_PARAM);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, NULL, sizeof(out), NULL) ==
             SAFETY_ERR_PARAM);

    safety_mailbox_rx_t uninitialized = { 0 };
    UT_CHECK(safety_mailbox_receive(&uninitialized, &mb, out, sizeof(out), NULL)
             == SAFETY_ERR_PARAM);

    /* Reject every protected header mutation without delivering a payload. */
    mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence);
    out[0] = 0xAAU;
    rx_len = 99U;
    mb.sequence ^= 1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), &rx_len) == SAFETY_ERR_CRC_FAIL);
    mb.sequence ^= 1U;
    mb.schema_id ^= 1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), &rx_len) == SAFETY_ERR_CRC_FAIL);
    mb.schema_id ^= 1U;
    mb.timestamp_ms ^= 1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), &rx_len) == SAFETY_ERR_CRC_FAIL);
    mb.timestamp_ms ^= 1U;
    mb.data_len ^= 1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), &rx_len) == SAFETY_ERR_CRC_FAIL);
    mb.data_len ^= 1U;
    mb.committed ^= 1U;
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), &rx_len) == SAFETY_ERR_CRC_FAIL);
    mb.committed ^= 1U;
    UT_CHECK(out[0] == 0xAAU && rx_len == 99U);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) == SAFETY_OK);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) == SAFETY_ERR_SEQUENCE);
    mb_fill(&mb, tb.now_ms, payload, 4U, rx.expected_sequence + 1U);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) == SAFETY_ERR_SEQUENCE);
    UT_CHECK(safety_mailbox_rx_init(&rx, &ops) == SAFETY_OK);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) == SAFETY_ERR_SEQUENCE);
    rx.expected_sequence = UINT32_MAX;
    mb_fill(&mb, tb.now_ms, payload, 4U, UINT32_MAX);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) == SAFETY_OK);
    mb_fill(&mb, tb.now_ms, payload, 4U, 0U);
    UT_CHECK(safety_mailbox_receive(&rx, &mb, out, sizeof(out), NULL) == SAFETY_OK);
    safety_mailbox_t invalid = { .data_len = SAFETY_MAILBOX_DATA_MAX + 1U };
    UT_CHECK(safety_mailbox_publish(NULL, &invalid) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mailbox_publish(&mb, NULL) == SAFETY_ERR_PARAM);
    UT_CHECK(safety_mailbox_publish(&mb, &invalid) == SAFETY_ERR_PARAM);

    return ut_summary("mailbox");
}
