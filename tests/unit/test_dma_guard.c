/* SPDX-License-Identifier: Apache-2.0 */
#include "unit_test.h"
#include "safety/dma_guard.h"

static safety_status_t submit(void *ctx, uint32_t src, uint32_t dst, uint32_t bytes)
{
    (*(unsigned *)ctx)++;
    UT_CHECK(src == 0x1000U && dst == UINT32_MAX - 15U && bytes == 16U);
    return SAFETY_ERR_STATE; /* Propagate a busy backend. */
}

int main(void)
{
    const safety_dma_window_t read = {0x1000U, 0x100FU};
    const safety_dma_window_t write = {UINT32_MAX - 15U, UINT32_MAX};
    unsigned calls = 0U;
    UT_CHECK(safety_dma_request(read, write, read.first, write.first, 16U, submit, &calls)
             == SAFETY_ERR_STATE);
    UT_CHECK(safety_dma_validate_transfer(read, write, read.last, write.last, 1U) == SAFETY_OK);
    const uint32_t invalid[][3] = {
        {read.first, write.first, 0U}, {read.first - 1U, write.first, 1U},
        {read.last + 1U, write.first, 1U}, {read.last, write.first, 2U},
        {read.first, write.first - 1U, 1U}, {read.first, write.last, 2U},
        {read.first, write.first, UINT32_MAX}
    };
    for (size_t i = 0U; i < sizeof(invalid) / sizeof(invalid[0]); i++)
    {
        UT_CHECK(safety_dma_request(read, write, invalid[i][0], invalid[i][1], invalid[i][2],
                                    submit, &calls) == SAFETY_ERR_DMA_VIOLATION);
    }
    UT_CHECK(safety_dma_request(read, write, read.first, write.first, 1U, NULL, NULL)
             == SAFETY_ERR_PARAM);
    UT_CHECK(calls == 1U);
    return ut_summary("dma_guard");
}
