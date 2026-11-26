/* SPDX-License-Identifier: Apache-2.0 */
/* QEMU MPS2 AN386 boot RAM and SRAM windows. */
ENTRY(Reset_Handler)

MEMORY
{
    BOOT (rx) : ORIGIN = 0x00000000, LENGTH = 256K
    SRAM (rw) : ORIGIN = 0x20000000, LENGTH = 64K
}

__FLASH_segment_start__ = ORIGIN(BOOT);
__FLASH_segment_end__ = ORIGIN(BOOT) + LENGTH(BOOT);
__SRAM_segment_start__ = ORIGIN(SRAM);
__SRAM_segment_end__ = ORIGIN(SRAM) + LENGTH(SRAM);
__stack_top__ = __SRAM_segment_end__;
__stack_size__ = 2K;

SECTIONS
{
    /* This region overlays the read-only application code MPU region. */
    .privileged_functions :
    {
        __privileged_functions_start__ = .;
        KEEP(*(.isr_vector))
        *(privileged_functions)
        __privileged_functions_used__ = .;
        . = __privileged_functions_start__ + 32K;
        __privileged_functions_end__ = .;
    } > BOOT

    .text :
    {
        *(.text .text.*)
        . = ALIGN(4);
        __syscalls_flash_start__ = .;
        *(freertos_system_calls)
        __syscalls_flash_end__ = .;
        *(.rodata .rodata.*)
    } > BOOT

    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > BOOT
    .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > BOOT

    .data :
    {
        __data_start__ = .;
        __privileged_data_start__ = .;
        *(privileged_data)
        __privileged_data_used__ = .;
        . = __privileged_data_start__ + 4K;
        __privileged_data_end__ = .;
        *(.data .data.*)
        . = ALIGN(4);
        __data_end__ = .;
    } > SRAM AT > BOOT
    __data_load__ = LOADADDR(.data);

    .image_crc :
    {
        __image_crc = .;
        LONG(0)
    } > BOOT

    .bss (NOLOAD) :
    {
        __bss_start__ = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > SRAM

    .noinit (NOLOAD) :
    {
        __retained_start__ = .;
        *(.noinit)
        . = ALIGN(4);
        __ram_used_end__ = .;
    } > SRAM
}

ASSERT(__privileged_functions_used__ <= __privileged_functions_end__,
       "FreeRTOS privileged code exceeds its MPU region")
ASSERT(__privileged_data_used__ <= __privileged_data_end__,
       "FreeRTOS privileged data exceeds its MPU region")
ASSERT(__ram_used_end__ <= __stack_top__ - __stack_size__, "SRAM overlaps the reset stack")

ASSERT(osr_vectors_end - osr_vectors == 48 * 4, "AN386 requires 48 vector entries")
ASSERT((osr_vectors & 255) == 0, "AN386 vector table alignment")

ASSERT(__image_crc == LOADADDR(.data) + SIZEOF(.data), "CRC follows the last load byte")
