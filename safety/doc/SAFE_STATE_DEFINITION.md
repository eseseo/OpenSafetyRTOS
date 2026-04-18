# Safe State Definition — OpenSafetyRTOS

**Document ID:** OSR-SS-001
**Version:** 1.0
**Status:** Approved (Draft for Phase 1 Review)
**Date:** 2026-04-18
**Author:** Agent-Safety
**Standard Reference:** ISO 26262 Part 8 Cl.13 (SEooC), Part 4 Cl.8 (Safe State)

---

## 1. Safe State 개념

### 1.1 정의

ISO 26262에서 Safe State란 **합리적으로 예측 가능한 손해 위험이 없는 동작 모드**를 의미한다 (ISO 26262-1:2018, 용어 1.132). OpenSafetyRTOS는 SEooC(Safety Element out of Context)로 배포되므로, 정확한 차량 기능(브레이크, 스티어링 등)을 사전에 알 수 없다. 따라서 본 문서는 **RTOS 컴포넌트 수준에서 달성 가능한 Safe State**를 정의하고, 구체적인 액추에이터 수준 Safe State(예: 전원 차단, 제동 인가)는 시스템 통합자(Integrator)의 책임임을 명시한다.

### 1.2 OpenSafetyRTOS Safe State의 범위

OpenSafetyRTOS가 제공하는 것:
- **결함 감지(Fault Detection):** CRC 오류, MPU 위반, 타임스탬프 만료, 범위 이탈 등을 감지
- **결함 격리(Fault Isolation):** QM 파티션의 실행을 정지시켜 ASIL-D 파티션 보호
- **결함 통보(Fault Notification):** 시스템 통합자가 등록한 콜백(AoU-07)을 통해 Safe State 진입 사실 및 원인 전달
- **알려진 상태 유지(Known State Hold):** MCU를 예측 가능한 정지 상태로 유지

OpenSafetyRTOS가 제공하지 않는 것 (시스템 통합자 책임):
- 차량 레벨 Safe State 액추에이터 제어 (예: 전원 릴레이 차단, 유압 브레이크 인가)
- 운전자 경고 HMI 동작
- 차량 통신 버스(CAN/Ethernet) Safe State 메시지 전송

### 1.3 SEooC 맥락에서의 Safe State

본 컴포넌트는 ISO 26262 Part 8 Cl.13에 따라 SEooC로 개발된다. Safe State의 완전한 구현은 통합 프로젝트(Integrating Project)에서 수행되어야 하며, OpenSafetyRTOS는 그 기반이 되는 **감지-격리-통보 메커니즘**을 ASIL-D 수준으로 제공한다.

**예시 (참고용, 통합자 구현 사례):**
| 차량 기능        | RTOS Safe State 진입 후 통합자 액션 예시          |
|----------------|--------------------------------------------------|
| 전동 파워스티어링 | 모터 전원 차단, 수동 스티어링 복원 안내           |
| ADAS 종방향 제어  | 엔진 토크 0 지령, 운전자 인계 요청 HMI 표시       |
| 배터리 관리 시스템 | 고전압 릴레이 차단, 저전압 유지 모드 전환         |

---

## 2. Safe State 유형 (계층적 정의)

Safe State는 결함의 심각도와 영향 범위에 따라 3개 레벨로 계층화된다.

```
결함 발생
    │
    ▼
┌─────────────────────────────────────────────────────────────────┐
│  Level 1: Degraded Mode (저하 운전 모드)                         │
│  SafetyFunction 감지 → QM 통보 → 감시 강화, 서비스 계속          │
└───────────────────────────────┬─────────────────────────────────┘
                                │ 회복 불가 또는 중증 결함
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│  Level 2: Controlled Stop (제어된 정지)                          │
│  QM 태스크 강제 종료 → 시스템 알려진 정지 상태 유지              │
└───────────────────────────────┬─────────────────────────────────┘
                                │ 치명적 결함 또는 Level 2 진입 실패
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│  Level 3: Emergency Reset (비상 리셋)                            │
│  하드웨어 Watchdog 만료 → MCU 전체 리셋                          │
└─────────────────────────────────────────────────────────────────┘
```

### 2.1 Level 1 — Degraded Mode (저하 운전 모드)

**정의:** SafetyFunction이 경미한 결함을 감지했으나 시스템이 제한된 기능으로 계속 동작할 수 있는 상태.

**특징:**
- SafetyFunction이 정상 동작 계속
- QM 파티션에 결함 발생 사실 통보 (Mailbox를 통한 역방향 알림 또는 공유 플래그)
- 감시 주기 단축, 로깅 강화
- 통합자 콜백 호출: `safety_degraded_cb(fault_code, fault_context)`

**지속 가능 조건:**
- ASIL-D 파티션 무결성 유지
- Watchdog kick 정상 수행
- 결함이 격리 가능한 범위 내

### 2.2 Level 2 — Controlled Stop (제어된 정지)

**정의:** SafetyFunction이 중대한 결함을 감지하고 QM 파티션의 모든 태스크를 강제 정지시킨 후 MCU를 알려진 정지 상태로 유지하는 상태.

**특징:**
- FreeRTOS 스케줄러 정지 (portDISABLE_INTERRUPTS 또는 SafetyFunction 전용 API)
- ASIL-D 메모리 영역 MPU 보호 유지
- SafetyFunction 자체는 단순 감시 루프만 실행
- Watchdog kick 계속 수행 (SafetyFunction이 살아있음을 증명)
- 통합자 콜백 호출: `safety_controlled_stop_cb(fault_code, fault_context)`

**종료 조건:**
- 통합자의 명시적 재시작 명령 (AoU-08 참조)
- 또는 하드웨어 Reset 신호

### 2.3 Level 3 — Emergency Reset (비상 리셋)

**정의:** SafetyFunction 자체가 동작 불능 상태이거나 Level 2 진입이 불가능하여 독립 하드웨어 Watchdog이 MCU 전체를 리셋하는 상태.

**특징:**
- 소프트웨어적 제어 없음 — 순수 하드웨어 동작
- MCU 전체 리셋: 모든 레지스터, RAM, 주변장치 초기화
- 리셋 후 부트 시퀀스 재실행 (ADR-004 참조)
- 리셋 원인은 RCC Reset Status Register에 기록 (또는 배터리 백업 SRAM)

---

## 3. Safe State 전이 조건

### 3.1 전이 조건 매트릭스

| 결함 유형                        | 전이 레벨       | 근거                                              |
|---------------------------------|----------------|--------------------------------------------------|
| Mailbox CRC 검증 실패 (1회)       | Level 1        | 일시적 데이터 오염, 기능 계속 가능                 |
| Mailbox CRC 검증 실패 (연속 N회)  | Level 2        | 지속적 데이터 무결성 위반 → 신뢰 불가              |
| Mailbox 타임스탬프 만료 (Stale)   | Level 1        | QM 태스크 지연, 감시 강화 필요                    |
| Mailbox 값 범위 이탈              | Level 1 → 2    | 범위 이탈 지속 시 Level 2로 에스컬레이션           |
| MPU HardFault (QM→ASIL-D 침범)   | Level 2        | FFI 위반 — 즉각 QM 격리 필요                      |
| MPU HardFault (반복/패턴)         | Level 3        | 공격적 침범 또는 심각한 소프트웨어 결함            |
| QM 태스크 Deadline Miss           | Level 1        | 성능 저하, 통합자 통보                             |
| QM 태스크 Deadline Miss (반복)    | Level 2        | 시스템 시간 보장 불가                              |
| SafetyFunction Watchdog 미kick    | Level 3        | SafetyFunction 자체 결함 → 하드웨어 리셋           |
| 전원 이상 감지 (전압 범위 이탈)    | Level 3        | MPU 설정 소실 위험 → 즉각 리셋                    |
| 부트 자가진단 실패 (ROM CRC)      | Level 3        | 코드 무결성 보장 불가 → 실행 금지                  |
| 부트 자가진단 실패 (RAM March)     | Level 3        | 메모리 신뢰성 보장 불가 → 실행 금지                |
| SafetyFunction 내부 스택 오버플로우| Level 3        | ASIL-D 파티션 자체 결함 → 하드웨어 리셋            |

### 3.2 레벨 에스컬레이션 규칙

```
에스컬레이션 조건:
  Level 1 → Level 2: 동일 결함이 FAULT_ESCALATION_COUNT (기본값: 3) 회 반복
                     또는 Level 1 진입 후 FAULT_RECOVERY_TIMEOUT_MS 내 회복 없음
  Level 2 → Level 3: Level 2 진입 후 Watchdog kick 중단 (SafetyFunction 동작 불능)
                     또는 Level 2 진입 시퀀스 자체 실패
```

---

## 4. Safe State 진입 시퀀스

### 4.1 Level 1 진입 시퀀스

```
SafetyFunction이 결함 감지
    │
    ├─ 1. 결함 로깅: safety_fault_log(fault_code, timestamp, context)
    │
    ├─ 2. 감시 강화: 감시 주기를 MONITOR_INTERVAL_NORMAL → MONITOR_INTERVAL_DEGRADED 로 단축
    │
    ├─ 3. QM 통보: safety_notify_qm(SAFETY_STATE_DEGRADED, fault_code)
    │       └─ QM Mailbox 역방향 채널 또는 MPU 허용 공유 플래그 사용
    │
    ├─ 4. 통합자 콜백: safety_degraded_cb(fault_code, fault_context) 호출
    │       └─ 콜백은 AoU-07에 따라 통합자가 반드시 등록해야 함
    │
    ├─ 5. Watchdog kick 계속 수행
    │
    └─ 6. 결함 카운터 증가 → 에스컬레이션 임계값 확인
```

### 4.2 Level 2 진입 시퀀스

```
SafetyFunction이 중대 결함 감지 또는 Level 1 에스컬레이션
    │
    ├─ 1. 결함 로깅 (Level 2): safety_fault_log(LEVEL2, fault_code, timestamp, context)
    │
    ├─ 2. QM 스케줄러 정지:
    │       ├─ portDISABLE_INTERRUPTS() — QM 인터럽트 마스킹
    │       └─ vTaskSuspendAll() 또는 SafetyFunction 전용 스케줄러 정지 API
    │
    ├─ 3. QM DMA 채널 정지:
    │       └─ DMA 채널 Enable 비트 클리어 (AoU-04: SafetyFunction이 DMA 제어권 보유)
    │
    ├─ 4. MPU 보호 재확인:
    │       └─ MPU 설정 레지스터 재검증 — 예상값과 불일치 시 Level 3으로 에스컬레이션
    │
    ├─ 5. 출력 상태 고정:
    │       └─ 통합자가 등록한 output_freeze_cb() 호출 — 출력 핀/버스 Safe 값으로 고정
    │
    ├─ 6. 통합자 콜백: safety_controlled_stop_cb(fault_code, fault_context) 호출
    │
    ├─ 7. Watchdog kick 계속 수행 (SafetyFunction 감시 루프만 실행)
    │
    └─ 8. 정지 상태 유지 루프:
            while (true) {
                watchdog_kick();
                safety_monitor_power();     /* 전원 이상 감지 → Level 3 */
                safety_check_mpu_regs();   /* MPU 위변조 감지 → Level 3 */
                delay_ms(HOLD_LOOP_MS);
            }
```

### 4.3 Level 3 진입 시퀀스

```
[경로 A: 소프트웨어 감지 후 의도적 Level 3]
    │
    ├─ 1. 결함 로깅 (배터리 백업 SRAM 또는 리셋 전 Flash Write)
    ├─ 2. Watchdog kick 중단 → 하드웨어 Watchdog 만료 대기
    └─ 3. MCU 전체 리셋 (IWDG/WWDG 타임아웃 후 자동)

[경로 B: SafetyFunction 이상 — 하드웨어 자동]
    │
    └─ SafetyFunction이 Watchdog kick 미수행 → IWDG 타임아웃 → MCU 리셋
       (소프트웨어 개입 없음 — 하드웨어 보장)

[리셋 후]
    └─ 부트 시퀀스 재실행 (ADR-004 참조)
       ├─ 리셋 원인 확인 (RCC_CSR 레지스터)
       ├─ 부트 자가진단 수행
       └─ 통합자 부트 콜백에서 리셋 원인 보고
```

---

## 5. Safe State 유지 조건

### 5.1 유지 기간

| 레벨    | 최대 유지 기간                                    | 유지 종료 조건                               |
|--------|--------------------------------------------------|---------------------------------------------|
| Level 1 | 무제한 (결함 해소 또는 에스컬레이션 전까지)        | 결함 해소 확인 + 통합자 승인 OR 에스컬레이션   |
| Level 2 | 무제한 (외부 재시작 또는 Watchdog 타임아웃 전까지) | 통합자의 명시적 재시작 명령 (AoU-08)          |
| Level 3 | N/A (MCU 리셋 후 자동 재부팅)                     | 부팅 완료 — 새 동작 사이클 시작               |

### 5.2 Level 2 유지 중 출력 상태

- **디지털 출력:** 통합자가 `output_freeze_cb()`에서 정의한 안전 값 유지
- **PWM/아날로그 출력:** 타이머 정지 또는 0% 듀티 사이클 (통합자 설정)
- **통신 버스:** CAN/SPI/I2C 전송 중단, 수신 무시
- **인터럽트:** SafetyFunction 전용 인터럽트만 활성 (BASEPRI로 QM ISR 마스킹)

### 5.3 Level 2 Safe State에서의 회복

Level 2에서의 회복은 **시스템 통합자의 명시적 승인 없이는 불가능**하다. 이는 의도치 않은 자동 복구가 더 위험한 상태를 초래할 수 있기 때문이다.

회복 절차 (통합자 구현 예시):
1. 통합자가 결함 원인 분석 완료
2. `safety_request_recovery(SAFETY_RECOVERY_KEY)` 호출
3. SafetyFunction이 회복 조건 재확인 (MPU 무결성, 결함 해소 여부)
4. 조건 충족 시 FreeRTOS 스케줄러 재시작
5. 결함 카운터 클리어 및 Level 1 감시 강화 모드로 전환

---

## 6. AoU (Assumption of Use) 연결

Safe State의 완전한 구현을 위해 시스템 통합자가 반드시 이행해야 하는 전제조건(AoU)을 정의한다.

### AoU-07: Safe State 콜백 등록 (신규)

**ID:** AoU-07
**제목:** Safe State 진입 콜백 함수 등록 의무
**내용:**
시스템 통합자는 OpenSafetyRTOS 초기화 시 다음 콜백 함수를 반드시 등록해야 한다:

```c
/* 통합자가 구현하고 등록해야 하는 콜백 */
safety_callbacks_t integrator_callbacks = {
    .on_degraded       = my_system_on_degraded,       /* Level 1 진입 시 호출 */
    .on_controlled_stop = my_system_on_controlled_stop, /* Level 2 진입 시 호출 */
    .on_reset_request  = my_system_on_reset_request,  /* Level 3 직전 (가능한 경우) */
    .output_freeze     = my_system_output_freeze,     /* 출력 핀 Safe 값 고정 */
};
safety_register_callbacks(&integrator_callbacks);
```

콜백이 등록되지 않은 경우, OpenSafetyRTOS는 Level 2 진입 직후 Level 3(리셋)으로 에스컬레이션한다.

**근거:** ISO 26262 Part 8 Cl.13.3 — SEooC 통합자는 전제조건(Assumption of Use)을 이행할 책임이 있다.

### AoU-08: Safe State 회복 절차 구현 의무 (신규)

**ID:** AoU-08
**제목:** Controlled Stop (Level 2) 회복 절차 구현 의무
**내용:**
시스템 통합자는 Level 2 Safe State에서 정상 동작으로 회복하는 절차를 반드시 구현해야 한다. 자동 회복(타이머 기반 재시작 등)은 금지된다.

요구사항:
- 회복 전 결함 원인 분석 완료 확인
- `safety_request_recovery()` 호출을 통한 SafetyFunction 확인 절차 준수
- 회복 후 시스템 상태 기록(로그) 의무
- 동일 결함으로 인한 반복 회복 횟수를 제한(통합자 정의)

**근거:** 자동 회복은 결함 원인이 해소되지 않은 상태에서 위험 동작이 재개될 수 있어 ASIL-D 요구사항에 위배된다.

---

## 7. 책임 경계 요약

| 기능                            | OpenSafetyRTOS (제공) | 시스템 통합자 (구현 책임) |
|--------------------------------|----------------------|------------------------|
| CRC/타임스탬프/범위 결함 감지    | O                    |                        |
| MPU 위반 감지 및 격리            | O                    |                        |
| Watchdog 관리                   | O                    |                        |
| Safe State 레벨 결정 및 전이    | O                    |                        |
| QM 스케줄러 정지                 | O                    |                        |
| 통합자 콜백 호출 메커니즘         | O                    |                        |
| 차량 레벨 Safe State 액션        |                      | O (AoU-07)            |
| 출력 핀/버스 Safe 값 정의         |                      | O (AoU-07)            |
| Level 2 회복 절차                |                      | O (AoU-08)            |
| 리셋 원인 기록 및 보고            |                      | O                      |
| HARA 및 차량 레벨 안전 목표 정의  |                      | O                      |

---

## 8. 참고 문서

| 문서 ID       | 제목                              | 연결                          |
|--------------|----------------------------------|-------------------------------|
| ADR-001      | Decomposition 전략               | ASIL Decomposition 근거        |
| ADR-002      | MPU 파티션 전략                  | Level 2 격리 메커니즘          |
| ADR-003      | SEooC 배포 전략                  | AoU 프레임워크 전체            |
| ADR-004      | 부트 시퀀스                      | Level 3 리셋 후 재부팅 절차    |
| ARCHITECTURE | 시스템 아키텍처                  | FFI 분석, 메모리 파티션        |
| SAFETY_PLAN  | 안전 계획                        | 전체 안전 프로세스 맥락        |

---

*본 문서는 ISO 26262 Part 8 Cl.13 SEooC 요구사항에 따라 작성되었으며, 통합 프로젝트의 HARA 수행 시 Safe State 근거 문서로 활용된다.*
