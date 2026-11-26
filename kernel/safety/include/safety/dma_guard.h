/* SPDX-License-Identifier: Apache-2.0 */
#ifndef SAFETY_DMA_GUARD_H
#define SAFETY_DMA_GUARD_H

#include "safety/safety_types.h"

/** Trusted board policy: inclusive bounds of a DMA-accessible QM memory window.
 * The integrator excludes safety memory, aliases, MMIO and DMA descriptors. */
typedef struct
{
    uint32_t first;
    uint32_t last;
} safety_dma_window_t;

/** SSR-008-MPU. Single contiguous, incrementing byte transfer. All arguments
 * are consumed by value. The privileged adapter supplies both windows and the
 * backend, and serializes channel ownership until completion. */
safety_status_t safety_dma_validate_transfer(safety_dma_window_t readable,
                                            safety_dma_window_t writable,
                                            uint32_t src, uint32_t dst, uint32_t bytes);
typedef safety_status_t (*safety_dma_submit_t)(void *ctx, uint32_t src, uint32_t dst,
                                               uint32_t bytes);
safety_status_t safety_dma_request(safety_dma_window_t readable, safety_dma_window_t writable,
                                   uint32_t src, uint32_t dst, uint32_t bytes,
                                   safety_dma_submit_t submit, void *ctx);
#endif
