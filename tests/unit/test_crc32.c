/**
 * @file test_crc32.c
 * @brief Known-answer tests for CRC-32 (IEEE 802.3).
 *
 * Traceability: SSR-015-MBX (mailbox integrity primitive).
 */
#include "unit_test.h"
#include "safety/crc32.h"

#include <string.h>

int main(void)
{
    /* Canonical check value: crc32("123456789") == 0xCBF43926. */
    {
        const uint8_t vec[] = "123456789";
        UT_CHECK_EQ_U32(safety_crc32(vec, 9U), 0xCBF43926U);
    }

    /* Empty buffer: CRC of zero bytes is 0x00000000 for this variant. */
    {
        const uint8_t b = 0U;
        UT_CHECK_EQ_U32(safety_crc32(&b, 0U), 0x00000000U);
    }

    /* "a" -> 0xE8B7BE43 (published IEEE 802.3 test vector). */
    {
        const uint8_t vec[] = "a";
        UT_CHECK_EQ_U32(safety_crc32(vec, 1U), 0xE8B7BE43U);
    }

    /* "The quick brown fox jumps over the lazy dog" -> 0x414FA339. */
    {
        const char *s = "The quick brown fox jumps over the lazy dog";
        UT_CHECK_EQ_U32(safety_crc32((const uint8_t *)s, strlen(s)),
                        0x414FA339U);
    }

    /* Incremental == one-shot. */
    {
        const uint8_t vec[] = "123456789";
        uint32_t acc = SAFETY_CRC32_INIT;
        acc = safety_crc32_update(acc, vec, 4U);
        acc = safety_crc32_update(acc, &vec[4], 5U);
        UT_CHECK_EQ_U32(safety_crc32_final(acc), 0xCBF43926U);
    }

    /* NULL data leaves the accumulator unchanged (defensive contract). */
    {
        UT_CHECK_EQ_U32(safety_crc32_update(0x12345678U, NULL, 4U),
                        0x12345678U);
    }

    /* Single-bit sensitivity: flipping one bit changes the CRC. */
    {
        uint8_t a[4] = {0x00U, 0x01U, 0x02U, 0x03U};
        uint8_t b[4] = {0x00U, 0x01U, 0x02U, 0x02U};
        UT_CHECK(safety_crc32(a, 4U) != safety_crc32(b, 4U));
    }

    return ut_summary("crc32");
}
