# OSR-ISSUE-005: 확인 조치(Confirmation Measures) 독립성 요건 부적합 — ISO 26262-2:2018 Table 1

| 항목 | 내용 |
|------|------|
| Issue 번호 | 내부 005 (GitHub Issue 미연동 — 연동 시 상호 참조 추가) |
| 분류 | Class B (Safety Related — 프로세스 부적합, 지배적 제약) |
| 발견일 | 2026-07-02 (구조 자체는 프로젝트 개시 시점부터 존재, OSR-RISK-001로 부분 인지) |
| 해결일 | — |
| 발견 단계 | 프로세스 감사 (OSR-CR-ARCH-001 F-14) |
| 담당자 | Safety Manager (PM/PO) |
| 검증자 | (해소 시 외부 ISA) |
| 상태 | **Open** |

## 문제 설명 (Problem Description)

1. ISO 26262-2:2018 Table 1은 **HARA 확인 검토에 전 ASIL에서 I3**, ASIL-D의
   Safety Plan/Safety Case 확인 검토·기능안전 감사·기능안전 평가에 **I3를 요구("shall")**
   한다. 기존 TEAM.md(v0.4)는 이를 "ASIL-D에 권장(Recommended)"으로 오기술하였다.
2. 현재 Agent-Safety / Agent-QA / Agent-VnV 는 **단일 유지관리자가 운영하는 AI 에이전트
   인스턴스**로, 인적/조직적 독립성 관점에서 **I2조차 충족하지 못한다** (동일 운영 주체).
3. 따라서 지금까지 수행된 모든 내부 검토(OSR-CR-HARA-001/002/003, OSR-CR-FSC-001/002/003,
   OSR-CR-SSRS-001/002, OSR-CR-ARCH-001)는 ISO 26262 적합 확인 조치가 아니다.

## 원인 분석 (Root Cause Analysis)

단일 유지관리자 오픈소스 프로젝트라는 구조적 제약. 초기에 OSR-RISK-001로 갭이 인지되었으나
"I2 수행 후 Phase 5에서 I3"라는 계획은 (a) Table 1 요건을 "권장"으로 오독했고,
(b) 에이전트 역할 분리를 I2로 과대평가했다.

## 수정 내용 (Fix Description) — 완화 및 요구 조치

**완화 (2026-07-02 적용 완료):**
- TEAM.md v0.5: Table 1 요건 정정, "Honest Independence Statement" 추가 —
  내부 검토를 **검증 지원 조치(verification-support measures)** 로 재분류
- SAFETY_PLAN.md v1.1 §6a: 확인 조치의 올바른 근거 조항(ISO 26262-2:2018 Cl.6.4.9) 및
  독립성 현실 명기, OSR-RISK-001 갱신 (지배적 제약으로 격상)
- 모든 신규 검토 기록에 "sub-I2, 검증 지원 조치" 명시 (OSR-CR-HARA-003 부터 적용)

**해소 조건 (본 이슈 Close 조건):**
- 외부 ISA(또는 독립 인적 검토자) 계약 및 **늦어도 Phase 2 종료 전** Phase 1 산출물
  전체(HARA, FSC, SSRS, AoU, 검토 기록)에 대한 소급 I3 확인 검토 완료
- 이후 Phase별 확인 조치·기능안전 감사·기능안전 평가의 I3 수행 체계 확립

## 영향 분석 (Impact Analysis)

- 영향받는 문서: 전체 SafetyCase — 모든 내부 승인("최종 승인" 포함)은 ISA 확인 전까지
  잠정(provisional) 지위
- ASIL 영향: 산출물 기술 내용 자체는 불변. **인증 관점에서는 본 이슈 미해소 시 ASIL 주장
  자체가 성립 불가** — 프로젝트의 지배적 제약(governing constraint)
- 관련: README의 주장 수위도 "targeting ASIL-D"로 하향 조정됨 (OSR-CR-ARCH-001 F-08)

## 검증 결과 (Verification Result)

미해결 (완화 조치만 적용). ISA 소급 검토 보고서 접수 시 본 기록 갱신 및 Close.
