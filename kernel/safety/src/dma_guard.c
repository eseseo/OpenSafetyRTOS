/* SPDX-License-Identifier: Apache-2.0 */
#include "safety/dma_guard.h"

static bool window_contains(safety_dma_window_t window, uint32_t start, uint32_t bytes)
{
    return (start >= window.first) && (start <= window.last) &&
           ((bytes - 1U) <= (window.last - start));
}

safety_status_t safety_dma_validate_transfer(safety_dma_window_t readable,
                                            safety_dma_window_t writable,
                                            uint32_t src, uint32_t dst, uint32_t bytes)
{
    safety_status_t result = SAFETY_ERR_DMA_VIOLATION;
    if ((bytes != 0U) && window_contains(readable, src, bytes) &&
        window_contains(writable, dst, bytes))
    {
        result = SAFETY_OK;
    }
    return result;
}

safety_status_t safety_dma_request(safety_dma_window_t readable, safety_dma_window_t writable,
                                   uint32_t src, uint32_t dst, uint32_t bytes,
                                   safety_dma_submit_t submit, void *ctx)
{
    safety_status_t result = SAFETY_ERR_PARAM;
    if (submit != NULL)
    {
        result = safety_dma_validate_transfer(readable, writable, src, dst, bytes);
        if (result == SAFETY_OK)
        {
            result = submit(ctx, src, dst, bytes);
        }
    }
    return result;
}
