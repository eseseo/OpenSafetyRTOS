# 확인 검토 기록: OSR-CR-SSRS-002

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CR-SSRS-002 |
| 검토 대상 | OSR-SSRS-001 v1.1 (Software Safety Requirements Specification) |
| 이전 검토 | OSR-CR-SSRS-001 (v1.0 — 🟡 조건부 승인) |
| 검토 유형 | 재검토 (Re-review) — ISO 26262 Part 2 Cl.8 ISSUE-001 C1 조치 확인 |
| 검토자 | Agent-QA (Agent-Safety와 독립된 에이전트 — Part 2 독립성 요건) |
| 검토일 | 2026-04-19 |
| 검토 표준 | ISO 26262 Part 6 Cl.7 (Software Safety Requirements) |
| 입력 문서 | OSR-SSRS-001 v1.1, OSR-CR-SSRS-001, ADR-002 v1.0 |

---

## 1. 검토 범위

본 재검토는 OSR-CR-SSRS-001에서 제기된 모든 이슈 항목의 조치 결과를 확인한다:

- **ISSUE-001 (Major / C1)**: SSR-004-MPU 리전 번호 ADR-002 불일치 → 조치 확인
- **NI-001**: SSR-018-MBX 검증 방법 단위 테스트 부재 → 조치 확인
- **NI-002**: MISRA-C 규칙 누락 (Rule 11.3, 2.2, 8.4, 13.2) → 조치 확인

전체 SSRS 재검토 없이 조치 항목 중심으로 검토한다. 이전 검토에서 이슈 없이 확인된 SSR(SSR-001~003, SSR-005~017, SSR-019~027 중 해당 항목 제외)은 범위 외로 한다.

---

## 2. ISSUE-001 C1 조치 확인 — SSR-004-MPU 리전 번호

### 2.1 OSR-CR-SSRS-001 지적 사항 (재인용)

> SSR-004가 정의하는 8개 MPU 리전은 ADR-002 §"MPU Region Assignment Strategy" 표와 리전 번호가 충돌한다. 특히 ADR-002 Region 0(4 GB Deny-all background)가 SSR-004에 누락되어 있으며, 이로 인해 미매핑 주소 공간에 대한 Deny-all 보장이 깨진다.

### 2.2 v1.1 조치 내용 확인

**SSR-004-MPU v1.1 리전 테이블 (조치 후):**

| 리전 | v1.0 (오류) | v1.1 (수정) | ADR-002 일치 |
|------|------------|------------|------------|
| Region 0 | ASIL-D Code | **Default background (Deny-all 4 GB)** | ✅ |
| Region 1 | ASIL-D Data | **ASIL-D Code (Flash)** | ✅ |
| Region 2 | QM Code/Data | **ASIL-D Data (SRAM)** | ✅ |
| Region 3 | QM→Safety Mailbox | **ASIL-D Stack** | ✅ |
| Region 4 | Peripheral — Watchdog | **QM Region (Flash + SRAM)** | ✅ |
| Region 5 | Peripheral — Timer | **QM→Safety Mailbox** | ✅ |
| Region 6 | Stack Guard — SafetyFunction | **Safety Stack Guard (No Access)** | ✅ |
| Region 7 | Stack Guard — FreeRTOS | **Peripheral / Device** | ✅ |

**Region 0 Deny-all 근거 기술 여부**: ✅ 확인 — SSR-004 v1.1에 "ARM Cortex-M4 MPU 높은 번호 오버라이드 원리" 및 "미매핑 주소 Fault 처리" 설명 포함

**ADR-002와의 1:1 일치**: ✅ 완전 일치 확인

### 2.3 파생 정정 확인 (SSR-005/006/007)

SSR-004 리전 번호 수정으로 인해 파생 참조 정정 필요 — 확인 결과:

| SSR | v1.0 참조 | v1.1 참조 | 결과 |
|-----|----------|----------|------|
| SSR-005 | "Region 0, Region 1" | "Region 1, Region 2, Region 3" | ✅ 정정됨 |
| SSR-006 | "ASIL-D Code 리전(Region 0)" | "ASIL-D Code 리전(Region 1)" | ✅ 정정됨 |
| SSR-007 | "MPU Region 5(Peripheral — Safety Timer)" | "MPU Region 7(Peripheral / Device) 서브 리전" | ✅ 정정됨 |

**ISSUE-001 C1 조치 결과: ✅ 완전 해소**

---

## 3. NI-001 조치 확인 — SSR-018-MBX 검증 방법

### 3.1 OSR-CR-SSRS-001 지적 사항 (재인용)

> SSR-018의 검증 방법이 코드 리뷰 단독이다. ASIL-D 요건상 소프트웨어 계약(function contract) 준수는 동적 테스트로도 검증되어야 한다.

### 3.2 v1.1 조치 내용 확인

SSR-018 v1.1 검증 방법:
- ✅ 코드 리뷰 (기존) 유지
- ✅ 단위 테스트 추가: `safety_mailbox_receive()` 반환값 비-SAFETY_OK 시 `*out` 버퍼 미사용 경로 검증
- ✅ MC/DC 100% 요건 명시
- ✅ 테스트 파일 위치 명시: `tests/unit/test_mailbox_contract.c`

**NI-001 조치 결과: ✅ 해소**

---

## 4. NI-002 조치 확인 — MISRA-C 규칙 보완

### 4.1 OSR-CR-SSRS-001 지적 사항 (재인용)

> §5 MISRA-C 적용 원칙에서 Rule 11.3 (포인터 형변환), 2.2 (데드 코드), 8.4 (외부 연결 선언), 13.2 (식 부작용) 누락.

### 4.2 v1.1 조치 내용 확인

§5 MISRA-C 테이블 추가 규칙 확인:

| 규칙 | 추가 여부 | 적용 컨텍스트 기술 |
|------|---------|----------------|
| Rule 11.3 | ✅ | MPU 레지스터 `volatile` 포인터 접근 |
| Rule 2.2 | ✅ | 정적 분석 도구 기반 데드 코드 제거 |
| Rule 8.4 | ✅ | 함수 선언/정의 헤더 프로토타입 일치 |
| Rule 13.2 | ✅ | Mailbox 검증 조건식 부작용 분리 |

**NI-002 조치 결과: ✅ 해소**

---

## 5. 추가 검토 사항

### 5.1 SSR-004 Peripheral 통합 (ADR-002 반영)

v1.0에서 Watchdog과 Safety Timer를 별도 리전(Region 4, Region 5)으로 분리했던 방식이 v1.1에서 Region 7(Peripheral/Device) 단일 리전 내 서브 리전 개념으로 통합되었다. 이는 ADR-002와 일치하며, Cortex-M4 8-리전 제약 내에서 유효하다.

**신규 이슈 없음.**

### 5.2 RTM(요구사항 추적 매트릭스) 영향

SSR-004~007의 리전 번호 정정은 RTM §4에서 FSR → SSR 추적 관계에 영향을 주지 않는다 (SSR ID 변경 없음, 내용 수정만). 추적성 완전성 유지 확인: ✅

---

## 6. 검토 결론

| 항목 | 결과 |
|------|------|
| ISSUE-001 C1 (Major) | ✅ 완전 해소 |
| NI-001 | ✅ 해소 |
| NI-002 | ✅ 해소 |
| 신규 이슈 | 없음 |
| 전체 결론 | **✅ 최종 승인 (Final Approval)** |

**OSR-SSRS-001 v1.1은 ISO 26262 Part 6 Cl.7 요건을 충족하는 것으로 확인된다.**

---

## 7. 승인 서명

| 역할 | 서명 | 날짜 |
|------|------|------|
| 검토자 (Agent-QA) | Agent-QA | 2026-04-19 |
| Safety Manager 확인 | — (PM/PO 승인 필요) | — |

---

## 8. 다음 단계

- Safety Manager의 OSR-CR-SSRS-002 서명 → SSRS v1.1 최종 확정
- SAFETY_CASE.md E-G1.1-02 상태 업데이트: 🟡 → ✅
- Phase 1 완료 → Phase 2 (Kernel 구현) 개시 준비
- 인증 증거 문서 영문화 (Task #26) — SSRS 최종 확정 후 진행
