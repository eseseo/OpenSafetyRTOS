# OpenSafetyRTOS — Assumptions of Use (AoU) Registry

**Document ID:** OSR-AOU-001
**Version:** 1.0
**Status:** Released — 정식(canonical) AoU 레지스트리
**Date:** 2026-07-02
**Author:** Agent-Safety
**Standard Reference:** ISO 26262-10:2018 Cl.9 (SEooC), ISO 26262-2:2018 Cl.6.4.9 (Confirmation Measures 대상 산출물)

---

## 1. 목적 및 범위

### 1.1 목적

본 문서는 OpenSafetyRTOS(SEooC)의 **사용 가정(Assumptions of Use, AoU)** 을 단일 정식 레지스트리로 정의한다. OpenSafetyRTOS는 ISO 26262-10:2018 Cl.9에 따라 SEooC로 개발되므로, 최종 아이템 컨텍스트에서 충족되어야 하는 전제조건을 통합자(Integrator)에게 명시적으로 전달해야 한다. 통합자는 각 AoU의 이행 여부를 자신의 안전 케이스에서 입증할 책임을 진다.

### 1.2 정식 레지스트리 선언 (ID 충돌 해소)

v1.0 이전에는 AoU ID가 개별 문서(HARA v1.2 §6.3, FSC v1.2, SAFE_STATE_DEFINITION v1.0 §6)에 분산 정의되어 **동일 ID가 서로 다른 의미로 사용되는 충돌**(AoU-04, AoU-05, AoU-08)이 존재했다. 본 문서 발행 시점부터 **AoU ID의 유일한 권위 출처는 OSR-AOU-001이다.** 타 문서의 AoU 참조는 본 레지스트리의 ID로 갱신되었으며, 과거 문서/검토 기록(safety/doc/reviews/)의 구 ID는 §4의 레거시 매핑 표로 해석한다.

---

## 2. AoU 목록 (Canonical List)

| ID | 제목 | 관련 SG | 관련 FSR |
|------|------|---------|----------|
| AoU-01 | 부팅 순서: safety_boot() 선행 실행 | SG-05, SG-02 | FSR-05-01~04, FSR-02-01 |
| AoU-02 | 외부 독립 하드웨어 Watchdog 연결 | SG-01, SG-03 | FSR-01-01, FSR-01-04 |
| AoU-03 | QM→SafetyFunction 데이터는 Mailbox API 전용 | SG-06 | FSR-06-01~04 |
| AoU-04 | SafetyFunction 전용 독립 하드웨어 타이머 (ISR 우선순위 최상) | SG-03 | FSR-03-01, FSR-03-02 |
| AoU-05 | 안전 타이머용 독립 클럭 소스 + 클럭 감시(CSS) 활성화 | SG-07 | FSR-07-01~03 |
| AoU-06 | MISRA-C:2012 준수 툴체인 (검증된 최적화 레벨) | 전체 | 전체 SSR |
| AoU-07 | 버스 마스터(DMA 등)의 ASIL-D 영역 쓰기 차단 구성 | SG-02 | FSR-02-04 |
| AoU-08 | 전원 감시(BOD/PVD) 구성 | SG-08 | FSR-08-01~03 |
| AoU-09 | Level 2 회복 절차 구현 | SG-04 | — (SAFE_STATE §5.3) |
| AoU-10 | QM 태스크 Unprivileged 실행 (FreeRTOS ARM_CM4_MPU 포트) | SG-02 | FSR-02-01~03 |
| AoU-11 | Safe State 콜백 등록 | SG-01, SG-03 | — (SAFE_STATE §4, AoU-07 구번호에서 재부여) |

> **참고:** AoU-11은 D4에서 지시된 정식 목록(AoU-01~10) 외 항목이다. 구 SAFE_STATE_DEFINITION v1.0의 AoU-07(콜백 등록 의무)이 신규 AoU-07(버스 마스터)과 충돌하여 재부여가 필요했고, 정식 목록에 해당 의무의 수용처가 없어 AoU-11로 신설하였다. 통합자 FTTI 명시 의무(구 HARA AoU-03)는 별도 항목 대신 AoU-04의 검증 지침에 통합하였다.

---

## 3. AoU 상세 정의

### AoU-01: 부팅 순서 — safety_boot() 선행 실행

- **요구 내용:** `safety_boot()`(MPU 초기화 + 부팅 자가진단 POST)는 어떠한 QM/FreeRTOS 코드보다 먼저 실행되어야 한다. POST 완료 및 MPU 활성화 이전에 QM 코드 실행 또는 차량 기능 활성화는 금지된다.
- **근거(Rationale):** FFI(SG-02)와 부팅 무결성(SG-05)은 SafetyFunction 초기화가 선행될 때만 성립한다. QM 코드가 먼저 실행되면 보호되지 않은 구간이 발생한다.
- **관련 SG/FSR:** SG-05, SG-02 / FSR-05-01~04, FSR-02-01
- **통합자 검증 지침:** 스타트업 코드(리셋 핸들러~main)를 검토하여 `safety_boot()` 호출이 `vTaskStartScheduler()` 및 모든 QM 초기화보다 선행함을 코드 리뷰 + 부팅 트레이스(로직 애널라이저 또는 ITM)로 확인한다.

### AoU-02: 외부 독립 하드웨어 Watchdog

- **요구 내용:** SafetyFunction 사양(윈도우/타임아웃 특성)에 부합하는 외부 또는 온칩 독립 하드웨어 Watchdog(예: IWDG — 독립 클럭 구동)이 SafetyFunction에 연결되어야 한다. QM 파티션은 해당 Watchdog을 제어할 수 없어야 한다.
- **근거:** SG-01(단일 고장 허용)의 최종 복구 수단 및 Level 3 백스톱은 SafetyFunction 자체 고장과 독립적인 하드웨어 감시를 전제로 한다.
- **관련 SG/FSR:** SG-01, SG-03 / FSR-01-01, FSR-01-04
- **통합자 검증 지침:** Watchdog 클럭 소스가 메인 PLL과 독립임을 데이터시트로 확인; 타임아웃 설정이 `WCRT×3 ≤ WATCHDOG_TIMEOUT_MS ≤ FTTI 예산`을 만족함을 파라미터 검토로 확인; kick 중단 시 실제 리셋 발생을 HW 테스트로 확인.

### AoU-03: Mailbox API 전용 통신

- **요구 내용:** QM 파티션에서 SafetyFunction으로의 모든 데이터 전달은 Mailbox API(`safety_mailbox_*`)를 유일한 경로로 사용해야 한다. 공유 변수, 직접 메모리 접근 등 우회 경로 구현은 금지된다.
- **근거:** SG-06(QM 데이터 검증 없이 신뢰 금지)의 3단계 검증(CRC/타임스탬프/범위)은 단일 통신 경로가 강제될 때만 유효하다.
- **관련 SG/FSR:** SG-06 / FSR-06-01~04
- **통합자 검증 지침:** 통합자 애플리케이션 코드에 대한 정적 분석으로 ASIL-D 심볼 직접 참조가 없음을 확인; 링커 맵에서 QM 오브젝트가 ASIL-D 데이터 섹션을 참조하지 않음을 확인.

### AoU-04: SafetyFunction 전용 독립 하드웨어 타이머

- **요구 내용:** SafetyFunction 주기 실행을 구동하는 전용 독립 하드웨어 타이머를 제공해야 하며, 해당 타이머 ISR의 NVIC 우선순위는 모든 QM 인터럽트보다 높아야 한다(또한 `configMAX_SYSCALL_INTERRUPT_PRIORITY`보다 높아 FreeRTOS 커널 critical section에 의해 마스킹되지 않아야 한다).
- **근거:** D1 아키텍처 결정(ADR-005 참조)에 따라 SafetyFunction은 FreeRTOS 태스크가 아니라 전용 타이머 ISR 컨텍스트에서 실행된다. 이는 QM 스케줄러 고장으로부터의 시간적 독립성(SG-03)의 전제이다.
- **관련 SG/FSR:** SG-03 / FSR-03-01, FSR-03-02
- **통합자 검증 지침:** ① NVIC 우선순위 설정 검토(안전 타이머 ISR < 수치적으로 모든 QM ISR, < `configMAX_SYSCALL_INTERRUPT_PRIORITY`). ② **FTTI 명시 의무(구 AoU-03):** 통합자는 자신의 아이템 FTTI를 문서화하고, SafetyFunction WCRT 기준값 10ms + 안전 주기(감시 주기) + Watchdog 타임아웃이 FTTI 예산 이내임을 정량적으로 입증해야 한다. FTTI가 10ms 기준값보다 엄격한 경우 WCRT 재분석/재검증이 필요하다.

### AoU-05: 독립 클럭 소스 및 클럭 감시

- **요구 내용:** 안전 타이머용 클럭 소스는 QM 파티션이 사용하는 메인 클럭 트리로부터 독립적이어야 하며(예: 독립 RC 오실레이터, LSI), MCU의 클럭 감시 기능(예: CSS — Clock Security System)이 활성화되어야 한다.
- **근거:** SG-07(클럭 보호): 클럭 이상(H-09) 시 교차 검증이 가능하려면 독립 기준 클럭이 필요하고, 하드웨어 클럭 감시가 1차 방어선이 된다.
- **관련 SG/FSR:** SG-07 / FSR-07-01~03
- **통합자 검증 지침:** 클럭 트리 다이어그램 검토로 독립성 확인; CSS(또는 등가 기능) 활성화 레지스터 설정 확인; 클럭 소스 결함 주입 테스트(외부 크리스털 제거/단락)로 감지 동작 확인.

### AoU-06: MISRA-C:2012 준수 툴체인

- **요구 내용:** SafetyFunction 컴파일에는 MISRA-C:2012 준수를 지원하고 도구 검증(TOOL_QUALIFICATION_PLAN.md)이 완료된 툴체인을, 검증된 최적화 레벨(qualified optimization level)로 사용해야 한다. 검증되지 않은 최적화 레벨/플래그 변경은 금지된다.
- **근거:** ASIL-D 코드의 소스 수준 검증(정적 분석, MC/DC)은 컴파일러가 검증된 설정으로 동작할 때만 오브젝트 코드에 대해 유효하다 (ISO 26262-8 Cl.11 도구 신뢰 수준).
- **관련 SG/FSR:** 전체 SG / 전체 SSR (구현 공통 전제)
- **통합자 검증 지침:** 빌드 로그에서 컴파일러 버전/플래그가 도구 검증 기록과 일치함을 확인; CI에서 플래그 변경 감지 게이트 구성.

### AoU-07: 버스 마스터(DMA 등) 구성 제한

- **요구 내용:** **MPU는 DMA 등 버스 마스터의 접근을 통제하지 않는다.** 따라서 통합자는 모든 버스 마스터(DMA, 이더넷 MAC, USB, 외부 마스터 등)가 ASIL-D 메모리 리전에 쓰기할 수 없도록 시스템을 구성해야 한다. QM의 DMA 사용은 SafetyFunction이 소유한 DMA 설정 경로(`safety_dma_validate_transfer()` 검증 API)를 통해서만 허용된다.
- **근거:** ARMv7-M MPU는 CPU 접근만 필터링한다(D1(f)). DMA를 통한 우회 쓰기는 FFI(SG-02)를 무력화하므로 소프트웨어 검증 + 통합자 구성으로 차단해야 한다.
- **관련 SG/FSR:** SG-02 / FSR-02-04
- **통합자 검증 지침:** 시스템 내 모든 버스 마스터 목록화(아키텍처 검토); 각 마스터의 주소 생성 경로가 SafetyFunction 검증을 경유하거나 하드웨어적으로 ASIL-D 영역에 도달 불가함을 분석; DMA로 ASIL-D 주소 쓰기 시도 결함 주입 테스트 수행.

### AoU-08: 전원 감시(BOD/PVD) 구성

- **요구 내용:** 하드웨어 Brown-out Detector(BOD) 또는 PVD(Programmable Voltage Detector)를 SAFE_STATE_DEFINITION.md에 정의된 임계값 정책에 따라 구성해야 하며, 해당 인터럽트는 SafetyFunction 전용으로 할당되어야 한다. BOD 트리거 전압 및 감지 지연은 통합자 전원 시스템 설계에 맞게 명시되어야 한다.
- **근거:** SG-08: 전원 이상(H-10)으로 인한 MPU 설정 소실은 소프트웨어 단독으로 감지할 수 없으며 하드웨어 전원 감시가 전제된다.
- **관련 SG/FSR:** SG-08 / FSR-08-01~03
- **통합자 검증 지침:** BOD/PVD 임계값이 MPU 레지스터 유지 최소 전압보다 높게 설정되었음을 데이터시트 기반으로 확인; 전원 강하 주입 테스트로 인터럽트 발생 및 Level 3 전이 확인.

### AoU-09: Level 2 회복 절차 구현

- **요구 내용:** 통합자는 Level 2(Controlled Stop)에서 정상 동작으로의 회복 절차를 구현해야 한다. 자동 회복(타이머 기반 재시작 등)은 금지되며, `safety_request_recovery()` 확인 절차를 준수해야 한다. 상세 요구사항은 SAFE_STATE_DEFINITION.md §5.3, §6(AoU-09)을 따른다.
- **근거:** 결함 원인 미해소 상태의 자동 재개는 위험 동작 재개로 이어질 수 있어 ASIL-D 요구에 위배된다.
- **관련 SG/FSR:** SG-04 (가용성/오탐 정책과 연계) / SAFE_STATE §5.3
- **통합자 검증 지침:** 회복 경로 코드 리뷰(자동 회복 부재 확인); 회복 절차 통합 테스트(결함 해소 전 회복 요청 거부 확인); 회복 이력 로깅 확인.

### AoU-10: QM 태스크 Unprivileged 실행

- **요구 내용:** QM 애플리케이션 태스크는 FreeRTOS **ARM_CM4_MPU 포트**를 사용하여 Unprivileged 모드로 실행되어야 한다. 특권(Privileged) 모드로 실행되는 QM 코드는 FreeRTOS 커널 자체(스케줄러/컨텍스트 스위치)로 제한된다. 대상 하드웨어는 MPU를 탑재한 ARM Cortex-M4/M7이어야 한다 (구 AoU-01의 MPU 하드웨어 요구 포함).
- **근거:** ARMv7-M에는 MPU 잠금이 없어 특권 코드는 MPU 레지스터를 재기록할 수 있다(D1(a)/(b), ADR-005). MPU의 예방적 보호는 Unprivileged QM 태스크에 대해서만 성립하며, 특권 커널 결함에 대해서는 검출적 통제(주기적 MPU/VTOR 무결성 검증, FSR-02-01/02)로 보완된다. QM 태스크가 특권으로 실행되면 이 방어 체계 전체가 무효화된다.
- **관련 SG/FSR:** SG-02 / FSR-02-01, FSR-02-02, FSR-02-03
- **통합자 검증 지침:** 모든 `xTaskCreate`/`xTaskCreateRestricted` 호출 검토 — QM 태스크가 privileged 플래그 없이 생성됨을 확인; QM 태스크에서 CONTROL 레지스터 읽기로 Unprivileged 상태 런타임 확인 테스트; 통합자 추가 특권 코드(ISR 포함)에 대한 FFI 영향 분석 수행.

### AoU-11: Safe State 콜백 등록

- **요구 내용:** 통합자는 OpenSafetyRTOS 초기화 시 Safe State 콜백(`on_degraded`, `on_controlled_stop`, `on_reset_request`, `output_freeze`)을 반드시 등록해야 한다. 미등록 시 OpenSafetyRTOS는 Level 2 진입 직후 Level 3으로 에스컬레이션한다. 상세는 SAFE_STATE_DEFINITION.md §6(AoU-11)을 따른다.
- **근거:** 차량 레벨 Safe State 액션(출력 고정, 액추에이터 안전값)은 SEooC인 RTOS가 알 수 없으므로 통합자 콜백으로만 실현 가능하다.
- **관련 SG/FSR:** SG-01, SG-03 / SAFE_STATE §4.1~4.2
- **통합자 검증 지침:** 초기화 코드에서 `safety_register_callbacks()` 호출 확인; 콜백 미등록 상태에서 Level 2 트리거 시 Level 3 에스컬레이션 동작 통합 테스트로 확인; 각 콜백의 WCET가 Safe State 전이 시간 예산 이내임을 분석.

---

## 4. 레거시 ID 매핑 (v1.0 이전 문서/검토 기록 해석용)

| 구 참조 (출처 문서) | 구 의미 | 정식 ID (OSR-AOU-001) |
|---------------------|---------|----------------------|
| AoU-01 (HARA v1.2 §6.3) | MPU 지원 하드웨어 요구 | AoU-10 (요구 내용에 포함) |
| AoU-02 (HARA v1.2 §6.3) | HW Watchdog 존재 요구 | AoU-02 (유지) |
| AoU-03 (HARA v1.2 §6.3, FSC v1.2 FSR-03-01) | FTTI 명시 요구 | AoU-04 (검증 지침 ②에 통합) |
| AoU-04 (HARA v1.2 §6.3) | 부팅 순서 제약 | AoU-01 |
| "AoU-04 참조" (HARA v1.2 §6.3, FSC v1.2 FSR-07-01) | 클럭 소스 독립성 | AoU-05 |
| AoU-04 (SAFE_STATE v1.0 §4.2) | SafetyFunction DMA 제어권 | AoU-07 |
| AoU-05 (HARA v1.2 §6.3) | QM 파티션 격리 책임 | AoU-10 |
| AoU-05 (FSC v1.2 §6 항목 2) | HW Watchdog 타이머 스펙 | AoU-02 |
| AoU-07 (SAFE_STATE v1.0 §6) | Safe State 콜백 등록 | AoU-11 |
| AoU-08 (HARA/FSC v1.2) | 전원 보호 설계 (BOD) | AoU-08 (유지) |
| AoU-08 (SAFE_STATE v1.0 §6) | Level 2 회복 절차 | AoU-09 |

---

## 5. Agent-QA 확인 검토 요청

본 문서는 **Agent-QA의 확인 검토(Confirmation Review, ISO 26262-2:2018 Cl.6.4.9)** 대상이다.

- **검토 기록:** OSR-CR-AOU-001 (예정)
- **검토 중점 사항:** 모든 SG/FSR의 통합 의존 전제가 AoU로 커버되는지; 레거시 매핑의 무결성; 각 AoU 검증 지침의 실행 가능성.

---

## 문서 이력

| 버전 | 날짜 | 작성자 | 변경 내용 |
|------|------|--------|----------|
| 1.0 | 2026-07-02 | Agent-Safety | 최초 발행 — 정식 AoU 레지스트리 신설. HARA v1.2/FSC v1.2/SAFE_STATE v1.0에 분산된 AoU ID 충돌(AoU-04/05/08) 해소, AoU-01~11 정의, 레거시 매핑 표 수록. D1 아키텍처 결정(ADR-005) 반영: AoU-04(전용 타이머 ISR), AoU-07(버스 마스터), AoU-10(Unprivileged QM) 신설. |

---

*본 레지스트리는 OpenSafetyRTOS의 모든 안전 문서에서 AoU ID의 유일한 권위 출처이다. AoU 추가/변경은 변경 영향 분석 및 Agent-QA 확인 검토를 수반해야 한다.*
