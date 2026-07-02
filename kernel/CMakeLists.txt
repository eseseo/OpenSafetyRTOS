# ---------------------------------------------------------------------------
# kernel/CMakeLists.txt
# Target: opensafety_kernel
#
# INTERFACE umbrella target: aggregates the kernel-facing pieces. The
# SafetyFunction partition below (kernel/safety) is a real STATIC library.
# QM kernel glue (kernel/src/*.c, FreeRTOS integration) is still to come;
# when added, change INTERFACE -> STATIC and move sources into
# target_sources(... PRIVATE ...).
# ---------------------------------------------------------------------------

add_library(opensafety_kernel INTERFACE)

# Public include directory — consumers get kernel/include on their include
# path (created together with the first QM kernel sources).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(opensafety_kernel
        INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
endif()

# Propagate global compile definitions to consumers
target_compile_definitions(opensafety_kernel
    INTERFACE
        $<$<BOOL:${SAFETY_ASIL_D}>:SAFETY_ASIL_D=1>
)

# ---------------------------------------------------------------------------
# Safety sub-library (ASIL-D partition) — STATIC, see kernel/safety/
# ---------------------------------------------------------------------------
add_subdirectory(safety)

# ---------------------------------------------------------------------------
# Installation rules (for SDK / package usage)
# ---------------------------------------------------------------------------
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/opensafety/kernel
        FILES_MATCHING PATTERN "*.h"
    )
endif()
