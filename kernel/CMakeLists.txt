# ---------------------------------------------------------------------------
# kernel/CMakeLists.txt
# Target: opensafety_kernel
#
# INTERFACE library: no .c sources exist yet in Phase 0.
# When kernel/src/*.c files are added, change INTERFACE -> STATIC and
# move sources into target_sources(... PRIVATE ...).
# ---------------------------------------------------------------------------

add_library(opensafety_kernel INTERFACE)

# Public include directory — consumers get kernel/include on their include path
target_include_directories(opensafety_kernel
    INTERFACE
        ${CMAKE_SOURCE_DIR}/kernel/include
)

# Propagate global compile definitions to consumers
target_compile_definitions(opensafety_kernel
    INTERFACE
        $<$<BOOL:${SAFETY_ASIL_D}>:SAFETY_ASIL_D=1>
)

# ---------------------------------------------------------------------------
# Safety sub-library (ASIL-D partition)
# ---------------------------------------------------------------------------
add_subdirectory(safety)

# ---------------------------------------------------------------------------
# Installation rules (for SDK / package usage)
# ---------------------------------------------------------------------------
install(DIRECTORY ${CMAKE_SOURCE_DIR}/kernel/include/
    DESTINATION include/opensafety/kernel
    FILES_MATCHING PATTERN "*.h"
)
