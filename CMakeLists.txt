cmake_minimum_required(VERSION 3.20)

project(OpenSafetyRTOS
    VERSION 0.1.0
    DESCRIPTION "Safety-Critical Real-Time Operating System (ASIL-D)"
    LANGUAGES C ASM
)

# ---------------------------------------------------------------------------
# Global build options
# ---------------------------------------------------------------------------
option(SAFETY_ASIL_D
    "Enable ASIL-D safety partition compiler flags and definitions"
    ON
)

set(TARGET_ARCH "arm-cortex-m" CACHE STRING
    "Target architecture: arm-cortex-m | riscv"
)
set_property(CACHE TARGET_ARCH PROPERTY STRINGS "arm-cortex-m" "riscv")

# ---------------------------------------------------------------------------
# Toolchain guard: warn if not cross-compiling
# ---------------------------------------------------------------------------
if(NOT CMAKE_CROSSCOMPILING)
    message(WARNING
        "CMAKE_CROSSCOMPILING is not set. "
        "Did you forget to pass -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-cortex-m4.cmake ?"
    )
endif()

# ---------------------------------------------------------------------------
# Global compiler standards
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)   # -std=c11, not -std=gnu11

# ---------------------------------------------------------------------------
# Global compile definitions
# ---------------------------------------------------------------------------
if(SAFETY_ASIL_D)
    add_compile_definitions(SAFETY_ASIL_D=1)
    message(STATUS "ASIL-D safety partition: ENABLED")
else()
    message(STATUS "ASIL-D safety partition: DISABLED")
endif()

add_compile_definitions(
    OPENSAFETY_RTOS_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    OPENSAFETY_RTOS_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    OPENSAFETY_RTOS_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------
add_subdirectory(kernel)
add_subdirectory(arch)
add_subdirectory(hal)

# ---------------------------------------------------------------------------
# Testing (CTest)
# ---------------------------------------------------------------------------
include(CTest)
enable_testing()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
message(STATUS "")
message(STATUS "=== OpenSafetyRTOS v${PROJECT_VERSION} ===")
message(STATUS "  TARGET_ARCH   : ${TARGET_ARCH}")
message(STATUS "  SAFETY_ASIL_D : ${SAFETY_ASIL_D}")
message(STATUS "  BUILD_TESTING : ${BUILD_TESTING}")
message(STATUS "  Toolchain     : ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "")
