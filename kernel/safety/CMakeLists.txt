# ---------------------------------------------------------------------------
# kernel/safety/CMakeLists.txt
# Target: opensafety_safety_function  (STATIC)
#
# This library contains ASIL-D safety functions. It is compiled with strict
# safety flags and isolated in its own partition so that QM (non-safety) code
# cannot be inadvertently linked against it without deliberate dependency.
#
# Sources are portable C99 — hardware/RTOS access is injected via ops
# structs — so the identical code is unit-tested on the host. Unit tests
# live in tests/unit/ (a standalone host-compiled CMake project).
# ---------------------------------------------------------------------------

add_library(opensafety_safety_function STATIC
    src/crc32.c
    src/mailbox.c
    src/dma_guard.c
    src/reset_history.c
    src/ivt_monitor.c
    src/channel_monitor.c
    src/safe_state.c
    src/watchdog.c
)

# ASIL-D partition marker — visible to all consumers
target_compile_definitions(opensafety_safety_function
    PUBLIC
        SAFETY_ASIL_D_PARTITION=1
        SAFETY_ASIL_D=1
)

# Public include directory for safety API headers
target_include_directories(opensafety_safety_function
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# When SAFETY_ASIL_D is ON, add strict safety flags to this target.
# (These are already in CMAKE_C_FLAGS via the toolchain file when cross-
# compiling, but we also set them explicitly here so host-build
# configurations pick them up.)
if(SAFETY_ASIL_D AND OPENSAFETY_SAFETY_FLAGS)
    separate_arguments(_osr_safety_flags UNIX_COMMAND "${OPENSAFETY_SAFETY_FLAGS}")
    target_compile_options(opensafety_safety_function
        PRIVATE
            ${_osr_safety_flags}
    )
endif()

# Link the safety library into the main kernel interface
target_link_libraries(opensafety_kernel
    INTERFACE
        opensafety_safety_function
)

# ---------------------------------------------------------------------------
# Installation rules
# ---------------------------------------------------------------------------
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include/opensafety/safety
    FILES_MATCHING PATTERN "*.h"
)
