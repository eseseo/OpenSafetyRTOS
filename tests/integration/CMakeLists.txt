# ---------------------------------------------------------------------------
# tests/integration/CMakeLists.txt
# Integration tests for the target architecture.
# ---------------------------------------------------------------------------

if(OPENSAFETY_QEMU_SMOKE)
    if(NOT CMAKE_CROSSCOMPILING OR NOT TARGET_ARCH STREQUAL "arm-cortex-m"
        OR NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm"
        OR NOT CMAKE_C_COMPILER_ID STREQUAL "GNU")
        message(FATAL_ERROR "Use cmake/toolchain-arm-cortex-m4.cmake for the QEMU boot test.")
    endif()

    add_subdirectory(qemu-cortex-m4)
endif()
