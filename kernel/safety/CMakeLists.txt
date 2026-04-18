# ---------------------------------------------------------------------------
# kernel/safety/CMakeLists.txt
# Target: opensafety_safety_function
#
# This library contains ASIL-D safety functions. It is compiled with strict
# safety flags and isolated in its own partition so that QM (non-safety) code
# cannot be inadvertently linked against it without deliberate dependency.
#
# INTERFACE library: no .c sources in Phase 0. Change to STATIC when
# kernel/safety/src/*.c files are added.
# ---------------------------------------------------------------------------

add_library(opensafety_safety_function INTERFACE)

# ASIL-D partition marker — visible to all consumers
target_compile_definitions(opensafety_safety_function
    INTERFACE
        SAFETY_ASIL_D_PARTITION=1
        SAFETY_ASIL_D=1
)

# Public include directory for safety API headers
target_include_directories(opensafety_safety_function
    INTERFACE
        ${CMAKE_SOURCE_DIR}/kernel/safety/include
)

# When SAFETY_ASIL_D is ON, add strict safety flags to this target.
# (These are already in CMAKE_C_FLAGS via the toolchain file when cross-
# compiling, but we also set them explicitly here so host-build / unit-test
# configurations pick them up.)
if(SAFETY_ASIL_D AND OPENSAFETY_SAFETY_FLAGS)
    target_compile_options(opensafety_safety_function
        INTERFACE
            ${OPENSAFETY_SAFETY_FLAGS}
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
install(DIRECTORY ${CMAKE_SOURCE_DIR}/kernel/safety/include/
    DESTINATION include/opensafety/safety
    FILES_MATCHING PATTERN "*.h"
)
