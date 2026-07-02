# OSR-ISSUE-002: HARA ASIL 결정 오류 (H-05/H-07/H-09/H-10) — Part 3 Table 4 오적용이 확인 검토 2회를 통과

| 항목 | 내용 |
|------|------|
| Issue 번호 | 내부 002 (GitHub Issue 미연동 — 연동 시 상호 참조 추가) |
| 분류 | Class B (Safety Related — 안전 관련 문서 불일치) |
| 발견일 | 2026-07-02 (오류 자체는 2026-04-18/19 검토 기록에서 발생) |
| 해결일 | 2026-07-02 |
| 발견 단계 | 프로세스 감사 / 교차 일관성 검토 (OSR-CR-ARCH-001 F-10) |
| 담당자 | Agent-Safety (HARA 정정) |
| 검증자 | Agent-QA (OSR-CR-HARA-003) |
| 상태 | **Resolved** — HARA v1.3 + OSR-CR-HARA-003 으로 해소 |

## 문제 설명 (Problem Description)

ISO 26262-3:2018 Table 4 기준으로 다음 ASIL 결정이 수치적으로 틀렸고, 이 오류가
확인 검토(OSR-CR-HARA-001, OSR-CR-HARA-002)를 **지적 없이 2회 통과**하였다:

| 위험원 | S/E/C | 기록된 ASIL | Table 4 정답 |
|--------|-------|------------|-------------|
| H-05 | S2/E4/C2 | ASIL-C ("Table 4 기준 정확"으로 승인) | **ASIL-B** |
| H-09 | S3/E2/C3 | ASIL-C ("수치 검증 완료"로 승인) | **ASIL-B** |
| H-10 | S3/E2/C3 | ASIL-C ("수치 검증 완료"로 승인) | **ASIL-B** |
| H-07 | S3/E2/C3 | ASIL-C (v1.0부터 잔존, 검토 2회 무지적 통과) | **ASIL-B** |

동일 검토(OSR-CR-HARA-002)가 H-04(S3/E4/C2 = ASIL-C)는 정확히 재계산하여 정오표까지
발행했으므로, 검토자의 Table 4 적용이 항목별로 비일관적이었다. 오류 방향은 모두
보수적(상위 등급)이어서 즉각적 안전 위험은 없으나, "독립 수치 검증 완료"라는 검토 기록의
증거 가치가 훼손된다.

## 원인 분석 (Root Cause Analysis)

검토자가 Table 4를 항목마다 개별 참조하지 않고 일부 항목에서 암산/유추로 판정.
검토 기록에 Table 4 전체 행렬 대조표가 첨부되지 않아 오류가 재검토에서도 재생산됨.
(OSR-CR-HARA-002에는 자기모순 문장 "ASIL-C가 아닌 ASIL-C"도 잔존 — 동일 원인.)

## 수정 내용 (Fix Description)

- HARA v1.3: H-05를 Table 4 정합 등급 **ASIL-B**로 기재하고, **문서화된 보수적 상향
  (conservative uplift)** 으로 ASIL-C 적용을 명시 (H-04에 적용한 것과 동일한 패턴).
  H-07/H-09/H-10도 동일 처리 (Table 4 = B, 문서화된 상향으로 C 적용 — HARA-NI-005~008).
- OSR-CR-HARA-003: 선행 검토 기록의 계산 오류를 공식적으로 정정(정오표)하고
  v1.3 처리를 확인.
- 선행 검토 기록(OSR-CR-HARA-001/002)은 이력 문서로서 수정하지 않음 — 정정은
  후속 기록(OSR-CR-HARA-003)으로만 수행.

## 영향 분석 (Impact Analysis)

- 영향받는 요구사항: 없음 (모든 하류 산출물이 상향된 등급 C 및 개발 수준 D 기준으로 이미
  작성되어 있어, Table 4 정합 등급이 B로 정정되어도 하류 ASIL 배정은 유효·보수적으로 유지)
- 영향받는 문서: HARA (v1.3), OSR-CR-HARA-003 (신규)
- ASIL 영향: 등급 하향 없음 — 적용 ASIL은 C로 유지(보수적 상향 명시), 개발 수준 D 불변

## 검증 결과 (Verification Result)

OSR-CR-HARA-003 (2026-07-02)에서 Table 4 전체 행렬 대조로 재검증 완료.
단, 동 검토 역시 sub-I2 독립성으로 수행되었으므로 최종 확증은 외부 ISA 소급 검토
(OSR-ISSUE-005 참조) 시 재확인 대상.
