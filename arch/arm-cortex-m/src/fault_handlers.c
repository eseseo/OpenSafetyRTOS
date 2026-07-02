/**
 * @file fault_handlers.c
 * @brief MemManage / BusFault / UsageFault / HardFault handling.
 *
 * All four fault vectors funnel into safety_fault_handle(), which records
 * the fault and requests Safe State Level 3 (immediate software reset via
 * AIRCR.SYSRESETREQ; the independent hardware watchdog is the backstop).
 *
 * @safety_req SSR-006
 */
#include "arch/fault.h"

/* Fault record lives in ASIL-D static memory (no dynamic allocation). */
static safety_fault_record_t g_fault_record;
static safety_ss_t *g_fault_ss = NULL;

safety_status_t safety_fault_init(safety_ss_t *ss)
{
    safety_status_t status = SAFETY_ERR_PARAM;

    if (ss != NULL)
    {
        g_fault_ss = ss;
        g_fault_record.magic = 0U;
        status = SAFETY_OK;
    }

    return status;
}

void safety_fault_handle(uint32_t cfsr, uint32_t hfsr, uint32_t mmfar,
                         uint32_t bfar, uint32_t stacked_pc,
                         uint32_t stacked_lr)
{
    g_fault_record.cfsr = cfsr;
    g_fault_record.hfsr = hfsr;
    g_fault_record.mmfar = mmfar;
    g_fault_record.bfar = bfar;
    g_fault_record.stacked_pc = stacked_pc;
    g_fault_record.stacked_lr = stacked_lr;
    g_fault_record.magic = SAFETY_FAULT_RECORD_MAGIC;

    if (g_fault_ss != NULL)
    {
        /* Level 3: immediate software reset (does not return on target). */
        (void)safety_ss_enter(g_fault_ss, SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    }

    /* If no safe-state manager is registered (boot-time fault before
     * initialization completes), fall through: on target the watchdog
     * backstop (AoU-02) resets the MCU; kicks have stopped. */
}

const safety_fault_record_t *safety_fault_get_record(void)
{
    return &g_fault_record;
}

/* ------------------------------------------------------------------ */
/* Target-only exception entry points                                   */
/* ------------------------------------------------------------------ */
#if defined(__arm__)

/* System Control Block fault status registers (ARMv7-M ARM, B3.2). */
#define SCB_CFSR   (*(volatile uint32_t *)0xE000ED28UL)
#define SCB_HFSR   (*(volatile uint32_t *)0xE000ED2CUL)
#define SCB_MMFAR  (*(volatile uint32_t *)0xE000ED34UL)
#define SCB_BFAR   (*(volatile uint32_t *)0xE000ED38UL)

/* Exception stack frame layout: r0,r1,r2,r3,r12,lr,pc,xpsr. */
void safety_fault_c_handler(const uint32_t *frame);

void safety_fault_c_handler(const uint32_t *frame)
{
    uint32_t pc = 0U;
    uint32_t lr = 0U;

    if (frame != NULL)
    {
        lr = frame[5];
        pc = frame[6];
    }

    safety_fault_handle(SCB_CFSR, SCB_HFSR, SCB_MMFAR, SCB_BFAR, pc, lr);

    /* safety_fault_handle() resets the MCU; never spin back into the
     * faulting context. */
    for (;;)
    {
        /* wait for watchdog backstop */
    }
}

/* Select MSP or PSP based on EXC_RETURN bit 2, then invoke the C handler. */
__attribute__((naked)) void HardFault_Handler(void);
__attribute__((naked)) void HardFault_Handler(void)
{
    __asm volatile(
        "tst lr, #4        \n"
        "ite eq            \n"
        "mrseq r0, msp     \n"
        "mrsne r0, psp     \n"
        "b safety_fault_c_handler \n");
}

__attribute__((alias("HardFault_Handler"))) void MemManage_Handler(void);
__attribute__((alias("HardFault_Handler"))) void BusFault_Handler(void);
__attribute__((alias("HardFault_Handler"))) void UsageFault_Handler(void);

#endif /* __arm__ */
