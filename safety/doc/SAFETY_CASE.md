# OpenSafetyRTOS — Safety Case

**Document ID:** OSR-SC-001
**Version:** 0.3
**Status:** In Progress
**Date:** 2026-04-19
**Target Standard:** ISO 26262 Part 2 (Functional Safety Management)

---

## 1. 목적 (Purpose)

이 문서는 OpenSafetyRTOS의 **SafetyCase** — 안전성 논증의 최상위 문서다.

OpenSafetyRTOS SafetyFunction이 **ASIL-D**를 달성했음을 구조적으로 논증하고, 해당 논증을 뒷받침하는 모든 증거 문서를 연결한다. 논증 구조는 **GSN (Goal Structuring Notation)** 의 단순화된 형태를 따른다.

이 문서는 살아있는 문서(living document)로, Phase별로 증거가 추가될 때마다 갱신된다. Phase 5 완료 시 외부 ISA(Independent Safety Assessor)에게 제출되는 인증 증거 패키지의 최상위 문서가 된다.

---

## 2. 최상위 안전 목표 (Top-Level Safety Goal)

### G1 — 최상위 목표

> **"OpenSafetyRTOS SafetyFunction은 QM 파티션(FreeRTOS)으로부터의 간섭으로부터 독립적으로 동작하며, 시스템 장애 시 Safe State로 전이한다."**

| 항목 | 내용 |
|------|------|
| 목표 ID | G1 |
| 근거 표준 | ISO 26262 Part 2 Cl.6, Part 6, Part 9 |
| 달성 조건 | G1.1 및 G1.2가 모두 충족될 때 |
| 현재 상태 | 🔴 미완성 (증거 수집 중) |

---

## 3. 논증 전략 (Strategy)

### S1 — ASIL Decomposition 전략

ISO 26262 Part 9에 따른 **ASIL Decomposition**을 통해 G1을 두 개의 독립적인 하위 목표로 분해한다.

```
G1: SafetyFunction이 ASIL-D 요건을 달성하고 Safe State로 전이한다
│
└── S1: ASIL Decomposition (ISO 26262 Part 9)
      ├── G1.1: SafetyFunction이 ASIL-D(D)로 개발됨
      └── G1.2: FreeRTOS QM(D)으로부터의 FFI(Freedom From Interference)가 달성됨
```

#### G1.1 — SafetyFunction ASIL-D(D) 개발

> SafetyFunction 파티션은 ISO 26262 Part 6 ASIL-D 요건에 따라 설계, 구현, 검증되었다.

| 항목 | 내용 |
|------|------|
| 목표 ID | G1.1 |
| 근거 | ISO 26262 Part 6 (Software), Part 9 Cl.5 |
| 해당 ASIL | ASIL-D(D) |
| 현재 상태 | 🔴 미완성 |

#### G1.2 — FFI (Freedom From Interference) 달성

> FreeRTOS QM(D) 파티션은 SafetyFunction 파티션의 데이터, 실행, 또는 타이밍에 간섭할 수 없다. 이는 ARM MPU 하드웨어 격리 및 소프트웨어 검증으로 달성된다.

| 항목 | 내용 |
|------|------|
| 목표 ID | G1.2 |
| 근거 | ISO 26262 Part 6 Cl.7.4.14, ADR-002 |
| 해당 ASIL | QM(D) 격리 → ASIL-D 수준 FFI |
| 현재 상태 | 🟡 진행중 |

---

## 4. 증거 연결 (Evidence Map)

각 목표를 뒷받침하는 증거 문서와 현재 상태를 연결한다.

| 목표 | 증거 문서 | 상태 |
|------|---------|------|
| G1.1 — ASIL-D(D) 개발 | HARA, FSC, SSRS, FMEA, 코드 리뷰, MC/DC 결과 | 🟡 진행중 |
| G1.2 — FFI 달성 | ADR-002, ARCHITECTURE.md §5, MPU 설정 코드, FFI 테스트 결과 | 🟡 진행중 |
| G1 — Safe State 전이 | SAFE_STATE_DEFINITION.md ✅, Watchdog 테스트 결과 | 🟡 진행중 |
| 프로세스 준수 | SAFETY_PLAN.md, CM_PLAN.md, QA 감사 기록 | 🟡 진행중 |
| 도구 신뢰성 | TOOL_QUALIFICATION_PLAN.md, 도구 검증 결과 | 🔴 미완성 |

### 증거 문서 상세

#### G1.1 증거

| 증거 ID | 문서 | 담당 | Phase |
|---------|------|------|-------|
| E-G1.1-01 | HARA (위험원 분석 및 위험도 평가) — OSR-HARA-001 v1.1 ✅ + 재확인 검토 OSR-CR-HARA-002 ✅ | Agent-Safety / Agent-QA | Phase 1 🟢 조건부 승인 (v1.1 — OSR-CR-HARA-002: Major 이슈 전원 해소, Minor 4건 잔존 — v1.2에서 정리 후 최종 승인 전환 가능) |
| E-G1.1-01b | FSC (기능안전 개념) — OSR-FSC-001 v1.1 ✅ + 재확인 검토 OSR-CR-FSC-002 ✅ | Agent-Safety / Agent-QA | Phase 1 🟡 진행중 (v1.1 — OSR-CR-FSC-002: 기존 Major 4건 해소 확인, 신규 Major 1건 발생 — SG-07/SG-08 FSR 미존재, v1.2 조치 및 Agent-QA 재검토 필요) |
| E-G1.1-02 | SSRS (소프트웨어 안전 요구사항) — OSR-SSRS-001 | Agent-Safety | Phase 1 🔴 미시작 |
| E-G1.1-03 | FMEA — `safety/doc/FMEA_TEMPLATE.md` | Agent-Safety | Phase 2~3 |
| E-G1.1-04 | MISRA-C 정적 분석 결과 | Agent-Safety | Phase 3 |
| E-G1.1-05 | 코드 리뷰 기록 (PR history) | Agent-QA | Phase 3 |
| E-G1.1-06 | MC/DC 커버리지 100% 결과 | Agent-VnV | Phase 4 |
| E-G1.1-07 | 단위 테스트 결과 | Agent-VnV | Phase 4 |

#### G1.2 증거

| 증거 ID | 문서 | 담당 | Phase |
|---------|------|------|-------|
| E-G1.2-01 | ADR-002 (MPU 파티션 전략) — `docs/ADR-002-mpu-partition-strategy.md` | Agent-Docs | Phase 0 ✅ |
| E-G1.2-02 | ARCHITECTURE.md §5 (FFI 논증) — `docs/ARCHITECTURE.md` | Agent-Docs | Phase 1~2 |
| E-G1.2-03 | MPU 초기화 코드 — `arch/arm-cortex-m/src/mpu.c` | Agent-Safety | Phase 3 |
| E-G1.2-04 | FFI 분석 보고서 — OSR-FFIA-001 | Agent-Safety | Phase 3 |
| E-G1.2-05 | FFI 테스트 결과 (QM→ASIL-D 접근 시 MPU Fault 확인) | Agent-VnV | Phase 4 |

#### G1 — Safe State 증거

| 증거 ID | 문서 | 담당 | Phase |
|---------|------|------|-------|
| E-G1-01 | SAFE_STATE_DEFINITION.md | Agent-Docs | Phase 1 |
| E-G1-02 | Watchdog 구현 코드 — `safety/src/watchdog.c` | Agent-Safety | Phase 3 |
| E-G1-03 | Safe State 전이 테스트 결과 | Agent-VnV | Phase 4 |

---

## 5. 미해결 사항 (Open Points)

Phase가 진행됨에 따라 아래 항목이 채워진다.

| # | 항목 | 담당 | 목표 Phase | 상태 |
|---|------|------|-----------|------|
| OP-001 | HARA 작성 및 Agent-QA 확인 검토 완료 | Agent-Safety / Agent-QA | Phase 1 | 🟡 진행중 — OSR-CR-HARA-002 (2026-04-19) 재검토 완료. Major 이슈 전원 해소 확인. Minor 4건 잔존 (오기 정정 성격). v1.2 정리 후 최종 승인 전환 예정. |
| OP-001a | HARA v1.2 — Minor 이슈(NI-001~004) 정정 및 SG-07/SG-08 AoU 항목 보완 | Agent-Safety | Phase 1 | 🔴 미시작 — OSR-CR-HARA-002 C1~C4 조치 필요 |
| OP-002 | SSRS (OSR-SSRS-001) 작성 및 승인 | Agent-Safety | Phase 1 | 🔴 미시작 |
| OP-003 | FSC (OSR-FSC-001) 작성 및 Agent-QA 확인 검토 완료 | Agent-Safety / Agent-QA | Phase 1 | 🟡 진행중 — OSR-CR-FSC-002 (2026-04-19) 재검토 완료. 기존 Major 4건 해소 확인. 신규 Major 1건 발생: SG-07/SG-08 FSR 완전 미존재. v1.2에서 FSR-07-xx/FSR-08-xx 섹션 신설 후 Agent-QA 재검토(OSR-CR-FSC-003) 필요. |
| OP-003b | FSC v1.2 — SG-07 FSR 섹션(§3.7) 및 SG-08 FSR 섹션(§3.8) 신설; Minor 이슈(C3~C6) 해소 | Agent-Safety | Phase 1 | 🔴 미시작 — OSR-CR-FSC-002 C1~C6 조치 필요 |
| OP-003a | SAFE_STATE_DEFINITION.md 작성 | Agent-Safety | Phase 1 | 🟢 완료 (2026-04-18) |
| OP-004 | FMEA 완성 | Agent-Safety | Phase 2~3 | 🟡 진행중 |
| OP-005 | FFI 분석 보고서 (OSR-FFIA-001) 작성 | Agent-Safety | Phase 3 | 🔴 미시작 |
| OP-006 | MPU 구현 코드 작성 및 리뷰 | Agent-Safety | Phase 3 | 🔴 미시작 |
| OP-007 | MC/DC 커버리지 100% 달성 및 결과 문서화 | Agent-VnV | Phase 4 | 🔴 미시작 |
| OP-008 | FFI 테스트 결과 문서화 | Agent-VnV | Phase 4 | 🔴 미시작 |
| OP-009 | TOOL_QUALIFICATION_PLAN.md 작성 | Agent-Build | Phase 0~1 | 🟢 완료 (2026-04-18) |
| OP-010 | Functional Safety Audit 수행 (OSR-FSA-001) | Agent-QA | Phase 3~4 | 🔴 미시작 |
| OP-011 | Functional Safety Assessment 수행 (OSR-FSAMNT-001) | Agent-QA | Phase 4~5 | 🔴 미시작 |
| OP-012 | 외부 ISA 선정 및 계약 (TÜV SÜD 등) | Safety Manager | Phase 5 | 🔴 미시작 |
| OP-013 | 외부 ISA Safety Assessment 완료 (ISA-OSR-001) | 외부 ISA | Phase 5 | 🔴 미시작 |

---

## 6. SafetyCase 완성 조건

모든 증거가 🟢 완료 상태일 때 외부 ISA 제출이 가능하다.

| 조건 | 확인 방법 | 현재 상태 |
|------|---------|---------|
| 모든 증거 문서 🟢 완료 | Evidence Map 전체 항목 확인 | 🔴 미완성 |
| Agent-QA Confirmation Measures 3종 완료 | OSR-CR-*, OSR-FSA-*, OSR-FSAMNT-* 기록 | 🔴 미완성 |
| Safety Manager 최종 서명 | SAFETY_CASE.md 서명란 | 🔴 미완성 |
| 외부 ISA 평가 보고서 수령 | ISA-OSR-001 | 🔴 미완성 |

### 최종 서명란 (Phase 5 완료 시 기입)

| 역할 | 이름 | 서명 | 날짜 |
|------|------|------|------|
| Safety Manager | | | |
| 외부 ISA | | | |

---

## 7. 문서 이력

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2026-04-18 | Agent-Docs | Safety Case 골격 초안 생성 |
| 0.2 | 2026-04-18 | Agent-QA | HARA 확인 검토 완료(OSR-CR-HARA-001) 반영. FSC 신규 증거 E-G1.1-01b 추가 및 확인 검토 완료(OSR-CR-FSC-001) 반영. OP-001 🟢, OP-003 🟢 갱신. |
| 0.3 | 2026-04-19 | Agent-QA | HARA v1.1 재확인 검토(OSR-CR-HARA-002) 결과 반영: Major 이슈 4건 해소 확인, Minor 4건 잔존(v1.2 정리 예정). FSC v1.1 재확인 검토(OSR-CR-FSC-002) 결과 반영: 기존 Major 4건 해소 확인, 신규 Major 1건 발생(SG-07/SG-08 FSR 미존재 — v1.2 조치 필요). E-G1.1-01 조건부 승인 상태로 갱신. E-G1.1-01b 진행중 유지(FSC-NI-001 미해소). OP-001a, OP-003b 신규 추가. |

---

*이 문서는 살아있는 문서다. Phase별로 증거가 추가될 때마다 갱신되어야 하며, 모든 갱신은 Safety Manager 승인이 필요하다.*
