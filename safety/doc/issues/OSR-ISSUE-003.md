# OSR-ISSUE-003: AoU ID 충돌 및 비일관 참조 — 정본 Assumption-of-Use 레지스트리 부재

| 항목 | 내용 |
|------|------|
| Issue 번호 | 내부 003 (GitHub Issue 미연동 — 연동 시 상호 참조 추가) |
| 분류 | Class B (Safety Related — 안전 관련 문서 불일치) |
| 발견일 | 2026-07-02 |
| 해결일 | 2026-07-02 |
| 발견 단계 | 교차 일관성 검토 (OSR-CR-ARCH-001 F-09) |
| 담당자 | Agent-Safety / Agent-Docs |
| 검증자 | Agent-QA |
| 상태 | **Resolved** — `safety/doc/AOU.md` (OSR-AOU-001 v1.0) 로 해소 |

## 문제 설명 (Problem Description)

SEooC 안전 논증의 핵심인 Assumption-of-Use(AoU) 항목이 정본 문서 없이 여러 문서에
분산 참조되어 ID 충돌과 대상 불일치가 발생하였다:

1. OSR-AOU-001 문서가 HARA §6.3, 검토 기록 등에서 반복 참조되었으나 **실제 파일이
   존재하지 않았음** (dangling reference).
2. AoU 개별 ID가 문서마다 다르게 배정됨 — 예: HARA/FSC에서 클럭 소스 독립성이
   AoU-04로, 검토 권고(OSR-CR-HARA-002)에서는 AoU-06으로 지칭; SG-03 파라미터 위임이
   "AoU-03"과 "AoU-XX"로 혼용; 전원 보호가 AoU-07(ADR-005의 DMA 위임)과 AoU-08(HARA
   §6.3)로 충돌 소지.
3. 통합자에게 전달되어야 할 의무 목록의 단일 출처(single source of truth)가 없어
   Integration Guide 작성 시 누락 위험.

## 원인 분석 (Root Cause Analysis)

AoU 항목이 각 분석 문서(HARA, FSC, ADR) 작성 중 필요할 때마다 지역적으로 번호를
부여받았고, 정본 레지스트리 생성이 어느 Phase 활동에도 배정되지 않았음.

## 수정 내용 (Fix Description)

- `safety/doc/AOU.md` (OSR-AOU-001 v1.0) 신설: **AoU-01..AoU-10 정본 레지스트리**.
  각 항목에 고유 ID, 내용, 근거 SG/FSR, 참조 문서를 단일 표로 확정.
- HARA v1.3 / FSC v1.3 / SSRS v1.2 / ADR-005 의 AoU 참조를 정본 ID로 정렬
  (해당 문서 소유 에이전트가 동시 적용).
- CM_PLAN v1.1 §5.1 ID 체계에 AOU 타입코드 등록.

## 영향 분석 (Impact Analysis)

- 영향받는 컴포넌트: 없음 (문서 체계)
- 영향받는 요구사항: AoU를 위임 대상으로 참조하는 모든 FSR/SSR (FSR-03-01, FSR-07-xx,
  FSR-08-01 등) — ID 참조만 변경, 내용 불변
- 영향받는 문서: HARA, FSC, SSRS, ADR-005, (향후) Integration Guide
- ASIL 영향: 없음. 단, AoU 누락은 SEooC 통합 시 시스템 레벨 안전 갭으로 이어질 수
  있었으므로 Class B로 분류

## 검증 결과 (Verification Result)

OSR-AOU-001 v1.0 발행 확인 (OSR-CR-ARCH-001 disposition). AoU 참조 정합성은
각 문서 v-bump 후 Agent-QA 교차 확인; 최종 확증은 Integration Guide 작성 시
(Phase 5) 및 외부 ISA 검토에서 재확인.
