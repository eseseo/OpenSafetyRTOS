# OpenSafetyRTOS — Architecture Design Document

**Version:** 0.1 (Draft)
**Status:** In Progress
**Target Standard:** ISO 26262 ASIL-D / IEC 61508 SIL-3

---

## 1. 프로젝트 철학

OpenSafetyRTOS는 **오픈소스 정신**으로 automotive safety-critical 환경에서 사용 가능한
RTOS를 만드는 것을 목표로 한다. 기존 RTOS를 처음부터 재작성하는 대신, 검증된 FreeRTOS
생태계를 QM 파티션으로 활용하고, 독립적인 Safety Function 레이어를 ASIL-D 수준으로
개발하는 **Decomposition 전략**을 채택한다.

---

## 2. 핵심 설계 전략: ISO 26262 Decomposition

ISO 26262 Part 9에서 허용하는 ASIL Decomposition 원칙을 따른다.

```
FreeRTOS        QM(D)          기존 RTOS 기능 (스케줄링, IPC, 드라이버)
SafetyFunction  ASIL-D(D)      안전 감시, MPU 강제, Fault 처리
─────────────────────────────────────────────────────
OpenSafetyRTOS  ASIL-D         전체 시스템 목표 ASIL
```

참고 사례: TTTech (Hypervisor ASIL-D + Guest OS QM), ETAS 동일 구조

---

## 3. 메모리 파티셔닝 및 FFI 원칙

Freedom From Interference(FFI)는 이 프로젝트의 **핵심 설계 요구사항**이다.
QM 파티션이 ASIL-D 파티션에 간섭할 수 없음을 MPU 하드웨어와 소프트웨어 검증으로 이중 보장한다.

### 3.1 메모리 영역 정의

```
┌──────────────────────────────────────────────────────────────────┐
│ 영역                  │ SafetyFunction 권한 │ FreeRTOS(QM) 권한  │
├──────────────────────────────────────────────────────────────────┤
│ ASIL-D Region         │ Read / Write        │ NO ACCESS          │
│ QM Region             │ Read Only           │ Read / Write       │
│ QM→Safety Mailbox     │ Read Only           │ Read / Write       │
└──────────────────────────────────────────────────────────────────┘
```

**불변 규칙:**
- QM(FreeRTOS)은 ASIL-D Region에 **절대 Write 불가** — MPU 하드웨어 강제
- SafetyFunction은 QM Region을 감시 목적으로 Read만 함
- SafetyFunction은 QM Region에 Write하지 않음 (Safety가 QM을 제어할 경우 별도 제어 채널 사용)

### 3.2 파티션 간 통신: Mailbox 패턴

QM 파티션이 SafetyFunction에 데이터를 전달해야 하는 예외적 경우, **Mailbox** 메커니즘을 사용한다.

```
 ┌──────────────┐         ┌─────────────────┐         ┌──────────────────┐
 │  FreeRTOS    │  Write  │  QM→Safety      │  Read   │  SafetyFunction  │
 │  (QM)        │────────▶│  Mailbox Region │────────▶│  (ASIL-D)        │
 └──────────────┘         └─────────────────┘         └──────────────────┘
                                                              │
                                              ┌───────────────┤
                                              ▼               ▼
                                        검증 통과         검증 실패
                                        데이터 사용       Fault 처리
```

**SafetyFunction의 Mailbox 수신 알고리즘:**

```c
safety_status_t safety_mailbox_receive(mailbox_t *mb, void *out, size_t len)
{
    /* ① CRC 검증 */
    if (crc32(mb->data, mb->data_len) != mb->crc) {
        return SAFETY_ERR_CRC_FAIL;
    }

    /* ② 타임스탬프 유효성 (Stale 데이터 감지) */
    if ((safety_get_tick() - mb->timestamp) > MAILBOX_MAX_AGE_MS) {
        return SAFETY_ERR_STALE;
    }

    /* ③ 값 범위 검증 */
    if (!safety_range_check(mb->data, mb->data_len, mb->schema)) {
        return SAFETY_ERR_RANGE;
    }

    /* 검증 통과 시 복사 */
    memcpy(out, mb->data, len);
    return SAFETY_OK;
}
```

SafetyFunction은 QM으로부터 수신한 데이터를 **절대 그냥 신뢰하지 않는다.**
이것이 소프트웨어 레벨 FFI 달성의 핵심이다.

---

## 4. 시스템 레이어 구조

```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                     │
├───────────────────────┬─────────────────────────────────┤
│   FreeRTOS (QM)       │   SafetyFunction (ASIL-D)       │
│  - Task Scheduling    │  - Watchdog Management          │
│  - IPC (Queue/Sem)    │  - MPU Configuration            │
│  - Device Drivers     │  - Task Deadline Monitor        │
│  - Memory Mgmt        │  - Fault Detection & Isolation  │
│                       │  - Safe State Manager           │
│                       │  - Mailbox Validation           │
├───────────────────────┴─────────────────────────────────┤
│              HAL (Hardware Abstraction Layer)            │
├─────────────────────────────────────────────────────────┤
│          ARM Cortex-M (Primary Target: M4/M7 + MPU)     │
└─────────────────────────────────────────────────────────┘
```

---

## 5. FFI 달성 메커니즘 요약

| 위협 시나리오                          | 대응 메커니즘                          |
|---------------------------------------|---------------------------------------|
| QM이 ASIL-D 메모리 덮어쓰기           | MPU — 하드웨어 차단 (접근 시 HardFault)|
| QM이 Mailbox에 잘못된 데이터 쓰기     | CRC + 범위 검증 — 소프트웨어 차단      |
| QM이 오래된 데이터 방치 (Stale)       | 타임스탬프 유효성 검사                 |
| QM 태스크가 Safety 태스크 굶기기       | Safety Function 최고 우선순위 고정     |
| QM 스택 오버플로우가 Safety 침범       | MPU 스택 가드 영역 설정               |

---

## 6. 타겟 플랫폼

| 플랫폼           | 상태       | 비고                            |
|-----------------|------------|---------------------------------|
| ARM Cortex-M4   | Primary    | MPU 8 region, FPU              |
| ARM Cortex-M7   | Primary    | MPU 16 region, Cache 포함      |
| RISC-V          | Future     | PMP(Physical Memory Protection)|

---

## 7. 인증 전략

- SafetyFunction 레이어: ASIL-D(D) 개발 프로세스 (MISRA-C, MC/DC, FMEA, 독립 검증)
- FreeRTOS: ISO 26262 Part 8 Cl.12 COTS 자격부여 + 사전 사용 이력 활용
- FFI 분석 문서: 본 문서를 기반으로 MPU 설정 정확성 + 인터럽트 경계 + 스택 분리 형식 증명
- Mailbox 검증 알고리즘: 단위 테스트 100% + MC/DC 커버리지 필수

---

## 8. 오픈소스 라이선스 전략

- FreeRTOS: MIT License 유지
- OpenSafetyRTOS SafetyFunction 레이어: Apache 2.0
- 인증 증거 문서: CC BY 4.0 (공개 공유)

---

*이 문서는 설계 진행에 따라 지속 업데이트됩니다.*
