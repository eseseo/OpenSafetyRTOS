# ---------------------------------------------------------------------------
# Toolchain file: ARM Cortex-M4 (arm-none-eabi)
# Usage:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-cortex-m4.cmake
# ---------------------------------------------------------------------------

# Inform CMake this is a cross-compilation build
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# ---------------------------------------------------------------------------
# Toolchain executables
# ---------------------------------------------------------------------------
set(TOOLCHAIN_PREFIX "arm-none-eabi")

find_program(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}-gcc   REQUIRED)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}-gcc   REQUIRED)
find_program(CMAKE_AR           ${TOOLCHAIN_PREFIX}-ar    REQUIRED)
find_program(CMAKE_RANLIB       ${TOOLCHAIN_PREFIX}-ranlib REQUIRED)
find_program(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}-objcopy)
find_program(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}-objdump)
find_program(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}-size)

# Use GCC as the ASM compiler driver (supports .s and .S files with CPP)
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})

# ---------------------------------------------------------------------------
# CPU / FPU configuration
# ---------------------------------------------------------------------------
set(CPU_FLAGS
    "-mcpu=cortex-m4"
    "-mthumb"
    "-mfpu=fpv4-sp-d16"
    "-mfloat-abi=hard"
)
string(JOIN " " CPU_FLAGS_STR ${CPU_FLAGS})

# ---------------------------------------------------------------------------
# Base compiler flags
# ---------------------------------------------------------------------------
set(BASE_C_FLAGS
    "${CPU_FLAGS_STR}"
    "-Os"                    # Optimize for size (suitable for ROM-constrained MCUs)
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-Wconversion"
    "-Wshadow"
    "-Wundef"
    "-ffreestanding"         # Do not assume standard library / hosted environment
    "-fno-exceptions"        # No C++ exceptions (C project, but guard anyway)
    "-fno-common"            # No tentative definitions (MISRA-friendly)
    "-fno-builtin"           # Do not replace calls with builtins silently
)
string(JOIN " " BASE_C_FLAGS_STR ${BASE_C_FLAGS})

# ---------------------------------------------------------------------------
# Safety flags (enabled when SAFETY_ASIL_D=ON)
# ---------------------------------------------------------------------------
# NOTE: These are appended at the directory/target level when SAFETY_ASIL_D
# is set, but we define them here so the toolchain file is the single source
# of truth for all flag categories.
set(SAFETY_C_FLAGS
    "-fstack-usage"          # Emit .su files with per-function stack usage
    "-fdata-sections"        # Place each data object in its own section
    "-ffunction-sections"    # Place each function in its own section
    # Linker will use --gc-sections to eliminate dead code (reduces attack surface)
)
string(JOIN " " SAFETY_C_FLAGS_STR ${SAFETY_C_FLAGS})

# ---------------------------------------------------------------------------
# Apply flags via CMake variables
# ---------------------------------------------------------------------------
set(CMAKE_C_FLAGS_INIT   "${BASE_C_FLAGS_STR}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS_STR} -x assembler-with-cpp")

# Append safety flags when the option is set
# (SAFETY_ASIL_D is a CMake cache variable set in the root CMakeLists.txt)
if(SAFETY_ASIL_D)
    set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} ${SAFETY_C_FLAGS_STR}")
endif()

# ---------------------------------------------------------------------------
# Linker flags
# ---------------------------------------------------------------------------
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${CPU_FLAGS_STR} -specs=nano.specs -specs=nosys.specs --gc-sections"
)

# ---------------------------------------------------------------------------
# Prevent CMake from testing the compiler with a host-style executable
# ---------------------------------------------------------------------------
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ---------------------------------------------------------------------------
# sysroot / search paths
# ---------------------------------------------------------------------------
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)   # Host tools (cmake, python…)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)    # Target libraries
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)    # Target headers
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---------------------------------------------------------------------------
# Convenience variables exposed to the rest of the build
# ---------------------------------------------------------------------------
set(OPENSAFETY_CPU_FLAGS     "${CPU_FLAGS_STR}"     CACHE INTERNAL "CPU flags for Cortex-M4")
set(OPENSAFETY_SAFETY_FLAGS  "${SAFETY_C_FLAGS_STR}" CACHE INTERNAL "ASIL-D safety compiler flags")
