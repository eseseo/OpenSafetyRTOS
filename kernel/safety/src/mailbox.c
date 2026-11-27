/**
 * @file mailbox.c
 * @brief QM -> SafetyFunction mailbox 3-stage validation implementation.
 *
 * @safety_req SSR-015-MBX, SSR-016, SSR-017
 */
#include "safety/mailbox.h"
#include "safety/crc32.h"

static uint32_t frame_crc(const safety_mailbox_t *frame)
{
    const uint32_t fields[] = { frame->sequence, frame->timestamp_ms,
                                frame->data_len, frame->schema_id };
    uint32_t crc = SAFETY_CRC32_INIT;
    for (size_t i = 0U; i < sizeof(fields) / sizeof(fields[0]); i++)
    {
        uint8_t bytes[4];
        for (uint32_t j = 0U; j < 4U; j++)
        {
            bytes[j] = (uint8_t)(fields[i] >> (8U * j));
        }
        crc = safety_crc32_update(crc, bytes, sizeof(bytes));
    }
    return safety_crc32_final(safety_crc32_update(crc, frame->data, frame->data_len));
}

safety_status_t safety_mailbox_publish(volatile safety_mailbox_t *mb,
                                       const safety_mailbox_t *frame)
{
    if ((mb == NULL) || (frame == NULL) || (frame->data_len > SAFETY_MAILBOX_DATA_MAX))
    {
        return SAFETY_ERR_PARAM;
    }
    mb->committed = 0U;
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    mb->sequence = frame->sequence;
    mb->timestamp_ms = frame->timestamp_ms;
    mb->schema_id = frame->schema_id;
    mb->data_len = frame->data_len;
    for (uint32_t i = 0U; i < frame->data_len; i++)
    {
        mb->data[i] = frame->data[i];
    }
    mb->crc = frame_crc(frame);
    __atomic_thread_fence(__ATOMIC_RELEASE);
    mb->committed = 1U;
    return SAFETY_OK;
}

/* Snapshot the QM-writable mailbox into ASIL-D memory field by field
 * (volatile source; avoids validate-then-reread TOCTOU hazards). */
static void mailbox_snapshot(safety_mailbox_t *dst,
                             const volatile safety_mailbox_t *src)
{
    uint32_t i;
    uint32_t copy_len;

    dst->committed = src->committed;
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    dst->sequence = src->sequence;
    dst->crc          = src->crc;
    dst->timestamp_ms = src->timestamp_ms;
    dst->data_len     = src->data_len;
    dst->schema_id    = src->schema_id;

    copy_len = dst->data_len;
    if (copy_len > SAFETY_MAILBOX_DATA_MAX)
    {
        copy_len = SAFETY_MAILBOX_DATA_MAX; /* length re-validated by caller */
    }
    for (i = 0U; i < copy_len; i++)
    {
        dst->data[i] = src->data[i];
    }
}

/* Validate a snapshot; returns the first failing check (SSR-015-MBX). */
static safety_status_t mailbox_validate(const safety_mailbox_rx_t *rx,
                                        const safety_mailbox_t *snap)
{
    safety_status_t status = SAFETY_OK;
    uint32_t now;
    uint32_t age;

    /* Structural check: length must fit the mailbox payload area. */
    if (snap->committed != 1U)
    {
        status = SAFETY_ERR_CRC_FAIL;
    }
    else if (snap->data_len > SAFETY_MAILBOX_DATA_MAX)
    {
        status = SAFETY_ERR_RANGE;
    }
    /* (1) CRC-32 integrity check */
    else if (frame_crc(snap) != snap->crc)
    {
        status = SAFETY_ERR_CRC_FAIL;
    }
    else
    {
        /* (2) Timestamp freshness check (unsigned wrap-safe difference) */
        now = rx->ops->get_tick_ms(rx->ops->ctx);
        age = now - snap->timestamp_ms;
        if (age > SAFETY_MAILBOX_MAX_AGE_MS)
        {
            status = SAFETY_ERR_STALE;
        }
        else if (snap->sequence != rx->expected_sequence)
        {
            status = SAFETY_ERR_SEQUENCE;
        }
        /* (3) Value range check against the expected schema */
        else if (!rx->ops->range_check(rx->ops->ctx, snap->data,
                                       snap->data_len, snap->schema_id))
        {
            status = SAFETY_ERR_RANGE;
        }
        else
        {
            status = SAFETY_OK;
        }
    }

    return status;
}

safety_status_t safety_mailbox_rx_init(safety_mailbox_rx_t *rx,
                                       const safety_mailbox_ops_t *ops)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if ((rx != NULL) && (ops != NULL) &&
        (ops->get_tick_ms != NULL) && (ops->range_check != NULL) &&
        (ops->escalate_level2 != NULL))
    {
        rx->ops = ops;
        rx->expected_sequence = 0U;
        rx->consecutive_failures = 0U;
        status = SAFETY_OK;
    }

    return status;
}

safety_status_t safety_mailbox_receive(safety_mailbox_rx_t *rx,
                                       const volatile safety_mailbox_t *mb,
                                       uint8_t *out, uint32_t out_len,
                                       uint32_t *rx_len)
{
    safety_status_t status;
    safety_mailbox_t snap;

    if ((rx == NULL) || (rx->ops == NULL) || (mb == NULL) || (out == NULL))
    {
        status = SAFETY_ERR_PARAM;
    }
    else
    {
        mailbox_snapshot(&snap, mb);

        status = mailbox_validate(rx, &snap);
        if ((status == SAFETY_OK) && (snap.data_len > out_len))
        {
            status = SAFETY_ERR_PARAM;
        }
        else
        {
            if (status == SAFETY_OK)
            {
                uint32_t i;
                for (i = 0U; i < snap.data_len; i++)
                {
                    out[i] = snap.data[i];
                }
                if (rx_len != NULL)
                {
                    *rx_len = snap.data_len;
                }
                rx->expected_sequence++;
                rx->consecutive_failures = 0U;
            }
            else
            {
                /* Consecutive-failure escalation (SSR-017): the limit-th
                 * consecutive validation failure requests Level 2. */
                rx->consecutive_failures++;
                if (rx->consecutive_failures >= SAFETY_MAILBOX_FAILURE_LIMIT)
                {
                    rx->ops->escalate_level2(rx->ops->ctx);
                    rx->consecutive_failures = 0U;
                }
            }
        }
    }

    return status;
}
