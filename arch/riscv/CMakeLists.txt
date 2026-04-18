# ---------------------------------------------------------------------------
# arch/riscv/CMakeLists.txt
# Target: opensafety_arch_riscv
#
# Placeholder for RISC-V port — Phase 0 stub.
# ---------------------------------------------------------------------------

add_library(opensafety_arch_riscv INTERFACE)

target_compile_definitions(opensafety_arch_riscv
    INTERFACE
        ARCH_RISCV=1
)

add_library(opensafety::arch ALIAS opensafety_arch_riscv)
