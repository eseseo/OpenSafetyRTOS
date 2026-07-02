# ---------------------------------------------------------------------------
# tests/CMakeLists.txt
#
# Unit tests are a STANDALONE host project (tests/unit has its own
# project()) and are deliberately NOT included here: this build tree may be
# cross-compiled with arm-none-eabi-gcc, while unit tests must build and
# run on the host. Build and run them with:
#
#   cmake -S tests/unit -B build-host
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
#
# Integration tests (QEMU / hardware-in-the-loop) belong to this build tree.
# ---------------------------------------------------------------------------

add_subdirectory(integration)
