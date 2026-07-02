/**
 * @file mailbox.h
 * @brief QM -> SafetyFunction mailbox with 3-stage validation.
 *
 * The mailbox is the only permitted QM -> ASIL-D data channel (AoU-03).
 * SafetyFunction never trusts mailbox content: every receive performs
 *   1. CRC-32 integrity check          -> SAFETY_ERR_CRC_FAIL
 *   2. timestamp freshness check       -> SAFETY_ERR_STALE
 *   3. value/range check via schema    -> SAFETY_ERR_RANGE
 *
 * SAFETY_MAILBOX_FAILURE_LIMIT consecutive validation failures escalate to
 * Safe State Level 2 (Controlled Stop) through the injected escalation hook.
 *
 * The mailbox memory is writable by the QM partition, so the receive path
 * first snapshots the mailbox into ASIL-D memory and validates the snapshot
 * (defends against modification between check and use).
 *
 * Time base and range checking are injected via safety_mailbox_ops_t so the
 * validation logic is host-testable.
 *
 * @safety_req SSR-015-MBX, SSR-016 (freshness), SSR-017 (escalation)
 */
#ifndef SAFETY_MAILBOX_H
#define SAFETY_MAILBOX_H

#include "safety/safety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum payload bytes per mailbox message. */
#ifndef SAFETY_MAILBOX_DATA_MAX
#define SAFETY_MAILBOX_DATA_MAX  (64U)
#endif

/** Maximum accepted message age in milliseconds (freshness bound). */
#ifndef SAFETY_MAILBOX_MAX_AGE_MS
#define SAFETY_MAILBOX_MAX_AGE_MS  (10U)
#endif

/** Consecutive validation failures that escalate to Safe State Level 2. */
#ifndef SAFETY_MAILBOX_FAILURE_LIMIT
#define SAFETY_MAILBOX_FAILURE_LIMIT  (3U)
#endif

/**
 * Shared-memory mailbox layout. Written by QM, read by SafetyFunction.
 * The CRC covers data[0 .. data_len-1].
 */
typedef struct
{
    uint32_t crc;          /**< CRC-32 (IEEE 802.3) over data[0..data_len-1] */
    uint32_t timestamp_ms; /**< Sender timestamp from the safety tick base */
    uint32_t data_len;     /**< Payload length in bytes (<= DATA_MAX) */
    uint32_t schema_id;    /**< Identifies the expected value schema */
    uint8_t  data[SAFETY_MAILBOX_DATA_MAX];
} safety_mailbox_t;

/** Injected operations (must remain valid for the receiver's lifetime). */
typedef struct
{
    /** Current time in ms from the SafetyFunction time base (AoU-04). */
    uint32_t (*get_tick_ms)(void *ctx);
    /**
     * Application/schema-specific value range check.
     * Return true if data conforms to schema_id. May be NULL only if all
     * schemas are unchecked by design (not recommended; SSR-015-MBX).
     */
    bool (*range_check)(void *ctx, const uint8_t *data, uint32_t len,
                        uint32_t schema_id);
    /** Request Safe State Level 2 (Controlled Stop). Must not be NULL. */
    void (*escalate_level2)(void *ctx);
    void *ctx;
} safety_mailbox_ops_t;

/** Receiver state (lives in ASIL-D memory). */
typedef struct
{
    const safety_mailbox_ops_t *ops;
    uint32_t consecutive_failures;
} safety_mailbox_rx_t;

/**
 * @brief Initialize a mailbox receiver.
 * @return SAFETY_OK, or SAFETY_ERR_PARAM on NULL/incomplete ops.
 */
safety_status_t safety_mailbox_rx_init(safety_mailbox_rx_t *rx,
                                       const safety_mailbox_ops_t *ops);

/**
 * @brief Receive and validate one message from the shared mailbox.
 *
 * On success, copies the payload (mb->data_len bytes) into @p out and resets
 * the consecutive-failure counter. On validation failure, increments the
 * counter; on the SAFETY_MAILBOX_FAILURE_LIMIT-th consecutive failure,
 * invokes ops->escalate_level2 (SSR-017) and resets the counter.
 *
 * @param rx      Receiver state.
 * @param mb      Shared mailbox (QM-writable memory; snapshotted internally).
 * @param out     Destination buffer in ASIL-D memory.
 * @param out_len Capacity of @p out in bytes; must be >= mb->data_len.
 * @param rx_len  Optional: receives the payload length actually copied.
 *
 * @return SAFETY_OK, SAFETY_ERR_PARAM, SAFETY_ERR_CRC_FAIL,
 *         SAFETY_ERR_STALE, or SAFETY_ERR_RANGE.
 */
safety_status_t safety_mailbox_receive(safety_mailbox_rx_t *rx,
                                       const volatile safety_mailbox_t *mb,
                                       uint8_t *out, uint32_t out_len,
                                       uint32_t *rx_len);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_MAILBOX_H */
