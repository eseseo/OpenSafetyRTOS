include(FetchContent)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
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
function(add_an386_image target)
    add_executable(${target} startup.S boot_post.S boot_test.c ${PROJECT_SOURCE_DIR}/hal/mps2/safety_timer.c
        ${PROJECT_SOURCE_DIR}/hal/mps2/safety_io.c)
    target_include_directories(${target} PRIVATE ${PROJECT_SOURCE_DIR}/hal/mps2)
    set_target_properties(${target} PROPERTIES SUFFIX ".elf" LINK_DEPENDS "${linker_script};${CMAKE_CURRENT_SOURCE_DIR}/image_crc.py")
    target_link_libraries(${target} PRIVATE freertos_kernel opensafety_safety_function)
    target_link_options(${target} PRIVATE -nostartfiles "LINKER:-T,${linker_script}"
        "LINKER:-z,noexecstack")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/image_crc.py"
            "${CMAKE_OBJCOPY}" $<TARGET_FILE:${target}> VERBATIM)
endfunction()

function(add_an386_test target test_name)
    add_an386_image(${target})
    add_test(NAME ${test_name}
        COMMAND "${QEMU_SYSTEM_ARM}" -machine mps2-an386 -display none -serial none -monitor none
            -semihosting-config enable=on,target=native -kernel $<TARGET_FILE:${target}>
    )
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 15)
endfunction()

add_an386_test(osr_qemu_cortex_m4 qemu_cortex_m4_boot)
add_an386_test(osr_qemu_ivt_reset qemu_ivt_level3_reset)
target_compile_definitions(osr_qemu_ivt_reset PRIVATE OSR_TEST_IVT_RESET=1)

add_an386_test(osr_qemu_stall_reset qemu_stall_output_watchdog_reset)
target_compile_definitions(osr_qemu_stall_reset PRIVATE OSR_TEST_STALL_RESET=1)

add_an386_test(osr_qemu_device_fault qemu_qm_timer_access_denied)
target_compile_definitions(osr_qemu_device_fault PRIVATE OSR_TEST_DEVICE_FAULT=1)

add_an386_image(osr_qemu_ram_post)
target_compile_definitions(osr_qemu_ram_post PRIVATE OSR_TEST_RAM_POST=1)
add_test(NAME qemu_ram_post_rejection COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/post_rejection.py" "${QEMU_SYSTEM_ARM}" "${CMAKE_OBJCOPY}"
    $<TARGET_FILE:osr_qemu_ram_post> SRAM)
add_test(NAME qemu_rom_post_rejection COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/post_rejection.py" "${QEMU_SYSTEM_ARM}" "${CMAKE_OBJCOPY}"
    $<TARGET_FILE:osr_qemu_cortex_m4> image)

add_an386_test(osr_qemu_reset_history qemu_consecutive_watchdog_resets)
target_compile_definitions(osr_qemu_reset_history PRIVATE OSR_TEST_RESET_HISTORY=1)
