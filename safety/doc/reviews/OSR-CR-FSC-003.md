# 확인 검토 기록: OSR-CR-FSC-003

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CR-FSC-003 |
| 검토 대상 | OSR-FSC-001 v1.2 (Functional Safety Concept) |
| 검토 유형 | 확인 검토 (Confirmation Review) — ISO 26262 Part 2 Cl.8 |
| 검토자 | Agent-QA (Agent-Safety와 독립된 에이전트 — Part 2 독립성 요건) |
| 검토일 | 2026-04-19 |
| 선행 검토 | OSR-CR-FSC-001 (Major 4건), OSR-CR-FSC-002 (조건부 승인 — Major C1/C2 해소 조건) |
| 검토 목적 | FSC v1.2에서 OSR-CR-FSC-002 Major 조건(C1: SG-07 FSR 신설, C2: SG-08 FSR 신설)이 완전히 해소되었는지 확인 |

---

## 1. 검토 범위 및 중점 사항

OSR-CR-FSC-002에서 조건부 승인된 FSC v1.1에 대해, Agent-Safety가 FSC v1.2에서 다음 조치를 수행했다:

| 조건 | 요구 조치 | FSC v1.2 대응 |
|------|---------|-------------|
| C1 (Major) | SG-07 FSR 섹션(§3.7) 신설 | §3.7 FSR-07-01/02/03 추가 |
| C2 (Major) | SG-08 FSR 섹션(§3.8) 신설 | §3.8 FSR-08-01/02/03 추가 |

본 검토는 C1/C2 해소 여부 확인을 **1차 목적**으로 하되, 전체 문서의 정합성도 아울러 검토한다.

---

## 2. C1 해소 확인: SG-07 FSR 섹션(§3.7) 검토

### 2.1 FSR-07-01 — SafetyFunction 전용 타이머 MPU 보호

| 검토 항목 | 결과 | 비고 |
|----------|------|------|
| SG-07 안전 목표 직접 대응 여부 | ✅ 충족 | QM 타이머 레지스터 접근 MPU 차단 — H-09 위험원 대응 |
| FSR-02-03(QM 타이머 수정 불가)과의 연계 명시 | ✅ 충족 | §3.7 FSR-07-01 근거에 연계 및 확장 관계 기술 |
| AoU-04 연계 명시 | ✅ 충족 | "AoU-04 연계: SafetyFunction 전용 클럭 소스 제공은 통합자 의무" |
| 구현 위치 타당성 | ✅ 적절 | arch/arm-cortex-m/src/mpu.c + HAL timer 초기화 |
| ASIL 표기 | ✅ 적절 | C (개발 수준: D — over-fulfillment) |

### 2.2 FSR-07-02 — 클럭 이상 능동 감지 메커니즘

| 검토 항목 | 결과 | 비고 |
|----------|------|------|
| 능동 감지 요구사항 구체화 여부 | ✅ 충족 | 주파수 편차 > ±5%, 독립 RC 오실레이터 또는 Watchdog 교차 검증 명시 |
| 감지 임계값 근거 | ✅ 충족 | "타이밍 보장 체계의 10% 오차 허용 범위 이내" — 합리적 근거 제시. §6 미결항목 7에서 TSC 검증 위임 명시 |
| 구현 위치 타당성 | ✅ 적절 | kernel/safety/src/clock_monitor.c (신규 모듈) |
| ASIL 표기 | ✅ 적절 | C (개발 수준: D — over-fulfillment) |

### 2.3 FSR-07-03 — 클럭 이상 감지 시 Safe State 전이

| 검토 항목 | 결과 | 비고 |
|----------|------|------|
| SG-07 안전 목표 직접 이행 | ✅ 충족 | Safe State Level 2 이상으로 전이 명시 |
| SAFE_STATE_DEFINITION.md 전이 조건 연계 | ✅ 충족 | §3.1 전이 조건 매트릭스 참조 기술 |
| 에스컬레이션 경로(Level 2 → Level 3) 명시 | ✅ 충족 | "회복 불가 판정 시 Level 3으로 에스컬레이션" |
| 구현 위치 타당성 | ✅ 적절 | kernel/safety/src/fault.c |
| ASIL 표기 | ✅ 적절 | C (개발 수준: D — over-fulfillment) |

**SG-07 FSR 섹션(§3.7) 검토 결론: ✅ C1 완전 해소 확인**

---

## 3. C2 해소 확인: SG-08 FSR 섹션(§3.8) 검토

### 3.1 FSR-08-01 — 전원 이상 감지 인터페이스

| 검토 항목 | 결과 | 비고 |
|----------|------|------|
| H-10(전원 이상 → MPU 설정 소실 → FFI 붕괴) 대응 첫 단계 | ✅ 충족 | BOD 인터럽트 SafetyFunction 전용 할당 명시 |
| AoU-08 연계 명시 | ✅ 충족 | "전원 이상 감지 회로 및 Brown-out 보호는 통합자 하드웨어 설계 의무" |
| 조기 감지(MPU 설정 소실 전) 논리 | ✅ 충족 | "MPU 설정 소실 전에 대응 절차를 개시할 수 있도록" |
| 구현 위치 타당성 | ✅ 적절 | kernel/safety/src/fault.c (BOD 인터럽트 핸들러) |
| ASIL 표기 | ✅ 적절 | C (개발 수준: D — over-fulfillment) |

### 3.2 FSR-08-02 — 전원 복구 후 MPU 무결성 검증

| 검토 항목 | 결과 | 비고 |
|----------|------|------|
| FFI 보호 없이 재기동 방지 논리 | ✅ 충족 | MPU 레지스터 실제값 vs 예상값 비교, 불일치 시 시작 차단 |
| Level 3 유지 조건 명시 | ✅ 충족 | "검증 실패 시 시스템 시작 차단 및 Safe State Level 3 유지" |
| 부팅 시퀀스 내 검증 위치 타당성 | ✅ 적절 | arch/arm-cortex-m/src/mpu.c 부팅 시퀀스 — MPU 초기화 직후 수행 |
| ASIL 표기 | ✅ 적절 | C (개발 수준: D — over-fulfillment) |

### 3.3 FSR-08-03 — Brown-out Reset 이력 기록

| 검토 항목 | 결과 | 비고 |
|----------|------|------|
| 감사 추적(Audit Trail) 요구사항 | ✅ 충족 | 비휘발성 메모리 기록 + Safety Manager 통보 인터페이스 |
| FSR-05-04(리셋 원인 기록)와의 관계 명시 | ✅ 충족 | "FSR-05-04와 연계하되 Brown-out 특화 기록 및 통보 인터페이스로 확장" |
| 구현 위치 타당성 | ✅ 적절 | kernel/safety/src/reset_cause.c |
| ASIL 표기 | ✅ 적절 | C (개발 수준: D — over-fulfillment) |

**SG-08 FSR 섹션(§3.8) 검토 결론: ✅ C2 완전 해소 확인**

---

## 4. 전체 FSR 체계 정합성 검토

### 4.1 SG → FSR 추적성 (Traceability) 확인

| 안전 목표 | ASIL | FSR 수 | FSR IDs | 추적 결과 |
|---------|------|--------|---------|----------|
| SG-01 | D | 4 | FSR-01-01~04 | ✅ |
| SG-02 | D | 4 | FSR-02-01~04 | ✅ |
| SG-03 | D | 3 | FSR-03-01~03 | ✅ |
| SG-04 | D | 2 | FSR-04-01~02 | ✅ |
| SG-05 | C | 4 | FSR-05-01~04 | ✅ |
| SG-06 | D | 4 | FSR-06-01~04 | ✅ |
| SG-07 | C | 3 | FSR-07-01~03 | ✅ (신규 v1.2) |
| SG-08 | C | 3 | FSR-08-01~03 | ✅ (신규 v1.2) |
| **합계** | | **27** | | **✅ 전 SG 커버** |

SG-01~SG-08 모두 FSR로 분해됨. 추적성 완전.

### 4.2 FSR ASIL ≥ 근거 SG ASIL 확인

| 근거 SG | SG ASIL | FSR-xx-xx | FSR 표기 ASIL | 충족 여부 |
|---------|---------|-----------|-------------|----------|
| SG-01 | D | FSR-01-xx | D | ✅ |
| SG-02 | D | FSR-02-xx | D | ✅ |
| SG-03 | D | FSR-03-xx | D | ✅ |
| SG-04 | D | FSR-04-01 | §3.4: B (개발:D) / §4 테이블: D | ⚠️ NI-001 |
| SG-04 | D | FSR-04-02 | D | ✅ |
| SG-05 | C | FSR-05-xx | C (개발:D) | ✅ |
| SG-06 | D | FSR-06-xx | D | ✅ |
| SG-07 | C | FSR-07-xx | C (개발:D) | ✅ |
| SG-08 | C | FSR-08-xx | C (개발:D) | ✅ |

### 4.3 Over-fulfillment 처리 (ISO 26262 Part 8 Cl.5) 적합성

- §5.2에 "ISO 26262 Part 8 Cl.5에 따라 허용된다" 명시 — ✅ 적절
- SG-05(ASIL-C) → ASIL-D 수준 개발: §5.2에 기술 — ✅
- SG-07(ASIL-C) → ASIL-D 수준 개발: §5.2에 기술 — ✅
- SG-08(ASIL-C) → ASIL-D 수준 개발: §5.2에 기술 — ✅

---

## 5. 지적 사항

### Major 이슈: 없음

OSR-CR-FSC-002의 Major 조건 C1(SG-07 FSR 신설)과 C2(SG-08 FSR 신설)가 FSC v1.2에서 완전히 해소되었음을 확인한다.

---

### Minor 이슈 (NI)

#### NI-001: FSR-04-01 ASIL 표기 불일치

| 항목 | 내용 |
|------|------|
| 이슈 ID | NI-001 |
| 위치 | §3.4 FSR-04-01 본문, "ASIL: B (ASIL-D로 개발, 상위 호환)" |
| 불일치 내용 | §4 FSR 요약 테이블에는 FSR-04-01 ASIL이 "D"로 표기됨. 그러나 §3.4 본문에는 "ASIL: B (ASIL-D로 개발, 상위 호환)"로 기술되어 있어 문서 내 일관성이 없음. |
| 원인 분석 | FSC v1.1에서 SG-04 ASIL이 B→D로 상향될 때(FSC-ISSUE-002 처리 중) FSR-04-01 본문 ASIL 표기가 같이 갱신되지 않은 것으로 추정. |
| 요구 조치 | §3.4 FSR-04-01 본문의 ASIL 표기를 "D (v1.1에서 SG-04 ASIL-D 상향에 따라 갱신)"으로 수정. §4 테이블(이미 D로 표기)과 일치시킬 것. |
| 긴급도 | Low — 실질적 ASIL 등급(D)은 §4 테이블 및 §5.2 기술이 정확하므로 안전 논증에 영향 없음. SSRS 착수를 블록하지 않음. |

---

## 6. 조치 요약 (C1~C6 체계)

| 조치 ID | 유형 | 위치 | 내용 | 긴급도 |
|--------|------|------|------|--------|
| C1 (FSC-003) | NI | §3.4 FSR-04-01 본문 ASIL 표기 | "B" → "D"로 수정 (§4 테이블과 일치) | Low |

---

## 7. 최종 결론

### OSR-CR-FSC-003 결론: ✅ 최종 승인

| 판정 기준 | 결과 |
|---------|------|
| OSR-CR-FSC-002 Major 조건 C1 해소 여부 | ✅ 완전 해소 — SG-07 §3.7 FSR-07-01/02/03 신설 확인 |
| OSR-CR-FSC-002 Major 조건 C2 해소 여부 | ✅ 완전 해소 — SG-08 §3.8 FSR-08-01/02/03 신설 확인 |
| 전체 SG(01~08) FSR 추적성 완전성 | ✅ 27개 FSR, 8개 SG 전부 커버 |
| FSR ASIL ≥ SG ASIL | ✅ (NI-001: 표기 불일치만 존재, 실질 ASIL 등급 정확) |
| ISO 26262 Part 8 Cl.5 over-fulfillment 처리 | ✅ §5.2에 명시 |
| Decomposition 전략 (Part 9) 연계 | ✅ §5.1/5.2 기술 |
| SEooC 컨텍스트 및 AoU 위임 | ✅ §1.3, 각 FSR 근거 내 AoU 연계 |

**OSR-FSC-001 v1.2는 ISO 26262 Part 3 Cl.8 기능안전 개념 요구사항을 충족하는 것으로 최종 승인한다.**

잔존 Minor 이슈(NI-001: FSR-04-01 ASIL 표기 불일치)는 FSC v1.3 또는 SSRS 작성 중 반영이 가능하며, SSRS(OSR-SSRS-001) 착수를 블록하지 않는다.

---

## 8. 후속 조치

| 항목 | 담당 | 우선순위 |
|------|------|--------|
| FSC v1.3: NI-001 정정 (FSR-04-01 ASIL 표기 B→D) | Agent-Safety | Low (SSRS 착수 후 병행 가능) |
| SSRS(OSR-SSRS-001) 착수 | Agent-Safety | **High — 즉시 착수 가능** |
| SAFETY_CASE.md OP-003 → 🟢 최종 승인으로 갱신 | Agent-QA | High |

---

## 9. 검토자 확인

| 역할 | 에이전트 | 검토일 |
|------|---------|--------|
| Agent-QA (Confirmation Reviewer) | Agent-QA | 2026-04-19 |
| 독립성 확인 | Agent-Safety ≠ Agent-QA — ISO 26262 Part 2 독립성 요건 충족 | ✅ |

---

*본 확인 검토 기록은 ISO 26262 Part 2 Cl.8에 따라 작성된 공식 Confirmation Review 문서입니다.*
*Agent-Safety와 독립된 Agent-QA가 수행하였으며, 이 문서는 인증 증거 패키지의 일부입니다.*
