# ADR-003: 배포 전략 — SEooC (Safety Element out of Context)

**날짜:** 2026-04-18
**상태:** 승인됨
**결정자:** PM/PO
**근거 표준:** ISO 26262 Part 8 Cl.13

---

## 컨텍스트

OpenSafetyRTOS는 오픈소스 프로젝트이므로 배포 방식이 오픈소스 철학과
ISO 26262 인증 요건을 동시에 충족해야 한다.

검토한 옵션:
1. 소스코드 배포만 (FreeRTOS 방식)
2. **소스코드 배포 + SEooC 인증 패키지** ← 채택
3. 하이퍼바이저 기반 배포 (TTTech MotionWise 방식)

---

## 결정

**SafetyFunction 레이어를 SEooC(Safety Element out of Context)로 개발·배포한다.**

```
OpenSafetyRTOS 배포 패키지
├── kernel/freertos/        소스 (MIT License)          — FreeRTOS 원본
├── kernel/safety/          소스 (Apache 2.0)           — SafetyFunction SEooC
├── safety/doc/             인증 증거 문서 (CC BY 4.0)  — HARA, FMEA, SSRS 등
├── docs/ASSUMPTION_OF_USE.md  SEooC 사용 가정 문서
└── docs/INTEGRATION_GUIDE.md  사용자 통합 가이드
```

---

## SEooC 개념

ISO 26262 Part 8 Cl.13에 따라, SafetyFunction은 최종 시스템 컨텍스트와 독립적으로
개발·검증된 안전 요소다. 사용자는 아래 조건만 충족하면 자신의 시스템에 통합 가능하다.

```
OpenSafetyRTOS (SEooC 개발)          사용자 시스템 (통합)
┌──────────────────────────┐         ┌──────────────────────────┐
│ SafetyFunction ASIL-D(D) │         │ 사용자 Application       │
│  - HARA 수행             │  ────▶  │  + FreeRTOS              │
│  - SSRS 작성             │         │  + SafetyFunction (SEooC) │
│  - FMEA 분석             │         │                           │
│  - MC/DC 100% 테스트     │         │ 사용자 통합 활동 필요:    │
│  - Assumption 정의       │         │  - Assumption 충족 확인   │
│  - 인증 증거 문서 공개   │         │  - 시스템 레벨 HARA      │
└──────────────────────────┘         │  - 통합 테스트           │
                                     └──────────────────────────┘
```

---

## 하이퍼바이저 방식 미채택 이유

TTTech MotionWise 등의 하이퍼바이저 방식은 하드웨어 가상화(MMU/SMMU)를 전제로 한다.

| 비교 항목 | 하이퍼바이저 | MPU 파티셔닝 (채택) |
|----------|------------|-------------------|
| 필요 HW | Cortex-R/A (MMU) | Cortex-M4/M7 (MPU) |
| 복잡도 | 매우 높음 | 적정 |
| 오픈소스 생태계 | 제한적 | FreeRTOS 그대로 활용 |
| Automotive MCU 적합성 | 일부만 가능 | 대부분 ECU 대상 가능 |

Cortex-M 기반 automotive ECU가 주 타겟인 이 프로젝트에서는
MPU 기반 파티셔닝이 올바른 선택이며, 하이퍼바이저는 불필요한 복잡도를 초래한다.

---

## Assumption of Use (사용 가정) 초안

사용자가 반드시 충족해야 하는 조건. 상세 내용은 `docs/ASSUMPTION_OF_USE.md` 참조.

| ID | 가정 항목 | 설명 |
|----|---------|------|
| AoU-01 | MPU 설정 | SafetyFunction이 제공하는 MPU 초기화 함수를 시스템 부팅 시 가장 먼저 호출할 것 |
| AoU-02 | 우선순위 | SafetyFunction 태스크가 시스템 내 최고 우선순위를 가질 것 |
| AoU-03 | Mailbox 전용 사용 | QM → SafetyFunction 데이터 전달은 반드시 Mailbox API만 사용할 것 |
| AoU-04 | 클럭/타이머 | SafetyFunction에 독립적인 하드웨어 타이머 소스 제공할 것 |
| AoU-05 | Watchdog | 외부 하드웨어 Watchdog을 SafetyFunction에 연결할 것 |
| AoU-06 | 컴파일러 | MISRA-C:2012 지원 컴파일러 사용, 최적화는 SafetyFunction 권고 수준 준수 |

---

## 패키지 배포 방식

| 방식 | 상태 | 비고 |
|------|------|------|
| git submodule | Phase 1부터 지원 | 기본 통합 방식 |
| CMake FetchContent | Phase 2부터 지원 | CMake 프로젝트 통합 |
| 패키지 매니저 (vcpkg/conan) | Phase 5 이후 | 인증 완료 후 |

---

## 결과 및 트레이드오프

- (+) 오픈소스 철학 완전 유지 — 소스 + 인증 문서 모두 공개
- (+) ISO 26262 SEooC 경로로 인증 가능 — 선례 존재 (Part 8 Cl.13)
- (+) Cortex-M 생태계 최적 — 하이퍼바이저 불필요
- (-) 사용자가 Assumption of Use 충족 책임 — 통합 가이드 품질이 중요
- (-) 시스템 레벨 인증은 사용자가 직접 수행해야 함

---

*다음 ADR: ADR-004 Assumption of Use 상세 정의 (Phase 1)*
