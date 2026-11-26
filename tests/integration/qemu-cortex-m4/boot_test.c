/* SPDX-License-Identifier: Apache-2.0 */
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "safety_timer.h"
#include "safety_io.h"
#include "safety/watchdog.h"
#include "safety/mailbox.h"
#include "safety/crc32.h"
#include "safety/channel_monitor.h"
#include "safety/ivt_monitor.h"

extern const uint8_t osr_vectors[];
extern const uint8_t osr_vectors_end[];
#define SCB_VTOR (*(volatile const uint32_t *)0xE000ED08UL)
static uint32_t vector_crc;
#ifdef OSR_TEST_RESET_HISTORY
#include "safety/reset_history.h"
/* QEMU starts SRAM at zero. Only the watchdog NMI advances this test oracle. */
static volatile uint32_t timeout_count __attribute__((section(".noinit")));
static volatile uint32_t level3_reset __attribute__((section(".noinit")));
static volatile safety_reset_history_t history __attribute__((section(".noinit")));
#endif
#ifdef OSR_TEST_DEVICE_FAULT
static volatile uint32_t device_witness __attribute__((section(".noinit")));
#define DEVICE_WITNESS 0x427511ABU
#endif
static safety_wdg_t watchdog;
#define CHECK_VECTOR 1U
#define CHECK_CHANNEL_RESPONSE 2U
#define CHECK_PERIPHERAL_MPU 4U
#ifdef OSR_TEST_STALL_RESET
#define STALL_WITNESS 0x57A11963U
static volatile uint32_t stall_witness __attribute__((section(".noinit")));
static volatile bool monitor_stalled;
#endif
#ifdef OSR_TEST_IVT_RESET
/* Survives SYSRESETREQ. Written only in the Level 3 reset callback. */
#define RESET_WITNESS 0x3AF41927UL
static volatile uint32_t reset_witness __attribute__((section(".noinit")));
#endif

extern const uint32_t __image_crc;
bool osr_image_valid(uint32_t first_word);
bool osr_image_valid(uint32_t first_word)
{
    const uintptr_t remaining_start = sizeof(first_word);
    uint32_t crc = safety_crc32_update(SAFETY_CRC32_INIT, (const uint8_t *)&first_word,
                                       sizeof(first_word));
    crc = safety_crc32_update(crc, (const uint8_t *)remaining_start,
                              (uintptr_t)&__image_crc - remaining_start);
    return safety_crc32_final(crc) == __image_crc;
}

static size_t vector_bytes(void)
{
    return (size_t)((uintptr_t)osr_vectors_end - (uintptr_t)osr_vectors);
}

#define TASK_STACK_BYTES 512U
#define PROBE_REGION_BYTES 128U
#define INITIALIZED_WORD UINT32_C(0x12345678)
#define MPU_CTRL (*(volatile const uint32_t *)UINT32_C(0xE000ED94))

static StackType_t probe_stack[TASK_STACK_BYTES / sizeof(StackType_t)]
    __attribute__((aligned(TASK_STACK_BYTES)));
static StaticTask_t probe_tcb;
static StackType_t verifier_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t verifier_tcb;
static volatile union
{
    struct
    {
        safety_mailbox_t mailbox;
        uint32_t progress;
    } frame;
    uint8_t region[PROBE_REGION_BYTES];
} shared __attribute__((aligned(PROBE_REGION_BYTES)));
SAFETY_STATIC_ASSERT(sizeof(shared) == PROBE_REGION_BYTES, shared_region_size);
static TaskHandle_t probe_handle;
static safety_channel_monitor_t monitor;
static volatile safety_ss_level_t notified_level;
static volatile bool stop_requested;
static volatile uint32_t accepted;
static volatile uint32_t stale_count;

static volatile uint32_t initialized_word = INITIALIZED_WORD;

static void semihost_call(uint32_t operation, const void *argument)
{
    register uint32_t r0 __asm__("r0") = operation;
    register const void *r1 __asm__("r1") = argument;
    __asm__ volatile("bkpt 0xab" : "+r"(r0) : "r"(r1) : "memory");
}

static void finish(const char *message, uint32_t exit_code)
    __attribute__((noreturn));

static void finish(const char *message, uint32_t exit_code)
{
    const uint32_t exit_args[] = { 0x20026U, exit_code };

    semihost_call(0x04U, message);  /* SYS_WRITE0 */
    semihost_call(0x20U, exit_args); /* SYS_EXIT_EXTENDED */
    for (;;)
    {
        __asm__ volatile("wfi");
    }
}

void osr_boot_test_fail(void)
{
    finish("OpenSafetyRTOS Cortex-M4 boot failed\n", 1U);
}

void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                  configSTACK_DEPTH_TYPE *depth)
{
    static StaticTask_t idle_tcb;
    static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

    *tcb = &idle_tcb;
    *stack = idle_stack;
    *depth = configMINIMAL_STACK_SIZE;
}

/* On the pinned eight-region port, static region 4 covers the 512 MiB
 * peripheral window. Task-switch regions 0..3 have lower priority. */
#define MPU_SELECT (*(volatile uint32_t *)0xE000ED98UL)
#define MPU_BASE   (*(volatile const uint32_t *)0xE000ED9CUL)
#define MPU_ATTR   (*(volatile uint32_t *)0xE000EDA0UL)
#define PRIV_DEVICE_ATTR (portMPU_REGION_PRIVILEGED_READ_WRITE | portMPU_REGION_EXECUTE_NEVER | \
                          (28U << 1U) | 1U)
SAFETY_STATIC_ASSERT(portGENERAL_PERIPHERALS_REGION == 4U, port_peripheral_slot);

static bool peripheral_policy(bool install)
{
    const uint32_t selected = MPU_SELECT;
    MPU_SELECT = portGENERAL_PERIPHERALS_REGION;
    if (install)
    {
        MPU_ATTR = PRIV_DEVICE_ATTR;
        __asm__ volatile("dsb\n\tisb" ::: "memory");
    }
    const bool valid = ((MPU_CTRL & 1U) != 0U) &&
        ((MPU_BASE & 0xFFFFFFE0UL) == 0x40000000UL) && (MPU_ATTR == PRIV_DEVICE_ATTR);
    MPU_SELECT = selected;
    return valid;
}

static void unprivileged_probe(void *argument)
{
    uint32_t control;
    (void)argument;

    __asm__ volatile("mrs %0, control" : "=r"(control));
    if ((control & 1U) != 0U)
    {
        shared.frame.progress = 1U;
        vTaskDelay(2U);
        __asm__ volatile("mrs %0, control" : "=r"(control));
        if ((control & 1U) != 0U)
        {
#ifdef OSR_TEST_DEVICE_FAULT
            /* Read-only access to Timer1 must fault before returning a value. */
            (void)*(volatile const uint32_t *)0x40001004UL;
            osr_boot_test_fail();
#endif
            const uint8_t sample = 37U;
            shared.frame.mailbox.timestamp_ms = 0U; /* Start of the monitoring epoch. */
            shared.frame.mailbox.schema_id = 1U;
            shared.frame.mailbox.data_len = 1U;
            shared.frame.mailbox.data[0] = sample;
            shared.frame.mailbox.crc = safety_crc32(&sample, 1U);
            __asm__ volatile("dmb" ::: "memory");
            shared.frame.progress = 2U;
        }
    }

    for (;;)
    {
        vTaskDelay(10U);
    }
}

static bool sample_valid(void *ctx, const uint8_t *data, uint32_t len, uint32_t schema)
{
    (void)ctx;
    return schema == 1U && len == 1U && data[0] <= 100U;
}

static void request_stop(void *ctx)
{
    (void)ctx;
    stop_requested = true;
}

void osr_watchdog_nmi(void);
void osr_watchdog_nmi(void)
{
#ifdef OSR_TEST_STALL_RESET
    bool output_was_safe = !mps2_output_enabled();
#endif
    mps2_output_set_enabled(false);
    if (!mps2_watchdog_first_timeout())
    {
        osr_boot_test_fail();
    }
#ifdef OSR_TEST_STALL_RESET
    if (!monitor_stalled || !output_was_safe || accepted == 0U ||
        stale_count != SAFETY_MAILBOX_FAILURE_LIMIT || !stop_requested)
    {
        osr_boot_test_fail();
    }
    stall_witness = STALL_WITNESS;
    __asm__ volatile("dsb" ::: "memory");
#endif
#ifdef OSR_TEST_RESET_HISTORY
    timeout_count++;
    __asm__ volatile("dsb" ::: "memory");
#endif
    /* Leave the interrupt asserted for the peripheral's second timeout. */
    for (;;)
    {
        __asm__ volatile("wfi");
    }
}

static void system_reset(void *ctx)
{
    volatile uint32_t * const aircr = (volatile uint32_t *)0xE000ED0CUL;
    (void)ctx;
    __asm__ volatile("cpsid i\n\tdsb" ::: "memory");
#ifdef OSR_TEST_IVT_RESET
    if (notified_level != SAFETY_SS_LEVEL3_EMERGENCY_RESET)
    {
        osr_boot_test_fail();
    }
    reset_witness = RESET_WITNESS;
    __asm__ volatile("dsb" ::: "memory");
#endif
#ifdef OSR_TEST_DEVICE_FAULT
    if (notified_level != SAFETY_SS_LEVEL3_EMERGENCY_RESET)
    {
        osr_boot_test_fail();
    }
    device_witness = DEVICE_WITNESS;
    __asm__ volatile("dsb" ::: "memory");
#endif
#ifdef OSR_TEST_RESET_HISTORY
    if (timeout_count != 3U || notified_level != SAFETY_SS_LEVEL3_EMERGENCY_RESET ||
        mps2_output_enabled())
    {
        osr_boot_test_fail();
    }
    level3_reset = 1U;
    __asm__ volatile("dsb" ::: "memory");
#endif
    *aircr = 0x05FA0004UL | (*aircr & 0x700U);
    __asm__ volatile("dsb" ::: "memory");
    for (;;)
    {
        __asm__ volatile("wfi");
    }
}

void osr_memmanage(void);
void osr_memmanage(void)
{
#ifdef OSR_TEST_DEVICE_FAULT
    const uint32_t cfsr = *(volatile const uint32_t *)0xE000ED28UL;
    const uint32_t address = *(volatile const uint32_t *)0xE000ED34UL;
    if ((cfsr & 0xFFU) != 0x82U || address != 0x40001004UL)
    {
        osr_boot_test_fail();
    }
#endif
    (void)safety_ss_enter(&monitor.state, SAFETY_SS_LEVEL3_EMERGENCY_RESET);
    system_reset(NULL);
}

static void notify_level(void *ctx, safety_ss_level_t level)
{
    (void)ctx;
    if (level >= SAFETY_SS_LEVEL2_CONTROLLED_STOP)
    {
        mps2_output_set_enabled(false);
    }
    notified_level = level;
}

static void monitor_cycle(uint32_t now_ms)
{
    uint8_t sample = 0U;
    safety_status_t status;
    if (!peripheral_policy(false) ||
        safety_ivt_check(SCB_VTOR, osr_vectors, vector_bytes(), vector_crc) != SAFETY_OK)
    {
        (void)safety_ss_enter(&monitor.state, SAFETY_SS_LEVEL3_EMERGENCY_RESET);
        osr_boot_test_fail();
    }
    if (safety_wdg_checkpoint(&watchdog, CHECK_PERIPHERAL_MPU) != SAFETY_OK ||
        safety_wdg_checkpoint(&watchdog, CHECK_VECTOR) != SAFETY_OK)
    {
        osr_boot_test_fail();
    }
    if (!stop_requested)
    {
        status = safety_channel_cycle(&monitor, now_ms, &shared.frame.mailbox, &sample, 1U, NULL);
        if (status == SAFETY_OK && sample == 37U)
        {
            accepted++;
        }
        else if (status == SAFETY_ERR_STALE)
        {
            stale_count++;
            if (stale_count < SAFETY_MAILBOX_FAILURE_LIMIT &&
                notified_level != SAFETY_SS_LEVEL1_DEGRADED)
            {
                osr_boot_test_fail();
            }
        }
        else
        {
            osr_boot_test_fail();
        }
    }
    if (stop_requested && mps2_output_enabled())
    {
        system_reset(NULL);
    }
    if (safety_wdg_checkpoint(&watchdog, CHECK_CHANNEL_RESPONSE) != SAFETY_OK ||
        safety_wdg_cycle_end(&watchdog) != SAFETY_OK)
    {
        osr_boot_test_fail();
    }
#ifdef OSR_TEST_STALL_RESET
    /* Stay in IRQ8 after several complete watchdog windows. NMI must still run. */
    if (stop_requested && now_ms >= 2U * SAFETY_WDG_TIMEOUT_MS)
    {
        monitor_stalled = true;
        for (;;)
        {
            __asm__ volatile("wfi");
        }
    }
#endif
}

static void verify_scheduler(void *argument)
{
    (void)argument;

    /* This privileged verifier is the first and highest-priority task.
     * Install the AN386 device policy before releasing the QM publisher. */
    if (!peripheral_policy(true))
    {
        osr_boot_test_fail();
    }
    vTaskDelay(10U);
    if ((MPU_CTRL & 1U) == 0U || shared.frame.progress != 2U)
    {
        osr_boot_test_fail();
    }

    mps2_output_set_enabled(true);
    if (!mps2_output_enabled())
    {
        osr_boot_test_fail();
    }

    /* Model a QM critical section: SysTick/PendSV are masked but IRQ8
     * must still validate the mailbox and detect a stalled publisher. */
    taskENTER_CRITICAL();
    if (!mps2_safety_timer_start(2U, monitor_cycle, osr_boot_test_fail))
    {
        osr_boot_test_fail();
    }
#ifdef OSR_TEST_IVT_RESET
    /* Select the identical reset-vector copy. IRQ8 still dispatches while
     * the safety monitor can observe the unexpected table address. */
    *(volatile uint32_t *)0xE000ED08UL = 0U;
    __asm__ volatile("dsb\n\tisb" ::: "memory");
#endif
#ifdef OSR_TEST_STALL_RESET
    /* This task never releases BASEPRI or participates in the stop action. */
    for (;;)
    {
        __asm__ volatile("wfi");
    }
#endif
    while (!stop_requested)
    {
        __asm__ volatile("wfi");
    }
    taskEXIT_CRITICAL();

    /* RTOS APIs execute in task context, outside the high-priority ISR. */
    vTaskSuspend(probe_handle);
    if (mps2_output_enabled() || accepted == 0U || stale_count != SAFETY_MAILBOX_FAILURE_LIMIT ||
        safety_ss_get_level(&monitor.state) != SAFETY_SS_LEVEL2_CONTROLLED_STOP ||
        notified_level != SAFETY_SS_LEVEL2_CONTROLLED_STOP ||
        eTaskGetState(probe_handle) != eSuspended)
    {
        osr_boot_test_fail();
    }
#if defined(OSR_TEST_IVT_RESET) || defined(OSR_TEST_DEVICE_FAULT) || defined(OSR_TEST_STALL_RESET)
    osr_boot_test_fail();
#endif
    finish("Cortex-M4 timer, mailbox and controlled stop passed\n", 0U);
}

void osr_boot_test_main(void);

void osr_boot_test_main(void)
{
    const TaskParameters_t probe = {
        .pvTaskCode = unprivileged_probe,
        .pcName = "probe",
        .usStackDepth = sizeof(probe_stack) / sizeof(probe_stack[0]),
        .uxPriority = 1U,
        .puxStackBuffer = probe_stack,
        .xRegions = {
            { (void *)&shared, sizeof(shared), portMPU_REGION_READ_WRITE
                | portMPU_REGION_EXECUTE_NEVER }
        },
        .pxTaskBuffer = &probe_tcb
    };

    static const safety_ss_ops_t state_ops = {
        .suspend_qm = request_stop, .app_notify = notify_level, .system_reset = system_reset
    };
#ifdef OSR_TEST_IVT_RESET
    if (reset_witness == RESET_WITNESS)
    {
        reset_witness = 0U;
        finish("VTOR mismatch entered Level 3 and restarted the Cortex-M4\n", 0U);
    }
#endif
    static const safety_wdg_ops_t watchdog_ops = { .hw_kick = mps2_watchdog_kick };
#ifdef OSR_TEST_STALL_RESET
    if (stall_witness == STALL_WITNESS)
    {
        stall_witness = 0U;
        finish("Scheduler stall inhibited output; watchdog NMI and second-stage reset passed\n", 0U);
    }
#endif
#ifdef OSR_TEST_DEVICE_FAULT
    if (device_witness == DEVICE_WITNESS)
    {
        device_witness = 0U;
        finish("QM timer read denied by MPU; Level 3 reset passed\n", 0U);
    }
#endif
    mps2_output_set_enabled(false);
    if (!mps2_watchdog_start(SAFETY_WDG_TIMEOUT_MS) ||
        safety_wdg_init(&watchdog, &watchdog_ops, CHECK_VECTOR | CHECK_CHANNEL_RESPONSE | CHECK_PERIPHERAL_MPU) != SAFETY_OK)
    {
        osr_boot_test_fail();
    }
#ifdef OSR_TEST_RESET_HISTORY
    safety_reset_cause_t cause = SAFETY_RESET_POWER_ON;
    if (level3_reset == 1U)
    {
        cause = SAFETY_RESET_SOFTWARE;
    }
    else if (timeout_count != 0U)
    {
        cause = SAFETY_RESET_WATCHDOG;
    }
    const safety_ss_level_t level = safety_boot_check_reset_cause(&history, cause);
    if (history.consecutive != timeout_count || timeout_count > 3U ||
        safety_ss_init(&monitor.state, &state_ops) != SAFETY_OK)
    {
        osr_boot_test_fail();
    }
    if (level3_reset == 1U && timeout_count == 3U &&
        level == SAFETY_SS_LEVEL3_EMERGENCY_RESET && !mps2_output_enabled())
    {
        finish("Three watchdog resets escalated to Level 3; warm restart preserved latch\n", 0U);
    }
    if (timeout_count > 0U)
    {
        const safety_ss_level_t expected = timeout_count == 3U ?
            SAFETY_SS_LEVEL3_EMERGENCY_RESET : SAFETY_SS_LEVEL2_CONTROLLED_STOP;
        if (level != expected || safety_ss_enter(&monitor.state, level) != SAFETY_OK ||
            !stop_requested || mps2_output_enabled() || timeout_count == 3U)
        {
            osr_boot_test_fail();
        }
    }
    else if (level != SAFETY_SS_LEVEL0_NORMAL)
    {
        osr_boot_test_fail();
    }
    for (;;)
    {
        __asm__ volatile("wfi");
    }
#endif
    vector_crc = safety_crc32(osr_vectors, vector_bytes());
    if (SCB_VTOR != (uintptr_t)osr_vectors ||
        safety_channel_init(&monitor, &state_ops, sample_valid) != SAFETY_OK ||
        initialized_word != INITIALIZED_WORD || shared.frame.progress != 0U
        || xTaskCreateRestrictedStatic(&probe, &probe_handle) != pdPASS
        || xTaskCreateStatic(verify_scheduler, "verify", configMINIMAL_STACK_SIZE,
                             NULL, 2U | portPRIVILEGE_BIT, verifier_stack, &verifier_tcb) == NULL)
    {
        osr_boot_test_fail();
    }

    vTaskStartScheduler();
    osr_boot_test_fail();
}
