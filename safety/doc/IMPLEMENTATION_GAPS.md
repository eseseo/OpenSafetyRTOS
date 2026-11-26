# Software implementation and release evidence gaps

Engineering review, 2026-11-26. Scope is the working tree based on dcc93e8,
SSRS v1.2, the host test suite and the AN386 QEMU images. The intended product
claim covers the SafetyFunction monitor over a QM FreeRTOS kernel. A claim
covering safety-rated scheduling or IPC would require separate requirements,
implementation assurance and assessment for those services.

| SSRS group | Evidence available | Evidence or implementation still needed for release |
| --- | --- | --- |
| 001–003 watchdog | Host checkpoint tests and AN386 two-stage watchdog reset after a stalled safety ISR | Select and integrate the independent watchdog. Demonstrate timeout and restart on the target. Resolve the once-per-cycle versus half-cycle kick interval in SSR-002, then establish the WCRT and FTTI bounds. |
| 004–009 partitioning | Simulated MPU write/readback and shadow comparison. AN386 peripheral access is privileged. An unprivileged timer read raises MemManage and resets the CPU. QEMU checks VTOR and the table CRC each monitor cycle and demonstrates a Level 3 reset after relocation. Host tests check source and destination windows before a DMA backend is called. | Production linker map, production timer and DMA configuration ownership, device-level DMA validation, and reconciliation of FreeRTOS context-switch regions with the static MPU shadow. Verify the effective permissions on the selected MCU. |
| 010–013 timing and safe states | IRQ8 runs during a FreeRTOS critical section on AN386. Tests observe Level 2 clearing the FPGA output during a permanent scheduler stall, plus CPU restart after the safety ISR also stalls. | Physical safe-output behavior and readback, production fault recording, and measured worst-case reaction time. |
| 014–019 communication | Payload CRC, time and schema checks, one monitored channel, consecutive failures and recovery tests. | Enforce the sole communication path on target. Justify message identity, repetition, loss, torn updates and header integrity for each application schema. Current CRC covers payload bytes only. |
| 020–023 startup and reset | AN386 boot checks a post-link image CRC and runs March C- over working SRAM before stack use. Two negative tests observe boot rejection. A retained CRC-protected record tracks watchdog and brown-out counts. A QEMU test observes two Level 2 boots, a third watchdog reset leading to Level 3, and the latch surviving a software reset. | Production reset-cause acquisition, retention across brown-outs, power-on classification and target memory coverage. QEMU uses an NMI witness for cause attribution. The test stores the expected CRC alongside the image and reports POST failures through semihosting. |
| 024–027 clock and power | The AN386 test has two timers driven by one modeled clock. | Independent time reference, clock tolerance and reaction logic, supply supervision, brown-out history and hardware fault tests. |

## Verification baseline

Host CTest passed 10 executables. Seven AN386 tests cover boot, interrupt-table relocation, permanent stalls, peripheral isolation, repeated watchdog resets and two POST rejection paths. Apple LLVM 21.0.0 host instrumentation at -O0 measured 632/641 lines,
250/284 branches and 62/85 MC/DC conditions across the compiled kernel and
architecture sources. ARM-only code, HAL, assembly, FreeRTOS and integration
firmware are outside those host figures. Coverage still needs per-decision
analysis, additional tests or justified exclusions and independent review.

## Customer delivery

Before declaring a supported release, baseline the MCU, board, compiler,
build options and FreeRTOS revision. Tie each safety requirement to a design,
implementation, verification result and disposition. Supply an integration
safety manual covering the assumptions of use, timer and watchdog ownership,
clock and power sources, safety outputs and acceptance tests. Record tool
qualification evidence, independent reviews and assessment for that baseline.

[Software safety requirements](SSRS.md) · [Assumptions of use](AOU.md) ·
[Safety Case and open points](SAFETY_CASE.md)
