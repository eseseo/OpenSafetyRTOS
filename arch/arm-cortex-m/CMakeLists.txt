# ---------------------------------------------------------------------------
# arch/arm-cortex-m/CMakeLists.txt
# Target: opensafety_arch_arm_cortex_m
#
# INTERFACE library: no .c/.S sources yet in Phase 0.
# ---------------------------------------------------------------------------

add_library(opensafety_arch_arm_cortex_m INTERFACE)

target_include_directories(opensafety_arch_arm_cortex_m
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(opensafety_arch_arm_cortex_m
    INTERFACE
        ARCH_ARM_CORTEX_M=1
        ARCH_ARM_CORTEX_M4=1
)

# Alias for cleaner linking in dependent targets
add_library(opensafety::arch ALIAS opensafety_arch_arm_cortex_m)
