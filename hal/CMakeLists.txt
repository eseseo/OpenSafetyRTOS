# ---------------------------------------------------------------------------
# hal/CMakeLists.txt
# Target: opensafety_hal
#
# Hardware Abstraction Layer — INTERFACE library (Phase 0 stub).
# ---------------------------------------------------------------------------

add_library(opensafety_hal INTERFACE)

target_include_directories(opensafety_hal
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Pull in architecture back-end
target_link_libraries(opensafety_hal
    INTERFACE
        opensafety::arch
)

# STM32 HAL back-end (only when targeting Cortex-M with STM32)
if(TARGET_ARCH STREQUAL "arm-cortex-m")
    add_subdirectory(stm32)
endif()

add_library(opensafety::hal ALIAS opensafety_hal)
