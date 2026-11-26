/* SPDX-License-Identifier: Apache-2.0 */
/* CMSDK APB Timer0 dispatches SSR-010 safety cycles. Timer1 measures elapsed
 * peripheral clocks even when delivery of an individual interrupt is delayed. */
#include "safety_timer.h"
#include <stddef.h>

#define REG32(addr) (*(volatile uint32_t *)(addr))
#define TIMER0_CTRL   REG32(0x40000000UL)
#define TIMER0_RELOAD REG32(0x40000008UL)
#define TIMER0_INT    REG32(0x4000000CUL)
#define TIMER1_CTRL   REG32(0x40001000UL)
#define TIMER1_VALUE  REG32(0x40001004UL)
#define TIMER1_RELOAD REG32(0x40001008UL)
#define IRQ8_PRIORITY (*(volatile uint8_t *)0xE000E408UL)
#define NVIC_ENABLE   REG32(0xE000E100UL)
#define NVIC_CLEAR_PENDING REG32(0xE000E280UL)
#define CLOCKS_PER_MS 25000U

static void (*run_cycle)(uint32_t);
static void (*timer_fault)(void);
static uint32_t previous_count;
static uint32_t elapsed_ms;
static uint32_t remainder;

bool mps2_safety_timer_start(uint32_t period_ms, void (*cycle)(uint32_t),
                             void (*on_fault)(void))
{
    if (run_cycle != NULL || cycle == NULL || on_fault == NULL || period_ms == 0U ||
        period_ms > UINT32_MAX / CLOCKS_PER_MS)
    {
        return false;
    }
    TIMER0_CTRL = 0U;
    TIMER1_CTRL = 0U;
    TIMER1_RELOAD = UINT32_MAX;
    previous_count = TIMER1_VALUE;
    TIMER1_CTRL = 1U;
    run_cycle = cycle;
    timer_fault = on_fault;
    TIMER0_RELOAD = CLOCKS_PER_MS * period_ms;
    TIMER0_INT = 1U;
    IRQ8_PRIORITY = 0U;
    NVIC_CLEAR_PENDING = 1U << 8;
    NVIC_ENABLE = 1U << 8;
    TIMER0_CTRL = 9U; /* Internal clock, interrupt enabled, counter enabled. */
    return true;
}

void Timer0_Handler(void)
{
    uint32_t current = TIMER1_VALUE;
    uint32_t clocks = previous_count - current;
    previous_count = current;
    TIMER0_INT = 1U;

    /* Unsigned subtraction spans a counter wrap. Service within one full
     * timer revolution (about 172 seconds at 25 MHz). */
    elapsed_ms += clocks / CLOCKS_PER_MS;
    remainder += clocks % CLOCKS_PER_MS;
    elapsed_ms += remainder / CLOCKS_PER_MS;
    remainder %= CLOCKS_PER_MS;
    if (IRQ8_PRIORITY != 0U)
    {
        TIMER0_CTRL = 0U;
        timer_fault();
    }
    else
    {
        run_cycle(elapsed_ms);
    }
    __asm__ volatile("dsb" ::: "memory");
}
