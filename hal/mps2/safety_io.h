/* SPDX-License-Identifier: Apache-2.0 */
#ifndef OSR_MPS2_SAFETY_IO_H
#define OSR_MPS2_SAFETY_IO_H
#include <stdbool.h>
#include <stdint.h>

/* AN386 test fixture: FPGA LED0 models an active-high output enable. */
void mps2_output_set_enabled(bool enabled);
bool mps2_output_enabled(void);

/* SSR-001/002-WDG: total reset timeout includes both countdown stages. */
bool mps2_watchdog_start(uint32_t reset_timeout_ms);
void mps2_watchdog_kick(void *ctx);
bool mps2_watchdog_first_timeout(void);
#endif
