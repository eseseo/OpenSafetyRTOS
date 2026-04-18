# ADR-004 — 부트 시퀀스: SafetyFunction 선행 초기화

**Document ID:** ADR-004
**Version:** 1.0
**Status:** Accepted
**Date:** 2026-04-18
**Author:** Agent-Safety
**Deciders:** Safety Manager, Architecture Lead
**Standard Reference:** ISO 26262 Part 4 Cl.8, Part 6 Cl.8 (소프트웨어 단위 설계)

---

## 1. 상태

**Accepted** — Phase 1 구현의 필수 전제조건. 이 결정을 번복하려면 별도 ADR 및 Safety Manager 승인이 필요하다.

---

## 2. 컨텍스트

### 2.1 문제 정의

OpenSafetyRTOS는 FreeRTOS(QM 파티션)와 SafetyFunction(ASIL-D 파티션)의 Decomposition 구조를 채택한다(ADR-001). 두 파티션 간 FFI(Freedom From Interference)는 MPU(Memory Protection Unit)를 통한 하드웨어 메모리 보호로 달성된다(ADR-002).

**핵심 문제:** FreeRTOS가 SafetyFunction보다 먼저, 또는 동시에 초기화된다면, MPU가 구성되기 전에 QM 코드가 실행되는 **시간 창(Time Window)**이 발생한다. 이 창에서 QM 코드는 아직 보호되지 않은 ASIL-D 메모리 영역에 접근할 수 있다.

```
[위험한 시나리오 — MPU 미구성 상태에서 FreeRTOS 실행]

Reset Vector
    │
    ├─ FreeRTOS_Init() 시작         ← QM 코드 실행 시작
    │       ├─ Heap 초기화
    │       └─ 태스크 생성          ← MPU 없음\! ASIL-D 메모리 접근 가능
    │
    └─ SafetyFunction_PreInit()     ← 이 시점에서야 MPU 구성
            └─ MPU 설정             ← 너무 늦음
```

이 시나리오는 **ASIL-D FFI 요구사항 위반**이며, ISO 26262 Part 9의 ASIL Decomposition 유효성을 무효화한다.

### 2.2 영향 범위

- ARM Cortex-M4/M7 모든 타겟 플랫폼
- startup 코드 (startup_stm32xxxx.s 또는 동등한 벡터 테이블 / 리셋 핸들러)
- 링커 스크립트 (SafetyFunction 스택의 ASIL-D 영역 배치)
- 통합자의 BSP(Board Support Package) 수정 범위

---

## 3. 결정

**SafetyFunction은 FreeRTOS보다 반드시 먼저 초기화된다.**

Reset Vector에서 가장 먼저 호출되는 함수는 `SafetyFunction_PreInit()`이며, 이 함수가 완료된 후에만 FreeRTOS 초기화가 허용된다. 이를 통해 MPU는 FreeRTOS의 첫 번째 명령어 실행 전부터 활성화된다.

---

## 4. 부트 시퀀스 상세 정의

### 4.1 전체 시퀀스

```
Reset Vector (리셋 핸들러 진입점)
    │
    ▼
SafetyFunction_PreInit()                        [ASIL-D]
    │
    ├─ Step 1: 스택 포인터 초기화
    │           SafetyFunction 전용 스택을 ASIL-D 메모리 영역에 설정
    │           (링커 스크립트로 .sf_stack 섹션이 ASIL-D 영역에 배치됨을 보장)
    │
    ├─ Step 2: MPU 구성 (ASIL-D Region 보호 최우선)
    │           ├─ ASIL-D Region: SafetyFunction만 R/W, QM NO ACCESS
    │           ├─ QM Region: QM R/W, SafetyFunction R/O
    │           ├─ Mailbox Region: QM W/O, SafetyFunction R/O
    │           └─ MPU_CTRL.ENABLE = 1 (MPU 활성화)
    │           [이 시점 이후 QM의 ASIL-D 메모리 접근 시 HardFault 발생]
    │
    ├─ Step 3: 하드웨어 Watchdog 초기화
    │           ├─ IWDG(Independent Watchdog) 활성화
    │           ├─ 타임아웃 = WATCHDOG_TIMEOUT_MS (기본값: 100ms)
    │           └─ 이 시점 이후 SafetyFunction이 주기적으로 kick하지 않으면 MCU 리셋
    │
    ├─ Step 4: 부트 자가진단 (Boot-time Self-test)
    │           ├─ 4a. ROM CRC 검증: Flash 영역 CRC32 계산 및 참조값 비교
    │           ├─ 4b. RAM March 테스트: ASIL-D 스택/데이터 영역 March-C 알고리즘
    │           ├─ 4c. CPU 레지스터 테스트: 범용 레지스터 0/1 패턴 테스트
    │           ├─ 4d. MPU 설정 검증: 방금 설정한 MPU 레지스터 값 재확인
    │           └─ [자가진단 실패 시] → Watchdog kick 중단 → Level 3 리셋
    │
    └─ Step 5: SafetyFunction_Init() 완료 선언
                └─ g_safety_init_complete = SAFETY_INIT_MAGIC (0xSAFE_A55A)
                   [원자적 쓰기 — ASIL-D Region 내 변수]

    ↓ [SafetyFunction_PreInit() 반환]

FreeRTOS_Init()                                 [QM]
    │
    ├─ Step 6: 힙 초기화
    │           └─ QM Region 내에서만 힙 할당 (MPU 이미 활성 → ASIL-D 접근 시 HardFault)
    │
    ├─ Step 7: FreeRTOS 태스크 생성
    │           └─ 모든 태스크 스택은 QM Region 내에 배치
    │
    └─ Step 8: 스케줄러 시작 (vTaskStartScheduler())

    ↓ [두 파티션 정상 동작]

Normal Operation
    ├─ SafetyFunction 감시 태스크 (최고 우선순위, 주기적 Watchdog kick)
    └─ FreeRTOS QM 태스크 (일반 동작, MPU 보호 하에 실행)
```

### 4.2 부트 자가진단 상세 요구사항

| 자가진단 항목       | 알고리즘/방법                          | 합격 기준                         | 실패 시 처리          |
|-------------------|--------------------------------------|----------------------------------|--------------------|
| ROM CRC 검증       | CRC32 (CCITT 다항식)                  | 계산값 == Flash에 저장된 참조 CRC  | Level 3 (Watchdog) |
| RAM March 테스트   | March-C 알고리즘 (IEC 61508 권고)     | 모든 셀 읽기/쓰기 패턴 통과       | Level 3 (Watchdog) |
| CPU 레지스터 테스트 | 0x00000000 / 0xFFFFFFFF 패턴 기록/검증| 쓴 값 == 읽은 값                  | Level 3 (Watchdog) |
| MPU 레지스터 검증  | 예상 설정값과 실제 레지스터 비교        | 전체 MPU 영역 설정 일치           | Level 3 (Watchdog) |
| 스택 포인터 검증   | SP 레지스터가 ASIL-D 스택 영역 내 확인 | STACK_BASE ≤ SP ≤ STACK_TOP      | Level 3 (Watchdog) |

**참고:** 자가진단은 FreeRTOS 스케줄러 시작 전에 완료되어야 한다. 자가진단 실행 중에도 Watchdog은 동작 중이므로, 각 자가진단 단계 사이에 Watchdog kick을 수행하여 자가진단 시간이 WATCHDOG_TIMEOUT_MS를 초과하지 않도록 구현해야 한다.

### 4.3 리셋 원인 추적

Level 3 리셋 이후 부팅 시, 리셋 원인을 반드시 기록하고 통합자에게 보고해야 한다.

```c
/* 부팅 시 리셋 원인 분류 (예시: STM32 기준) */
typedef enum {
    RESET_CAUSE_POWER_ON     = 0x01,  /* 정상 전원 인가 */
    RESET_CAUSE_WATCHDOG_HW  = 0x02,  /* 하드웨어 Watchdog (Level 3 — SafetyFunction 결함) */
    RESET_CAUSE_WATCHDOG_SW  = 0x03,  /* 소프트웨어 Watchdog (Level 3 — 의도적) */
    RESET_CAUSE_POWER_FAULT  = 0x04,  /* 전원 이상 */
    RESET_CAUSE_BOOT_SELFTEST= 0x05,  /* 부트 자가진단 실패 */
    RESET_CAUSE_EXTERNAL     = 0x06,  /* 외부 리셋 핀 */
    RESET_CAUSE_UNKNOWN      = 0xFF,  /* 원인 미상 */
} reset_cause_t;

reset_cause_t safety_get_reset_cause(void);  /* 부팅 시 RCC_CSR 레지스터 분석 */
```

---

## 5. 거부된 대안

### 5.1 대안 A: 동시 초기화 (Simultaneous Init)

**설명:** FreeRTOS와 SafetyFunction을 병렬로 초기화하되, 각자 자신의 영역만 초기화.

**거부 이유:** 이 방식은 여전히 MPU 구성 완료 전에 FreeRTOS 코드가 실행되는 시간 창을 허용한다. 또한 초기화 순서의 레이스 컨디션이 발생할 수 있으며, ASIL-D 개발에서 이런 불확실성은 허용되지 않는다.

**구체적 위험:** FreeRTOS 힙 초기화 중 `pvPortMalloc()`이 MPU 구성 전에 ASIL-D 메모리 범위를 침범할 가능성을 정적 분석으로 완전히 배제하기 어렵다.

### 5.2 대안 B: FreeRTOS 먼저, SafetyFunction 나중

**설명:** FreeRTOS를 먼저 시작하고, SafetyFunction을 FreeRTOS 태스크 중 하나로 초기화.

**거부 이유:** FreeRTOS 첫 번째 태스크가 실행되기 전까지 MPU가 없는 상태이며, 이는 ADR-002의 근본 전제를 위반한다. 또한 SafetyFunction을 FreeRTOS 태스크로 실행하면 FreeRTOS 스케줄러에 대한 의존성이 발생하여 FFI가 깨진다.

### 5.3 대안 C: 별도 MCU에서 SafetyFunction 실행

**설명:** SafetyFunction을 별도 MCU(보조 MCU)에서 실행.

**거부 이유:** 비용 및 복잡도 증가. OpenSafetyRTOS의 목표는 단일 MCU에서 Decomposition으로 ASIL-D를 달성하는 것이다. 별도 MCU 전략은 다른 제품 계층에서 고려할 수 있다.

---

## 6. AoU(Assumption of Use) 영향

### AoU-09: 스타트업 코드 수정 금지 (신규)

**ID:** AoU-09
**제목:** SafetyFunction_PreInit() 이전 FreeRTOS 호출 금지
**내용:**
시스템 통합자는 Reset Handler(리셋 벡터 진입점)에서 `SafetyFunction_PreInit()`이 `FreeRTOS_Init()` 또는 `vTaskStartScheduler()`보다 먼저 호출됨을 보장해야 한다.

금지 행위:
- Reset Handler에서 FreeRTOS API를 `SafetyFunction_PreInit()` 이전에 호출
- `SafetyFunction_PreInit()`을 FreeRTOS 태스크 내에서 호출
- MPU 설정 함수를 SafetyFunction 외부에서 직접 호출하여 SafetyFunction의 MPU 설정 덮어쓰기
- 부트 자가진단을 건너뛰는 컴파일 옵션 사용 (디버그 빌드라도 자가진단 생략 금지)

**검증 방법:** 링커 맵 파일 분석 + 부트 시퀀스 코드 리뷰 (독립 검증자 수행)

**근거:** 이 AoU를 위반하면 ADR-001, ADR-002의 FFI 보장이 무효화되어 ASIL-D 인증을 받을 수 없다.

---

## 7. 구현 파일 참조

| 파일 경로                                      | 내용                              |
|-----------------------------------------------|----------------------------------|
| `arch/arm-cortex-m/startup_safety.s`          | Reset Handler — SafetyFunction_PreInit() 호출 순서 |
| `kernel/safety/safety_preinit.c`              | SafetyFunction_PreInit() 구현    |
| `kernel/safety/safety_selftest.c`             | 부트 자가진단 알고리즘            |
| `kernel/safety/safety_watchdog.c`             | IWDG 초기화 및 kick 관리         |
| `arch/arm-cortex-m/mpu_config.c`              | MPU 영역 설정 구현               |
| `docs/ADR-002-mpu-partition-strategy.md`      | MPU 영역 정의 상세               |
| `safety/doc/SAFE_STATE_DEFINITION.md`         | Safe State Level 3 리셋 상세     |

---

## 8. 결정의 결과 (Consequences)

**긍정적:**
- MPU는 FreeRTOS의 첫 번째 명령어 실행 전부터 활성화 → FFI 시간적 완결성(Temporal Completeness) 달성
- Watchdog은 FreeRTOS 초기화 전부터 동작 → 초기화 단계 hang 감지 가능
- 부트 자가진단이 FreeRTOS 실행 전 통과 → 코드/메모리 무결성 보장 후 실행
- 리셋 원인 추적으로 현장(Field) 결함 분석 가능

**부정적 / 트레이드오프:**
- 부트 시간 증가: 자가진단으로 약 수십~수백 ms 추가 (자가진단 알고리즘 최적화 필요)
- 통합자 스타트업 코드 수정 의무 (AoU-09) → 기존 BSP 사용 시 수정 작업 필요
- 자가진단 알고리즘 자체도 ASIL-D 요구사항(MC/DC 커버리지)으로 개발해야 함

---

## 9. 검토 이력

| 날짜       | 버전 | 작성자        | 변경 내용              |
|-----------|------|--------------|----------------------|
| 2026-04-18 | 1.0  | Agent-Safety | 초안 작성 및 승인      |

---

*본 ADR은 ISO 26262 Part 4 Cl.8(안전 관련 소프트웨어 아키텍처 설계) 요구사항에 따라 설계 결정 사항을 문서화한 것이다. 구현 전 Safety Manager 및 독립 검증자의 검토가 필요하다.*
