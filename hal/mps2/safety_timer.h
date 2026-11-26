/* SPDX-License-Identifier: Apache-2.0 */
#ifndef OSR_MPS2_SAFETY_TIMER_H
#define OSR_MPS2_SAFETY_TIMER_H

#include <stdbool.h>
#include <stdint.h>

/* AN386, 25 MHz APB. Initialize once from privileged code. The callback
 * runs at IRQ priority zero and must never call FreeRTOS or block. */
bool mps2_safety_timer_start(uint32_t period_ms, void (*cycle)(uint32_t now_ms),
                             void (*on_fault)(void));
void Timer0_Handler(void);

#endif
