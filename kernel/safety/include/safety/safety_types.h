/**
 * @file safety_types.h
 * @brief Common types for the OpenSafetyRTOS SafetyFunction (ASIL-D partition).
 *
 * MISRA-C:2012 note: only fixed-width integer types from <stdint.h> are used
 * in the safety partition (Dir 4.6).
 *
 * @safety_req SSR-001
 */
#ifndef SAFETY_TYPES_H
#define SAFETY_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Result codes returned by all SafetyFunction APIs.
 * Zero is success; every failure is a distinct non-zero value so that a
 * corrupted return value is unlikely to alias SAFETY_OK.
 */
typedef enum
{
    SAFETY_OK             = 0x00,  /**< Operation succeeded */
    SAFETY_ERR_CRC_FAIL   = 0x1A,  /**< CRC integrity check failed */
    SAFETY_ERR_SEQUENCE   = 0x71,  /**< Expected publication sequence missing */
    SAFETY_ERR_STALE      = 0x2B,  /**< Data older than freshness bound */
    SAFETY_ERR_RANGE      = 0x3C,  /**< Value/range/schema check failed */
    SAFETY_ERR_PARAM      = 0x4D,  /**< Invalid parameter (NULL, size, ...) */
    SAFETY_ERR_STATE      = 0x5E,  /**< Operation invalid in current state */
    SAFETY_ERR_INTEGRITY  = 0x6F,  /**< Configuration integrity violation */
    SAFETY_ERR_DMA_VIOLATION = 0x70  /**< Transfer outside the permitted windows */
} safety_status_t;

/**
 * Compile-time assertion usable in C99 (pre-C11 _Static_assert).
 * Produces a negative-size array type on failure.
 */
#define SAFETY_STATIC_ASSERT(cond, name) \
    typedef char safety_static_assert_##name[(cond) ? 1 : -1]

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_TYPES_H */
