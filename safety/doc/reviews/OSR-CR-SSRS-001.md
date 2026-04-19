# 확인 검토 기록: OSR-CR-SSRS-001

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CR-SSRS-001 |
| 검토 대상 | OSR-SSRS-001 v1.0 (Software Safety Requirements Specification) |
| 검토 유형 | 확인 검토 (Confirmation Review) — ISO 26262 Part 2 Cl.8 |
| 검토자 | Agent-QA (Agent-Safety와 독립된 에이전트 — Part 2 독립성 요건) |
| 검토일 | 2026-04-19 |
| 검토 표준 | ISO 26262 Part 6 Cl.7 (Software Safety Requirements) |
| 입력 문서 | OSR-SSRS-001 v1.0, OSR-FSC-001 v1.2 (최종 승인), ADR-002 (MPU 파티션 전략) |

---

## 1. 검토 범위

OSR-SSRS-001 v1.0 전체 27개 SSR에 대해 다음을 확인한다:

1. FSR → SSR 추적성 완전성
2. 각 SSR의 구현 가능성 (특정 함수/모듈로 구현 가능한 형태)
3. 각 SSR의 검증 가능성 (단위/통합 테스트로 Pass/Fail 판정 가능)
4. SSR ASIL ≥ 근거 FSR ASIL
5. ADR-002(MPU 파티션 전략) 및 ARCHITECTURE.md와의 정합성
6. MISRA-C:2012 적용 규칙의 ASIL-D 적절성

---

## 2. SSR 모듈별 검토 결과

### 2.1 Watchdog (SSR-001~003-WDG)

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-001-WDG | ✅ `safety_watchdog_init()` 명확히 정의 | ✅ 타임아웃 미kick 시 리셋 확인 가능 | D | ✅ |
| SSR-002-WDG | ✅ 상수 조건 명확 (`≤50%`, `≥WCRT×3`) | ✅ 코드 리뷰 + 단위 테스트 명시 | D | ✅ |
| SSR-003-WDG | ✅ 금지 함수 부재 검사 가능 | ✅ 정적 분석 명시 | D | ✅ |

WDG 모듈: 이슈 없음.

### 2.2 MPU (SSR-004~009-MPU)

#### SSR-004-MPU: MPU 초기화 — **⚠️ ISSUE-001 (Major)**

SSR-004에서 정의한 MPU 리전 번호 체계가 ADR-002(MPU Partition Strategy, 승인된 문서)와 불일치한다.

**ADR-002 MPU 리전 정의 (기준):**

| Region | Name | 용도 |
|--------|------|------|
| 0 | Default background (deny-all) | 미커버 주소 접근 시 기본 차단 |
| 1 | ASIL-D Code (Flash) | SafetyFunction 실행 코드 |
| 2 | ASIL-D Data (SRAM) | SafetyFunction 변수/BSS |
| 3 | ASIL-D Stack | SafetyFunction 스택 |
| 4 | QM Region | FreeRTOS 코드 + 데이터 |
| 5 | QM→Safety Mailbox | 파티션 간 통신 |
| 6 | Safety Stack Guard | 스택 오버플로우 감지 |
| 7 | Peripheral/Device | NVIC, SysTick, Watchdog 레지스터 |

**SSR-004 MPU 리전 정의 (검토 대상):**

| Region | Name | 용도 |
|--------|------|------|
| 0 | ASIL-D Code | SafetyFunction 코드 |
| 1 | ASIL-D Data | SafetyFunction 데이터 |
| 2 | QM Code/Data | FreeRTOS |
| 3 | QM→Safety Mailbox | 통신 |
| 4 | Safety Watchdog 레지스터 | 페리페럴 |
| 5 | Safety Timer 레지스터 | 페리페럴 |
| 6 | SafetyFunction 스택 가드 | 스택 보호 |
| 7 | FreeRTOS 스택 가드 | 스택 보호 |

**불일치 내용 및 위험:**

1. **배경 deny-all 리전(Region 0) 누락**: ADR-002에서 Region 0은 4GB 전체를 deny-all로 설정하는 배경 리전이다. 이는 ARM Cortex-M MPU에서 미커버 주소 공간에 대한 기본 차단을 제공하는 핵심 fail-secure 메커니즘이다. SSR-004에서는 이 리전이 없고 Region 0이 곧바로 ASIL-D Code로 시작한다. 배경 deny-all 없이는 8개 리전이 커버하지 못하는 주소 공간(예: 미매핑 SRAM 영역, 미사용 Flash 영역)에 대한 접근이 허용된다.

2. **리전 번호 충돌**: ADR-002 Region 1=ASIL-D Code(Flash), SSR-004 Region 0=ASIL-D Code — 같은 내용이 다른 번호에 배치되어 있어, 구현 시 두 문서 중 어느 것을 기준으로 삼을지 불명확하다.

3. **SafetyFunction 스택 분리 누락**: ADR-002는 ASIL-D Stack(Region 3)을 ASIL-D Code/Data와 별도 리전으로 분리하여 스택 영역을 명시적으로 보호하지만, SSR-004에는 ASIL-D Stack 전용 리전이 없다.

4. **Peripheral 리전 세분화**: ADR-002는 Peripheral을 Region 7 하나로 통합하고 "NVIC, SysTick, Watchdog 레지스터"라고 기술했지만, SSR-004는 Region 4(Watchdog)와 Region 5(Timer)로 분리했다. 이는 ADR-002 확장으로 볼 수 있으나 ADR-002에 반영이 필요하다.

**요구 조치 (C1):**
SSR-004를 ADR-002와 정합되도록 수정한다. 배경 deny-all 리전을 Region 0으로 명시하고, 리전 번호를 ADR-002 기준으로 재정렬하거나, ADR-002를 SSR-004 기준으로 공식 업데이트하는 ADR 개정(ADR-002 v1.1)을 발행한다. **두 문서는 동일한 리전 번호 체계를 공유해야 한다.**

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-004-MPU | ⚠️ ADR-002 불일치 (배경 deny-all 누락) | ✅ | D | ⚠️ **ISSUE-001** |
| SSR-005-MPU | ✅ | ✅ 통합 테스트 (Agent-VnV) | D | ✅ |
| SSR-006-MPU | ✅ 링커 스크립트 + VTOR 검증 명확 | ✅ | D | ✅ |
| SSR-007-MPU | ✅ | ✅ | D | ✅ (SSR-004 수정 후 리전 번호 연동 필요) |
| SSR-008-MPU | ✅ `safety_dma_validate_transfer()` 경계값 테스트 명확 | ✅ | D | ✅ |
| SSR-009-MPU | ✅ `mpu_expected_config[]` 배열 비교 구체적 | ✅ | D | ✅ |

### 2.3 Safe State FSM (SSR-010~013-FSM)

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-010-FSM | ✅ `configMAX_PRIORITIES - 1` 값 명시, vTaskPrioritySet 금지 조건 | ✅ 코드 리뷰 + 정적 분석 | D | ✅ |
| SSR-011-FSM | ✅ Level 1 전이 4단계 순서 명확 | ✅ 단위 테스트 명시 | D | ✅ |
| SSR-012-FSM | ✅ Level 2 전이 5단계 + WCET ≤ WDG×10% 제약 명시 | ✅ 단위 테스트 + WCET 분석 | D | ✅ |
| SSR-013-FSM | ✅ Level 3 전이 Critical Section + while(1) 명확 | ✅ 코드 리뷰 + 통합 테스트 | D | ✅ |

SSR-013-FSM의 "인터럽트 비활성화 + Watchdog kick 중단 + while(1)" 조합이 Level 3의 의도를 정확히 구현하는 방법으로 기술됨. 적절하다.

### 2.4 Mailbox (SSR-014~019-MBX)

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-014-MBX | ✅ 단일 경로 강제 | ✅ 정적 분석 | D | ✅ |
| SSR-015-MBX | ✅ CRC32 다항식 명시 (0x04C11DB7, ISO 3309) | ✅ **MC/DC 필수 명시** | D | ✅ |
| SSR-016-MBX | ✅ 3 경계 케이스 명시 | ✅ | D | ✅ |
| SSR-017-MBX | ✅ schema 기반 범위 검증 | ✅ **MC/DC 필수 명시** | D | ✅ |
| SSR-018-MBX | ✅ 계약 명확 | ⚠️ 코드 리뷰만 — NI-001 | D | ⚠️ NI |
| SSR-019-MBX | ✅ 4 케이스 명확 | ✅ | D | ✅ |

#### SSR-018-MBX: 검증 방법 보완 권고 (NI-001)

SSR-018의 검증 방법이 "코드 리뷰 — 모든 safety_mailbox_receive() 호출 지점에서 반환값 확인 코드 존재 여부"로만 기술되어 있다. ASIL-D 요건에서 코드 리뷰만으로는 동적 거동(실제 SAFETY_OK가 아닌 반환 시 *out 버퍼가 사용되지 않음)을 완전히 보장하기 어렵다.

**권고 조치**: 단위 테스트 추가 — "반환값 != SAFETY_OK 시 *out 버퍼를 SafetyFunction 로직에 전달하지 않음"을 테스트 케이스로 명시하면 검증 완전성이 향상된다. 구현 후 Agent-VnV 테스트 케이스 작성 시 반영 권고.

### 2.5 Boot Self-Test (SSR-020~023-BST)

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-020-BST | ✅ `__flash_crc_expected` 참조값 명확 | ✅ 1바이트 변조 테스트 케이스 | C(개발:D) | ✅ |
| SSR-021-BST | ✅ March C- 알고리즘 6단계 명시 | ✅ 시뮬레이터 테스트 | C(개발:D) | ✅ |
| SSR-022-BST | ✅ FreeRTOS 차단 조건 명확 | ✅ 통합 테스트 | C(개발:D) | ✅ |
| SSR-023-BST | ✅ 3케이스 조건 명확 (POR/WDG/BOR 3회) | ✅ | C(개발:D) | ✅ |

SSR-021-BST의 March C- 알고리즘 6단계 표기 `(↑w0)(↑r0w1)(↑r1w0)(↓r0w1)(↓r1w0)(↓r0)`가 올바른 March C- 순서와 일치함을 확인. 적절하다.

### 2.6 Clock Monitor (SSR-024~025-CLK)

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-024-CLK | ✅ ±5% 임계값 명시, 교차 검증 방법 명시 | ✅ 4 경계 케이스 명시 | C(개발:D) | ✅ |
| SSR-025-CLK | ✅ 연속 3회 Level 3 에스컬레이션 | ✅ 2 케이스 명시 | C(개발:D) | ✅ |

### 2.7 Brown-out Detector (SSR-026~027-BOD)

| SSR ID | 구현 가능성 | 검증 가능성 | ASIL | 결과 |
|--------|-----------|-----------|------|------|
| SSR-026-BOD | ✅ NVIC 우선순위 조건 명시 | ✅ 통합 테스트 | C(개발:D) | ✅ |
| SSR-027-BOD | ✅ 비휘발성 기록 형식(타임스탬프+원인+횟수) 명시 | ✅ 단위 테스트 | C(개발:D) | ✅ |

---

## 3. 전체 FSR → SSR 추적성 확인

| FSR ID | FSR 내용 요약 | SSR ID(s) | 추적 여부 |
|--------|------------|---------|---------|
| FSR-01-01 | HW Watchdog 독립 감시 | SSR-001, SSR-003 | ✅ |
| FSR-01-02 | SafetyFunction 스택/힙 물리적 분리 | SSR-004 | ✅ |
| FSR-01-03 | 내부 오류 → Level 3 | SSR-011, SSR-013 | ✅ |
| FSR-01-04 | Watchdog kick ≤50%; 타임아웃 ≥WCRT×3 | SSR-002 | ✅ |
| FSR-02-01 | ASIL-D MPU 보호 | SSR-004, SSR-005 | ✅ |
| FSR-02-02 | IVT ASIL-D 배치 | SSR-006 | ✅ |
| FSR-02-03 | Safety 타이머 QM 수정 불가 | SSR-007 | ✅ |
| FSR-02-04 | QM DMA → ASIL-D 차단 | SSR-008 | ✅ |
| FSR-03-01 | 위험 감지 → Safe State ≤[X]ms | SSR-012 | ✅ |
| FSR-03-02 | safety_task 최고 우선순위 | SSR-010 | ✅ |
| FSR-03-03 | 전이 함수 WCET ≤ WDG×10% | SSR-012, SSR-013 | ✅ |
| FSR-04-01 | CRC32 이상 사용 | SSR-015 | ✅ |
| FSR-04-02 | 일시적/영구적 오류 구분 알고리즘 | SSR-011, SSR-019 | ✅ |
| FSR-05-01 | 부팅 Flash CRC 검증 | SSR-020 | ✅ |
| FSR-05-02 | 부팅 RAM March 테스트 | SSR-021 | ✅ |
| FSR-05-03 | 자가진단 실패 → FreeRTOS 차단 | SSR-022 | ✅ |
| FSR-05-04 | 리셋 원인 기록 | SSR-023 | ✅ |
| FSR-06-01 | Mailbox 단일 경로 강제 | SSR-014 | ✅ |
| FSR-06-02 | 3단계 검증 | SSR-015, SSR-016, SSR-017 | ✅ |
| FSR-06-03 | 검증 실패 데이터 사용 금지 | SSR-018 | ✅ |
| FSR-06-04 | 연속 N=3 실패 → Level 2 | SSR-019 | ✅ |
| FSR-07-01 | Safety 타이머 레지스터 MPU 보호 | SSR-007 | ✅ |
| FSR-07-02 | 클럭 편차 ±5% 감지 | SSR-024 | ✅ |
| FSR-07-03 | 클럭 이상 → Level 2 이상 | SSR-025 | ✅ |
| FSR-08-01 | BOD 인터럽트 인터페이스 | SSR-026 | ✅ |
| FSR-08-02 | 부팅 시 MPU 무결성 검증 | SSR-009 | ✅ |
| FSR-08-03 | BOR 이력 비휘발성 기록 | SSR-023, SSR-027 | ✅ |

**FSR → SSR 추적성: 27/27 FSR 모두 커버 ✅**

---

## 4. SSR ASIL ≥ FSR ASIL 확인

| SSR 그룹 | 근거 FSR ASIL | SSR 표기 ASIL | 충족 여부 |
|---------|------------|-------------|---------|
| SSR-001~003-WDG | D | D | ✅ |
| SSR-004~009-MPU | D | D | ✅ (ISSUE-001 해소 후 유효) |
| SSR-010~013-FSM | D | D | ✅ |
| SSR-014~019-MBX | D | D | ✅ |
| SSR-020~023-BST | C | C(개발:D) | ✅ |
| SSR-024~025-CLK | C | C(개발:D) | ✅ |
| SSR-026~027-BOD | C | C(개발:D) | ✅ |

---

## 5. MISRA-C:2012 적용 규칙 검토

§5에 명시된 6개 규칙을 검토한다.

| 규칙 | 내용 | ASIL-D 적절성 |
|------|------|------------|
| Rule 18.2 (NULL 역참조 이전 NULL 검사) | ✅ 핵심 포인터 안전 규칙 | ✅ |
| Rule 10.1~10.4 (묵시적 형변환 금지) | ✅ 타입 안전성 | ✅ |
| Rule 15.5 (단일 종료점) | ✅ 분석 용이성 | ✅ |
| Rule 14.2 (for 루프 카운터) | ✅ 루프 명확성 | ✅ |
| Rule 17.7 (함수 반환값 사용) | ✅ 검증 실패 데이터 사용 금지와 직결 (SSR-018 보완) | ✅ |
| Rule 20.1~20.14 (전처리기 헤더 가드) | ✅ | ✅ |

**추가 권고 MISRA-C 규칙 (NI-002):**

현재 나열된 6개 규칙은 기본적인 것들이다. ASIL-D SafetyFunction에는 아래 추가 규칙도 명시적으로 적용 범위에 포함시킬 것을 권고한다:

| 추가 규칙 | 내용 | 관련 SSR |
|---------|------|---------|
| Rule 2.2 (Dead code 금지) | 검증 불가 코드 제거 | 전체 |
| Rule 8.4 (외부 연결 함수 선언 일치) | API 계약 안전성 | SSR-001, SSR-004 등 |
| Rule 11.3 (포인터 캐스팅 제한) | MPU 레지스터 주소 캐스팅 시 특히 중요 | SSR-004~007 |
| Rule 13.2 (비교 연산 피연산자 평가 순서) | CRC 검증 등 복잡한 조건식 | SSR-015~017 |

이는 Minor 권고 사항으로 SSRS v1.1 또는 MISRA Compliance Matrix 문서에서 처리 가능하다.

---

## 6. 지적 사항 요약

### Major 이슈

#### ISSUE-001: SSR-004-MPU — ADR-002 MPU 리전 번호 체계와 불일치

| 항목 | 내용 |
|------|------|
| 이슈 ID | ISSUE-001 |
| 위치 | OSR-SSRS-001 v1.0 §2.2 SSR-004-MPU |
| 심각도 | Major — 구현 단계에서 ADR-002와 SSR-004 중 어느 문서를 기준으로 삼을지 불명확하며, 배경 deny-all 리전 누락 시 미커버 주소 접근이 허용되는 안전 문제 발생 |
| 불일치 내용 | ADR-002: Region 0 = deny-all background (전제), Region 1~7 = 실제 영역. SSR-004: Region 0 = ASIL-D Code (deny-all 없음) |
| 안전 영향 | 8개 리전이 커버하지 못하는 주소 공간에 대한 접근이 기본 허용됨. ARM MPU의 기본값은 "no access" 또는 "privileged access"이나, 이는 MPU 버전에 따라 다르며 명시적 deny-all 리전이 없으면 fail-secure 보장이 약해짐 |
| 요구 조치 (C1) | 아래 두 방법 중 하나 선택: **방법 A**: SSR-004를 ADR-002 리전 번호 체계에 맞게 수정 (배경 deny-all Region 0 추가, ASIL-D Code Region 1로 이동 등). **방법 B**: ADR-002를 SSR-004 기준으로 개정 (ADR-002 v1.1 발행). 어느 방법을 선택하더라도 두 문서의 리전 번호가 동일해야 하며, SSR-007에서 "Region 5"로 참조하는 Safety Timer 리전 번호도 함께 업데이트해야 함 |

---

### Minor 이슈 (NI)

#### NI-001: SSR-018-MBX 검증 방법 — 코드 리뷰만 명시

| 항목 | 내용 |
|------|------|
| 이슈 ID | NI-001 |
| 위치 | §2.4 SSR-018-MBX, "검증 방법" |
| 내용 | "코드 리뷰"만으로는 *out 버퍼의 런타임 비사용을 동적으로 보장하기 어려움 |
| 권고 | 단위 테스트 케이스 추가: "반환값 SAFETY_ERR_* 시 *out의 내용이 SafetyFunction 로직에 전달되지 않음을 확인하는 테스트" |
| 긴급도 | Low — Rule 17.7(함수 반환값 사용 강제)로 일부 보완됨 |

#### NI-002: MISRA-C 적용 규칙 목록 보완

| 항목 | 내용 |
|------|------|
| 이슈 ID | NI-002 |
| 위치 | §5 MISRA-C:2012 적용 원칙 |
| 내용 | Rule 11.3, Rule 2.2, Rule 8.4, Rule 13.2 등 MPU 레지스터 접근 및 CRC 검증 코드에 특히 중요한 규칙이 미포함 |
| 권고 | SSRS v1.1 또는 별도 MISRA Compliance Matrix 문서에 추가 규칙 명시 |
| 긴급도 | Low — 기존 6개 규칙이 최소 필수 요건을 커버함 |

---

## 7. 조치 요약표

| 조치 ID | 유형 | 위치 | 내용 | 긴급도 |
|--------|------|------|------|--------|
| C1 | Major | SSR-004 + ADR-002 | MPU 리전 번호 체계 일치 (두 문서 간 배경 deny-all 포함 리전 번호 통일) | **High** |
| C2 | NI | SSR-018 | 검증 방법에 단위 테스트 케이스 추가 | Low |
| C3 | NI | §5 | MISRA Rule 11.3, 2.2, 8.4, 13.2 추가 명시 | Low |

---

## 8. 최종 결론

### OSR-CR-SSRS-001 결론: 🟡 조건부 승인

| 판정 기준 | 결과 |
|---------|------|
| FSR→SSR 추적성 완전성 (27/27) | ✅ 완전 |
| 구현 가능성 (각 SSR이 함수/모듈로 구현 가능) | ✅ (SSR-004 수정 후 유효) |
| 검증 가능성 (테스트로 Pass/Fail 판정 가능) | ✅ (NI-001 권고 반영 권고) |
| SSR ASIL ≥ FSR ASIL | ✅ 전체 |
| ADR-002 정합성 | ⚠️ **ISSUE-001: SSR-004 불일치 — 수정 필요** |
| MISRA-C 규칙 적절성 | ✅ (NI-002 보완 권고) |

**SSRS v1.0은 전체적으로 잘 작성되었으며, FSR 27개에 대한 완전한 추적성과 구체적인 구현/검증 방법을 제공한다. 단, SSR-004의 MPU 리전 번호가 ADR-002와 불일치하는 Major 이슈(ISSUE-001)가 발견되었다. 이를 SSRS v1.1 또는 ADR-002 v1.1에서 해소한 후 최종 승인으로 전환한다.**

ISSUE-001 해소는 구현 착수 전에 완료해야 하며, NI 항목(NI-001, NI-002)은 구현 착수와 병행 처리가 가능하다.

---

## 9. 후속 조치

| 항목 | 담당 | 우선순위 |
|------|------|--------|
| SSRS v1.1: SSR-004 수정 (ADR-002 또는 ADR-002 v1.1과 MPU 리전 번호 통일) | Agent-Safety | **High** |
| ADR-002 v1.1 (필요 시): SSR-004 기준으로 Peripheral 리전 세분화 반영 | Agent-Safety / Agent-Docs | High |
| Agent-QA 재검토 (OSR-CR-SSRS-002): SSRS v1.1 ISSUE-001 해소 확인 | Agent-QA | SSRS v1.1 완료 후 |
| NI-001: 구현 단계에서 Agent-VnV 테스트 케이스에 반영 | Agent-VnV | Low |
| NI-002: MISRA Compliance Matrix 또는 SSRS v1.1에 추가 규칙 명시 | Agent-Safety | Low |

---

## 10. 검토자 확인

| 역할 | 에이전트 | 검토일 |
|------|---------|--------|
| Agent-QA (Confirmation Reviewer) | Agent-QA | 2026-04-19 |
| 독립성 확인 | Agent-Safety ≠ Agent-QA — ISO 26262 Part 2 독립성 요건 충족 | ✅ |

---

*본 확인 검토 기록은 ISO 26262 Part 2 Cl.8에 따라 작성된 공식 Confirmation Review 문서입니다.*
