/* SPDX-License-Identifier: Apache-2.0 */
/* SSR-021/022. Destructive word March C- before any use of the C stack.
 * Inputs r0=start, r1=end (exclusive). LR is the only return storage. */
.syntax unified
.thumb
.section .text.boot_post,"ax",%progbits
.global osr_march_ram
.type osr_march_ram,%function
.thumb_func
osr_march_ram:
    cmp r0, r1
    bhs 9f
    movs r4, #0
    mvns r5, r4
    mov r2, r0
1:  str r4, [r2], #4
    cmp r2, r1
    blo 1b
#ifdef OSR_TEST_RAM_POST
    str r5, [r0]           /* Wrong word before the first verification pass. */
#endif

.macro march_up expected, next
    mov r2, r0
1:  ldr r3, [r2]
    cmp r3, \expected
    bne osr_ram_failure
    str \next, [r2], #4
    cmp r2, r1
    blo 1b
.endm

.macro march_down expected, next
    mov r2, r1
1:  ldr r3, [r2, #-4]!
    cmp r3, \expected
    bne osr_ram_failure
    str \next, [r2]
    cmp r2, r0
    bhi 1b
.endm
    march_up r4, r5
    march_up r5, r4
    march_down r4, r5
    march_down r5, r4
    mov r2, r0
1:  ldr r3, [r2], #4
    cmp r3, r4
    bne osr_ram_failure
    cmp r2, r1
    blo 1b
9:  bx lr

/* Test-fixture POST failures use only registers and read-only literals. */
osr_ram_failure:
    ldr r1, =ram_error
    b post_failure
.global osr_rom_failure
.thumb_func
osr_rom_failure:
    ldr r1, =rom_error
post_failure:
    ldr r2, =0x40028000
    movs r3, #0
    str r3, [r2]
    dsb
    movs r0, #4
    bkpt 0xab
    ldr r1, =exit_args
    movs r0, #0x20
    bkpt 0xab
1:  b 1b
.section .rodata.boot_post,"a",%progbits
ram_error: .asciz "POST rejected SRAM before scheduler start\n"
rom_error: .asciz "POST rejected image before scheduler start\n"
.balign 4
exit_args: .word 0x20026, 2
.section .note.GNU-stack,"",%progbits
