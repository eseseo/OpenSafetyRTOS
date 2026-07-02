# OpenSafetyRTOS — Software Safety Plan

**Document ID:** OSR-SP-001
**Version:** 1.1
**Status:** In Review
**Date:** 2026-07-02
**Target Standard:** ISO 26262-2 (Safety Management) / ISO 26262-6 (Software) / ISO 26262-8 (Supporting Processes) / ISO 26262-9 (ASIL Decomposition)

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
| Derive software safety requirements from system-level safety goals | Software Safety Requirements Specification (OSR-SSRS-001) | Safety Engineer | Done (v1.2) |
| Define SafetyFunction functional requirements (watchdog, MPU, mailbox validation, safe state) | OSR-SSRS-001 | Safety Engineer | Done (v1.2) |
| Define FFI requirements (what interference scenarios must be prevented) | FFI Analysis (OSR-FFIA-001) | Safety Engineer | Pending |
| Internal review of SSRS (verification-support; sub-I2 — see §6a) | OSR-CR-SSRS-001/-002 | Agent-QA (internal) | Done — ISA confirmation review pending (OSR-RISK-001) |

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

## 4a. Safety Case Document Status (as of 2026-07-02)

| Document | ID / Version | Review Status |
|---|---|---|
| HARA | OSR-HARA-001 v1.3 | Approved (internal) via **OSR-CR-HARA-003** (2026-07-02, closes prior conditional approvals OSR-CR-HARA-001/-002) — contingent on external ISA confirmation review (OSR-ISSUE-005) |
| FSC | OSR-FSC-001 v1.3 | Approved (internal) via OSR-CR-FSC-003; ISA confirmation review pending |
| SSRS | OSR-SSRS-001 v1.2 | Approved (internal) via OSR-CR-SSRS-002; ISA confirmation review pending |
| AoU Registry | OSR-AOU-001 v1.0 (`safety/doc/AOU.md`) | New — canonical Assumption-of-Use registry (AoU-01..10); resolves prior AoU ID collisions (OSR-ISSUE-003) |
| Architecture / FFI model | ADR-005 (`docs/ADR-005`) | New — privilege/FFI model (no MPU lock on ARMv7-M; detective MPU-config integrity check; SafetyFunction in independent-timer ISR; Level 3 via AIRCR.SYSRESETREQ) |
| Consolidated review record | OSR-CR-ARCH-001 (2026-07-02) | Audit trail of the 2026-07-02 architecture & cross-document consistency review |

> All internal review approvals above are **verification-support measures** performed at
> sub-I2 independence (see §6a). None constitutes an ISO 26262-conformant confirmation
> measure until confirmed by the external ISA.

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

## 6a. Confirmation Measures and Independence (ISO 26262-2:2018 Cl.6.4.9)

Confirmation measures (confirmation reviews, functional safety audit, functional safety
assessment) are specified in **ISO 26262-2:2018 Cl.6.4.9** with required independence
per **Table 1**. Earlier project documents cited these as "Part 2 Cl.8/9/10"; that
citation is corrected here and in OSR-CR-ARCH-001 (finding F-10).

Per ISO 26262-2:2018 Table 1, I3 independence is **required** for the HARA confirmation
review (all ASILs) and for ASIL-D safety plan/safety case confirmation reviews, the
functional safety audit, and the functional safety assessment.

**Project reality (governing constraint — OSR-RISK-001, OSR-ISSUE-005):** all Phase 0–1
reviews to date were performed by Agent-QA, an AI agent operated by the same single
maintainer as the author agents. This does not meet I2, let alone I3. These internal
reviews are therefore classified as **verification-support measures** (see
`docs/TEAM.md` v0.5, "Honest Independence Statement"). ISO-conformant confirmation
measures require independent human reviewers/an external ISA, whose engagement is
scheduled **no later than Phase 2 exit** (retrospective I3 confirmation review of all
Phase 1 artifacts).

---

## 7. Safety Development Standards

| Domain | Standard / Tool |
|---|---|
| Coding standard | MISRA-C:2012 |
| Static analysis | Cppcheck (defect detection, TCL2) + PC-lint Plus / Polyspace (MISRA-C:2012, TCL3) — per OSR-TQ-001 v1.1 (supersedes earlier "to be selected") |
| Coverage measurement | MC/DC — target: 100% for safety partition |
| Requirements management | GitHub Issues + RTM document (OSR-RTM-001) |
| Version control | Git with branch protection; `safety/` branches require 2 reviewers |
| Defect tracking | GitHub Issues with `safety` label |

---

## 8. Document Revision History

| Version | Date | Author | Changes |
|---|---|---|---|
| 0.1 | 2026-04-18 | Safety Manager | Initial draft |
| 1.1 | 2026-07-02 | Safety Manager | Added §4a document-status table reflecting current truth (HARA v1.3 conditionally approved pending OSR-CR-HARA-003 closure conditions; FSC v1.3 approved via OSR-CR-FSC-003; SSRS v1.2 approved via OSR-CR-SSRS-002; OSR-AOU-001 v1.0 new; ADR-005 new). Added §6a: confirmation measures cited correctly as ISO 26262-2:2018 Cl.6.4.9 with Table 1 independence; internal Agent-QA reviews reclassified as verification-support measures (sub-I2); ISA engagement moved to no later than Phase 2 exit. Fixed Phase 1 reviewer-role contradiction with docs/TEAM.md (SSRS review performed by Agent-QA, not ISA). Updated Phase 1 activity statuses. Updated OSR-RISK-001 mitigation. (Per OSR-CR-ARCH-001 F-10/F-11/F-14.) |

---

*This document is a living artifact. It must be updated as the project progresses through each phase. All changes require Safety Manager approval.*

---

## 리스크 레지스터 (Risk Register)

| ID | 리스크 | 영향 | 가능성 | 완화 계획 |
|----|------|------|-------|---------|
| OSR-RISK-001 | 내부 Agent-QA 검토는 단일 운영자가 구동하는 AI 에이전트 구조로 인해 I2조차 충족하지 못함 (ISO 26262-2:2018 Table 1: HARA 확인 검토는 전 ASIL에서 I3 필수, ASIL-D 확인 조치 전반 I3 필수) — 확인 조치(Confirmation Measures) 요건 미충족 | 인증 불가 수준의 프로세스 부적합 (지배적 제약) | 높음 | 내부 리뷰를 검증 지원 조치(verification-support)로 재분류 (TEAM.md v0.5, §6a). **늦어도 Phase 2 종료 전** 외부 ISA를 고용하여 Phase 1 산출물 전체에 대한 소급 I3 확인 검토 수행. 내부 리뷰 기록은 외부 ISA 검토 기반 자료로 활용. 이슈 기록: OSR-ISSUE-005 (Class B, Open) |
| OSR-RISK-002 | FreeRTOS COTS 자격부여 증거 부족 | Part 8 Cl.12 미준수 지적 | 중간 | FreeRTOS 사전 사용 이력 수집, MIT 라이선스 검토, QM(D) 처리 근거 문서화 |
| OSR-RISK-003 | SEooC Assumption of Use 미충족 통합자 존재 | 시스템 레벨 안전성 저하 | 낮음 | AoU 문서 명확화, Integration Guide 상세 작성, 통합자 체크리스트 제공 |
