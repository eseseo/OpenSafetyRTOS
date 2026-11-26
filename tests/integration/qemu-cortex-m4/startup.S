/* SPDX-License-Identifier: Apache-2.0 */
.syntax unified
.thumb

.section .isr_vector, "a", %progbits
.macro an386_vectors
.word __stack_top__
.word Reset_Handler
.word osr_watchdog_nmi           /* CMSDK watchdog first timeout */
.word osr_boot_test_fail         /* HardFault */
.word osr_memmanage              /* MemManage */
.word osr_boot_test_fail         /* BusFault */
.word osr_boot_test_fail         /* UsageFault */
.word 0, 0, 0, 0
.word vPortSVCHandler
.word osr_boot_test_fail         /* DebugMonitor */
.word 0
.word xPortPendSVHandler
.word xPortSysTickHandler
.rept 8
.word osr_boot_test_fail
.endr
.word Timer0_Handler
.rept 23
.word osr_boot_test_fail
.endr
.endm

an386_vectors
.balign 256
.global osr_vectors
osr_vectors:
    an386_vectors
.global osr_vectors_end
osr_vectors_end:

.section .text.Reset_Handler, "ax", %progbits
.global Reset_Handler
.type Reset_Handler, %function
.thumb_func
Reset_Handler:
    cpsid i
    ldr r0, =__SRAM_segment_start__
    ldr r1, =__retained_start__
    bl osr_march_ram
    ldr r0, =__ram_used_end__
    ldr r1, =__SRAM_segment_end__
    bl osr_march_ram
    ldr r0, =0xE000ED08
    ldr r1, =osr_vectors
    str r1, [r0]
    dsb
    isb
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
    /* Address zero is a valid boot address on AN386. Read the first word
     * in assembly, then use the C CRC implementation for the remaining bytes. */
    movs r0, #0
    ldr r0, [r0]
    bl osr_image_valid
    cmp r0, #0
    beq osr_rom_failure
    cpsie i
    bl osr_boot_test_main
    b osr_boot_test_fail
.size Reset_Handler, . - Reset_Handler

.section .note.GNU-stack, "", %progbits
