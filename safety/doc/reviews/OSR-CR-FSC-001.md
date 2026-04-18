# OSR-CR-FSC-001: FSC 확인 검토 기록

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CR-FSC-001 |
| 검토 대상 | OSR-FSC-001 (FSC.md) Rev 1.0.0 |
| 검토자 | Agent-QA (독립 — Agent-Safety와 별개) |
| 검토 기준 | ISO 26262 Part 2 Cl.8 (Confirmation Review), Part 3 Cl.8 (FSC) |
| 입력 문서 | OSR-FSC-001, OSR-HARA-001, OSR-SS-001, ARCHITECTURE.md |
| 날짜 | 2026-04-18 |

---

## 검토 결과 요약

**판정: 조건부 승인 (CONDITIONAL PASS)**

주요 강점: 18개 FSR이 6개 SG를 전반적으로 커버하며, Decomposition 전략(ISO 26262 Part 9)의 독립성 요건을 FSR-02-01~04로 구체화한 것이 우수함. ASIL-D 개발 프로세스로 하위 ASIL 요구사항(ASIL-B, C)을 상위 호환(over-fulfillment) 처리한 논증이 Part 9 Cl.5.4를 올바르게 인용함.

주요 우려: SG-01에 대한 FSR 커버리지 갭(H-05 Watchdog 고장 모드의 명시적 FSR 부재), FSR-04-02(일시적/영구적 오류 구분)의 구현 기준 미정, FSR-03-03(인터럽트 불가 구간)의 검증 가능성, SG-06 데이터 유효성 오류 에스컬레이션 FSR 부재.

---

## 항목별 검토

### ✅ 적합 항목

1. **SG → FSR 분해 추적성:** 6개 모든 안전 목표(SG-01~06)에 대해 FSR이 도출되었으며, FSR 요약 테이블(§4)에서 근거 SG, ASIL, 구현 위치 예상이 체계적으로 제시됨. 검토자가 역추적(FSR → SG) 및 순추적(SG → FSR)을 모두 수행할 수 있음.

2. **DMA FFI 경로 FSR 포함 (FSR-02-04):** ARCHITECTURE.md §5.2에서 MPU를 우회하는 DMA 위협을 별도로 분석한 결과가 FSR-02-04로 전개되어 있음. MPU 기반 FFI만으로는 불충분하다는 점을 FSC 수준에서 인식하고 있음을 확인.

3. **Decomposition 독립성 논증 (§5):** SafetyFunction ASIL-D(D)와 FreeRTOS QM(D)의 독립성이 FFI 달성(FSR-02-01~04)에 의해 보장됨을 §5.3에서 트리 구조로 명시하였음. ISO 26262 Part 9 Cl.5.4 over-fulfillment 개념을 명시적으로 인용함.

4. **Safe State 전이 기능의 원자성 보장 (FSR-03-03):** 인터럽트 불가 구간(interrupt-disabled)에서 Safe State 전이를 실행한다는 요구사항은 타이밍 보장의 핵심이며, 이를 FSR 수준에서 명시한 것은 올바른 접근임.

5. **미결 사항 위임 테이블 (§6):** [X]ms, Watchdog 스펙, DMA 채널 할당, RAM March 알고리즘 등 6개 미결 항목에 대해 위임 대상(통합자 AoU, TSC, 플랫폼 HAL)을 체계적으로 명시하였음. SEooC FSC의 적절한 처리 방식임.

6. **FSR-05-04(리셋 원인 기록)의 포함:** 단순 자가진단 Pass/Fail 외에 비정상 리셋(Watchdog, 예외) 감지 및 기록 FSR을 포함한 것은 반복 결함 감지 측면에서 ASIL-C 수준을 상회하는 양호한 설계 요구사항임.

7. **Mailbox 3단계 검증 순서 명시 (FSR-06-02):** CRC → 타임스탬프 → 범위의 순서와 각 단계의 보안 목적(replay 방어, stale 데이터, 물리적 비현실 값)을 명시하였음. 검증 가능한 구체적 FSR임.

---

### ⚠️ 지적사항 (Issues)

**ISSUE-001: SG-01에 대한 H-05 커버 FSR 명시 부재**

- **설명:** HARA OSR-CR-HARA-001 ISSUE-005에서도 지적하였듯, H-05(Watchdog kick 실패)는 SG-01 FSR에서 처리된다고 HARA에 명시되어 있다. FSC §3.1(SG-01)의 FSR 목록에서:
  - FSR-01-01: "SafetyFunction은 HW Watchdog에 의해 독립적으로 감시되어야 한다" — Watchdog이 SafetyFunction을 감시하는 메커니즘은 있음.
  - 그러나 "SafetyFunction이 데드락에 빠진 경우 어떻게 복구하는가"에 대한 응답 시간 또는 Watchdog 타임아웃 설계 요구사항이 FSR 수준에서 없음.
  - FSR-01-01은 Watchdog 존재를 요구하나, Watchdog 타임아웃 설정이 SafetyFunction 데드락 감지에 충분한지(즉, 허용 가능한 제어 불능 시간 이내에 리셋이 발생하는지)를 보장하는 FSR이 없음.
  - 이 갭은 SG-01(ASIL-D)의 검증 불가 상태를 만듦.
- **심각도: Major**
- **조치:** Agent-Safety는 FSR-01-04(가칭)를 추가할 것: "HW Watchdog 타임아웃 주기는 SafetyFunction 데드락으로 인한 최대 허용 제어 불능 시간 이내여야 한다 (구체적 값은 통합자 AoU 또는 TSC에서 결정)". 이를 §4 FSR 요약 테이블에 추가하고 H-05 → FSR-01-04 추적성을 완성할 것.

---

**ISSUE-002: FSR-04-02(일시적/영구적 오류 구분)가 구현 기준 없이 검증 불가**

- **설명:** FSR-04-02는 "일시적 오류(transient fault)와 영구적 오류(permanent fault)를 구분하는 메커니즘을 제공해야 한다"고 요구하나:
  - "구분" 기준이 완전히 미정임. §6 미결 사항 표에서 "재시도 횟수, 시간 윈도우 등 정의 필요"로 위임하였으나, FSC 단계에서 구분 알고리즘의 기능 수준 프레임워크 조차 정의되지 않음.
  - 예를 들어: "N회 연속 실패 → 영구 오류로 분류" 또는 "T ms 이내 자가 해소 → 일시 오류"와 같은 기능 수준 요구사항이 FSR로서 필요함.
  - TSC로 완전 위임 시: FSR-04-02가 기능 안전 요구사항(FSR)으로서의 역할을 하지 못하며, 사실상 "TSC에서 결정하라"는 메모가 됨. ISO 26262 Part 3 Cl.8에서 FSR은 기능 수준에서 검증 가능해야 함.
  - SG-04(ASIL-B)는 FSR-04-01(CRC32)과 FSR-04-02로만 커버됨. FSR-04-02가 검증 불가하면 SG-04 커버리지가 불완전함.
- **심각도: Major**
- **조치:** Agent-Safety는 FSR-04-02를 다음과 같이 구체화할 것: "일시적 오류는 단일 오류 발생 후 T_window ms 이내 자가 해소되고 재발이 없는 경우로 정의한다. 동일 오류가 N_threshold회 이상 연속 발생하면 영구 오류로 분류한다. T_window 및 N_threshold는 TSC에서 결정한다." — 이렇게 하면 기능 수준 알고리즘이 FSR에 정의되고 파라미터만 TSC로 위임됨.

---

**ISSUE-003: SG-06 커버리지 갭 — 검증 실패 오류 에스컬레이션 FSR 부재**

- **설명:** §3.6(SG-06)에서 FSR-06-01~03을 도출하였다. FSR-06-03에서 "검증 실패 데이터는 폐기하고 오류 카운터를 증가시켜야 함"을 요구하나:
  - 오류 카운터가 임계값에 도달했을 때의 응답(Safe State 전이 트리거)을 요구하는 FSR이 없음.
  - SAFE_STATE_DEFINITION.md §3.1 전이 조건 매트릭스에서 "Mailbox CRC 검증 실패 (연속 N회) → Level 2"가 정의되어 있음에도, 이 에스컬레이션 로직이 FSR 수준에서 요구사항으로 정의되지 않음.
  - FSR-06-03은 "오류 카운터 증가"를 언급하나, 카운터 임계값 초과 시 Safe State 전이 요구사항이 없으면 이 FSR만으로 SG-06(검증 없이 신뢰 금지)의 완전한 달성을 보장할 수 없음.
  - 지속적 검증 실패 상황에서 시스템이 계속 동작하는 것은 SG-06 위반 시나리오임.
- **심각도: Major**
- **조치:** Agent-Safety는 FSR-06-04(가칭)를 추가할 것: "Mailbox 검증 실패 카운터가 정해진 임계값을 초과하면 Safe State Level 2 이상으로 전이해야 한다. 임계값은 TSC에서 결정한다." FSR-06-04를 §4 요약 테이블에 추가.

---

**ISSUE-004: FSR-03-03(인터럽트 불가 구간) 최대 허용 시간 미정 — 시스템 실시간성 위험**

- **설명:** FSR-03-03은 "Safe State 전이 함수는 인터럽트 불가(interrupt-disabled) 구간에서 실행되어야 한다"고 요구한다. 이는 Safe State 전이의 원자성 보장 측면에서 올바르나:
  - 인터럽트 비활성화 구간이 무제한으로 허용되면, SafetyFunction 자신의 Watchdog kick 인터럽트(또는 타이머 인터럽트)도 차단될 수 있음.
  - 인터럽트 비활성화 구간의 최대 허용 시간(WCET)을 §6 미결 사항으로 위임하고 있으나, 이 제약이 없으면 FSR-01-01(HW Watchdog 감시)과 FSR-03-03이 상충할 수 있음.
  - 구체적으로: 인터럽트 비활성화 → Watchdog kick 불가 → Watchdog 타임아웃 → Level 3 리셋이라는 의도치 않은 연쇄가 발생 가능.
  - Level 2 전이 시퀀스(SAFE_STATE_DEFINITION.md §4.2)에서 Level 2 유지 루프는 watchdog_kick()을 호출함. 이 루프 진입 이전의 인터럽트 비활성화 시간이 Watchdog 타임아웃보다 짧아야 함.
- **심각도: Major**
- **조치:** Agent-Safety는 FSR-03-03에 다음 제약을 추가할 것: "인터럽트 불가 구간의 최대 실행 시간은 HW Watchdog 타임아웃 주기의 50% 이내여야 한다 (구체적 값은 TSC WCET 분석에서 결정)." 또한 §6 미결 사항 6번 항목에 이 제약의 검증 방법(WCET 분석 도구)을 명시할 것.

---

**ISSUE-005: Decomposition 독립성 요건의 FSR 기반 논증에서 소프트웨어 공유 자원 누락**

- **설명:** §5 Decomposition 전략에서 FFI 독립성이 FSR-02-01~04(MPU, IVT, 타이머, DMA)로 보장된다고 논증함. 그러나 ARCHITECTURE.md §5.2에 명시된 **스택 포인터 공유 위협** — "QM 스택 오버플로우 → SafetyFunction 스택 침범"에 대응하는 FSR-02-05(MPU 스택 가드 영역 설정)가 명시적 FSR로 도출되지 않음.
  - ARCHITECTURE.md에서는 대응 메커니즘으로 "MPU 스택 가드 + 각 파티션 전용 스택 영역"이 명시됨.
  - FSR-01-02("SafetyFunction 스택/힙은 QM 파티션과 물리적으로 분리")가 존재하나, 이는 SG-01의 FSR로 분류되어 있고 SG-02(FFI) FSR 체인에서 스택 가드가 누락됨.
  - Decomposition 독립성 논증(§5.1 표)에서 FFI 보장은 FSR-02-01~04에만 연결되어 있어 스택 가드 요구사항이 빠져 있음.
- **심각도: Minor**
- **조치:** Agent-Safety는 FSR-02-05(가칭)를 추가하거나, FSR-01-02가 SG-02 FFI 체인에도 기여함을 §5.1 Decomposition 표에서 명시할 것. 추적성 표에서 FSR-01-02가 SG-01과 SG-02 모두의 근거임을 이중 기재할 것.

---

**ISSUE-006: SG-05 FSR에 CPU 레지스터 자가진단 항목 누락**

- **설명:** HARA OSR-HARA-001 §3 H-07 설명에서 "CPU 레지스터, RAM, Flash, MPU 설정 등에 결함"을 감지하지 못하는 경우가 위험원으로 기술되었다. FSC §3.5에서 도출된 SG-05 FSR:
  - FSR-05-01: ROM CRC 검증 (Flash 커버)
  - FSR-05-02: RAM March 테스트 (RAM 커버)
  - FSR-05-03: FreeRTOS 초기화 차단 (로직)
  - FSR-05-04: 리셋 원인 기록 (로직)
  - **CPU 레지스터 자가진단(Register File Test) FSR 없음.** ARM Cortex-M4/M7의 CPU 코어 레지스터(R0-R15, CPSR, 부동소수점 레지스터 등) 결함 검출 FSR이 도출되지 않음.
  - **MPU 설정 레지스터 자가진단 FSR 없음.** H-07 설명에 "MPU 설정"이 명시적으로 포함되어 있음에도 불구하고 부팅 시 MPU 설정 검증 FSR이 없음.
  - SAFE_STATE_DEFINITION.md §3.1에서 "부트 자가진단 실패 (ROM CRC, RAM March) → Level 3"으로만 분류되어 있어 CPU 레지스터 진단 응답도 명시 없음.
- **심각도: Minor**
- **조치:** Agent-Safety는 FSR-05-05(가칭)를 추가할 것: "부팅 시 CPU 코어 레지스터 자가진단(Register File Test)을 수행해야 한다." FSR-05-06(가칭): "부팅 시 MPU 설정 레지스터 예상값과 실제값 비교 검증을 수행해야 한다." 또는 FSR-05-01의 범위에 Flash + CPU 레지스터 + MPU 설정을 포함한다고 명시적으로 확장.

---

### 📋 권고사항 (Recommendations)

**REC-001: FSR → TSR 분해 계획 명시**
§6 미결 사항에서 TSC로 위임되는 항목이 6개임. FSC → TSC 이행 계획(예: 각 미결 FSR 파라미터의 TSC 결정 기한, 책임 에이전트)을 §6에 추가하면 Phase 2 계획 수립 시 입력으로 활용될 수 있음.

**REC-002: FSR 검증 방법(Verification Method) 열 추가**
§4 FSR 요약 테이블에 "검증 방법(Verification Method)" 열을 추가하여 각 FSR이 어떤 방법(단위 테스트, MPU 접근 시험, WCET 분석, 형식 증명 등)으로 검증될지 예시를 기재하면, Phase 3~4 Agent-VnV의 테스트 계획 수립을 용이하게 함.

**REC-003: FSR-02-03 구현 위치 명확화 필요**
FSR-02-03("SafetyFunction 전용 타이머/클럭은 QM이 수정 불가")의 구현 위치(HAL_Timer)만으로는 타이머 레지스터가 MPU로 보호되는지, 아니면 소프트웨어 API 접근 제어로만 보호되는지 불명확함. TSC 단계에서 "MPU peripheral region 설정"으로 하드웨어 보호를 명시할 것을 권고.

**REC-004: Mailbox 에러 카운터 초기화 정책 FSR 고려**
FSR-06-03에서 "오류 카운터 증가"를 요구하나, 오류 카운터의 초기화 조건(예: Level 1 복구 후 리셋, 통합자 명시 명령에 의해서만 가능)을 FSR 수준에서 언급하지 않음. 카운터가 임의로 초기화되면 FSR-04-02의 영구/일시 구분 메커니즘이 우회될 수 있음. TSC 단계에서 반영 권고.

---

## 최종 판정

[ ] 승인 (Approved)
[X] 조건부 승인 (Conditional Approval)
[ ] 반려 (Rejected)

**조건:**

1. **[Major — ISSUE-001]** H-05 Watchdog 타임아웃 설계 FSR-01-04 추가 (SG-01 커버리지 완성)
2. **[Major — ISSUE-002]** FSR-04-02 기능 수준 구분 알고리즘 프레임워크 추가 (파라미터만 TSC 위임)
3. **[Major — ISSUE-003]** FSR-06-04 추가 — 검증 실패 카운터 임계값 초과 시 Safe State 전이 요구
4. **[Major — ISSUE-004]** FSR-03-03에 인터럽트 불가 구간 최대 시간 제약 추가 (Watchdog kick 충돌 방지)
5. **[Minor — ISSUE-005]** Decomposition 독립성 표에 스택 가드(FSR-01-02 또는 FSR-02-05) 연결 명시
6. **[Minor — ISSUE-006]** SG-05 FSR에 CPU 레지스터 자가진단 및 MPU 설정 검증 항목 추가 또는 기존 FSR 범위 확장 명시

**조건 이행 후 Agent-QA 재검토 없이 승인 가능한 항목:** ISSUE-005, ISSUE-006 (범위 확장 또는 명시적 언급으로 처리 가능)

**Agent-QA 재검토 필요 항목:** ISSUE-001, ISSUE-002, ISSUE-003, ISSUE-004 (FSR 추가 및 SG 커버리지 변경이 있는 항목)

---

## 서명

| 역할 | 이름 | 날짜 |
|------|------|------|
| 검토자 (독립 QA) | Agent-QA | 2026-04-18 |
| 피검토자 | Agent-Safety | — (검토 결과 수령 대기) |

---

*본 검토는 ISO 26262 Part 2 Cl.8 Confirmation Measure — Confirmation Review 요건에 따라 수행되었다. 검토자(Agent-QA)는 피검토 문서 작성자(Agent-Safety)와 독립적이며, 검토 과정에서 작성자로부터 어떠한 지시도 받지 않았다.*
