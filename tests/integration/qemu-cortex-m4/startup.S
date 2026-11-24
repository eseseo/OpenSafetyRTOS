/* SPDX-License-Identifier: Apache-2.0 */
.syntax unified
.thumb

.section .isr_vector, "a", %progbits
.word __stack_top__
.word Reset_Handler
.word osr_boot_test_fail         /* NMI */
.word osr_boot_test_fail         /* HardFault */
.word osr_boot_test_fail         /* MemManage */
.word osr_boot_test_fail         /* BusFault */
.word osr_boot_test_fail         /* UsageFault */
.word 0, 0, 0, 0
.word vPortSVCHandler
.word osr_boot_test_fail         /* DebugMonitor */
.word 0
.word xPortPendSVHandler
.word xPortSysTickHandler

.section .text.Reset_Handler, "ax", %progbits
.global Reset_Handler
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    /* Enable CP10 and CP11 before entering hard-float C code. */
    ldr r0, =0xE000ED88
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]
    dsb
    isb

    ldr r0, =__data_load__
    ldr r1, =__data_start__
    ldr r2, =__data_end__
1:
    cmp r1, r2
    bhs 2f
    ldr r3, [r0], #4
    str r3, [r1], #4
    b 1b

2:
    ldr r1, =__bss_start__
    ldr r2, =__bss_end__
    movs r3, #0
3:
    cmp r1, r2
    bhs 4f
    str r3, [r1], #4
    b 3b

4:
    bl osr_boot_test_main
    b osr_boot_test_fail
.size Reset_Handler, . - Reset_Handler

.section .note.GNU-stack, "", %progbits
