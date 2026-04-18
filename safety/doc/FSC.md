# OSR-FSC-001: Functional Safety Concept (기능안전 개념)

| 항목           | 내용                                      |
|--------------|------------------------------------------|
| 문서 ID        | OSR-FSC-001                              |
| 버전           | 1.0.0                                    |
| 상태           | Draft                                    |
| 작성일          | 2026-04-18                               |
| 작성자          | Agent-Safety                             |
| 참조 표준        | ISO 26262 Part 3 Cl.8                    |
| 입력 문서        | OSR-HARA-001 (HARA)                      |
| 연관 문서        | OSR-CR-FSC-001 (Confirmation Review 기록) |

---

## 1. 목적 및 범위

### 1.1 목적

본 문서는 HARA(OSR-HARA-001)에서 도출된 안전 목표(Safety Goals, SG)를 기능 수준의 안전 요구사항(Functional Safety Requirements, FSR)으로 분해한다. ISO 26262 Part 3 Cl.8의 기능안전 개념(Functional Safety Concept) 요구사항을 충족하기 위해 작성되었으며, 이후 기술안전 개념(Technical Safety Concept, TSC) 수립의 입력으로 활용된다.

### 1.2 범위

- **대상 시스템**: OpenSafetyRTOS
- **구성**: FreeRTOS QM(D) + SafetyFunction ASIL-D(D) (ISO 26262 Part 9 Decomposition 적용)
- **개발 컨텍스트**: SEooC (Safety Element out of Context) — ISO 26262 Part 8 Cl.13
- **목표 하드웨어**: ARM Cortex-M4/M7
- **Safe State 정의**:
  - Level 1: Degraded Operation (저하된 동작)
  - Level 2: Controlled Stop (제어된 정지)
  - Level 3: Emergency Reset (비상 리셋)

### 1.3 SEooC 컨텍스트 명시

OpenSafetyRTOS는 SEooC로 개발된다. 즉, 최종 통합 시스템(item)의 컨텍스트 없이 개발되므로 다음이 적용된다.

- 통합자(Integrator)에 대한 **사용 가정(Assumptions of Use, AoU)** 을 별도 문서에 명시한다.
- 본 FSC에서 미결 파라미터([X]ms 등)는 통합자 AoU로 위임된다.
- 통합자는 AoU를 검토하고 자신의 HARA/FSC와의 일치 여부를 확인할 책임을 진다.

---

## 2. 기능안전 개념 개요

### 2.1 전체 아키텍처 개요

OpenSafetyRTOS의 기능안전 개념은 ISO 26262 Part 9의 **Decomposition** 전략을 핵심으로 한다.

```
┌─────────────────────────────────────────────────────────┐
│                   OpenSafetyRTOS                        │
│                                                         │
│  ┌──────────────────────┐  ┌──────────────────────────┐ │
│  │  SafetyFunction      │  │  FreeRTOS Kernel         │ │
│  │  ASIL-D(D)           │  │  QM(D)                   │ │
│  │                      │  │                          │ │
│  │  - Safety Monitor    │  │  - Task Scheduler        │ │
│  │  - Safe State FSM    │  │  - Memory Manager        │ │
│  │  - Boot Self-Test    │  │  - QM Application Tasks  │ │
│  │  - Mailbox Validator │  │                          │ │
│  └──────────┬───────────┘  └──────────────────────────┘ │
│             │  Mailbox API (제어된 통신)                  │
│  ┌──────────▼───────────────────────────────────────┐   │
│  │           MPU (Memory Protection Unit)           │   │
│  │   ASIL-D 메모리 ↔ QM 메모리 공간 분리               │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │     독립 Hardware Watchdog (SafetyFunction 전용)  │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 2.2 핵심 안전 메커니즘

#### 2.2.1 Decomposition 구조 (ISO 26262 Part 9)

SafetyFunction(ASIL-D)이 FreeRTOS QM(D) 위에서 **독립적으로** 동작한다. FreeRTOS는 QM 등급으로 개발되며, SafetyFunction이 ASIL-D 요구사항을 충족함으로써 전체 시스템의 ASIL-D를 달성한다.

- FreeRTOS QM(D): 기본 스케줄링 및 QM 응용 기능 제공
- SafetyFunction ASIL-D(D): 모든 안전 관련 기능을 담당하며 FreeRTOS의 결함에 대해 독립성 유지

#### 2.2.2 공간 분리 (Spatial Separation) — MPU

ARM Cortex-M4/M7의 MPU(Memory Protection Unit)를 활용하여 ASIL-D 메모리 영역과 QM 메모리 영역을 하드웨어 수준에서 물리적으로 분리한다. QM 파티션은 ASIL-D 코드/데이터 영역에 대한 읽기/쓰기 권한이 없다.

#### 2.2.3 시간 분리 (Temporal Monitoring) — 독립 Watchdog

SafetyFunction 전용 하드웨어 Watchdog 타이머를 운영한다. SafetyFunction이 정상 동작하는 경우에만 Watchdog을 갱신(kick)하며, SafetyFunction의 응답 불능 시 하드웨어 수준에서 시스템 리셋을 강제한다. 이 Watchdog은 QM 파티션에서 제어할 수 없다.

#### 2.2.4 제어된 통신 — Mailbox

QM 파티션에서 SafetyFunction으로의 모든 데이터 전달은 Mailbox API를 유일한 경로로 사용한다. Mailbox는 CRC32 검증, 타임스탬프 검증, 범위 검증의 3단계 검증을 수행하며, 검증에 실패한 데이터는 SafetyFunction 로직에 사용되지 않는다.

---

## 3. 안전 목표 → 기능안전 요구사항 분해

### 3.1 SG-01: SafetyFunction은 단일 고장으로 완전히 기능 상실해서는 안 됨 (ASIL-D)

**안전 목표 설명**: SafetyFunction은 단일 점 고장(Single-Point Fault)에 의해 완전히 기능을 상실하지 않아야 한다.

#### FSR-01-01
- **요구사항**: SafetyFunction은 하드웨어 Watchdog에 의해 독립적으로 감시되어야 한다.
- **근거**: SafetyFunction 자체의 소프트웨어 고장(무한 루프, 응답 불능 등) 발생 시 하드웨어 수준에서 복구 메커니즘을 제공하기 위함.
- **ASIL**: D

#### FSR-01-02
- **요구사항**: SafetyFunction의 스택/힙은 QM 파티션과 물리적으로 분리되어야 한다.
- **근거**: QM 파티션의 메모리 오염이 SafetyFunction의 실행 컨텍스트를 훼손하지 않도록 방지.
- **ASIL**: D

#### FSR-01-03
- **요구사항**: SafetyFunction 자체 내부 오류 발생 시 Safe State Level 3(Emergency Reset)으로 전이해야 한다.
- **근거**: SafetyFunction의 내부 오류는 안전 기능 자체의 신뢰성 손상을 의미하므로 최고 수준의 Safe State로 전이하여 시스템 안전성을 보장.
- **ASIL**: D

---

### 3.2 SG-02: QM 파티션이 SafetyFunction에 간섭 불가 (FFI) (ASIL-D)

**안전 목표 설명**: QM 파티션은 SafetyFunction의 정상 동작을 방해(Freedom From Interference)할 수 없어야 한다.

#### FSR-02-01
- **요구사항**: ASIL-D 메모리 영역은 QM 파티션에서 접근 불가능해야 한다 (MPU 강제 적용).
- **근거**: QM 코드/데이터의 버그 또는 악성 접근이 ASIL-D 영역을 오염시키는 것을 하드웨어 수준에서 차단.
- **ASIL**: D

#### FSR-02-02
- **요구사항**: 인터럽트 벡터 테이블(IVT)은 ASIL-D 영역에 배치되어야 한다.
- **근거**: QM 파티션이 IVT를 수정하여 예외 핸들러를 하이재킹하는 것을 방지.
- **ASIL**: D

#### FSR-02-03
- **요구사항**: SafetyFunction 전용 타이머/클럭 자원은 QM이 수정할 수 없어야 한다.
- **근거**: QM이 SafetyFunction 타이머 주기를 변조하여 시간 보장(Timing Guarantee)을 훼손하는 것을 방지.
- **ASIL**: D

#### FSR-02-04
- **요구사항**: QM DMA는 ASIL-D 메모리 영역에 접근할 수 없어야 한다.
- **근거**: DMA는 MPU를 우회하는 버스 마스터이므로, QM이 DMA를 통해 ASIL-D 영역에 간접 접근하는 경로를 별도로 차단해야 함.
- **ASIL**: D

---

### 3.3 SG-03: 위험 감지 후 [X]ms 이내 Safe State 전이 (ASIL-D)

**안전 목표 설명**: 위험 조건 감지부터 Safe State 전이 완료까지의 응답 시간이 [X]ms를 초과해서는 안 된다. ([X]는 통합자 AoU로 위임)

#### FSR-03-01
- **요구사항**: 위험 조건 감지에서 Safe State 전이 개시까지의 시간은 [X]ms를 초과해서는 안 된다.
- **근거**: 안전 목표 SG-03의 직접 구현. 구체적 [X] 값은 통합자 AoU(AoU-03)에서 결정.
- **ASIL**: D

#### FSR-03-02
- **요구사항**: SafetyFunction 태스크는 시스템 내 최고 우선순위를 가져야 한다.
- **근거**: QM 태스크에 의한 선점(preemption)으로 SafetyFunction 응답 지연이 발생하지 않도록 보장.
- **ASIL**: D

#### FSR-03-03
- **요구사항**: Safe State 전이 함수는 인터럽트 불가(interrupt-disabled) 구간에서 실행되어야 한다.
- **근거**: Safe State 전이 중 인터럽트로 인한 실행 중단이 전이 완료 시간을 보장하지 못하게 되는 것을 방지.
- **ASIL**: D

---

### 3.4 SG-04: 오탐으로 인한 불필요한 Safe State 전이 최소화 (ASIL-B)

**안전 목표 설명**: 일시적 오류(transient fault) 또는 검증 오류 등으로 인한 불필요한 Safe State 전이(false positive)를 최소화하여 시스템 가용성을 보호해야 한다.

#### FSR-04-01
- **요구사항**: Mailbox 검증은 단일 비트 오류를 오탐으로 처리하지 않도록 CRC32 이상의 오류 검출 코드를 사용해야 한다.
- **근거**: 단일 비트 오류가 데이터 오류로 오인되어 불필요한 Safe State 전이를 유발하지 않도록 충분한 Hamming Distance를 가진 CRC를 적용.
- **ASIL**: B (ASIL-D로 개발, 상위 호환)

#### FSR-04-02
- **요구사항**: 일시적 오류(transient fault)와 영구적 오류(permanent fault)를 구분하는 메커니즘을 제공해야 한다.
- **근거**: 일시적 오류는 재시도 또는 경고로 처리하고, 영구적 오류만 Safe State 전이를 유발함으로써 불필요한 시스템 정지를 억제.
- **ASIL**: B (ASIL-D로 개발, 상위 호환)

---

### 3.5 SG-05: 부팅 자가진단 완료, 결함 시 시작 거부 (ASIL-C)

**안전 목표 설명**: 시스템 시작 시 자가진단(Power-On Self-Test, POST)을 수행하고, 결함이 발견된 경우 FreeRTOS 및 응용 소프트웨어의 시작을 거부해야 한다.

#### FSR-05-01
- **요구사항**: 부팅 시 ROM(Flash) CRC 검증을 수행해야 한다.
- **근거**: 펌웨어 이미지 손상 또는 비정상 플래시 상태를 감지하여 손상된 코드가 실행되는 것을 방지.
- **ASIL**: C (ASIL-D로 개발, 상위 호환)

#### FSR-05-02
- **요구사항**: 부팅 시 RAM March 테스트를 수행해야 한다.
- **근거**: 메모리 셀 결함(Stuck-At, Coupling Fault 등)을 감지하여 결함 있는 메모리로 시스템이 동작하는 것을 방지.
- **ASIL**: C (ASIL-D로 개발, 상위 호환)

#### FSR-05-03
- **요구사항**: 부팅 자가진단 실패 시 FreeRTOS 초기화를 차단해야 한다.
- **근거**: 자가진단 실패 결과를 무시하고 시스템이 계속 부팅되는 것을 하드웨어 수준 또는 초기 부트로더에서 차단.
- **ASIL**: C (ASIL-D로 개발, 상위 호환)

#### FSR-05-04
- **요구사항**: 리셋 원인을 기록하고, 비정상 리셋(Watchdog 리셋, 예외 리셋 등) 감지 시 Safe State로 진입해야 한다.
- **근거**: 비정상 리셋 반복 발생은 지속적 결함을 의미하므로, 이를 감지하고 기록하여 추가 고장 전파를 방지.
- **ASIL**: C (ASIL-D로 개발, 상위 호환)

---

### 3.6 SG-06: QM 데이터 검증 없이 신뢰 금지 (ASIL-D)

**안전 목표 설명**: QM 파티션에서 전달되는 모든 데이터는 검증 없이 SafetyFunction 로직에 사용되어서는 안 된다.

#### FSR-06-01
- **요구사항**: QM→SafetyFunction 모든 데이터는 Mailbox API를 통해서만 전달되어야 한다.
- **근거**: 직접 공유 메모리 접근 등 Mailbox를 우회하는 통신 경로는 검증 메커니즘을 무효화하므로 단일 통신 경로를 강제.
- **ASIL**: D

#### FSR-06-02
- **요구사항**: Mailbox 수신 시 ① CRC 검증 → ② 타임스탬프 검증 → ③ 범위 검증의 3단계를 정해진 순서대로 수행해야 한다.
- **근거**: CRC 실패 시 조기 종료로 후속 검증 비용을 절감하고, 타임스탬프 검증으로 재생 공격(replay) 및 stale 데이터를, 범위 검증으로 물리적 비현실 값을 차단.
- **ASIL**: D

#### FSR-06-03
- **요구사항**: 검증 실패한 데이터는 SafetyFunction 로직에 사용되어서는 안 된다.
- **근거**: 검증 실패 데이터를 부분적으로라도 사용하면 SG-06의 의도를 위반. 실패한 데이터는 폐기하고 오류 카운터를 증가시켜야 함.
- **ASIL**: D

---

## 4. FSR 요약 테이블

| FSR-ID    | 내용 (요약)                                              | 근거 SG | ASIL          | 구현 위치 (예상)                        |
|-----------|--------------------------------------------------------|---------|---------------|-----------------------------------------|
| FSR-01-01 | SafetyFunction은 HW Watchdog에 의해 독립 감시            | SG-01   | D             | SafetyFunction / HAL_Watchdog           |
| FSR-01-02 | SafetyFunction 스택/힙은 QM 파티션과 물리적 분리          | SG-01   | D             | MPU 설정 / 링커 스크립트                  |
| FSR-01-03 | SafetyFunction 내부 오류 시 Safe State Level 3 전이      | SG-01   | D             | SafetyFunction / SafeStateFSM           |
| FSR-02-01 | ASIL-D 메모리 영역은 QM에서 접근 불가 (MPU 강제)           | SG-02   | D             | MPU 설정 / 부팅 초기화                    |
| FSR-02-02 | 인터럽트 벡터 테이블은 ASIL-D 영역에 배치                  | SG-02   | D             | 링커 스크립트 / 부팅 초기화                |
| FSR-02-03 | SafetyFunction 전용 타이머/클럭은 QM이 수정 불가           | SG-02   | D             | SafetyFunction / HAL_Timer              |
| FSR-02-04 | QM DMA는 ASIL-D 메모리 영역에 접근 불가                   | SG-02   | D             | DMA 채널 설정 / HAL_DMA                  |
| FSR-03-01 | 위험 감지 → Safe State 전이 개시까지 [X]ms 이내            | SG-03   | D             | SafetyFunction / SafeStateFSM           |
| FSR-03-02 | SafetyFunction 태스크는 시스템 내 최고 우선순위 보유        | SG-03   | D             | FreeRTOS 태스크 설정                      |
| FSR-03-03 | Safe State 전이 함수는 인터럽트 불가 구간에서 실행          | SG-03   | D             | SafeStateFSM / 인터럽트 제어 루틴         |
| FSR-04-01 | Mailbox 검증에 CRC32 이상 사용 (단일 비트 오탐 방지)        | SG-04   | B (개발: D)   | Mailbox / CRC 모듈                       |
| FSR-04-02 | 일시적/영구적 오류 구분 메커니즘 제공                       | SG-04   | B (개발: D)   | SafetyFunction / 오류 분류기             |
| FSR-05-01 | 부팅 시 ROM CRC 검증 수행                                 | SG-05   | C (개발: D)   | BootSelfTest / CRC 모듈                  |
| FSR-05-02 | 부팅 시 RAM March 테스트 수행                              | SG-05   | C (개발: D)   | BootSelfTest / RAM 테스트 루틴           |
| FSR-05-03 | 부팅 자가진단 실패 시 FreeRTOS 초기화 차단                  | SG-05   | C (개발: D)   | BootSelfTest / 부팅 시퀀서              |
| FSR-05-04 | 리셋 원인 기록 및 비정상 리셋 감지 시 Safe State 진입        | SG-05   | C (개발: D)   | BootSelfTest / 리셋 원인 레지스터 HAL    |
| FSR-06-01 | QM→SafetyFunction 데이터는 Mailbox API만 경유             | SG-06   | D             | Mailbox API / 아키텍처 설계 규칙         |
| FSR-06-02 | Mailbox 수신 시 CRC → 타임스탬프 → 범위 3단계 검증         | SG-06   | D             | Mailbox / 검증 파이프라인                |
| FSR-06-03 | 검증 실패 데이터는 SafetyFunction 로직에 사용 금지          | SG-06   | D             | Mailbox / SafetyFunction 입력 처리       |

---

## 5. Decomposition 전략 연계

### 5.1 ISO 26262 Part 9 Decomposition 근거

OpenSafetyRTOS는 ISO 26262 Part 9의 Decomposition 전략을 통해 ASIL-D 시스템 요구사항을 충족한다.

| 시스템 요구 ASIL | 구성 요소 A              | 구성 요소 B         | 독립성 요건        |
|----------------|------------------------|--------------------|--------------------|
| ASIL-D         | SafetyFunction ASIL-D(D) | FreeRTOS QM(D)     | FFI 보장 (SG-02)  |

### 5.2 안전 목표별 Decomposition 연계

#### SG-01, SG-02, SG-03, SG-06 (ASIL-D)
- SafetyFunction ASIL-D(D) 개발로 직접 충족.
- SafetyFunction은 FreeRTOS QM(D)의 결함에 대해 독립성을 유지함.
- MPU 기반 FFI(FSR-02-01~04)가 Decomposition의 독립성 요건을 만족시킴.

#### SG-04 (ASIL-B)
- Mailbox 검증 알고리즘이 ASIL-B 요구사항을 담당.
- OpenSafetyRTOS는 Mailbox를 ASIL-D 수준으로 개발하므로 ASIL-B 요구사항에 대해 상위 호환(over-fulfillment).
- ASIL-D 개발 프로세스로 ASIL-B 요구사항 충족 가능 (ISO 26262 Part 9 Cl.5.4).

#### SG-05 (ASIL-C)
- 부팅 자가진단 모듈(BootSelfTest)이 ASIL-C 요구사항을 담당.
- OpenSafetyRTOS는 BootSelfTest를 ASIL-D 수준으로 개발하므로 ASIL-C 요구사항에 대해 상위 호환.

### 5.3 전체 시스템 ASIL-D 달성 논증

```
시스템 ASIL-D 달성
├── SafetyFunction ASIL-D(D): 안전 기능 직접 구현
│   ├── 독립 Watchdog (FSR-01-01)
│   ├── Safe State FSM (FSR-01-03, FSR-03-01~03)
│   ├── Mailbox 검증 (FSR-06-01~03, FSR-04-01~02)
│   └── Boot Self-Test (FSR-05-01~04)
│
├── FreeRTOS QM(D): 비안전 기능 제공, SafetyFunction과 독립
│   └── FFI 보장: MPU(FSR-02-01~04) + 통신 제어(FSR-06-01)
│
└── Decomposition 독립성: ISO 26262 Part 9 충족
    └── SafetyFunction은 FreeRTOS의 임의 고장에 무관하게 안전 기능 유지
```

---

## 6. 미결 사항 (To be refined in TSC)

아래 항목들은 현재 FSC 단계에서 결정할 수 없으며, 기술안전 개념(Technical Safety Concept, TSC) 또는 통합자(Integrator) AoU에서 구체화되어야 한다.

| 번호 | 미결 항목                              | 위임 대상              | 관련 FSR       | 비고                               |
|-----|--------------------------------------|----------------------|----------------|------------------------------------|
| 1   | [X]ms 응답 시간 파라미터 결정           | 통합자 AoU-03          | FSR-03-01      | 응용 도메인(자동차, 산업 등)에 따라 상이 |
| 2   | 특정 하드웨어 Watchdog 타이머 스펙      | 통합자 AoU-05          | FSR-01-01      | MCU 모델(STM32H7 등)에 따라 상이     |
| 3   | DMA 채널 할당 정책                     | 플랫폼별 HAL           | FSR-02-04      | 플랫폼 HAL에서 구체적 채널 매핑 정의   |
| 4   | RAM March 테스트 알고리즘 선택          | TSC / 하드웨어 FMEA    | FSR-05-02      | March C-, March X 등 알고리즘 선택   |
| 5   | 일시적/영구적 오류 구분 임계값           | TSC                   | FSR-04-02      | 재시도 횟수, 시간 윈도우 등 정의 필요  |
| 6   | 인터럽트 불가 구간 최대 허용 시간        | TSC / 실시간성 분석     | FSR-03-03      | WCET(Worst-Case Execution Time) 분석 |

---

## 7. Agent-QA 확인 검토 요청

이 문서는 **Agent-QA의 확인 검토(Confirmation Review, ISO 26262 Part 2 Cl.8)** 대상이다.

- **검토 기록**: OSR-CR-FSC-001
- **검토 대상 표준**: ISO 26262 Part 3 Cl.8 (Functional Safety Concept)
- **검토 항목**:
  - 모든 SG(SG-01~06)가 FSR로 완전히 분해되었는지 추적성(Traceability) 확인
  - FSR의 ASIL 등급이 근거 SG의 ASIL 이상인지 확인
  - Decomposition 전략이 ISO 26262 Part 9 요구사항을 충족하는지 확인
  - SEooC 컨텍스트 및 AoU 위임 항목이 적절히 명시되었는지 확인
  - 미결 사항의 위임 대상이 명확히 정의되었는지 확인

---

## 문서 이력

| 버전    | 날짜         | 변경 내용          | 작성자        |
|--------|------------|------------------|--------------|
| 1.0.0  | 2026-04-18 | 최초 작성 (Draft)  | Agent-Safety |

---

*본 문서는 ISO 26262 Part 3 Cl.8에 따라 작성된 기능안전 개념 문서입니다.*
*SEooC 개발 컨텍스트(ISO 26262 Part 8 Cl.13)에 따라 미결 파라미터는 통합자 AoU로 위임됩니다.*
