# ---------------------------------------------------------------------------
# hal/stm32/CMakeLists.txt
# Target: opensafety_hal_stm32
#
# STM32-specific HAL implementation — INTERFACE stub (Phase 0).
# ---------------------------------------------------------------------------

add_library(opensafety_hal_stm32 INTERFACE)

target_include_directories(opensafety_hal_stm32
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_definitions(opensafety_hal_stm32
    INTERFACE
        HAL_STM32=1
)

target_link_libraries(opensafety_hal_stm32
    INTERFACE
        opensafety_hal
)
