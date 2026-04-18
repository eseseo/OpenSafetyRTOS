# OpenSafetyRTOS — Software Safety Plan

**Document ID:** OSR-SP-001
**Version:** 0.1 (Draft)
**Status:** In Review
**Date:** 2026-04-18
**Target Standard:** ISO 26262-6 (Software) / ISO 26262-9 (ASIL Decomposition)

---

## 1. Purpose and Scope

This Safety Plan defines the software safety development activities, evidence requirements, roles, and phase gates for the OpenSafetyRTOS project. It covers only the **SafetyFunction partition**, which is developed at **ASIL-D(D)** under the ISO 26262 Part 9 decomposition strategy.

FreeRTOS (QM(D) partition) is handled separately via COTS qualification per ISO 26262-8 Clause 12 and pre-existing use argument.

**This plan does not cover:**
- FreeRTOS internal development activities
- Application software built on top of OpenSafetyRTOS
- Hardware development (MPU, MCU selection)

---

## 2. Project Safety Goal

| Item | Detail |
|---|---|
| Safety Goal | SafetyFunction partition shall operate at ASIL-D(D) such that, in combination with FreeRTOS QM(D), the combined system achieves ASIL-D |
| Decomposition Reference | `docs/ARCHITECTURE.md` Section 2, `docs/ADR-001-decomposition-strategy.md` |
| FFI Requirement | The QM partition shall have no capability to corrupt or interfere with SafetyFunction data or execution — enforced by ARM MPU and verified by software validation |
| Fault Tolerance | Single-point fault tolerance: SafetyFunction shall detect and respond to any single fault in the QM partition within the defined fault-tolerant time interval |

---

## 3. Decomposition Strategy Reference

This project applies **ASIL Decomposition per ISO 26262-9 Section 5**. The architectural basis for the decomposition, including partition boundaries, MPU configuration, Freedom From Interference arguments, and communication protocols, is documented in:

- `docs/ARCHITECTURE.md` — Full architectural design document
- `docs/ADR-001-decomposition-strategy.md` — Decision record for decomposition adoption
- `docs/ADR-002-mpu-partition-strategy.md` — MPU enforcement strategy

Any modification to partition boundaries, MPU region assignments, or Mailbox communication protocols requires updating all three documents and re-review by @safety-team.

---

## 4. Safety Activities by Phase

### Phase 0 — Project Setup and Planning

| Activity | Output | Owner | Status |
|---|---|---|---|
| Define ASIL decomposition strategy | ADR-001 | Safety Manager | Done |
| Establish safety development process | This document (OSR-SP-001) | Safety Manager | In Progress |
| Define MISRA-C deviation process | `safety/doc/misra_deviations/` | Safety Engineer | Pending |
| Set up CODEOWNERS and branch protection | `.github/CODEOWNERS` | Core Team | Done |
| Establish traceability tool/method | Safety RTM (OSR-RTM-001) | Safety Engineer | Pending |

### Phase 1 — Safety Requirements Specification

| Activity | Output | Owner | Status |
|---|---|---|---|
| Derive software safety requirements from system-level safety goals | Software Safety Requirements Specification (OSR-SSRS-001) | Safety Engineer | Pending |
| Define SafetyFunction functional requirements (watchdog, MPU, mailbox validation, safe state) | OSR-SSRS-001 | Safety Engineer | Pending |
| Define FFI requirements (what interference scenarios must be prevented) | FFI Analysis (OSR-FFIA-001) | Safety Engineer | Pending |
| Peer review of SSRS | SSRS review record | Independent Safety Assessor | Pending |

### Phase 2 — Architectural Design

| Activity | Output | Owner | Status |
|---|---|---|---|
| Define partition boundaries and MPU region assignments | `docs/ARCHITECTURE.md`, ADR-002 | Arch Team | In Progress |
| Define SafetyFunction internal module structure | Architecture design document | Safety Engineer | Pending |
| Define inter-partition Mailbox protocol | `docs/ARCHITECTURE.md` Section 3.2 | Safety Engineer | In Progress |
| FMEA — identify failure modes and mitigations | `safety/doc/FMEA_TEMPLATE.md` | Safety Engineer | In Progress |
| Architecture review against SSRS | Architecture review record | Safety Manager + ISA | Pending |

### Phase 3 — Detailed Design and Implementation

| Activity | Output | Owner | Status |
|---|---|---|---|
| Implement SafetyFunction modules (MISRA-C compliant) | Source in `safety/src/` | Safety Engineer | Pending |
| Implement MPU initialization and region locking | `arch/` + `kernel/safety/` | Arch Team + Safety Engineer | Pending |
| Implement Mailbox receive with CRC + timestamp + range validation | `safety/src/mailbox.c` | Safety Engineer | Pending |
| Implement watchdog management | `safety/src/watchdog.c` | Safety Engineer | Pending |
| Implement safe state manager and HardFault escalation | `safety/src/safe_state.c` | Safety Engineer | Pending |
| MISRA-C static analysis on all safety source | Static analysis report | Safety Engineer | Pending |
| Code review (2 reviewers, including @safety-team) | Pull request review records | Safety Team | Ongoing |

### Phase 4 — Verification and Testing

| Activity | Output | Owner | Status |
|---|---|---|---|
| Unit testing of all SafetyFunction modules | Unit test reports | Safety Engineer | Pending |
| MC/DC coverage measurement and verification (100% required) | Coverage reports | Safety Engineer | Pending |
| Integration testing: QM + SafetyFunction on target hardware | Integration test reports | Safety Engineer | Pending |
| FFI testing: attempt QM write to ASIL-D region, verify MPU fault | FFI test report (OSR-FFIA-001 Appendix) | Safety Engineer | Pending |
| Fault injection testing: inject mailbox corruption, stale data, stack overflow | Fault injection test report | Safety Engineer | Pending |
| Performance testing: worst-case execution time for safety functions | WCET report | Safety Engineer | Pending |
| Independent review of test evidence | ISA review record | Independent Safety Assessor | Pending |

### Phase 5 — Release and Certification Evidence

| Activity | Output | Owner | Status |
|---|---|---|---|
| Compile safety case evidence package | Safety Evidence Package (SEP) | Safety Manager | Pending |
| Final MISRA-C compliance confirmation | Final static analysis summary | Safety Engineer | Pending |
| Confirm all SSRS requirements are verified (RTM complete) | Completed RTM | Safety Engineer | Pending |
| Final ISA sign-off | ISA assessment report | Independent Safety Assessor | Pending |
| Tag and release safety-certified version | Git tag `v1.0.0-asild` | Safety Manager | Pending |

---

## 5. Required Evidence for Certification

The following evidence artifacts are required to support an ISO 26262 ASIL-D compliance claim for the SafetyFunction partition. Each artifact must be reviewed and approved before the evidence package is finalized.

| # | Evidence Artifact | Document ID | Description |
|---|---|---|---|
| 1 | Software Safety Requirements Specification | OSR-SSRS-001 | Functional and non-functional safety requirements derived from system safety goals. Traceable to FMEA items. |
| 2 | Software Architectural Design | `docs/ARCHITECTURE.md` | Full partition architecture, memory map, inter-partition communication protocol, FFI argument. |
| 3 | FMEA | `safety/doc/FMEA_TEMPLATE.md` | Failure Mode and Effects Analysis covering all safety-relevant components. Updated throughout development. |
| 4 | Unit Tests with MC/DC Coverage | Test reports in `safety/tests/` | Unit test suite with 100% statement, branch, and MC/DC coverage for all SafetyFunction modules. |
| 5 | Integration Tests | Integration test report | System-level tests verifying QM/Safety partition interaction, MPU enforcement, and fault response on target hardware. |
| 6 | FFI Analysis | OSR-FFIA-001 | Formal argument that QM partition interference with SafetyFunction is prevented by MPU hardware and software validation. Includes test evidence. |
| 7 | MISRA-C Analysis Report | Static analysis outputs | Tool-generated MISRA-C:2012 compliance report with deviation records where applicable. |
| 8 | Requirements Traceability Matrix | OSR-RTM-001 | Bidirectional traceability from safety requirements through design, implementation, and test. |
| 9 | Code Review Records | Pull request history | Review records showing 2-reviewer approval including @safety-team for all safety partition changes. |
| 10 | ISA Assessment Report | ISA-OSR-001 | Independent Safety Assessor report confirming adequacy of safety development process and evidence. |

---

## 6. Roles and Responsibilities

| Role | Responsibilities |
|---|---|
| **Safety Manager** | Overall accountability for the safety plan; approves safety phase gates; manages ISA relationship; signs off on evidence package |
| **Safety Engineer** | Day-to-day safety development activities; MISRA-C implementation; writing and executing safety tests; maintaining FMEA; updating RTM |
| **Independent Safety Assessor (ISA)** | Independent review of safety requirements, architecture, FMEA, and test evidence; confirms process adequacy per ISO 26262 Part 2; provides assessment report required for certification |
| **Arch Team** | MPU configuration and architecture-specific safety integration; reviews ADR-002 and `arch/` changes |
| **Core Team** | CI/CD pipeline integrity; repository administration; non-safety contributions |

---

## 7. Safety Development Standards

| Domain | Standard / Tool |
|---|---|
| Coding standard | MISRA-C:2012 |
| Static analysis | To be selected (e.g., PC-lint Plus, Polyspace, Helix QAC) |
| Coverage measurement | MC/DC — target: 100% for safety partition |
| Requirements management | GitHub Issues + RTM document (OSR-RTM-001) |
| Version control | Git with branch protection; `safety/` branches require 2 reviewers |
| Defect tracking | GitHub Issues with `safety` label |

---

## 8. Document Revision History

| Version | Date | Author | Changes |
|---|---|---|---|
| 0.1 | 2026-04-18 | Safety Manager | Initial draft |

---

*This document is a living artifact. It must be updated as the project progresses through each phase. All changes require Safety Manager approval.*

---

## 리스크 레지스터 (Risk Register)

| ID | 리스크 | 영향 | 가능성 | 완화 계획 |
|----|------|------|-------|---------|
| OSR-RISK-001 | Phase 0~4에서 내부 Agent-QA(I2)가 독립성 검토 수행 — ASIL-D I3 요건 미충족 | 인증 기관 지적 가능성 | 중간 | Phase 5에서 외부 ISA 고용하여 I3 달성. 내부 리뷰 기록은 외부 ISA 검토 기반 자료로 활용 |
| OSR-RISK-002 | FreeRTOS COTS 자격부여 증거 부족 | Part 8 Cl.12 미준수 지적 | 중간 | FreeRTOS 사전 사용 이력 수집, MIT 라이선스 검토, QM(D) 처리 근거 문서화 |
| OSR-RISK-003 | SEooC Assumption of Use 미충족 통합자 존재 | 시스템 레벨 안전성 저하 | 낮음 | AoU 문서 명확화, Integration Guide 상세 작성, 통합자 체크리스트 제공 |
