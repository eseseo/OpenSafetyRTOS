/* SPDX-License-Identifier: Apache-2.0 */
#include "safety_io.h"

#define REGISTER(address) (*(volatile uint32_t *)(address))
#define FPGA_LED REGISTER(0x40028000UL)
#define WDG_LOAD REGISTER(0x40008000UL)
#define WDG_CTRL REGISTER(0x40008008UL)
#define WDG_CLEAR REGISTER(0x4000800CUL)
#define WDG_RAW_STATUS REGISTER(0x40008010UL)
#define WDG_LOCK REGISTER(0x40008C00UL)
#define WDG_UNLOCK 0x1ACCE551UL
#define HALF_MS_CLOCKS 12500U

void mps2_output_set_enabled(bool enabled)
{
    FPGA_LED = enabled ? 1U : 0U;
    __asm__ volatile("dsb" ::: "memory");
}

bool mps2_output_enabled(void)
{
    return (FPGA_LED & 1U) != 0U;
}

bool mps2_watchdog_start(uint32_t reset_timeout_ms)
{
    bool armed = false;
    if ((reset_timeout_ms > 0U) && (reset_timeout_ms <= UINT32_MAX / HALF_MS_CLOCKS) &&
        (WDG_CTRL == 0U))
    {
        const uint32_t load = reset_timeout_ms * HALF_MS_CLOCKS - 1U;
        WDG_LOCK = WDG_UNLOCK;
        WDG_LOAD = load;
        WDG_CTRL = 3U; /* First timeout raises NMI, second requests reset. */
        WDG_LOCK = 1U;
        __asm__ volatile("dsb" ::: "memory");
        armed = WDG_CTRL == 3U && WDG_LOAD == load && WDG_LOCK == 1U;
    }
    return armed;
}

void mps2_watchdog_kick(void *ctx)
{
    (void)ctx;
    WDG_LOCK = WDG_UNLOCK;
    WDG_CLEAR = 1U;
    WDG_LOCK = 1U;
    __asm__ volatile("dsb" ::: "memory");
}

bool mps2_watchdog_first_timeout(void)
{
    return (WDG_RAW_STATUS & 1U) != 0U;
}
