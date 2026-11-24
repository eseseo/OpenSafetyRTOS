/* SPDX-License-Identifier: Apache-2.0 */
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#define TASK_STACK_BYTES 512U
#define PROBE_REGION_BYTES 32U
#define INITIALIZED_WORD UINT32_C(0x12345678)
#define MPU_CTRL (*(volatile const uint32_t *)UINT32_C(0xE000ED94))

static StackType_t probe_stack[TASK_STACK_BYTES / sizeof(StackType_t)]
    __attribute__((aligned(TASK_STACK_BYTES)));
static StaticTask_t probe_tcb;
static StackType_t verifier_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t verifier_tcb;
static volatile uint32_t probe_progress[PROBE_REGION_BYTES / sizeof(uint32_t)]
    __attribute__((aligned(PROBE_REGION_BYTES)));
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

static void unprivileged_probe(void *argument)
{
    uint32_t control;
    (void)argument;

    __asm__ volatile("mrs %0, control" : "=r"(control));
    if ((control & 1U) != 0U)
    {
        probe_progress[0] = 1U;
        vTaskDelay(2U);
        __asm__ volatile("mrs %0, control" : "=r"(control));
        if ((control & 1U) != 0U)
        {
            probe_progress[0] = 2U;
        }
    }

    for (;;)
    {
        vTaskDelay(10U);
    }
}

static void verify_scheduler(void *argument)
{
    (void)argument;

    vTaskDelay(10U);
    if ((MPU_CTRL & 1U) == 0U || probe_progress[0] != 2U)
    {
        osr_boot_test_fail();
    }

    finish("OpenSafetyRTOS Cortex-M4 boot passed\n", 0U);
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
            { (void *)probe_progress, sizeof(probe_progress), portMPU_REGION_READ_WRITE
                | portMPU_REGION_EXECUTE_NEVER }
        },
        .pxTaskBuffer = &probe_tcb
    };

    if (initialized_word != INITIALIZED_WORD || probe_progress[0] != 0U
        || xTaskCreateRestrictedStatic(&probe, NULL) != pdPASS
        || xTaskCreateStatic(verify_scheduler, "verify", configMINIMAL_STACK_SIZE,
                             NULL, 2U | portPRIVILEGE_BIT, verifier_stack, &verifier_tcb) == NULL)
    {
        osr_boot_test_fail();
    }

    vTaskStartScheduler();
    osr_boot_test_fail();
}
