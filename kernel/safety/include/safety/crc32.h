/**
 * @file crc32.h
 * @brief CRC-32 (IEEE 802.3) for SafetyFunction integrity checks.
 *
 * Polynomial 0x04C11DB7 (reflected form 0xEDB88320), init 0xFFFFFFFF,
 * final XOR 0xFFFFFFFF, reflected input/output.
 * Known-answer: crc32("123456789") == 0xCBF43926.
 *
 * @safety_req SSR-015-MBX (mailbox integrity), SSR-004 (MPU shadow integrity)
 */
#ifndef SAFETY_CRC32_H
#define SAFETY_CRC32_H

#include "safety/safety_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** CRC-32 initial accumulator value (pass to safety_crc32_update). */
#define SAFETY_CRC32_INIT  (0xFFFFFFFFU)

/**
 * @brief Incremental CRC-32 update over a byte buffer.
 *
 * @param crc  Running accumulator (start with SAFETY_CRC32_INIT).
 * @param data Byte buffer; must not be NULL when len > 0.
 * @param len  Number of bytes.
 * @return Updated accumulator (not yet finalized).
 */
uint32_t safety_crc32_update(uint32_t crc, const uint8_t *data, size_t len);

/**
 * @brief Finalize an accumulator produced by safety_crc32_update().
 */
uint32_t safety_crc32_final(uint32_t crc);

/**
 * @brief One-shot CRC-32 over a buffer.
 *
 * @param data Byte buffer; must not be NULL when len > 0.
 * @param len  Number of bytes.
 * @return CRC-32 value (0 if data is NULL and len > 0 cannot be computed;
 *         callers must validate pointers before use).
 */
uint32_t safety_crc32(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* SAFETY_CRC32_H */
