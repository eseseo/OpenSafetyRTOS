# ---------------------------------------------------------------------------
# arch/CMakeLists.txt
# Selects the correct architecture back-end based on TARGET_ARCH.
# ---------------------------------------------------------------------------

message(STATUS "Architecture selected: ${TARGET_ARCH}")

if(TARGET_ARCH STREQUAL "arm-cortex-m")
    add_subdirectory(arm-cortex-m)

elseif(TARGET_ARCH STREQUAL "riscv")
    add_subdirectory(riscv)

else()
    message(FATAL_ERROR
        "Unsupported TARGET_ARCH='${TARGET_ARCH}'. "
        "Valid values: arm-cortex-m, riscv"
    )
endif()
