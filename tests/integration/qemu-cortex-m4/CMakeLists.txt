include(FetchContent)

find_program(QEMU_SYSTEM_ARM qemu-system-arm REQUIRED)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
set(FREERTOS_PORT GCC_ARM_CM4_MPU)

# FreeRTOS-Kernel V11.3.1 (MIT).
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
FetchContent_Declare(freertos_kernel
    URL https://codeload.github.com/FreeRTOS/FreeRTOS-Kernel/tar.gz/3a22924e0a9ddbbc8b0758881c33b3422a5cc20d
    URL_HASH SHA256=e7d25915c043f4695eb066117cc60a42e7c58e421d971ebaf6fe2ec97208d658
)
FetchContent_MakeAvailable(freertos_kernel)

set(linker_script ${CMAKE_CURRENT_SOURCE_DIR}/memory.ld)
add_executable(osr_qemu_cortex_m4 startup.S boot_test.c)
set_target_properties(osr_qemu_cortex_m4 PROPERTIES
    SUFFIX ".elf"
    LINK_DEPENDS "${linker_script}"
)
target_link_libraries(osr_qemu_cortex_m4 PRIVATE freertos_kernel)
target_link_options(osr_qemu_cortex_m4 PRIVATE
    -nostartfiles
    "LINKER:-T,${linker_script}"
    "LINKER:-z,noexecstack"
)

add_test(NAME qemu_cortex_m4_boot
    COMMAND "${QEMU_SYSTEM_ARM}" -machine mps2-an386 -display none -serial none -monitor none
        -semihosting-config enable=on,target=native -kernel $<TARGET_FILE:osr_qemu_cortex_m4>
)
set_tests_properties(qemu_cortex_m4_boot PROPERTIES TIMEOUT 15)
