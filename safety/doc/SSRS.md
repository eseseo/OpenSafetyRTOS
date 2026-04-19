# OSR-SSRS-001: Software Safety Requirements Specification (소프트웨어 안전 요구사항 명세)

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-SSRS-001 |
| 버전 | 1.0.0 |
| 상태 | Draft |
| 작성일 | 2026-04-19 |
| 작성자 | Agent-Safety |
| 참조 표준 | ISO 26262 Part 6 Cl.7 (Software Safety Requirements) |
| 입력 문서 | OSR-FSC-001 v1.2 ✅ (최종 승인 — OSR-CR-FSC-003) |
| 연관 문서 | OSR-HARA-001 v1.2, ARCHITECTURE.md, SAFE_STATE_DEFINITION.md |

---

## 1. 목적 및 범위

### 1.1 목적

본 문서는 OSR-FSC-001(기능안전 개념)의 FSR(Functional Safety Requirements)을 ISO 26262 Part 6 Cl.7에 따라 소프트웨어 안전 요구사항(Software Safety Requirements, SSR)으로 분해한다.

각 SSR은 다음을 만족한다:
- **고유 ID (SSR-NNN)**: 코드, 테스트와의 양방향 추적성 제공
- **구현 가능성**: 특정 함수/모듈로 구현될 수 있는 형태로 기술
- **검증 가능성**: 단위 테스트 또는 통합 테스트로 Pass/Fail 판정 가능
- **ASIL 등급 명시**: 구현 시 적용할 개발 프로세스 수준 결정

### 1.2 범위

- **대상**: OpenSafetyRTOS SafetyFunction 레이어 (ASIL-D(D) 파티션)
- **하드웨어**: ARM Cortex-M4/M7 (MPU 포함)
- **언어**: C (MISRA-C:2012 필수 적용)
- **컨텍스트**: SEooC (ISO 26262 Part 8 Cl.13)

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
- **요구사항**: SafetyFunction 주기 태스크(safety_task)는 매 실행 사이클 종료 시 `safety_watchdog_kick()` 함수를 호출해야 한다. kick 주기는 SafetyFunction 태스크 실행 주기의 50% 이내이어야 하며, Watchdog 타임아웃(`WATCHDOG_TIMEOUT_MS`)은 SafetyFunction WCRT × 3 이상으로 설정해야 한다.
  - 조건: `WATCHDOG_KICK_PERIOD_MS ≤ SAFETY_TASK_PERIOD_MS × 0.5`
  - 조건: `WATCHDOG_TIMEOUT_MS ≥ SAFETY_TASK_WCRT_MS × 3`
- **근거 FSR**: FSR-01-04
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — 상수 관계 확인; 단위 테스트 — kick 미수행 시 타임아웃 경과 내 리셋 발생 확인
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
- **요구사항**: `safety_mpu_init()` 함수는 `SafetyFunction_PreInit()` 내부에서 가장 먼저 호출되어야 한다. `safety_mpu_init()` 반환 전에 다음 MPU 리전이 모두 활성화 상태이어야 한다:
  - Region 0: ASIL-D Code (Read/Execute, Privileged Only)
  - Region 1: ASIL-D Data (Read/Write, Privileged Only)
  - Region 2: QM Code/Data (Read/Write, Unprivileged 접근 허용)
  - Region 3: QM→Safety Mailbox (QM: Read/Write, Safety: Read Only)
  - Region 4: Peripheral — Safety Watchdog 레지스터 (Privileged Only)
  - Region 5: Peripheral — Safety Timer 레지스터 (Privileged Only)
  - Region 6: Stack Guard — SafetyFunction 스택 하단 32B (No Access)
  - Region 7: Stack Guard — FreeRTOS 스택 상단 32B (No Access)
- **근거 FSR**: FSR-02-01, FSR-01-02
- **ASIL**: D
- **검증 방법**: 단위 테스트 — `safety_mpu_init()` 호출 후 MPU_CTRL 레지스터 ENABLE 비트 확인; 통합 테스트 — 각 리전 접근 시도 시 예상 동작 확인
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c`, `safety_mpu_init()`

#### SSR-005-MPU: ASIL-D 영역 QM 접근 시 HardFault
- **요구사항**: QM 컨텍스트(Unprivileged 모드)에서 ASIL-D 메모리 영역(Region 0, Region 1)에 읽기 또는 쓰기를 시도할 경우 MPU 예외(MemManage Fault 또는 HardFault)가 발생해야 한다. 해당 예외 핸들러는 반드시 Safe State Level 3 전이 함수를 호출해야 한다.
- **근거 FSR**: FSR-02-01
- **ASIL**: D
- **검증 방법**: 통합 테스트 — QM 태스크에서 ASIL-D 메모리 주소 쓰기 시도 → MemManage Fault 발생 → Safe State Level 3 전이 확인 (Agent-VnV 수행)
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c`; 예외 핸들러: `arch/arm-cortex-m/src/fault_handlers.c`

#### SSR-006-MPU: IVT ASIL-D 영역 배치
- **요구사항**: 인터럽트 벡터 테이블(IVT)은 링커 스크립트에 의해 ASIL-D Code 리전(Region 0) 내에 배치되어야 한다. SCB->VTOR 레지스터 값은 SafetyFunction 초기화 완료 후 ASIL-D Code 리전 기준 주소와 일치해야 한다.
- **근거 FSR**: FSR-02-02
- **ASIL**: D
- **검증 방법**: 빌드 검증 — 링커 맵에서 `.isr_vector` 섹션 위치 확인; 단위 테스트 — SCB->VTOR 값 검증
- **구현 위치**: 링커 스크립트 `arch/arm-cortex-m/ldscripts/cortex-m4.ld`

#### SSR-007-MPU: Safety Timer 레지스터 MPU 보호
- **요구사항**: SafetyFunction 전용 타이머의 레지스터 공간은 MPU Region 5(Peripheral — Safety Timer)로 설정되어야 하며, Unprivileged(QM) 접근 시 MemManage Fault가 발생해야 한다.
- **근거 FSR**: FSR-02-03, FSR-07-01
- **ASIL**: D
- **검증 방법**: 통합 테스트 — QM 태스크에서 Safety Timer 레지스터 주소 접근 시 MemManage Fault 발생 확인
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c` (Region 5 설정)

#### SSR-008-MPU: DMA 채널 접근 제어
- **요구사항**: QM 파티션에 할당된 DMA 채널의 소스/목적지 주소로 ASIL-D 메모리 리전 내 주소가 사용되어서는 안 된다. `safety_dma_validate_transfer()` 함수는 DMA 전송 요청 시 소스/목적지 주소가 ASIL-D 리전에 해당하지 않는지 검사하고, 해당 시 `SAFETY_ERR_DMA_VIOLATION`을 반환해야 한다.
- **근거 FSR**: FSR-02-04
- **ASIL**: D
- **검증 방법**: 단위 테스트 — ASIL-D 주소 범위 경계값(boundary) 포함 각 케이스로 `safety_dma_validate_transfer()` 테스트
- **구현 위치**: `kernel/safety/src/dma_guard.c`, `safety_dma_validate_transfer()`

#### SSR-009-MPU: MPU 무결성 검증 (부팅 시)
- **요구사항**: `safety_mpu_integrity_check()` 함수는 `safety_mpu_init()` 직후 호출되어야 한다. 함수는 MPU_RBAR, MPU_RASR 레지스터의 실제 값을 컴파일 타임에 결정된 기대 값(`mpu_expected_config[]`)과 비교하여, 불일치 발생 시 `SAFETY_ERR_MPU_CORRUPT`를 반환해야 한다. 반환값이 오류인 경우 시스템 부팅은 즉시 중단되어야 한다.
- **근거 FSR**: FSR-08-02
- **ASIL**: D
- **검증 방법**: 단위 테스트 — 의도적 MPU 레지스터 오염 후 `safety_mpu_integrity_check()` 호출 시 `SAFETY_ERR_MPU_CORRUPT` 반환 확인
- **구현 위치**: `arch/arm-cortex-m/src/mpu.c`, `safety_mpu_integrity_check()`

---

### 2.3 Safe State FSM 관련 SSR (FSR-01-03, FSR-03-01, FSR-03-02, FSR-03-03 분해)

#### SSR-010-FSM: SafetyFunction 태스크 최고 우선순위
- **요구사항**: SafetyFunction 주기 태스크(`safety_task`)의 FreeRTOS 우선순위는 시스템 내 모든 태스크 중 최고값(`configMAX_PRIORITIES - 1`)으로 설정되어야 한다. 이 우선순위는 런타임에 `vTaskPrioritySet()`으로 변경될 수 없어야 한다.
- **근거 FSR**: FSR-03-02
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — `xTaskCreate()` 호출 시 우선순위 값 확인; 정적 분석 — `vTaskPrioritySet()` 호출 대상 검사
- **구현 위치**: `kernel/safety/src/safety_task.c`

#### SSR-011-FSM: Safe State Level 1 전이
- **요구사항**: `safety_enter_level1()` 함수는 다음 동작을 원자적으로 수행해야 한다:
  1. 내부 FSM 상태를 `SAFE_STATE_L1`로 전환
  2. 사용자 등록 콜백(`safety_l1_callback`) 호출 (등록된 경우)
  3. 오류 카운터 및 원인 코드를 내부 로그에 기록
  4. Watchdog kick 계속 수행 (시스템 동작 유지)
- **근거 FSR**: FSR-01-03 (내부 오류 시 Level 3), FSR-04-02 (일시적 오류 응답)
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
  3. Watchdog kick **중단** — Watchdog 타임아웃에 의한 MCU 리셋 대기
  4. 타임아웃 대기 중 무한 루프 (`while(1)`)
  - 전체 수행 시간(Step 1~3)은 `WATCHDOG_TIMEOUT_MS × 10%` 이내이어야 하며, Step 4에서 Watchdog 리셋이 발생하기 전까지 다른 코드가 실행되어서는 안 된다.
- **근거 FSR**: FSR-01-03, FSR-03-01, FSR-03-03
- **ASIL**: D
- **검증 방법**: 코드 리뷰 — critical section 범위 확인; 통합 테스트 — Level 3 전이 후 MCU 리셋 발생 확인
- **구현 위치**: `kernel/safety/src/safe_state.c`, `safety_enter_level3()`

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
- **검증 방법**: 코드 리뷰 — 모든 `safety_mailbox_receive()` 호출 지점에서 반환값 확인 코드 존재 여부 검사
- **구현 위치**: `safety_mailbox_receive()` 계약 문서 + 호출 코드 리뷰

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
  3. 연속 비정상 리셋 횟수가 `ABNORMAL_RESET_MAX`(기본값 3)를 초과하는 경우 `safety_enter_level3()` 호출
- **근거 FSR**: FSR-05-04, FSR-08-03
- **ASIL**: C (개발 수준: D)
- **검증 방법**: 단위 테스트 — POR(정상), WDG Reset(Level 2), BOR 연속 3회 이상(Level 3) 케이스 확인
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
- **요구사항**: SafetyFunction 주기 태스크는 매 사이클마다 `safety_clock_monitor_check()`를 호출해야 한다. 반환값이 `SAFETY_ERR_CLOCK_DEVIATION`인 경우 즉시 `safety_enter_level2()`를 호출해야 한다. 연속 `CLOCK_FAULT_MAX`(기본값 3)회 감지 시 `safety_enter_level3()`으로 에스컬레이션해야 한다.
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
| SSR-002-WDG | Watchdog kick 주기 ≤ 50%, 타임아웃 ≥ WCRT×3 | FSR-01-04 | D | watchdog.c |
| SSR-003-WDG | Watchdog 비활성화 함수 금지 | FSR-01-01 | D | watchdog.c |
| SSR-004-MPU | MPU 초기화 (8 리전) 및 순서 | FSR-02-01, FSR-01-02 | D | mpu.c |
| SSR-005-MPU | ASIL-D 영역 QM 접근 시 MemManage Fault → Level 3 | FSR-02-01 | D | mpu.c, fault_handlers.c |
| SSR-006-MPU | IVT ASIL-D 영역 배치 (VTOR 검증) | FSR-02-02 | D | 링커 스크립트 |
| SSR-007-MPU | Safety Timer 레지스터 MPU 보호 | FSR-02-03, FSR-07-01 | D | mpu.c |
| SSR-008-MPU | QM DMA → ASIL-D 영역 전송 차단 | FSR-02-04 | D | dma_guard.c |
| SSR-009-MPU | 부팅 시 MPU 레지스터 무결성 검증 | FSR-08-02 | D | mpu.c |
| SSR-010-FSM | safety_task 최고 우선순위 고정 | FSR-03-02 | D | safety_task.c |
| SSR-011-FSM | Safe State Level 1 전이 함수 | FSR-01-03, FSR-04-02 | D | safe_state.c |
| SSR-012-FSM | Safe State Level 2 전이 함수 (WCET ≤ WDG×10%) | FSR-03-01, FSR-03-03 | D | safe_state.c |
| SSR-013-FSM | Safe State Level 3 전이 함수 (Critical Section) | FSR-01-03, FSR-03-03 | D | safe_state.c |
| SSR-014-MBX | Mailbox 단일 경로 강제 | FSR-06-01 | D | mailbox.c |
| SSR-015-MBX | CRC32 검증 (검증 실패 시 조기 반환) | FSR-06-02, FSR-04-01 | D | mailbox.c, crc32.c |
| SSR-016-MBX | 타임스탬프 유효성 검증 | FSR-06-02 | D | mailbox.c |
| SSR-017-MBX | 범위 검증 (schema 기반) | FSR-06-02 | D | mailbox.c, range_check.c |
| SSR-018-MBX | 검증 실패 데이터 사용 금지 (계약) | FSR-06-03 | D | mailbox.c (계약) |
| SSR-019-MBX | 연속 N=3 실패 시 Level 2 에스컬레이션 | FSR-06-04, FSR-04-02 | D | mailbox.c |
| SSR-020-BST | Flash ROM CRC 검증 | FSR-05-01 | C(개발:D) | boot_selftest.c |
| SSR-021-BST | RAM March C- 테스트 | FSR-05-02 | C(개발:D) | boot_selftest.c |
| SSR-022-BST | 자가진단 실패 시 FreeRTOS 시작 차단 → Level 3 | FSR-05-03 | C(개발:D) | safety_init.c |
| SSR-023-BST | 리셋 원인 기록; WDG/BOR 리셋 시 Level 2~3 | FSR-05-04, FSR-08-03 | C(개발:D) | reset_cause.c |
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
| FSR-01-03 | SafetyFunction 내부 오류 → Level 3 | SSR-011-FSM, SSR-013-FSM |
| FSR-01-04 | Watchdog kick ≤ 50%; 타임아웃 ≥ WCRT×3 | SSR-002-WDG |
| FSR-02-01 | ASIL-D 영역 QM 접근 불가 (MPU) | SSR-004-MPU, SSR-005-MPU |
| FSR-02-02 | IVT ASIL-D 영역 배치 | SSR-006-MPU |
| FSR-02-03 | SafetyFunction 전용 타이머 QM 수정 불가 | SSR-007-MPU |
| FSR-02-04 | QM DMA → ASIL-D 접근 불가 | SSR-008-MPU |
| FSR-03-01 | 위험 감지 → Safe State 전이 ≤ [X]ms | SSR-012-FSM |
| FSR-03-02 | safety_task 최고 우선순위 | SSR-010-FSM |
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
| 정수 연산 | Rule 10.1~10.4 | 묵시적 형변환 금지 |
| 분기 | Rule 15.5 | 단일 종료점 원칙 (함수당 return 1개) |
| 루프 | Rule 14.2 | for 루프 카운터 변수는 루프 내에서만 수정 |
| 함수 | Rule 17.7 | 모든 함수 반환값은 사용되거나 명시적으로 무시 |
| 전처리기 | Rule 20.1~20.14 | 헤더 가드 필수, 재귀 매크로 금지 |

---

## 6. 미결 사항

| 번호 | 미결 항목 | 위임 대상 | 관련 SSR |
|-----|---------|---------|---------|
| 1 | SafetyFunction WCRT 값 (WCET 분석 결과) | TSC / 플랫폼 WCET 분석 | SSR-002-WDG, SSR-012-FSM |
| 2 | [X]ms Safe State 응답 시간 구체화 | 통합자 AoU-03 | SSR-012-FSM |
| 3 | MAILBOX_MAX_AGE_MS 구체적 값 | TSC | SSR-016-MBX |
| 4 | MAILBOX_FAIL_MAX N 파라미터 확정 | TSC | SSR-019-MBX |
| 5 | ABNORMAL_RESET_MAX 파라미터 확정 | TSC | SSR-023-BST |
| 6 | CLOCK_DEVIATION_THRESHOLD_PCT 플랫폼 검증 | TSC / 하드웨어 플랫폼 | SSR-024-CLK |
| 7 | 비휘발성 메모리 구현체 (EEPROM/Flash) | 통합자 HAL | SSR-027-BOD |
| 8 | NVIC BOD 인터럽트 우선순위 값 | TSC / 플랫폼 HAL | SSR-026-BOD |

---

## 7. Agent-QA 확인 검토 요청

본 문서는 **Agent-QA의 확인 검토(Confirmation Review, ISO 26262 Part 2 Cl.8)** 대상이다.

- **검토 기록**: OSR-CR-SSRS-001 (예정)
- **검토 표준**: ISO 26262 Part 6 Cl.7
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

---

*본 문서는 ISO 26262 Part 6 Cl.7에 따라 작성된 소프트웨어 안전 요구사항 명세입니다.*
*SEooC 개발 컨텍스트(ISO 26262 Part 8 Cl.13)에 따라 미결 파라미터는 TSC 또는 통합자 AoU로 위임됩니다.*
