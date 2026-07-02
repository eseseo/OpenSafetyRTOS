# OpenSafetyRTOS — Safety Case

**Document ID:** OSR-SC-001
**Version:** 0.6
**Status:** In Progress
**Date:** 2026-07-02
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
| 근거 | ISO 26262-9:2018 Cl.6 (공존 기준/FFI), ADR-002, ADR-005 (특권/실행 컨텍스트 모델 — 발행 2026-07-02) |
| 해당 ASIL | QM(D) 격리 → ASIL-D 수준 FFI |
| 현재 상태 | 🟡 진행중 — v0.6: D1 개정으로 FFI 논증이 "MPU 예방(Unprivileged QM) + 주기적 검출(특권 커널 결함)" 이원 구조로 재정의됨 (FSC v1.3 §2.2.2) |

---

## 4. 증거 연결 (Evidence Map)

각 목표를 뒷받침하는 증거 문서와 현재 상태를 연결한다.

| 목표 | 증거 문서 | 상태 |
|------|---------|------|
| G1.1 — ASIL-D(D) 개발 | HARA, FSC, SSRS, FMEA, 코드 리뷰, MC/DC 결과 | 🟡 진행중 |
| G1.2 — FFI 달성 | ADR-002, ARCHITECTURE.md §5, MPU 설정 코드, FFI 테스트 결과 | 🟡 진행중 |
| G1 — Safe State 전이 | SAFE_STATE_DEFINITION.md ✅, Watchdog 테스트 결과 | 🟡 진행중 |
| SEooC 사용 가정 | OSR-AOU-001 (정식 AoU 레지스트리, `safety/doc/AOU.md`) | 🟢 v1.0 발행 (2026-07-02) — 확인 검토(OSR-CR-AOU-001) 🔴 미시작 |
| 프로세스 준수 | SAFETY_PLAN.md, CM_PLAN.md, QA 감사 기록 | 🟡 진행중 |
| 도구 신뢰성 | TOOL_QUALIFICATION_PLAN.md, 도구 검증 결과 | 🔴 미완성 |

### 증거 문서 상세

#### G1.1 증거

| 증거 ID | 문서 | 담당 | Phase |
|---------|------|------|-------|
| E-G1.1-01 | HARA (위험원 분석 및 위험도 평가) — OSR-HARA-001 v1.3 | Agent-Safety / Agent-QA | Phase 1 🟡 조건부 승인 (v1.2 — OSR-CR-HARA-002: Major 전원 해소, Minor NI-001~004 정정 완료. v1.3 개정: ASIL 표기 정정/보수적 상향 근거(HARA-NI-005~008), AoU 정식 ID 전환, H-05 추적 재배정 — 최종 승인은 OSR-CR-HARA-003 검토 대기) |
| E-G1.1-01b | FSC (기능안전 개념) — OSR-FSC-001 v1.3 | Agent-Safety / Agent-QA | Phase 1 🟡 (v1.2 🟢 최종 승인 — OSR-CR-FSC-003: Major C1/C2 완전 해소, 27개 FSR SG-01~08 전부 커버. **v1.3 개정(D1/D2 아키텍처, FSR-02-01/02/04·03-01/02·01-04·04-01, NI-001 FSR-04-01 ASIL 오기 해소)은 재확인 검토 예정**) |
| E-G1.1-02 | SSRS (소프트웨어 안전 요구사항) — OSR-SSRS-001 v1.2 | Agent-Safety / Agent-QA | Phase 1 🟡 (v1.1 🟢 최종 승인 — OSR-CR-SSRS-002: ISSUE-001 C1 + NI-001 + NI-002 전원 해소, Safety Manager 서명 잔여. **v1.2 개정(D1/D2 반영: SSR-002/004/006/008/009/010/011/013/023/025)은 재확인 검토 예정**) |
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
| E-G1.2-06 | ADR-005 (특권/실행 컨텍스트 모델 — D1/D2 결정 문서) | Agent-Docs | Phase 1 🟢 발행 (2026-07-02) — 확인 검토는 OP-016 일괄 재확인에 포함 |
| E-G1.2-07 | MPU/VTOR 주기 무결성 검증 결함 주입 테스트 결과 (SSR-006/SSR-009) | Agent-VnV | Phase 4 🔴 미시작 |

#### G1 — Safe State 증거

| 증거 ID | 문서 | 담당 | Phase |
|---------|------|------|-------|
| E-G1-01 | SAFE_STATE_DEFINITION.md v1.1 (D2: Level 3 SYSRESETREQ 즉시 리셋 + Watchdog 백스톱) | Agent-Docs | Phase 1 🟡 (v1.0 승인; v1.1 개정 재확인 예정) |
| E-G1-02 | Watchdog 구현 코드 — `safety/src/watchdog.c` | Agent-Safety | Phase 3 |
| E-G1-03 | Safe State 전이 테스트 결과 | Agent-VnV | Phase 4 |

---

## 5. 미해결 사항 (Open Points)

Phase가 진행됨에 따라 아래 항목이 채워진다.

| # | 항목 | 담당 | 목표 Phase | 상태 |
|---|------|------|-----------|------|
| OP-001 | HARA 작성 및 Agent-QA 확인 검토 완료 | Agent-Safety / Agent-QA | Phase 1 | 🟡 조건부 승인 — OSR-CR-HARA-002 (2026-04-19) 재검토 완료, Major 전원 해소. **v1.3 (2026-07-02) 개정: ASIL 표기 정정(H-05/H-07/H-09/H-10 보수적 상향 근거), 인용 정정, H-05→SG-01 재배정, AoU 정식 ID.** OSR-CR-HARA-003 (2026-07-02) 종결 검토 완료 — 승인, 외부 ISA 확인 검토 조건부 (OSR-ISSUE-005). |
| OP-001a | HARA v1.2 — Minor 이슈(NI-001~004) 정정 및 SG-07/SG-08 AoU 항목 보완 | Agent-Safety | Phase 1 | 🟢 완료 (2026-04-19) — HARA v1.2 Minor NI-001~004 모두 정정 완료. git commit 984e8a2. |
| OP-002 | SSRS (OSR-SSRS-001) 작성 및 승인 | Agent-Safety / Agent-QA | Phase 1 | 🟡 v1.1 최종 승인 완료 (2026-04-19, OSR-CR-SSRS-002; Safety Manager 서명 잔여). **v1.2 (2026-07-02) 개정: D1/D2 반영(타이머 ISR 실행, 주기 MPU/VTOR 검증, DMA 소유권, SYSRESETREQ, SSR-023 경계 정합) — 재확인 검토 필요.** |
| OP-003 | FSC (OSR-FSC-001) 작성 및 Agent-QA 확인 검토 완료 | Agent-Safety / Agent-QA | Phase 1 | 🟡 v1.2 최종 승인 완료 (2026-04-19, OSR-CR-FSC-003; Major C1/C2 해소, 27개 FSR SG-01~08 커버). **v1.3 (2026-07-02) 개정: D1/D2 반영 및 NI-001(FSR-04-01 ASIL 오기) 해소 — 재확인 검토 필요.** |
| OP-003b | FSC v1.2 — SG-07 FSR 섹션(§3.7) 및 SG-08 FSR 섹션(§3.8) 신설 | Agent-Safety | Phase 1 | 🟢 완료 (2026-04-19) — §3.7 FSR-07-01~03, §3.8 FSR-08-01~03 신설. OSR-CR-FSC-003에서 해소 확인. |
| OP-003a | SAFE_STATE_DEFINITION.md 작성 | Agent-Safety | Phase 1 | 🟡 v1.0 완료 (2026-04-18); v1.1 개정 (2026-07-02, D2/AoU 정식 ID) — 재확인 검토 필요 |
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
| OP-014 | OSR-AOU-001 정식 AoU 레지스트리 발행 (AoU ID 충돌 해소) 및 확인 검토 | Agent-Safety / Agent-QA | Phase 1 | 🟡 v1.0 발행 완료 (2026-07-02); 확인 검토(OSR-CR-AOU-001) 🔴 미시작 |
| OP-015 | ADR-005 (특권/실행 컨텍스트 모델 — D1/D2) 작성 및 ADR-002/004 정합 | Agent-Docs | Phase 1 | 🟢 완료 (2026-07-02 발행) |
| OP-016 | HARA v1.3 / FSC v1.3 / SSRS v1.2 / SAFE_STATE v1.1 개정분 일괄 재확인 검토 | Agent-QA | Phase 1 | 🔴 미시작 |

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
| 0.4 | 2026-04-19 | Agent-QA | HARA v1.2 Minor 정정 완료(OP-001a 🟢) 및 FSC v1.2 최종 승인(OSR-CR-FSC-003) 반영. SSRS v1.0 Draft 작성(OP-002 🟡). E-G1.1-01 v1.2 갱신. E-G1.1-01b 🟢 최종 승인. E-G1.1-02 🟡 진행중. OP 상태 전반 갱신. |
| 0.5 | 2026-04-19 | Agent-QA | SSRS v1.1 최종 승인(OSR-CR-SSRS-002) 반영. E-G1.1-02 🟢 최종 승인. OP-002 🟢 완료. GitHub 레포지토리명 확정: eseseo/opensafetyrtos. README URL 플레이스홀더 정정. |
| 0.6 | 2026-07-02 | Agent-Safety | 외부 평가 및 중앙 설계 결정(D1~D8) 반영: ① 증거 상태를 실제 산출물 상태와 정합 — HARA v1.3(🟡 조건부 승인, OSR-CR-HARA-003 대기), FSC v1.3/SSRS v1.2/SAFE_STATE v1.1(승인분 + 개정분 재확인 예정, 🟡). ② 신규 증거 — OSR-AOU-001 정식 AoU 레지스트리(🟢 발행/검토 미시작), E-G1.2-06 ADR-005(작성 중), E-G1.2-07 MPU/VTOR 주기 검증 결함 주입 테스트(🔴). ③ G1.2 근거 인용 정정 — "Part 6 Cl.7.4.14"→ISO 26262-9:2018 Cl.6(공존 기준/FFI), D1 이원 보호 구조(예방+검출) 반영. ④ OP-014(AoU 레지스트리), OP-015(ADR-005), OP-016(개정분 일괄 재확인 검토) 신설; OP-001/002/003/003a 상태를 정직하게 🟡로 재조정. |

---

*이 문서는 살아있는 문서다. Phase별로 증거가 추가될 때마다 갱신되어야 하며, 모든 갱신은 Safety Manager 승인이 필요하다.*
