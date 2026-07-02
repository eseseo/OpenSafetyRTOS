# OSR-SSRS-001: Software Safety Requirements Specification (소프트웨어 안전 요구사항 명세)

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-SSRS-001 |
| 버전 | 1.2.0 |
| 상태 | v1.1 최종 승인 (OSR-CR-SSRS-002); v1.2 개정 — D1/D2 아키텍처 결정 반영, 재확인 검토 예정 |
| 작성일 | 2026-07-02 (최초 작성 2026-04-19) |
| 작성자 | Agent-Safety |
| 참조 표준 | ISO 26262-6:2018 Cl.6 (Specification of Software Safety Requirements) |
| 입력 문서 | OSR-FSC-001 v1.3 (v1.2 최종 승인 — OSR-CR-FSC-003; v1.3 개정 반영) |
| 연관 문서 | OSR-HARA-001 v1.3, OSR-AOU-001 v1.0, ARCHITECTURE.md, SAFE_STATE_DEFINITION.md, ADR-005 (발행 2026-07-02) |

---

## 1. 목적 및 범위

### 1.1 목적

본 문서는 OSR-FSC-001(기능안전 개념)의 FSR(Functional Safety Requirements)을 ISO 26262-6:2018 Cl.6에 따라 소프트웨어 안전 요구사항(Software Safety Requirements, SSR)으로 분해한다.

각 SSR은 다음을 만족한다:
- **고유 ID (SSR-NNN)**: 코드, 테스트와의 양방향 추적성 제공
- **구현 가능성**: 특정 함수/모듈로 구현될 수 있는 형태로 기술
- **검증 가능성**: 단위 테스트 또는 통합 테스트로 Pass/Fail 판정 가능
- **ASIL 등급 명시**: 구현 시 적용할 개발 프로세스 수준 결정

### 1.2 범위

- **대상**: OpenSafetyRTOS SafetyFunction 레이어 (ASIL-D(D) 파티션)
- **하드웨어**: ARM Cortex-M4/M7 (MPU 포함)
- **언어**: C (MISRA-C:2012 필수 적용)
- **컨텍스트**: SEooC (ISO 26262-10:2018 Cl.9)
- **실행 컨텍스트 (v1.2, D1/ADR-005)**: SafetyFunction은 FreeRTOS 태스크가 아니라 **전용 독립 하드웨어 타이머의 ISR 컨텍스트**(이하 "안전 주기 실행", `safety_cycle`) 및 스케줄러 기동 전 부팅 컨텍스트에서 실행된다 (AoU-04). QM 태스크는 FreeRTOS ARM_CM4_MPU 포트로 Unprivileged 실행된다 (AoU-10). 본 문서에서 "SafetyFunction 주기 실행"은 안전 타이머 ISR에서의 주기 수행을 의미한다.

### 1.3 SSR ID 체계

```
SSR-NNN-[구성요소약어]
  NNN: 세 자리 순번 (001부터 시작)
  구성요소: WDG(Watchdog), MPU(MPU), MBX(Mailbox), FSM(SafeState FSM),
             BST(BootSelfTest), CLK(ClockMonitor), BOD(BrownOutDetector)
```

예: `SSR-001-WDG` — Watchdog 관련 1번 SSR

---

## 2. 소프트웨어 안전 요구사항

### 2.1 Watchdog 관련 SSR (FSR-01-01, FSR-01-04 분해)

#### SSR-001-WDG: HW Watchdog 초기화
- **요구사항**: `safety_watchdog_init()` 함수는 시스템 부팅 시 SafetyFunction 초기화 단계에서 한 번 호출되어야 한다. 함수 실행 완료 후 HW Watchdog 타이머는 활성화 상태이어야 하며, 첫 번째 kick 미수행 시 `WATCHDOG_TIMEOUT_MS` 경과 후 MCU 리셋이 발생해야 한다.
- **근거 FSR**: FSR-01-01
- **ASIL**: D
- **검증 방법**: 단위 테스트 — `safety_watchdog_init()` 호출 후 kick 미수행 시 Watchdog 타임아웃 발생 확인 (시뮬레이터 또는 하드웨어)
- **구현 위치**: `kernel/safety/src/watchdog.c`, `safety_watchdog_init()`
- **MISRA-C**: 해당 함수 내 모든 포인터 역참조 이전 NULL 검사 필수 (Rule 18.2)

#### SSR-002-WDG: Watchdog Kick 주기
- **요구사항**: SafetyFunction 주기 실행(`safety_cycle` — 안전 타이머 ISR, AoU-04)은 매 사이클 종료 시 `safety_watchdog_kick()` 함수를 호출해야 한다. kick 주기는 안전 주기의 50% 이내이어야 하며, Watchdog 타임아웃(`WATCHDOG_TIMEOUT_MS`)은 하한과 상한을 모두 만족해야 한다.
  - 조건: `WATCHDOG_KICK_PERIOD_MS ≤ SAFETY_CYCLE_PERIOD_MS × 0.5`
  - 조건: `SAFETY_CYCLE_WCRT_MS × 3 ≤ WATCHDOG_TIMEOUT_MS` (오탐 리셋 방지 하한)
  - 조건: `WATCHDOG_TIMEOUT_MS ≤ FTTI_BUDGET_MS` (허용 제어 불능 시간 상한 — 통합자 AoU-04에서 확정)
- **근거 FSR**: FSR-01-04
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — 상수 관계(하한/상한 포함) 확인; 단위 테스트 — kick 미수행 시 타임아웃 경과 내 리셋 발생 확인
- **구현 위치**: `kernel/safety/src/watchdog.c`, `safety_watchdog_kick()`; `kernel/safety/include/watchdog_config.h` (상수 정의)

#### SSR-003-WDG: Watchdog 비활성화 금지
- **요구사항**: `safety_watchdog_disable()` 또는 이에 준하는 함수는 SafetyFunction 코드베이스에 존재해서는 안 된다. Watchdog 레지스터에 대한 직접 쓰기는 `safety_watchdog_init()` 및 `safety_watchdog_kick()` 외부에서 금지된다.
- **근거 FSR**: FSR-01-01
- **ASIL**: D
- **검증 방법**: 정적 분석 — Watchdog 레지스터 주소에 대한 모든 쓰기 접근 목록화 및 허용 함수 외 접근 Zero 확인
- **구현 위치**: `kernel/safety/src/watchdog.c`

---

### 2.2 MPU 관련 SSR (FSR-01-02, FSR-02-01, FSR-02-02, FSR-02-03, FSR-02-04 분해)

#### SSR-004-MPU: MPU 초기화 순서
- **요구사항**: `safety_mpu_init()` 함수는 `SafetyFunction_PreInit()` 내부에서 가장 먼저 호출되어야 한다. `safety_mpu_init()` 반환 전에 다음 MPU 리전이 모두 활성화 상태이어야 한다 (ADR-002 §"MPU Region Assignment Strategy" 기준 — 권위 문서):

  | 리전 번호 | 이름 | 기본 주소 | SafetyFunction 접근 | QM 접근 | 비고 |
  |----------|------|---------|-------------------|--------|------|
  | **Region 0** | Default background | 0x00000000 | Privileged RW | **No Access** | 전체 4 GB Deny-all; 명시적 리전 미매핑 주소 → 모두 Fault. Fail-secure 기본값 필수 |
  | Region 1 | ASIL-D Code (Flash) | 링커 스크립트 | Execute + Read (Privileged) | No Access | SafetyFunction 실행 코드 + 벡터 테이블 |
  | Region 2 | ASIL-D Data + Stack (SRAM) | 링커 스크립트 | Read + Write (Privileged, XN) | No Access | SafetyFunction 변수, BSS, 스택, MPU 섀도우 |
  | Region 3 | QM Code (Flash) | 링커 스크립트 | Read + Execute | Read + Execute | FreeRTOS 커널 + QM 애플리케이션 코드 |
  | Region 4 | QM Data (SRAM) | 링커 스크립트 | Read + Write (XN) | Read + Write (XN) | FreeRTOS 힙, 태스크 스택. Flash(0x08xxxxxxx)와 SRAM(0x20xxxxxx)은 불연속이고 리전은 2의 거듭제곱 정렬 필수이므로 QM 코드/데이터는 별도 리전 (ADR-002) |
  | Region 5 | QM→Safety Mailbox | 링커 스크립트 (256 B min) | Read + Write | Read + Write | ARMv7-M AP 인코딩상 "QM 쓰기 / Safety 읽기 전용"은 표현 불가 — 쓰기 방향 규율은 Mailbox API 소프트웨어 규약으로 강제 (ADR-002, SSR-014~019) |
  | Region 6 | Safety Stack Guard | 링커 스크립트 (32 B min, 정렬) | No Access | No Access | QM 영역과 ASIL-D SRAM 사이 가드 — 특권(커널) 스택 오버플로 시 MemManage Fault (ADR-002) |
  | Region 7 | Peripheral / Device | 디바이스별 | Privileged RW (Device, XN) | 디바이스별 제한 (SRD 카브아웃) | IWDG·Safety Timer·DMA 컨트롤러 윈도우는 SRD 비트로 Privileged 전용 분리. SCS(NVIC/VTOR)는 MPU 보호 불가 → SSR-006 주기 검증으로 보완 |

  > **Region 0 (Deny-all background) 필수 근거**: ARM Cortex-M4 MPU는 리전 번호가 높을수록 낮은 번호를 오버라이드한다. Region 0를 4 GB Deny-all로 설정하면 Region 1~7에 명시적으로 허용되지 않은 모든 주소에 대한 접근이 자동으로 Fault 처리된다. 이 리전 없이는 미매핑 주소(예: 예비 SRAM, Flash 경계 이후)가 무제한 접근 가능한 취약 구간으로 남아 FFI 요구사항을 위반한다.

  > **보호 모델 (v1.2, D1)**: 위 표의 "QM 접근" 열은 QM 태스크가 FreeRTOS ARM_CM4_MPU 포트를 통해 **Unprivileged 모드**로 실행됨을 전제한다 (AoU-10). ARMv7-M에는 MPU 잠금이 없어 특권 코드(FreeRTOS 커널 포함)는 MPU 레지스터를 재기록할 수 있으므로, MPU는 Unprivileged QM 태스크에 대한 **예방적** 통제이며 특권 커널 결함에 대해서는 SSR-009-MPU의 주기적 무결성 검증(**검출적** 통제)으로 보완된다.

- **근거 FSR**: FSR-02-01, FSR-01-02
- **ASIL**: D
- **검증 방법**: 단위 테스트 — `safety_mpu_init()` 호출 후 MPU_CTRL 레지스터 ENABLE 비트 확인; 각 리전 MPU_RBAR/MPU_RASR 읽기 검증 (`safety_mpu_integrity_check()` 호출); 통합 테스트 — 각 리전 접근 시도 시 예상 동작 확인 (Agent-VnV 수행)
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c`, `safety_mpu_init()`
- **변경 이력**: v1.1 — OSR-CR-SSRS-001 ISSUE-001 C1 조치: ADR-002 §MPU Region Assignment와 일치하도록 리전 번호 전면 재정렬; Region 0 Deny-all background 복원. v1.2 — ADR-002 확정 배치와 재정렬: 물리적으로 불가능한 단일 QM(Flash+SRAM) 리전을 QM Code/Data로 분리, ASIL-D Data와 Stack 통합, Mailbox 접근권 하드웨어 표현 한계 명시

#### SSR-005-MPU: ASIL-D 영역 QM 접근 시 HardFault
- **요구사항**: QM 컨텍스트(Unprivileged 모드)에서 ASIL-D 메모리 영역(Region 1: ASIL-D Code, Region 2: ASIL-D Data + Stack)에 읽기 또는 쓰기를 시도할 경우 MPU 예외(MemManage Fault 또는 HardFault)가 발생해야 한다. 해당 예외 핸들러는 반드시 Safe State Level 3 전이 함수를 호출해야 한다.
- **근거 FSR**: FSR-02-01
- **ASIL**: D
- **검증 방법**: 통합 테스트 — QM 태스크에서 ASIL-D 메모리 주소 쓰기 시도 → MemManage Fault 발생 → Safe State Level 3 전이 확인 (Agent-VnV 수행)
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c`; 예외 핸들러: `arch/arm-cortex-m/src/fault_handlers.c`

#### SSR-006-MPU: IVT ASIL-D 영역 배치 및 주기 검증
- **요구사항**: 인터럽트 벡터 테이블(IVT)은 링커 스크립트에 의해 ASIL-D Code 리전(Region 1) 내에 배치되어야 한다. SCB->VTOR 레지스터 값은 SafetyFunction 초기화 완료 후 ASIL-D Code 리전(Region 1) 기준 주소와 일치해야 한다. 또한 SafetyFunction은 **매 안전 주기마다** ① SCB->VTOR 값이 기대값과 일치하는지, ② 벡터 테이블 내용의 CRC32가 부팅 시 계산·저장된 참조값과 일치하는지 검증해야 한다. 불일치 감지 시 `safety_enter_level3()`을 호출해야 한다.
  - 근거: System Control Space(SCS)의 VTOR 레지스터는 특권 코드에 대해 MPU로 보호할 수 없으므로(D1(e)), 예외 핸들러 하이재킹은 주기적 검출로만 대응 가능하다.
- **근거 FSR**: FSR-02-02
- **ASIL**: D
- **검증 방법**: 빌드 검증 — 링커 맵에서 `.isr_vector` 섹션 위치 확인; 단위 테스트 — SCB->VTOR 값 검증; 결함 주입 테스트 — VTOR 변조 및 벡터 테이블 1워드 변조 후 다음 안전 주기 내 Level 3 전이 확인
- **구현 위치**: 링커 스크립트 `arch/arm-cortex-m/ldscripts/cortex-m4.ld`; 주기 검증: `kernel/safety/src/ivt_monitor.c`
- **변경 이력**: v1.2 — D1(e) 조치: 매 안전 주기 VTOR/벡터 테이블 CRC 검증 추가

#### SSR-007-MPU: Safety Timer 레지스터 MPU 보호
- **요구사항**: SafetyFunction 전용 타이머의 레지스터 공간은 MPU Region 7(Peripheral / Device)의 서브 리전으로 포함되어야 하며, Unprivileged(QM) 접근 시 MemManage Fault가 발생해야 한다. Region 7 내 Safety Timer 레지스터 주소 범위는 `mpu_config.h`의 `MPU_PERIPHERAL_SAFE_TIMER_BASE` 상수로 정의되고, `safety_mpu_integrity_check()` 검증 대상에 포함되어야 한다.
- **근거 FSR**: FSR-02-03, FSR-07-01
- **ASIL**: D
- **검증 방법**: 통합 테스트 — QM 태스크에서 Safety Timer 레지스터 주소 접근 시 MemManage Fault 발생 확인
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c` (Region 7 Peripheral 설정, Safety Timer 서브 리전)

#### SSR-008-MPU: DMA 컨트롤러 소유권 및 전송 검증
- **요구사항**: **MPU는 DMA 접근을 통제하지 않는다** (D1(f)). 따라서: ① DMA 컨트롤러 설정 레지스터(채널 소스/목적지/제어)에 대한 쓰기는 SafetyFunction 코드에서만 수행되어야 하며, QM 파티션의 DMA 사용은 SafetyFunction이 제공하는 요청 API를 통해서만 허용된다. ② `safety_dma_validate_transfer()` 함수는 DMA 전송 요청 시 소스/목적지 주소가 ASIL-D 리전에 해당하지 않는지 검사하고, 해당 시 `SAFETY_ERR_DMA_VIOLATION`을 반환하며 채널을 설정하지 않아야 한다. ③ DMA 이외 버스 마스터의 통제는 통합자 의무(AoU-07, OSR-AOU-001)로 위임된다.
- **근거 FSR**: FSR-02-04
- **ASIL**: D
- **검증 방법**: 정적 분석 — DMA 설정 레지스터 주소에 대한 쓰기 접근이 SafetyFunction 모듈 외부에 존재하지 않음(Zero) 확인; 단위 테스트 — ASIL-D 주소 범위 경계값(boundary) 포함 각 케이스로 `safety_dma_validate_transfer()` 테스트
- **구현 위치**: `kernel/safety/src/dma_guard.c`, `safety_dma_validate_transfer()`
- **변경 이력**: v1.2 — D1(f) 조치: DMA 설정 레지스터의 SafetyFunction 배타 소유 요구 추가, AoU-07 위임 명시

#### SSR-009-MPU: MPU 무결성 검증 (부팅 시 + 매 안전 주기)
- **요구사항**: `safety_mpu_integrity_check()` 함수는 ① `safety_mpu_init()` 직후, ② **매 안전 주기(safety_cycle)마다** 호출되어야 한다. 함수는 MPU_RBAR, MPU_RASR 레지스터의 실제 값을 CRC32로 보호된 shadow 설정(`mpu_expected_config[]` + `mpu_expected_config_crc` — 사용 전 shadow 자체의 CRC 검증 필수)과 비교하여, 불일치 발생 시 `SAFETY_ERR_MPU_CORRUPT`를 반환해야 한다.
  - **부팅 시** 오류: 시스템 부팅 즉시 중단 (Level 3 유지).
  - **런타임(주기 검사)** 오류: 즉시 `safety_mpu_init()` 재수행으로 보호를 복원하고 결함을 기록하며 Level 1 감시 강화에 진입한다. 동일 결함이 `FAULT_ESCALATION_COUNT`(기본값 3)회 반복되면 `safety_enter_level2()`로 에스컬레이션한다 (SAFE_STATE_DEFINITION §3.1/§3.2).
  - 검출 지연은 안전 주기(`SAFETY_CYCLE_PERIOD_MS`)로 상한되며 FTTI 예산 내에 배분되어야 한다 (D1(d)).
- **근거 FSR**: FSR-08-02, FSR-02-01
- **ASIL**: D
- **검증 방법**: 단위 테스트 — 의도적 MPU 레지스터 오염 후 `safety_mpu_integrity_check()` 호출 시 `SAFETY_ERR_MPU_CORRUPT` 반환 확인; shadow 설정 오염 시 자체 CRC 실패 처리 확인; 결함 주입 테스트 — 런타임 MPU 변조 후 다음 안전 주기 내 재초기화 및 결함 기록 확인, 3회 반복 시 Level 2 전이 확인
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c`, `safety_mpu_integrity_check()`
- **변경 이력**: v1.2 — D1(d) 조치: 부팅 1회 검증에서 매 안전 주기 검증으로 확장, CRC 보호 shadow 설정 도입, 런타임 재초기화/에스컬레이션 정책 추가, 근거 FSR에 FSR-02-01 추가

---

### 2.3 Safe State FSM 관련 SSR (FSR-01-03, FSR-03-01, FSR-03-02, FSR-03-03 분해)

#### SSR-010-FSM: SafetyFunction 실행 컨텍스트 및 인터럽트 우선순위
- **요구사항**: SafetyFunction 주기 실행(`safety_cycle`)은 FreeRTOS 태스크가 아니라 **전용 안전 타이머의 ISR**(`safety_timer_irq_handler()`)에서 수행되어야 한다 (AoU-04). 해당 ISR의 NVIC 우선순위는:
  1. 모든 QM 인터럽트보다 높아야 하며 (수치적으로 더 낮은 값),
  2. `configMAX_SYSCALL_INTERRUPT_PRIORITY`보다 높아 FreeRTOS 커널 critical section(`taskENTER_CRITICAL()` 등)에 의해 마스킹될 수 없어야 한다.
  안전 타이머 ISR의 NVIC 우선순위 레지스터 값은 초기화 후 변경되어서는 안 되며, 매 안전 주기마다 기대값과의 일치가 검증되어야 한다.
- **근거 FSR**: FSR-03-02
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — NVIC 우선순위 설정 값 및 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 대비 관계 확인; 통합 테스트 — QM 태스크가 커널 critical section을 점유한 상태에서 안전 타이머 ISR의 주기 실행 지속 확인 (Agent-VnV)
- **구현 위치**: `kernel/safety/src/safety_task.c` (safety_cycle 진입점), `arch/arm-cortex-m/src/safety_timer.c` (타이머/NVIC 초기화)
- **변경 이력**: v1.2 — D1(c) 조치: 종전 "FreeRTOS 최고 우선순위 태스크(`configMAX_PRIORITIES - 1`)" 방식을 전용 타이머 ISR 실행으로 개정 (QM 스케줄러 고장에 대한 시간 독립성 확보)

#### SSR-011-FSM: Safe State Level 1 전이
- **요구사항**: `safety_enter_level1()` 함수는 다음 동작을 원자적으로 수행해야 한다:
  1. 내부 FSM 상태를 `SAFE_STATE_L1`로 전환
  2. 사용자 등록 콜백(`safety_l1_callback`) 호출 (등록된 경우)
  3. 오류 카운터 및 원인 코드를 내부 로그에 기록
  4. Watchdog kick 계속 수행 (시스템 동작 유지)
- **근거 FSR**: FSR-04-02 (일시적 오류 응답 — Level 1 저하 운전) *(v1.2 정정: 종전 FSR-01-03 추적은 Level 3 요구사항으로서 Level 1 진입 함수와 논리적 연관이 없어 제거)*
- **ASIL**: D
- **검증 방법**: 단위 테스트 — Level 1 전이 후 FSM 상태, 콜백 호출, 로그 기록 확인
- **구현 위치**: `kernel/safety/src/safe_state.c`, `safety_enter_level1()`

#### SSR-012-FSM: Safe State Level 2 전이
- **요구사항**: `safety_enter_level2()` 함수는 다음 동작을 순서대로 수행해야 한다:
  1. 내부 FSM 상태를 `SAFE_STATE_L2`로 전환
  2. 사용자 등록 콜백(`safety_l2_callback`) 호출 — 차량 기능 안전 정지 수행
  3. FreeRTOS 애플리케이션 태스크 일시 정지 (`vTaskSuspendAll()`)
  4. 오류 원인 코드를 비휘발성 메모리에 기록 (가능한 경우)
  5. Watchdog kick 계속 수행 (복구 가능 대기)
  - 전체 수행 시간 (Step 1~4)은 `WATCHDOG_TIMEOUT_MS × 10%` 이내이어야 한다.
- **근거 FSR**: FSR-03-01, FSR-03-03
- **ASIL**: D
- **검증 방법**: 단위 테스트 — Level 2 전이 후 FSM 상태, 콜백 호출 순서 확인; WCET 분석 — 전이 함수 실행 시간 측정
- **구현 위치**: `kernel/safety/src/safe_state.c`, `safety_enter_level2()`

#### SSR-013-FSM: Safe State Level 3 전이 (Emergency Reset)
- **요구사항**: `safety_enter_level3()` 함수는 다음 동작을 인터럽트 불가 구간(critical section)에서 수행해야 한다:
  1. 모든 인터럽트 비활성화 (`__disable_irq()`)
  2. 리셋 원인 코드를 SRAM 상단 고정 주소(`RESET_CAUSE_SRAM_ADDR`)에 기록
  3. **즉시 소프트웨어 리셋 요청**: `SCB->AIRCR`에 `VECTKEY | SYSRESETREQ` 기록 후 `__DSB()` 실행 — MCU 즉시 리셋
  4. **백스톱(backstop)**: 리셋 요청이 유효하지 않은 경우에 대비하여 Watchdog kick을 **중단**하고 무한 루프(`while(1)`)에서 하드웨어 Watchdog 타임아웃에 의한 리셋을 대기
  - 전체 수행 시간(Step 1~3)은 `WATCHDOG_TIMEOUT_MS × 10%` 이내이어야 하며, Step 3 이후 다른 코드가 실행되어서는 안 된다.
  - **FTTI 비고 (D2)**: SYSRESETREQ에 의한 즉시 리셋은 종전 "Watchdog 만료 대기" 방식 대비 최대 `WATCHDOG_TIMEOUT_MS`의 Level 3 진입 지연을 제거하여 FTTI 예산 소모를 줄인다. Watchdog kick 중단은 SYSRESETREQ가 실패하는 예외적 경우의 백스톱으로만 유지된다.
- **근거 FSR**: FSR-01-03, FSR-03-01, FSR-03-03
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — critical section 범위 및 AIRCR 기록 시퀀스(VECTKEY 포함) 확인; 통합 테스트 — Level 3 전이 시 즉시 리셋 발생 확인; 결함 주입 — SYSRESETREQ 무효화 시뮬레이션에서 Watchdog 백스톱 리셋 확인
- **구현 위치**: `kernel/safety/src/safe_state.c`, `safety_enter_level3()`
- **변경 이력**: v1.2 — D2 조치: 1차 수단을 AIRCR.SYSRESETREQ 즉시 리셋으로 변경, Watchdog kick 중단은 백스톱으로 강등, FTTI 비고 추가

---

### 2.4 Mailbox 관련 SSR (FSR-04-01, FSR-04-02, FSR-06-01, FSR-06-02, FSR-06-03, FSR-06-04 분해)

#### SSR-014-MBX: Mailbox 단일 통신 경로 강제
- **요구사항**: QM 파티션에서 SafetyFunction으로의 데이터 전달은 `safety_mailbox_receive()` 함수를 통해서만 허용된다. `safety_mailbox_receive()` 이외의 경로로 QM 데이터를 SafetyFunction 변수에 직접 대입하는 코드는 코드베이스에 존재해서는 안 된다.
- **근거 FSR**: FSR-06-01
- **ASIL**: D
- **검증 방법**: 정적 분석 — QM 메모리 리전 내 주소를 SafetyFunction 변수에 할당하는 패턴 검색 및 Zero 확인
- **구현 위치**: `kernel/safety/src/mailbox.c`, `safety_mailbox_receive()`

#### SSR-015-MBX: Mailbox CRC32 검증
- **요구사항**: `safety_mailbox_receive()` 함수는 수신 데이터에 대해 CRC32(다항식 0x04C11DB7, ISO 3309)를 계산하여 Mailbox 헤더의 `crc32` 필드와 비교해야 한다. CRC 불일치 시 함수는 데이터를 복사하지 않고 `SAFETY_ERR_CRC_FAIL`을 반환해야 한다. CRC 검증은 타임스탬프 검증 이전에 수행되어야 한다.
- **근거 FSR**: FSR-06-02, FSR-04-01
- **ASIL**: D
- **검증 방법**: 단위 테스트 — 정상 CRC / 1비트 오류 / 다비트 오류 / 모두-0 케이스에서 반환값 확인; MC/DC 커버리지 100% 필수
- **구현 위치**: `kernel/safety/src/mailbox.c`, `kernel/safety/src/crc32.c`

#### SSR-016-MBX: Mailbox 타임스탬프 검증
- **요구사항**: CRC 검증 통과 후, `safety_mailbox_receive()` 함수는 현재 SafetyFunction 틱 카운터(`safety_get_tick()`)와 Mailbox 헤더의 `timestamp` 필드 차이가 `MAILBOX_MAX_AGE_MS`를 초과하는지 확인해야 한다. 초과 시 `SAFETY_ERR_STALE`을 반환하고 데이터를 복사하지 않아야 한다.
- **근거 FSR**: FSR-06-02
- **ASIL**: D
- **검증 방법**: 단위 테스트 — timestamp = 현재 - (MAILBOX_MAX_AGE_MS - 1) (통과), = 현재 - MAILBOX_MAX_AGE_MS (경계), > MAILBOX_MAX_AGE_MS (실패) 케이스 확인
- **구현 위치**: `kernel/safety/src/mailbox.c`

#### SSR-017-MBX: Mailbox 범위 검증
- **요구사항**: 타임스탬프 검증 통과 후, `safety_mailbox_receive()` 함수는 `safety_range_check()` 함수를 호출하여 데이터가 Mailbox 헤더의 `schema`에 정의된 최솟값/최댓값 내에 있는지 확인해야 한다. 범위 초과 시 `SAFETY_ERR_RANGE`를 반환하고 데이터를 복사하지 않아야 한다.
- **근거 FSR**: FSR-06-02
- **ASIL**: D
- **검증 방법**: 단위 테스트 — 최솟값, 최솟값-1, 최댓값, 최댓값+1 경계 케이스 확인; MC/DC 커버리지 100% 필수
- **구현 위치**: `kernel/safety/src/mailbox.c`, `kernel/safety/src/range_check.c`

#### SSR-018-MBX: 검증 실패 데이터 사용 금지
- **요구사항**: `safety_mailbox_receive()` 함수가 `SAFETY_OK` 이외의 값을 반환한 경우, 호출자(SafetyFunction 로직)는 출력 버퍼(`*out`)의 내용을 안전 로직에 사용해서는 안 된다. 함수 계약: 반환값이 `SAFETY_OK`가 아닌 경우 `*out` 버퍼 내용은 정의되지 않은(undefined) 상태이다.
- **근거 FSR**: FSR-06-03
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — 모든 `safety_mailbox_receive()` 호출 지점에서 반환값 확인 코드 존재 여부 검사; 단위 테스트 — `safety_mailbox_receive()` 반환값이 `SAFETY_OK`가 아닐 때 `*out` 버퍼 내용을 읽는 경로가 실행되지 않음을 경계 조건 포함 테스트로 확인 (Agent-VnV 수행, MC/DC 100%)
- **구현 위치**: `safety_mailbox_receive()` 계약 문서 + 호출 코드 리뷰; 단위 테스트: `tests/unit/test_mailbox_contract.c`
- **변경 이력**: v1.1 — OSR-CR-SSRS-001 NI-001 조치: 코드 리뷰 단독 검증에서 단위 테스트 병행으로 검증 방법 강화

#### SSR-019-MBX: 연속 검증 실패 에스컬레이션
- **요구사항**: SafetyFunction은 각 Mailbox 슬롯별 연속 검증 실패 횟수를 `mailbox_fail_count[]` 카운터로 추적해야 한다. 카운터가 `MAILBOX_FAIL_MAX`(기본값 N=3)에 도달하면 `safety_enter_level2()`를 호출해야 한다. 단일 실패(1회)는 Level 1 전이 후 카운터 증가만 수행하며, 다음 사이클에서 수신 재시도한다. 성공 시 카운터는 0으로 초기화된다.
- **근거 FSR**: FSR-06-04, FSR-04-02
- **ASIL**: D
- **검증 방법**: 단위 테스트 — 1회 실패(Level 1), 2회 연속 실패(Level 1 유지), 3회 연속 실패(Level 2), 성공 후 카운터 초기화 케이스
- **구현 위치**: `kernel/safety/src/mailbox.c`

---

### 2.5 부팅 자가진단 관련 SSR (FSR-05-01, FSR-05-02, FSR-05-03, FSR-05-04 분해)

#### SSR-020-BST: ROM CRC 검증
- **요구사항**: `safety_boot_flash_crc_check()` 함수는 SafetyFunction 코드 영역(Flash, ASIL-D Region) 전체에 대해 CRC32를 계산하여 링커가 기록한 참조값(`__flash_crc_expected`)과 비교해야 한다. 불일치 시 `SAFETY_ERR_FLASH_CRC`를 반환해야 한다.
- **근거 FSR**: FSR-05-01
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — 정상 Flash CRC (통과), Flash 내용 1바이트 변조 후 CRC (실패) 확인
- **구현 위치**: `kernel/safety/src/boot_selftest.c`, `safety_boot_flash_crc_check()`

#### SSR-021-BST: RAM March 테스트
- **요구사항**: `safety_boot_ram_march_test()` 함수는 QM 파티션과 독립된 ASIL-D Data 리전에 대해 March C- 알고리즘을 적용하여 메모리 셀 결함을 검출해야 한다. 결함 감지 시 `SAFETY_ERR_RAM_FAULT`를 반환해야 한다.
  - March C- 순서: (↑w0)(↑r0w1)(↑r1w0)(↓r0w1)(↓r1w0)(↓r0)
- **근거 FSR**: FSR-05-02
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — 정상 RAM(통과), 의도적 셀 오염 후 검출 확인 (시뮬레이터)
- **구현 위치**: `kernel/safety/src/boot_selftest.c`, `safety_boot_ram_march_test()`

#### SSR-022-BST: 부팅 자가진단 실패 시 FreeRTOS 차단
- **요구사항**: `SafetyFunction_PreInit()` 내에서 `safety_boot_flash_crc_check()` 또는 `safety_boot_ram_march_test()`가 오류를 반환하는 경우, `vTaskStartScheduler()`는 절대 호출되어서는 안 되며, 시스템은 `safety_enter_level3()`을 호출해야 한다.
- **근거 FSR**: FSR-05-03
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 통합 테스트 — Flash CRC 오류 주입 후 FreeRTOS 스케줄러 미시작 + Level 3 전이 확인
- **구현 위치**: `kernel/safety/src/safety_init.c`, `SafetyFunction_PreInit()`

#### SSR-023-BST: 리셋 원인 기록 및 비정상 리셋 감지
- **요구사항**: `safety_boot_check_reset_cause()` 함수는 부팅 시 MCU의 리셋 원인 레지스터(RCC_CSR 또는 플랫폼 등가 레지스터)를 읽어 다음을 수행해야 한다:
  1. 리셋 원인 코드(POR/Watchdog/Software/BOR 등)를 `last_reset_cause` 변수에 저장
  2. Watchdog 리셋 또는 BOR(Brown-out Reset) 감지 시 `safety_enter_level2()`를 호출
  3. 연속 비정상 리셋 횟수가 `ABNORMAL_RESET_MAX`(기본값 3) **이상**인 경우 (즉, 연속 3회째 비정상 리셋에서) `safety_enter_level3()` 호출 *(v1.2 정정: 종전 "초과(>)" 표기와 검증 케이스 "3회 이상(≥)"의 경계 불일치 해소 — ≥ 기준으로 통일)*
- **근거 FSR**: FSR-05-04, FSR-08-03
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — POR(정상), WDG Reset(Level 2), 비정상 리셋 연속 2회(Level 2 유지), 연속 3회 이상(Level 3) 경계 케이스 확인
- **구현 위치**: `kernel/safety/src/reset_cause.c`, `safety_boot_check_reset_cause()`

---

### 2.6 클럭 모니터 관련 SSR (FSR-07-01, FSR-07-02, FSR-07-03 분해)

#### SSR-024-CLK: 클럭 이상 감지 (주파수 편차)
- **요구사항**: `safety_clock_monitor_check()` 함수는 SafetyFunction 전용 타이머와 독립 RC 오실레이터(또는 LSI) 카운터를 교차 비교하여 주파수 편차를 계산해야 한다. 편차가 `±CLOCK_DEVIATION_THRESHOLD_PCT`(기본값 5%)를 초과하는 경우 `SAFETY_ERR_CLOCK_DEVIATION`을 반환해야 한다.
- **근거 FSR**: FSR-07-02
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — 편차 0%(통과), ±4.9%(통과), ±5.0%(경계), ±5.1%(실패) 케이스 확인
- **구현 위치**: `kernel/safety/src/clock_monitor.c`, `safety_clock_monitor_check()`

#### SSR-025-CLK: 클럭 이상 시 Safe State 전이
- **요구사항**: SafetyFunction 주기 실행(safety_cycle)은 매 사이클마다 `safety_clock_monitor_check()`를 호출해야 한다. 반환값이 `SAFETY_ERR_CLOCK_DEVIATION`인 경우 즉시 `safety_enter_level2()`를 호출해야 한다. 연속 `CLOCK_FAULT_MAX`(기본값 3)회 감지 시 `safety_enter_level3()`으로 에스컬레이션해야 한다.
- **근거 FSR**: FSR-07-03
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — 1~2회 감지(Level 2), 3회 연속 감지(Level 3) 케이스 확인
- **구현 위치**: `kernel/safety/src/safety_task.c` (호출), `kernel/safety/src/clock_monitor.c` (구현)

---

### 2.7 전원 이상 감지 관련 SSR (FSR-08-01, FSR-08-02, FSR-08-03 분해)

#### SSR-026-BOD: Brown-out Detector 인터럽트 핸들러
- **요구사항**: BOD(Brown-out Detector) 인터럽트 핸들러 `safety_bod_irq_handler()`는 다음을 수행해야 한다:
  1. 리셋 원인 코드 `RESET_CAUSE_BOR`를 SRAM 고정 주소에 기록
  2. `safety_enter_level3()` 호출 (Watchdog 타임아웃에 의한 리셋 유도)
  - BOD 인터럽트의 NVIC 우선순위는 FreeRTOS 태스크 인터럽트보다 높아야 한다.
- **근거 FSR**: FSR-08-01
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 통합 테스트 — BOD 인터럽트 트리거 후 Level 3 전이 및 리셋 원인 기록 확인
- **구현 위치**: `kernel/safety/src/fault.c`, `safety_bod_irq_handler()`

#### SSR-027-BOD: Brown-out Reset 이력 비휘발성 기록
- **요구사항**: `safety_boot_check_reset_cause()` 함수가 BOR 원인을 감지하는 경우, `safety_nvram_log_reset(RESET_CAUSE_BOR)` 함수를 호출하여 비휘발성 메모리(EEPROM 또는 Flash 전용 페이지)에 리셋 이력을 기록해야 한다. 기록 형식은 타임스탬프, 원인 코드, 연속 발생 횟수를 포함해야 한다.
- **근거 FSR**: FSR-08-03
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — BOR 리셋 후 NVRAM 기록 내용 검증 (타임스탬프, 원인 코드, 횟수)
- **구현 위치**: `kernel/safety/src/reset_cause.c`, `safety_nvram_log_reset()`

---

## 3. SSR 요약 테이블

| SSR ID | 내용 요약 | 근거 FSR | ASIL | 구현 위치 |
|--------|---------|---------|------|---------|
| SSR-001-WDG | HW Watchdog 초기화 | FSR-01-01 | D | watchdog.c |
| SSR-002-WDG | Watchdog kick 주기 ≤ 50%; WCRT×3 ≤ 타임아웃 ≤ FTTI 예산 | FSR-01-04 | D | watchdog.c |
| SSR-003-WDG | Watchdog 비활성화 함수 금지 | FSR-01-01 | D | watchdog.c |
| SSR-004-MPU | MPU 초기화 (8 리전) 및 순서 | FSR-02-01, FSR-01-02 | D | mpu.c |
| SSR-005-MPU | ASIL-D 영역 QM 접근 시 MemManage Fault → Level 3 | FSR-02-01 | D | mpu.c, fault_handlers.c |
| SSR-006-MPU | IVT ASIL-D 영역 배치 + 매 주기 VTOR/벡터 CRC 검증 | FSR-02-02 | D | 링커 스크립트, ivt_monitor.c |
| SSR-007-MPU | Safety Timer 레지스터 MPU 보호 | FSR-02-03, FSR-07-01 | D | mpu.c |
| SSR-008-MPU | DMA 설정 SafetyFunction 배타 소유 + QM DMA 전송 검증 | FSR-02-04 | D | dma_guard.c |
| SSR-009-MPU | 부팅 + 매 안전 주기 MPU 무결성 검증 (CRC shadow, 불일치 시 재초기화) | FSR-08-02, FSR-02-01 | D | mpu.c |
| SSR-010-FSM | safety_cycle 전용 안전 타이머 ISR 실행 (QM 인터럽트·커널 마스킹보다 높은 우선순위) | FSR-03-02 | D | safety_task.c, safety_timer.c |
| SSR-011-FSM | Safe State Level 1 전이 함수 | FSR-04-02 | D | safe_state.c |
| SSR-012-FSM | Safe State Level 2 전이 함수 (WCET ≤ WDG×10%) | FSR-03-01, FSR-03-03 | D | safe_state.c |
| SSR-013-FSM | Safe State Level 3 전이 함수 (SYSRESETREQ 즉시 리셋 + WDG 백스톱) | FSR-01-03, FSR-03-03 | D | safe_state.c |
| SSR-014-MBX | Mailbox 단일 경로 강제 | FSR-06-01 | D | mailbox.c |
| SSR-015-MBX | CRC32 검증 (검증 실패 시 조기 반환) | FSR-06-02, FSR-04-01 | D | mailbox.c, crc32.c |
| SSR-016-MBX | 타임스탬프 유효성 검증 | FSR-06-02 | D | mailbox.c |
| SSR-017-MBX | 범위 검증 (schema 기반) | FSR-06-02 | D | mailbox.c, range_check.c |
| SSR-018-MBX | 검증 실패 데이터 사용 금지 (계약) | FSR-06-03 | D | mailbox.c (계약) |
| SSR-019-MBX | 연속 N=3 실패 시 Level 2 에스컬레이션 | FSR-06-04, FSR-04-02 | D | mailbox.c |
| SSR-020-BST | Flash ROM CRC 검증 | FSR-05-01 | C(개발:D) | boot_selftest.c |
| SSR-021-BST | RAM March C- 테스트 | FSR-05-02 | C(개발:D) | boot_selftest.c |
| SSR-022-BST | 자가진단 실패 시 FreeRTOS 시작 차단 → Level 3 | FSR-05-03 | C(개발:D) | safety_init.c |
| SSR-023-BST | 리셋 원인 기록; WDG/BOR 리셋 시 Level 2; 연속 3회(≥) 비정상 리셋 → Level 3 | FSR-05-04, FSR-08-03 | C(개발:D) | reset_cause.c |
| SSR-024-CLK | 클럭 편차 ±5% 초과 감지 | FSR-07-02 | C(개발:D) | clock_monitor.c |
| SSR-025-CLK | 클럭 이상 시 Level 2, 연속 3회 → Level 3 | FSR-07-03 | C(개발:D) | clock_monitor.c |
| SSR-026-BOD | BOD 인터럽트 핸들러 → Level 3 전이 | FSR-08-01 | C(개발:D) | fault.c |
| SSR-027-BOD | BOR 이력 비휘발성 메모리 기록 | FSR-08-03 | C(개발:D) | reset_cause.c |

**합계: 27개 SSR** (FSR 27개와 1:1 이상 대응)

---

## 4. 요구사항 추적 매트릭스 (RTM) 요약

### FSR → SSR 추적

| FSR ID | FSR 내용 요약 | SSR ID(s) |
|--------|------------|---------|
| FSR-01-01 | SafetyFunction HW Watchdog 독립 감시 | SSR-001-WDG, SSR-003-WDG |
| FSR-01-02 | SafetyFunction 스택/힙 물리적 분리 | SSR-004-MPU |
| FSR-01-03 | SafetyFunction 내부 오류 → Level 3 | SSR-013-FSM |
| FSR-01-04 | Watchdog kick ≤ 50%; WCRT×3 ≤ 타임아웃 ≤ FTTI 예산 | SSR-002-WDG |
| FSR-02-01 | ASIL-D 영역 QM 접근 불가 (MPU 예방) + 주기 무결성 검증 (검출) | SSR-004-MPU, SSR-005-MPU, SSR-009-MPU |
| FSR-02-02 | IVT ASIL-D 영역 배치 + 주기 VTOR/벡터 CRC 검증 | SSR-006-MPU |
| FSR-02-03 | SafetyFunction 전용 타이머 QM 수정 불가 | SSR-007-MPU |
| FSR-02-04 | DMA 설정 배타 소유 + QM DMA 전송 검증 | SSR-008-MPU |
| FSR-03-01 | 위험 감지 → Safe State 전이 ≤ 10ms (WCRT 기준값) | SSR-012-FSM |
| FSR-03-02 | safety_cycle 전용 타이머 ISR 실행 | SSR-010-FSM |
| FSR-03-03 | 전이 함수 WCET ≤ WDG×10% | SSR-012-FSM, SSR-013-FSM |
| FSR-04-01 | Mailbox CRC32 이상 사용 | SSR-015-MBX |
| FSR-04-02 | 일시적/영구적 오류 구분 알고리즘 | SSR-011-FSM, SSR-019-MBX |
| FSR-05-01 | 부팅 Flash CRC 검증 | SSR-020-BST |
| FSR-05-02 | 부팅 RAM March 테스트 | SSR-021-BST |
| FSR-05-03 | 자가진단 실패 → FreeRTOS 차단 | SSR-022-BST |
| FSR-05-04 | 리셋 원인 기록; 비정상 리셋 → Safe State | SSR-023-BST |
| FSR-06-01 | Mailbox 단일 경로 강제 | SSR-014-MBX |
| FSR-06-02 | 3단계 검증 (CRC→타임스탬프→범위) | SSR-015-MBX, SSR-016-MBX, SSR-017-MBX |
| FSR-06-03 | 검증 실패 데이터 사용 금지 | SSR-018-MBX |
| FSR-06-04 | 연속 N=3 실패 → Level 2 | SSR-019-MBX |
| FSR-07-01 | Safety 타이머 레지스터 MPU 보호 | SSR-007-MPU |
| FSR-07-02 | 클럭 편차 ±5% 감지 | SSR-024-CLK |
| FSR-07-03 | 클럭 이상 → Level 2 이상 | SSR-025-CLK |
| FSR-08-01 | BOD 인터럽트 인터페이스 | SSR-026-BOD |
| FSR-08-02 | 부팅 시 MPU 무결성 검증 | SSR-009-MPU |
| FSR-08-03 | BOR 이력 비휘발성 기록 | SSR-023-BST, SSR-027-BOD |

---

## 5. MISRA-C:2012 적용 원칙

ASIL-D SSR 구현 시 다음 MISRA-C:2012 규칙이 필수 적용된다:

| 범주 | 규칙 | 내용 |
|------|------|------|
| 포인터 | Rule 18.2 | NULL 역참조 이전 NULL 검사 필수 |
| 포인터 형변환 | **Rule 11.3** | 포인터를 다른 오브젝트 타입으로 캐스팅 금지 (예: `uint32_t *` → `uint8_t *`); MPU 레지스터 접근 시 `volatile` 포인터를 통한 직접 접근만 허용 |
| 정수 연산 | Rule 10.1~10.4 | 묵시적 형변환 금지 |
| 데드 코드 | **Rule 2.2** | 도달 불가능한 코드(dead code) 금지; 컴파일러 최적화로 제거되지 않음을 정적 분석 도구로 확인 |
| 외부 연결 | **Rule 8.4** | 외부 연결을 가지는 함수/변수는 선언 헤더와 정의 파일이 모두 존재해야 하며 헤더에 명시적 프로토타입 필수 |
| 식 부작용 | **Rule 13.2** | 식 평가 순서에 의존하는 부작용(side effect) 포함 식 금지; 특히 `safety_mailbox_receive()` 내 CRC/타임스탬프/범위 검증 조건식에서 함수 호출 부작용 분리 필수 |
| 분기 | Rule 15.5 | 단일 종료점 원칙 (함수당 return 1개) |
| 루프 | Rule 14.2 | for 루프 카운터 변수는 루프 내에서만 수정 |
| 함수 | Rule 17.7 | 모든 함수 반환값은 사용되거나 명시적으로 무시 |
| 전처리기 | Rule 20.1~20.14 | 헤더 가드 필수, 재귀 매크로 금지 |

> **v1.1 추가 (NI-002 조치)**: Rule 11.3, 2.2, 8.4, 13.2 추가. ASIL-D SafetyFunction 코드에서 특히 중요한 규칙으로, MPU 레지스터 접근(11.3), 정적 분석 기반 데드 코드 제거(2.2), 함수 선언/정의 일관성(8.4), Mailbox 검증 체인 내 부작용 없는 조건식(13.2) 준수를 보장한다.

---

## 6. 미결 사항

| 번호 | 미결 항목 | 위임 대상 | 관련 SSR |
|-----|---------|---------|---------|
| 1 | SafetyFunction WCRT 값 (WCET 분석 결과) | TSC / 플랫폼 WCET 분석 | SSR-002-WDG, SSR-012-FSM |
| 2 | FTTI 대비 10ms WCRT 기준값 충분성 확인 및 `FTTI_BUDGET_MS` 확정 | 통합자 AoU-04 (OSR-AOU-001) | SSR-002-WDG, SSR-012-FSM |
| 3 | MAILBOX_MAX_AGE_MS 구체적 값 | TSC | SSR-016-MBX |
| 4 | MAILBOX_FAIL_MAX N 파라미터 확정 | TSC | SSR-019-MBX |
| 5 | ABNORMAL_RESET_MAX 파라미터 확정 | TSC | SSR-023-BST |
| 6 | CLOCK_DEVIATION_THRESHOLD_PCT 플랫폼 검증 | TSC / 하드웨어 플랫폼 | SSR-024-CLK |
| 7 | 비휘발성 메모리 구현체 (EEPROM/Flash) | 통합자 HAL | SSR-027-BOD |
| 8 | NVIC BOD 인터럽트 우선순위 값 | TSC / 플랫폼 HAL | SSR-026-BOD |
| 9 | `SAFETY_CYCLE_PERIOD_MS` 확정 (MPU/VTOR 주기 검증 검출 지연 상한 — FTTI 예산 배분) | TSC / 통합자 AoU-04 | SSR-006-MPU, SSR-009-MPU |
| 10 | 안전 타이머 및 NVIC 우선순위 플랫폼 매핑 | 통합자 AoU-04 / 플랫폼 HAL | SSR-010-FSM |

---

## 7. Agent-QA 확인 검토 요청

본 문서는 **Agent-QA의 확인 검토(Confirmation Review, ISO 26262-2:2018 Cl.6.4.9)** 대상이다.

- **검토 기록**: OSR-CR-SSRS-001 (완료 — v1.0 조건부 승인), OSR-CR-SSRS-002 (완료 — v1.1 최종 승인), v1.2 변경분 재확인 검토 (예정 — D1/D2 반영분: SSR-002/004/006/008/009/010/011/013/023/025)
- **검토 표준**: ISO 26262-6:2018 Cl.6
- **검토 중점 사항**:
  - 모든 FSR(FSR-01-xx ~ FSR-08-xx)이 SSR로 추적 가능한지 확인
  - 각 SSR이 구현·테스트 가능한 형태로 기술되었는지 확인
  - SSR ASIL ≥ 근거 FSR ASIL 확인
  - MC/DC 커버리지 요구 SSR 목록이 ASIL-D 요구사항에 부합하는지 확인
  - MISRA-C 적용 규칙이 ASIL-D 요건을 충족하는지 확인

---

## 문서 이력

| 버전 | 날짜 | 변경 내용 | 작성자 |
|------|------|---------|--------|
| 1.0.0 | 2026-04-19 | 최초 작성 (Draft) — OSR-FSC-001 v1.2 최종 승인(OSR-CR-FSC-003) 기반, FSR 27개 → SSR 27개 분해 | Agent-Safety |
| 1.1.0 | 2026-04-19 | OSR-CR-SSRS-001 조치: ISSUE-001 C1 (SSR-004 MPU 리전 번호 ADR-002 일치 재정렬, Region 0 Deny-all background 복원), NI-001 (SSR-018 단위 테스트 추가), NI-002 (MISRA-C Rule 11.3/2.2/8.4/13.2 추가); SSR-005/006/007 리전 번호 참조 정정. OSR-CR-SSRS-002에서 최종 승인. | Agent-Safety |
| 1.2.0 | 2026-07-02 | 외부 평가 및 중앙 설계 결정(D1/D2) 반영: ① D1(c) — SSR-010 개정: FreeRTOS 최고 우선순위 태스크(`configMAX_PRIORITIES-1`) 방식을 전용 안전 타이머 ISR 실행(모든 QM 인터럽트 및 `configMAX_SYSCALL_INTERRUPT_PRIORITY`보다 높은 NVIC 우선순위)으로 변경; §1.2 실행 컨텍스트 명시("주기 태스크"→"주기 실행/safety_cycle" 전면 정비 — SSR-002/025). ② D1(d) — SSR-009 확장: 부팅 1회 → 부팅+매 안전 주기 MPU 무결성 검증, CRC 보호 shadow 설정, 런타임 재초기화/에스컬레이션 정책; 근거 FSR-02-01 추가. ③ D1(e) — SSR-006 확장: 매 안전 주기 VTOR/벡터 테이블 CRC 검증. ④ D1(a)/(b) — SSR-004에 AoU-10(Unprivileged QM, ARM_CM4_MPU 포트) 및 예방/검출 통제 구분 주석 추가. ⑤ D1(f) — SSR-008 개정: DMA 설정 레지스터 SafetyFunction 배타 소유 + AoU-07 위임. ⑥ D2 — SSR-013 개정: Level 3 1차 수단 AIRCR.SYSRESETREQ 즉시 리셋, Watchdog 백스톱 강등, FTTI 비고. ⑦ SSR-002 Watchdog 타임아웃 상한(≤ FTTI_BUDGET_MS) 추가. ⑧ SSR-011 근거 FSR 정정(FSR-01-03 제거 → FSR-04-02 단독) 및 RTM 갱신. ⑨ SSR-023 경계 불일치 해소(초과→이상, 연속 3회 → Level 3) 및 검증 케이스 정합. ⑩ 표준 인용 정정(Part 6 Cl.7→ISO 26262-6:2018 Cl.6, Part 8 Cl.13→ISO 26262-10:2018 Cl.9, Part 2 Cl.8→Cl.6.4.9); AoU 참조 OSR-AOU-001 정식 ID 전환; §6 미결 항목 9/10 신설; 헤더 승인 상태 정합(v1.1 최종 승인 반영). | Agent-Safety |

---

*본 문서는 ISO 26262-6:2018 Cl.6에 따라 작성된 소프트웨어 안전 요구사항 명세입니다.*
*SEooC 개발 컨텍스트(ISO 26262-10:2018 Cl.9)에 따라 미결 파라미터는 TSC 또는 통합자 AoU(OSR-AOU-001)로 위임됩니다.*
