# ---------------------------------------------------------------------------
# tests/unit/CMakeLists.txt — STANDALONE host-compiled unit test project.
#
# This is deliberately its own project(): the main build tree is cross-
# compiled for ARM (cmake/toolchain-arm-cortex-m4.cmake), while unit tests
# must build and run on the host. Do NOT add_subdirectory() this file from
# the cross build.
#
# Usage (from the repository root):
#   cmake -S tests/unit -B build-host
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# ---------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)

project(OpenSafetyRTOS-UnitTests LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_testing()

set(OSR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

add_compile_options(-Wall -Wextra -Wconversion -Wshadow -Wundef)

# The code under test: the exact same sources the target build compiles
# (hardware access is injected via ops structs; target-only register
# back-ends are guarded by __arm__ and drop out on the host).
add_library(osr_under_test STATIC
    ${OSR_ROOT}/kernel/safety/src/crc32.c
    ${OSR_ROOT}/kernel/safety/src/mailbox.c
    ${OSR_ROOT}/kernel/safety/src/safe_state.c
    ${OSR_ROOT}/kernel/safety/src/watchdog.c
    ${OSR_ROOT}/arch/arm-cortex-m/src/mpu.c
    ${OSR_ROOT}/arch/arm-cortex-m/src/fault_handlers.c
)

target_include_directories(osr_under_test
    PUBLIC
        ${OSR_ROOT}/kernel/safety/include
        ${OSR_ROOT}/arch/arm-cortex-m/include
)

function(osr_add_unit_test name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE osr_under_test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

osr_add_unit_test(test_crc32)
osr_add_unit_test(test_mailbox)
osr_add_unit_test(test_safe_state)
osr_add_unit_test(test_watchdog)
osr_add_unit_test(test_mpu)
osr_add_unit_test(test_fault)
