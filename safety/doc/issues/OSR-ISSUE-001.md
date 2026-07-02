# OSR-ISSUE-001: MPU 특권 모델 갭 — ARMv7-M MPU 설정 잠금 부재로 FFI 논증 무효화 위험

| 항목 | 내용 |
|------|------|
| Issue 번호 | 내부 001 (GitHub Issue 미연동 — 연동 시 상호 참조 추가) |
| 분류 | Class B (Safety Related) |
| 발견일 | 2026-07-02 |
| 해결일 | 2026-07-02 |
| 발견 단계 | 아키텍처/문서 교차 일관성 검토 (OSR-CR-ARCH-001 F-01, F-02, F-03) |
| 담당자 | Agent-Safety (설계), Agent-Docs (ADR) |
| 검증자 | Agent-QA |
| 상태 | **Resolved** — `docs/ADR-005` 로 해소 |

## 문제 설명 (Problem Description)

기존 안전 논증(ADR-002, FMEA-002, FSC FSR-02-xx)은 "MPU 설정을 초기화 후 잠근다
(locked after init)"는 전제 위에 FFI(Freedom From Interference)를 주장하였다. 그러나:

1. **ARMv7-M(Cortex-M4)에는 MPU 설정 잠금(lock) 하드웨어 기능이 없다.** 특권(privileged)
   소프트웨어는 언제든 MPU 레지스터를 재설정할 수 있다.
2. FreeRTOS 커널은 특권 모드로 실행되므로, 커널(또는 특권 모드로 진입한 QM 코드) 결함이
   MPU 설정 자체를 훼손할 수 있어 "예방적(preventive)" FFI 보장이 성립하지 않는다.
3. DMA 및 NVIC 경유 간섭 경로는 MPU로 강제(enforce)되지 않는다.

## 원인 분석 (Root Cause Analysis)

아키텍처 문서가 ARM MPU의 실제 하드웨어 능력(잠금 기능 부재, 특권 모드 우회)을 검증하지
않고 이상화된 보호 모델을 가정함. 초기 확인 검토들이 문서 간 정합성은 검토했으나 하드웨어
전제 자체의 타당성은 검토 범위에 넣지 않았음.

## 수정 내용 (Fix Description)

`docs/ADR-005` (privilege/FFI model)로 해소:

- QM 태스크는 FreeRTOS ARM_CM4_MPU 포트를 통해 **비특권(unprivileged)** 실행 —
  QM 태스크에 대한 MPU 보호는 예방적으로 유지
- FreeRTOS 커널은 특권 유지 → 커널 결함에 대한 MPU 보호는 **검출적(detective)** 으로
  재정의: SafetyFunction이 주기마다 MPU 설정 무결성 검사 수행
- SafetyFunction은 FreeRTOS 태스크가 아닌 **독립 타이머 ISR**에서 실행 (스케줄러 의존 제거)
- Safe State Level 3 = 즉시 AIRCR.SYSRESETREQ, 워치독은 백스톱
- DMA는 MPU로 게이트되지 않음 → 통합자 의무로 위임 (AoU-07, OSR-AOU-001)

## 영향 분석 (Impact Analysis)

- 영향받는 컴포넌트: MPU 초기화/검증 (arch/arm-cortex-m/src/mpu.c), SafetyFunction 실행 모델, Safe State FSM
- 영향받는 요구사항: FSR-02-01~04, FSR-08-02, SSR-004~009-MPU, SSR-010-FSM (실행 모델 전제)
- 영향받는 문서: ADR-002 (ADR-005가 보완), FMEA-002/003, ARCHITECTURE.md §5, SAFETY_CASE FFI 논증
- ASIL 영향: FFI 논증의 성격 변경 (예방 → QM 비특권화 + 검출적 무결성 검사 조합). 안전 목표 자체는 불변

## 검증 결과 (Verification Result)

ADR-005 발행 및 초기 구현(kernel/safety, arch/arm-cortex-m: mpu.c, fault handlers) 반영 확인
(OSR-CR-ARCH-001 disposition 참조). 검출적 메커니즘(주기적 MPU 설정 무결성 검사)의 실효성은
Phase 4 fault-injection 테스트로 검증 예정 — 테스트 증거 확보 전까지 FMEA-002 상태 Open 유지.
