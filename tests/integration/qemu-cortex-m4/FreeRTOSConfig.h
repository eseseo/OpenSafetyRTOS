/* SPDX-License-Identifier: Apache-2.0 */
#ifndef OSR_QEMU_FREERTOS_CONFIG_H
#define OSR_QEMU_FREERTOS_CONFIG_H

#define configCPU_CLOCK_HZ                       25000000UL
#define configTICK_RATE_HZ                       1000U
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configMAX_PRIORITIES                     3U
#define configMINIMAL_STACK_SIZE                 128U
#define configMAX_TASK_NAME_LEN                  8U

#define configUSE_PREEMPTION                     1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      0
#define configUSE_TIMERS                         0
#define configUSE_TRACE_FACILITY                 0
#define configQUEUE_REGISTRY_SIZE                0
#define configSUPPORT_STATIC_ALLOCATION          1
#define configSUPPORT_DYNAMIC_ALLOCATION         0

#define configENABLE_MPU                         1
#define configUSE_MPU_WRAPPERS_V1                0
#define configENFORCE_SYSTEM_CALLS_FROM_KERNEL_ONLY 1
#define configSYSTEM_CALL_STACK_SIZE             128U
#define configPROTECTED_KERNEL_OBJECT_POOL_SIZE  4
#define configKERNEL_INTERRUPT_PRIORITY          0xE0U
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     0x80U

#define INCLUDE_vTaskSuspend                     1
#define INCLUDE_eTaskGetState                    1
#define INCLUDE_vTaskDelay                       1

void osr_boot_test_fail(void) __attribute__((noreturn));
#define configASSERT(condition) do { if (!(condition)) { osr_boot_test_fail(); } } while (0)

#endif
