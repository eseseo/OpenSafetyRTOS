# OpenSafetyRTOS — Hazard Analysis and Risk Assessment (HARA)

**Document ID:** OSR-HARA-001
**Version:** 1.0
**Status:** Draft — Pending Agent-QA Confirmation Review
**Date:** 2026-04-18
**Author:** Agent-Safety
**Standard Reference:** ISO 26262 Part 3 Cl.15
**Review Record:** OSR-CR-HARA-001

---

## 1. 목적 및 범위 (Purpose and Scope)

### 1.1 목적

본 문서는 OpenSafetyRTOS SafetyFunction 파티션에 대한 위험 분석 및 위험도 평가(HARA)를 수행한다. 이 HARA는 ISO 26262 Part 3 Cl.15의 요구사항에 따라 작성되었으며, 식별된 위험원으로부터 안전 목표(Safety Goals)를 도출하고 각 안전 목표에 ASIL 등급을 할당한다.

### 1.2 SEooC HARA임을 명시

**본 HARA는 SEooC(Safety Element out of Context) 레벨에서 수행된다.**

OpenSafetyRTOS는 ISO 26262 Part 8 Cl.13에 따라 SEooC로 개발된다. 이는 구체적인 차량 기능(브레이크, 조향, 파워트레인 등)을 알지 못하는 상태에서 RTOS 요소 레벨의 위험을 분석한다는 것을 의미한다. 따라서:

- 본 HARA에서의 운영 상황(Operational Situations)은 일반적인 차량용 ECU에 적용 가능한 가정적 상황으로 정의된다.
- 식별된 안전 목표(SG-01 ~ SG-06)는 OpenSafetyRTOS를 통합하는 시스템이 자신의 시스템 레벨 HARA에서 이를 참조하고 충족 여부를 확인하기 위한 **Assumed Safety Goals**이다.
- 실제 차량 레벨 안전 목표와의 매핑은 시스템 통합자가 Assumption of Use(AoU) 문서(OSR-AOU-001)와 연계하여 수행해야 한다.

### 1.3 분석 대상

**분석 대상:** OpenSafetyRTOS SafetyFunction 파티션 (ASIL-D(D) 요소)

OpenSafetyRTOS의 아키텍처는 다음 두 파티션으로 구성된다:
- **SafetyFunction 파티션 (ASIL-D):** 안전 모니터링, Watchdog 관리, Safe State 전이, FFI(Freedom from Interference) 집행, 부팅 자가진단을 담당
- **QM 파티션 (FreeRTOS QM(D)):** 일반 애플리케이션 로직을 담당하며 SafetyFunction으로부터 격리됨

본 HARA는 **SafetyFunction 파티션**의 고장 모드를 분석 대상으로 한다.

### 1.4 분석 범위

분석 범위는 SafetyFunction의 기능 실패가 차량 레벨 위험으로 이어지는 경로이며, 구체적으로 다음을 포함한다:

- SafetyFunction의 완전 기능 상실 또는 부분 기능 상실
- QM 파티션으로부터의 간섭(FFI 실패)
- Safe State 전이 메커니즘 실패
- Watchdog 관리 실패
- Mailbox 데이터 검증 실패
- 부팅 시 자가진단 실패
- 타이밍 제약 위반

**분석 범위 외:** 구체적인 차량 기능의 위험(브레이크 고장 자체 등), 하드웨어 레벨 고장(MCU 자체 결함 등 — 별도 FIT rate 분석 대상)

---

## 2. 운영 상황 (Operational Situations)

아래 운영 상황은 일반적인 차량용 ECU를 탑재한 차량의 가정적 운영 상황으로 정의된다. OpenSafetyRTOS는 SEooC이므로, 통합자는 자신의 실제 사용 시나리오와 이 운영 상황의 적합성을 검토해야 한다.

| ID | 명칭 | 설명 | 비고 |
|-----|------|------|------|
| OS-01 | 정상 주행 | 차량이 정상 속도(> 0 km/h)로 주행 중인 상태. ECU가 지속적으로 안전 기능을 수행하며 실시간 모니터링이 활성 상태. | 가장 빈번하게 발생하는 운영 상황. 노출도 높음(E4). |
| OS-02 | 시동/부팅 | 차량 점화 ON부터 모든 ECU가 정상 동작 상태에 도달하기까지의 전환 구간. OpenSafetyRTOS가 초기화, 자가진단, MPU 설정 등을 수행하는 단계. | 결함 있는 상태로 운행 시작 가능성이 있는 특수 상황. |
| OS-03 | 주차/저속 | 차량이 정지 상태이거나 저속(< 10 km/h) 주행 중인 상태. 주차장 진입/출구, 저속 조향 등. ECU는 활성 상태이나 고속 위험은 낮음. | 속도가 낮아 일부 위험의 심각도가 감소하나 완전히 제거되지는 않음. |
| OS-04 | 긴급 상황 | 긴급 제동, ABS/ESC 활성 상태, 충돌 회피 기동 등 고위험 동적 상황. 실시간 응답 요구사항이 가장 엄격한 상황. | 이 상황에서의 SafetyFunction 실패는 가장 심각한 결과로 이어짐. |
| OS-05 | 통신 오류 상황 | CAN 버스 오류, 게이트웨이 통신 단절, QM↔SafetyFunction Mailbox 오류 등 통신 이상이 발생한 상황. | SafetyFunction의 데이터 검증 및 오류 처리 능력이 중요한 상황. |

---

## 3. 위험원 분석 (Hazard Identification)

각 위험원은 SafetyFunction 고장 모드와 그로 인한 차량 레벨 잠재 영향을 명시한다.

| ID | SafetyFunction 고장 모드 | 차량 레벨 잠재 영향 | 관련 운영 상황 |
|----|----------------------|-----------------|-------------|
| H-01 | SafetyFunction 완전 기능 상실 (crash/lockup/예외 미처리) | 안전 모니터링 부재 → 시스템 레벨 위험 통제 불가. 하위 시스템(브레이크, 조향 등)의 고장 감지 및 Safe State 전이가 불가능해짐. | OS-01, OS-04 |
| H-02 | FFI 실패 — QM 파티션이 ASIL-D 메모리 영역을 침범 | SafetyFunction 데이터 오염 → 잘못된 안전 판단. MPU 미설정 또는 설정 오류로 인해 QM이 Safety 변수/스택을 덮어씀으로써 SafetyFunction이 오동작하거나 침묵 고장(silent fault) 발생. | OS-01, OS-02, OS-04 |
| H-03 | Safe State 전이 실패 (위험 조건 감지했으나 전이 미수행) | 위험 상태 지속. SafetyFunction이 고장을 감지하고도 Safe State 전이 명령을 실행하지 못함으로써, 안전 요구사항을 위반한 상태로 시스템이 계속 동작함. | OS-01, OS-04 |
| H-04 | 오탐(False Positive) Safe State 전이 | 불필요한 차량 기능 중단 → 주행 중 예기치 않은 정지. 정상 상황에서 SafetyFunction이 고장으로 오판하고 Safe State 전이를 개시함으로써, 운전자가 제어 상실의 위험에 노출될 수 있음. | OS-01, OS-03 |
| H-05 | Watchdog kick 실패 (SafetyFunction 자체 deadlock 또는 무한 대기) | HW Watchdog reset → 제어 불능 순간 발생. SafetyFunction 자신이 데드락 상태에 빠져 Watchdog을 kick하지 못하면 MCU가 강제 리셋되고, 리셋 과정에서 짧은 제어 불능 상태가 발생함. | OS-01, OS-04 |
| H-06 | Mailbox 검증 우회 또는 오검증 | 잘못된 QM 데이터를 신뢰 → 오동작 명령 실행. SafetyFunction이 QM 파티션으로부터 받은 데이터의 무결성을 충분히 검증하지 않거나 검증 로직에 결함이 있어, 오염되거나 변조된 데이터에 기반하여 안전 판단을 내림. | OS-01, OS-05 |
| H-07 | 부팅 자가진단 실패 미감지 (Silent fault at startup) | 결함 있는 상태로 운행 시작. 부팅 시 CPU 레지스터, RAM, Flash, MPU 설정 등에 결함이 존재함에도 자가진단이 이를 감지하지 못하고 정상으로 판단하여 시스템이 기동됨. | OS-02 |
| H-08 | 타이밍 위반 (SafetyFunction 응답 지연 > 허용 한계) | 실시간 위험 대응 실패. SafetyFunction의 위험 감지부터 Safe State 전이 개시까지의 응답 시간이 안전 요구사항을 초과함으로써, 긴급 상황에서 적시에 안전 상태로 전이하지 못함. | OS-01, OS-04 |

---

## 4. 위험도 평가 (Risk Assessment) — S/E/C 매트릭스

### 4.1 평가 기준 정의 (ISO 26262 Part 3)

**심각도 (Severity, S):**

| 등급 | 정의 |
|------|------|
| S0 | 부상 없음 (No injuries) |
| S1 | 경상 (Light and moderate injuries) |
| S2 | 중상 (Severe and life-threatening injuries, survival probable) |
| S3 | 치명적 부상 또는 사망 (Life-threatening injuries, survival uncertain, or fatal injuries) |

**노출도 (Exposure, E):**

| 등급 | 정의 |
|------|------|
| E0 | 불가능한 수준 (Incredible) |
| E1 | 매우 낮음 (Very low probability) |
| E2 | 낮음 (Low probability) |
| E3 | 중간 (Medium probability) |
| E4 | 높음 (High probability) |

**통제 가능성 (Controllability, C):**

| 등급 | 정의 |
|------|------|
| C0 | 일반적으로 통제 가능 (Controllable in general) |
| C1 | 단순히 통제 가능 (Simply controllable) |
| C2 | 보통 통제 가능 (Normally controllable) |
| C3 | 통제 어려움 또는 불가능 (Difficult to control or uncontrollable) |

### 4.2 ASIL 결정 매트릭스 (ISO 26262 Part 3 Table 4)

|  | C1 | C2 | C3 |
|--|----|----|-----|
| **S1, E1** | QM | QM | QM |
| **S1, E2** | QM | QM | QM |
| **S1, E3** | QM | QM | ASIL-A |
| **S1, E4** | QM | ASIL-A | ASIL-B |
| **S2, E1** | QM | QM | QM |
| **S2, E2** | QM | QM | ASIL-A |
| **S2, E3** | QM | ASIL-A | ASIL-B |
| **S2, E4** | ASIL-A | ASIL-B | ASIL-C |
| **S3, E1** | QM | QM | ASIL-A |
| **S3, E2** | QM | ASIL-A | ASIL-B |
| **S3, E3** | ASIL-A | ASIL-B | ASIL-C |
| **S3, E4** | ASIL-B | ASIL-C | ASIL-D |

### 4.3 위험원별 최악 경우(Worst Case) S/E/C 평가 및 ASIL 결정

---

#### H-01: SafetyFunction 완전 기능 상실

**최악 경우 운영 상황:** OS-04 (긴급 상황)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S3** | SafetyFunction이 완전히 기능을 상실하면 어떠한 고장도 감지하지 못한다. 긴급 제동/ABS 활성 상황에서 하위 시스템 고장에 대한 감시가 전혀 없으면 차량 제어 상실로 이어져 치명적 사고 가능. |
| Exposure | **E4** | SafetyFunction은 차량 시동 후 지속적으로 동작한다. 정상 주행 상황(OS-01)은 차량 사용 시간 대부분을 차지하므로, SafetyFunction이 동작 중인 상황에 대한 노출도는 매우 높음. |
| Controllability | **C3** | SafetyFunction 완전 기능 상실은 운전자에게 경고 없이 발생할 수 있으며, 안전 모니터링 자체가 부재한 상태이므로 운전자가 이를 인식하고 보상 조치를 취하기 매우 어려움. |

**결정 ASIL: ASIL-D**

---

#### H-02: FFI 실패 — QM이 ASIL-D 메모리 침범

**최악 경우 운영 상황:** OS-01 (정상 주행) / OS-04 (긴급 상황)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S3** | QM 파티션이 SafetyFunction 메모리를 오염시키면 SafetyFunction이 잘못된 데이터로 안전 판단을 내리거나 예외 없이 오동작할 수 있다. 침묵 고장(Silent Fault)의 경우 차량이 안전 모니터링이 정상인 것으로 오인하고 운행하며, 실제 고장 발생 시 감지 불가. 치명적 결과 가능. |
| Exposure | **E4** | MPU 미설정 또는 초기화 오류는 시스템이 구동되는 전체 시간 동안 지속되는 상태이므로 노출도 높음. |
| Controllability | **C3** | 메모리 오염은 운전자가 전혀 인식할 수 없다. 오동작이 발생할 때는 이미 제어 불능 상태일 가능성이 높음. |

**결정 ASIL: ASIL-D**

---

#### H-03: Safe State 전이 실패

**최악 경우 운영 상황:** OS-04 (긴급 상황)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S3** | 위험 조건(예: 하위 시스템 고장)을 감지했음에도 Safe State로 전이하지 못하면, 위험 상태가 계속 유지된다. 긴급 상황에서는 이로 인해 사고가 발생하고 치명적 부상/사망으로 이어질 수 있음. |
| Exposure | **E4** | SafetyFunction의 Safe State 전이 메커니즘은 지속적으로 동작 중이므로 전환 실패 가능성에 대한 노출도는 높음. |
| Controllability | **C3** | 위험 조건이 감지되었으나 Safe State 전이가 되지 않는 상황에서, 운전자는 시스템이 이미 고장을 인식하고 있다고 착각할 수 있으며, 실제 위험 상황에서 스스로 안전 조치를 취하기 매우 어려움. |

**결정 ASIL: ASIL-D**

---

#### H-04: 오탐(False Positive) Safe State 전이

**최악 경우 운영 상황:** OS-01 (정상 주행 중)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S2** | 정상 주행 중 불필요한 Safe State 전이가 발생하면 차량 제어 기능이 갑작스럽게 축소 또는 중단될 수 있다. 이는 후방 차량과의 충돌 위험을 높이며 심각한 부상을 초래할 수 있으나, 운전자가 즉각 인식할 수 있는 경우가 많아 치명적 사고까지는 아닌 경우가 대부분. Safe State 설계(Level 1 Degraded → Level 2 Controlled Stop)에 따라 완전 정지가 아닌 제한 운행으로 전환될 수 있음. |
| Exposure | **E4** | 정상 주행(OS-01)은 차량 운용 시간의 대부분을 차지하므로 노출도 높음. |
| Controllability | **C2** | 갑작스러운 차량 제동/기능 축소에 대해 운전자는 일반적으로 즉각 인식하고 대응 가능. 완전한 제어 상실이 아닌 예기치 않은 상황에 대한 적응이 필요한 수준. |

**결정 ASIL: ASIL-B**

---

#### H-05: Watchdog kick 실패 (SafetyFunction 데드락)

**최악 경우 운영 상황:** OS-04 (긴급 상황)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S2** | SafetyFunction 데드락으로 Watchdog kick이 실패하면 HW Watchdog이 MCU를 리셋한다. 리셋 과정에서 짧은 제어 불능 상태가 발생한다. 단, MCU 리셋 후 정상 부팅이 되면 시스템이 복구되는 시나리오이므로 항상 치명적이지는 않음. 긴급 상황에서의 일시적 제어 상실은 심각한 부상 초래 가능. |
| Exposure | **E3** | SafetyFunction 데드락은 설계 결함이나 특수한 타이밍 조건에서 발생하므로 발생 빈도는 중간 수준. 지속적 동작 중이나 특정 이벤트 조합이 필요. |
| Controllability | **C2** | MCU 리셋 후 재부팅 과정은 짧은 시간 내에 완료될 수 있으며, 이 과정에서 운전자가 차량 제어를 유지할 수 있는 경우가 있음. 그러나 긴급 상황에서는 어려울 수 있음. |

**결정 ASIL: ASIL-B**

---

#### H-06: Mailbox 검증 우회 또는 오검증

**최악 경우 운영 상황:** OS-05 (통신 오류 상황) / OS-01 (정상 주행)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S3** | SafetyFunction이 QM으로부터 받은 오염된 데이터를 유효한 것으로 신뢰하면, 잘못된 안전 판단을 내리거나 오동작 명령을 실행할 수 있다. 통신 오류 상황에서 조작된 또는 비트 오류가 발생한 데이터가 검증 없이 처리되면 치명적 결과 가능. H-02와 마찬가지로 침묵 오동작 가능성. |
| Exposure | **E4** | Mailbox 통신은 QM↔Safety 간 지속적으로 발생하므로 노출도 높음. |
| Controllability | **C3** | 잘못된 데이터에 기반한 오동작은 운전자가 예측하거나 보상 조치를 취하기 매우 어려운 방식으로 나타날 수 있음. |

**결정 ASIL: ASIL-D**

---

#### H-07: 부팅 자가진단 실패 미감지 (Silent fault at startup)

**최악 경우 운영 상황:** OS-02 (시동/부팅)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S3** | 자가진단이 결함을 감지하지 못하면 결함 있는 상태로 차량이 운행을 시작한다. 이후 정상 주행 또는 긴급 상황에서 SafetyFunction이 오동작하여 치명적 결과를 초래할 수 있음. |
| Exposure | **E2** | 부팅 자가진단 실패(미감지)는 부팅 사이클 중 특정 초기화 결함 또는 하드웨어 열화 상황에서 발생한다. 운행 전체 기간 중 부팅 횟수는 제한적이며 이 조건의 동시 발생은 낮음. |
| Controllability | **C3** | 운전자는 부팅 자가진단이 성공/실패했는지 알 수 없으며, 결함 있는 상태에서의 오동작은 예측 불가능하여 대응이 매우 어려움. |

**결정 ASIL: ASIL-C**

---

#### H-08: 타이밍 위반 (SafetyFunction 응답 지연)

**최악 경우 운영 상황:** OS-04 (긴급 상황)

| 파라미터 | 등급 | 근거 |
|---------|------|------|
| Severity | **S3** | 긴급 상황에서 SafetyFunction의 응답이 허용 시간을 초과하면, 이미 사고가 발생한 이후에 Safe State 전이가 개시될 수 있다. 실시간 대응 실패는 치명적 결과로 직결될 수 있음. |
| Exposure | **E3** | 타이밍 위반은 시스템 부하가 높거나 예외적인 이벤트 조합이 발생하는 상황에서 나타날 수 있다. 정상 운행 중에도 발생 가능하나 긴급 상황과의 동시 발생이 필요하므로 중간 수준. |
| Controllability | **C3** | 응답 지연으로 인해 SafetyFunction이 적시에 작동하지 않는 상황에서, 운전자는 시스템이 이미 대응 중이라고 기대하므로 자체 대응이 매우 어려움. |

**결정 ASIL: ASIL-D**

---

### 4.4 ASIL 결정 요약표

| 위험원 ID | 고장 모드 요약 | 최악 운영 상황 | S | E | C | ASIL |
|----------|------------|-------------|---|---|---|------|
| H-01 | SafetyFunction 완전 기능 상실 | OS-04 | S3 | E4 | C3 | **ASIL-D** |
| H-02 | FFI 실패 — QM의 ASIL-D 메모리 침범 | OS-01/04 | S3 | E4 | C3 | **ASIL-D** |
| H-03 | Safe State 전이 실패 | OS-04 | S3 | E4 | C3 | **ASIL-D** |
| H-04 | 오탐(False Positive) Safe State 전이 | OS-01 | S2 | E4 | C2 | **ASIL-B** |
| H-05 | Watchdog kick 실패 | OS-04 | S2 | E3 | C2 | **ASIL-B** |
| H-06 | Mailbox 검증 우회 또는 오검증 | OS-01/05 | S3 | E4 | C3 | **ASIL-D** |
| H-07 | 부팅 자가진단 실패 미감지 | OS-02 | S3 | E2 | C3 | **ASIL-C** |
| H-08 | 타이밍 위반 (응답 지연) | OS-04 | S3 | E3 | C3 | **ASIL-D** |

---

## 5. 안전 목표 (Safety Goals)

ASIL-D 및 ASIL-C 위험원으로부터 안전 목표를 도출한다. 안전 목표는 SafetyFunction이 **달성해야 하는 최상위 안전 요구사항**이며, 이로부터 기능 안전 요구사항(FSR) 및 기술 안전 요구사항(TSR)이 전개된다.

| SG-ID | 안전 목표 | 근거 위험원 | ASIL | 안전 상태 |
|-------|---------|-----------|------|---------|
| SG-01 | SafetyFunction은 단일 고장으로 인해 완전히 기능을 상실해서는 안 된다. | H-01 | ASIL-D | Level 2 (Controlled Stop) 또는 Level 3 (Emergency Reset) |
| SG-02 | QM 파티션은 어떠한 경우에도 SafetyFunction의 데이터 또는 실행 흐름에 간섭할 수 없어야 한다 (FFI: Freedom from Interference). | H-02 | ASIL-D | 해당 없음 (예방적 목표) |
| SG-03 | SafetyFunction은 위험 조건 감지 후 [X]ms 이내에 Safe State로 전이를 개시해야 한다. | H-03, H-08 | ASIL-D | Level 1 / Level 2 / Level 3 (심각도에 따라) |
| SG-04 | SafetyFunction은 오탐으로 인한 불필요한 Safe State 전이를 최소화해야 한다. | H-04 | ASIL-B | Level 1 (Degraded) 유지 선호 |
| SG-05 | SafetyFunction은 부팅 시 자가진단(Power-On Self Test)을 완료하고, 결함 발견 시 시스템 시작을 거부해야 한다. | H-07 | ASIL-C | Level 3 (Emergency Reset) — 시동 불허 |
| SG-06 | SafetyFunction은 QM 파티션으로부터 수신된 데이터를 검증 없이 신뢰해서는 안 된다. | H-06 | ASIL-D | Level 2 (Controlled Stop) — 검증 실패 시 |

### 5.1 SG-03 파라미터 정의

SG-03의 응답 시간 파라미터 **[X]ms**는 OpenSafetyRTOS가 SEooC로 개발되므로, 통합 시스템의 FTTI(Fault Tolerant Time Interval)에 따라 결정된다. 이는 Assumption of Use(AoU) 문서(OSR-AOU-001)에서 통합자가 명시해야 하는 파라미터이다.

**참조값(Reference Value):** 10ms

이 참조값은 일반적인 차량 제어 시스템(ABS, ESC 등)의 FTTI 요구사항을 기반으로 한 권고값이다. 실제 적용 시 통합자는 자신의 시스템 FTTI 분석 결과에 따라 이 값을 조정해야 한다.

- 참조값 10ms보다 엄격한 요구사항을 가진 시스템: 통합자가 별도 검증 필요
- 10ms 이상의 FTTI를 가진 시스템: 본 참조값 적용 가능

### 5.2 안전 목표와 위험원 매핑

```
H-01 (ASIL-D) ──────────────────────→ SG-01
H-02 (ASIL-D) ──────────────────────→ SG-02
H-03 (ASIL-D) ──┐
                 ├──────────────────→ SG-03
H-08 (ASIL-D) ──┘
H-04 (ASIL-B) ──────────────────────→ SG-04
H-07 (ASIL-C) ──────────────────────→ SG-05
H-06 (ASIL-D) ──────────────────────→ SG-06
H-05 (ASIL-B) ── (SG-01에 의해 부분적으로 포괄됨; 별도 FSR 요구)
```

**참고:** H-05(ASIL-B)는 SG-01의 하위 시나리오로 볼 수 있으나, Watchdog 관련 특수성(리셋 기반 복구 메커니즘)으로 인해 별도의 기능 안전 요구사항에서 다루어진다.

---

## 6. SEooC 특이사항 (SEooC-Specific Considerations)

### 6.1 SEooC HARA의 본질적 한계

본 HARA는 SEooC 레벨에서 수행되며, 다음과 같은 본질적 한계를 갖는다:

- **차량 기능 불명:** 실제 통합 대상 차량 기능(브레이크, 조향, 파워트레인 제어 등)을 알 수 없으므로, 위험원의 차량 레벨 영향은 "가능한 최악 시나리오"를 기반으로 가정됨.
- **운영 상황 가정:** 정의된 운영 상황(OS-01 ~ OS-05)은 일반적인 차량용 ECU를 가정한 것이며, 특수 용도(농기계, 건설기계 등) 또는 비전통적 아키텍처에서는 적합하지 않을 수 있음.
- **ASIL 보수성:** SEooC 특성상 ASIL 평가는 가장 불리한 통합 환경을 가정하여 보수적으로 결정됨.

### 6.2 통합자의 의무

OpenSafetyRTOS를 차량 시스템에 통합하는 시스템 통합자는 다음을 수행해야 한다:

1. **시스템 레벨 HARA 수행:** 자신의 시스템 기능과 통합 환경을 고려한 독립적 HARA를 수행한다.
2. **안전 목표 정렬:** 본 HARA에서 도출된 SG-01 ~ SG-06이 시스템 레벨 안전 목표를 충족하는지 확인한다.
3. **AoU 준수:** 통합자는 OSR-AOU-001(Assumption of Use) 문서를 검토하고, 모든 사용 가정을 충족하는지 확인한다.
4. **SG-03 파라미터 결정:** 자신의 시스템 FTTI 분석에 기반하여 [X]ms 값을 결정하고 문서화한다.
5. **추가 위험원 식별:** 통합 환경에서 발생할 수 있는 추가적인 위험원을 식별하고, 필요 시 추가 안전 목표를 도출한다.

### 6.3 Assumption of Use (AoU) 연계

본 HARA는 OSR-AOU-001 문서와 다음 항목에서 연계된다:

| AoU 항목 | 관련 안전 목표 | 설명 |
|---------|-------------|------|
| AoU-01: MPU 지원 하드웨어 요구 | SG-02 | FFI 구현은 ARM MPU를 전제로 하며, MPU 미지원 플랫폼에서는 SG-02 달성 불가 |
| AoU-02: HW Watchdog 존재 요구 | SG-01, SG-03 | SafetyFunction 복구는 독립적 HW Watchdog을 전제로 함 |
| AoU-03: FTTI 명시 요구 | SG-03 | 통합자는 자신의 FTTI를 OSR-AOU-001에 기재해야 함 |
| AoU-04: 부팅 순서 제약 | SG-05 | SafetyFunction BIST 완료 전 차량 기능 활성화 금지 |
| AoU-05: QM 파티션 격리 책임 | SG-02, SG-06 | QM 영역의 코드/데이터 관리는 통합자 책임 |

### 6.4 배포 모델 (Distribution Model)

ISO 26262 Part 2에서 정의된 ASIL 분해(Decomposition)에 따라 OpenSafetyRTOS는 다음 구조로 배포된다:

```
OpenSafetyRTOS ASIL-D(D)
├── SafetyFunction 파티션   ASIL-D(D) ← 본 HARA의 분석 대상
└── FreeRTOS QM 파티션      QM(D)     ← FFI 보장 시 안전 요구사항 없음
```

이 배포 모델 하에서 SG-01 ~ SG-06은 SafetyFunction 파티션에 할당된다.

---

## 7. 추적성 (Traceability)

### 7.1 위험원 → 안전 목표 추적성

| 위험원 | ASIL | 안전 목표 |
|------|------|---------|
| H-01 | ASIL-D | SG-01 |
| H-02 | ASIL-D | SG-02 |
| H-03 | ASIL-D | SG-03 |
| H-04 | ASIL-B | SG-04 |
| H-05 | ASIL-B | — (SG-01 FSR에서 처리) |
| H-06 | ASIL-D | SG-06 |
| H-07 | ASIL-C | SG-05 |
| H-08 | ASIL-D | SG-03 |

### 7.2 안전 목표 → 후속 문서 추적성

| 안전 목표 | 후속 문서 |
|---------|---------|
| SG-01 ~ SG-06 | OSR-FSR-001 (기능 안전 요구사항) |
| SG-01 ~ SG-06 | OSR-FMEA-001 (FMEA — 각 SG 대응 고장 모드 분석) |
| SG-03 ([X]ms) | OSR-AOU-001 (Assumption of Use — 통합자 파라미터) |
| SG-05 (BIST) | OSR-SSR-001 (소프트웨어 안전 요구사항) |

---

## 8. 개정 이력 (Revision History)

| 버전 | 날짜 | 작성자 | 변경 내용 |
|-----|------|------|---------|
| 1.0 | 2026-04-18 | Agent-Safety | 최초 작성 — 8개 위험원 식별, 6개 안전 목표 도출 |

---

## 9. Agent-QA 확인 검토 요청

이 문서는 **Agent-QA의 확인 검토(Confirmation Review)** 대상이다.

**근거 표준:** ISO 26262 Part 2 Cl.8 (Confirmation Measures — Confirmation Review)

**검토 기록:** OSR-CR-HARA-001

확인 검토 시 Agent-QA는 다음 항목을 검증해야 한다:

| 검토 항목 | 체크포인트 |
|---------|----------|
| 완전성 (Completeness) | 모든 관련 운영 상황이 고려되었는가? 8개 이상의 위험원이 식별되었는가? |
| 정확성 (Correctness) | S/E/C 파라미터 할당이 ISO 26262 정의에 부합하는가? ASIL 결정이 Part 3 Table 4에 따라 올바르게 수행되었는가? |
| 일관성 (Consistency) | 안전 목표가 위험원과 논리적으로 연결되는가? ASIL 등급이 일관성 있게 할당되었는가? |
| SEooC 적합성 | 가정적 운영 상황이 일반적 차량용 ECU 환경에 적합한가? AoU 연계가 적절히 명시되었는가? |
| 추적성 (Traceability) | 모든 위험원이 안전 목표에 추적되는가? 후속 문서로의 추적 경로가 명시되었는가? |

**검토 완료 후 Agent-QA는 OSR-CR-HARA-001에 검토 결과, 발견된 이슈, 승인/반려 판정을 기록해야 한다.**

---

*본 문서(OSR-HARA-001)는 OpenSafetyRTOS 안전 케이스(Safety Case)의 기초 문서이며, 모든 후속 안전 분석 및 요구사항 도출의 출발점이다. 문서의 모든 변경은 변경 영향 분석(Change Impact Analysis)을 수반해야 하며, 재검토 후 버전 이력에 기록되어야 한다.*
