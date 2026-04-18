# ---------------------------------------------------------------------------
# tests/CMakeLists.txt
# Adds unit and integration test subdirectories for CTest.
# ---------------------------------------------------------------------------

# Unit tests — fast, no hardware dependency, run on host
add_subdirectory(unit)

# Integration tests — may require hardware-in-the-loop or QEMU
add_subdirectory(integration)
