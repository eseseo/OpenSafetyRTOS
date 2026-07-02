# ---------------------------------------------------------------------------
# arch/arm-cortex-m/CMakeLists.txt
# Target: opensafety_arch_arm_cortex_m  (STATIC)
#
# ARMv7-M MPU configuration and fault handlers. The register back-end and
# exception entry points are guarded by __arm__, so the pure logic (shadow
# CRC, region validation, integrity compare) also compiles on the host and
# is unit-tested by the standalone tests/unit project.
# ---------------------------------------------------------------------------

add_library(opensafety_arch_arm_cortex_m STATIC
    src/mpu.c
    src/fault_handlers.c
)

target_include_directories(opensafety_arch_arm_cortex_m
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(opensafety_arch_arm_cortex_m
    PUBLIC
        ARCH_ARM_CORTEX_M=1
        ARCH_ARM_CORTEX_M4=1
)

# MPU shadow integrity uses the safety CRC-32; fault handlers escalate via
# the safe-state manager.
target_link_libraries(opensafety_arch_arm_cortex_m
    PUBLIC
        opensafety_safety_function
)

# Alias for cleaner linking in dependent targets
add_library(opensafety::arch ALIAS opensafety_arch_arm_cortex_m)
