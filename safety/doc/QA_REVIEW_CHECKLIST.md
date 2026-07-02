# Confirmation Measures 체크리스트 (Agent-QA)

**문서 ID:** OSR-QA-001
**버전:** 1.1 (2026-07-02)
**근거:** ISO 26262-2:2018 Cl.6.4.9 (확인 조치 — 확인 검토 / 기능안전 감사 / 기능안전 평가)
**수행자:** Agent-QA (Agent-Safety와 별개 에이전트)

> **v1.1 정정 (조항 인용):** 구버전이 인용한 "Part 2 Cl.8/9/10"은 ISO 26262-2:2018의
> 실제 조항 번호가 아니다. 확인 조치는 **Cl.6.4.9**에 규정된다 (OSR-CR-ARCH-001 F-10).
>
> **v1.1 독립성 주석:** Agent-QA는 단일 유지관리자가 운영하는 AI 에이전트로, ISO 26262-2:2018
> Table 1의 인적/조직적 독립성(I2/I3)을 충족하지 못한다. 본 체크리스트에 따른 내부 검토는
> **검증 지원 조치(verification-support)** 이며, ISO 적합 확인 조치는 외부 ISA가 수행한다
> (docs/TEAM.md v0.5, OSR-ISSUE-005 참조).
>
> **v1.1 검토 ID 규칙 정렬:** 검토 기록 ID는 실사용 규칙 **`OSR-CR-<대상문서코드>-<회차 3자리>`**
> 를 따른다 (예: OSR-CR-HARA-001, OSR-CR-FSC-003). 구버전의 "OSR-CR-001~007" 일련번호
> 방식은 폐기한다 (CM_PLAN v1.1 §5.1 참조).

---

## Confirmation Measure 1: 확인 검토 (Confirmation Review) — Cl.6.4.9

Agent-Safety가 작성한 SafetyCase 산출물을 단계별로 독립 검토한다.

### HARA 검토 (OSR-CR-HARA-NNN)
- [ ] 운영 상황(Operational Situation) 목록이 실제 차량 사용 환경을 충분히 커버함
- [ ] 각 위험원(Hazard)에 대해 최악 조건(Worst Case)이 고려됨
- [ ] ASIL 등급 산정 근거 (S/E/C 매트릭스)가 문서화됨
- [ ] ASIL-D 이상 항목에 대해 안전 목표(Safety Goal)가 명확히 정의됨
- [ ] 안전 목표가 검증 가능한 형태로 기술됨

### FSC 검토 (OSR-CR-FSC-NNN)
- [ ] 각 안전 목표가 기능안전 요구사항으로 분해됨
- [ ] Decomposition 전략(QM + ASIL-D)이 FSC에 반영됨
- [ ] FFI 달성 수단이 기능 수준에서 명시됨
- [ ] 각 FSR → HARA 안전 목표 추적 가능

### TSC 검토 (OSR-CR-TSC-NNN)
- [ ] ARCHITECTURE.md와 TSC 내용이 정합함
- [ ] QM/ASIL-D 파티션 경계가 명시됨
- [ ] HW/SW 인터페이스 기술됨
- [ ] 각 TSR → FSR 추적 가능

### SSRS 검토 (OSR-CR-SSRS-NNN)
- [ ] 모든 SSR에 고유 ID (SSR-NNN) 부여됨
- [ ] 각 SSR → TSR 추적 가능
- [ ] SSR이 구현·테스트 가능한 형태로 기술됨
- [ ] Mailbox 3단계 검증 SSR 존재
- [ ] MPU 파티션 보호 SSR 존재

### FMEA 검토 (OSR-CR-FMEA-NNN)
- [ ] 모든 SafetyFunction 모듈의 Failure Mode 식별됨
- [ ] 완화 조치가 SSR과 연결됨
- [ ] Single Point Failure 대응 명시됨

### FFI 분석 검토 (OSR-CR-FFIA-NNN)
- [ ] QM→ASIL-D 모든 간섭 경로 식별됨 (메모리/인터럽트/타이밍/전원)
- [ ] MPU 설정이 각 경로를 차단함 명시됨
- [ ] HardFault → Safe State 전이 증명됨

### V&V 결과 검토 (OSR-CR-VNV-NNN)
- [ ] SafetyFunction MC/DC 100% 달성
- [ ] 테스트 작성자 = Agent-VnV (Agent-Safety 아님) 확인
- [ ] 모든 SSR에 대응 테스트 케이스 존재 (RTM)

---

## Confirmation Measure 2: 기능안전 감사 (Functional Safety Audit) — Cl.6.4.9

기능안전 프로세스가 Safety Plan대로 수행되고 있는지 감사한다.
산출물의 내용이 아닌 **프로세스 준수 여부**를 확인.

| 감사 항목 | 확인 질문 | 결과 |
|----------|----------|------|
| Safety Plan 이행 | 각 Phase 활동이 SAFETY_PLAN.md 계획대로 수행됨? | |
| 산출물 존재 | 각 Phase 종료 시 필수 산출물이 git에 커밋됨? | |
| 리뷰 이력 | 모든 safety/* PR에 Agent-QA 승인 기록 존재? | |
| 트레이스 | SSR → 코드 → 테스트 연결이 RTM에 기록됨? | |
| MISRA 준수 | 정적 분석 결과가 PR에 첨부됨? | |
| 변경 관리 | 설계 변경 시 ADR 작성됨? | |
| 독립성 | Agent-Safety ≠ Agent-VnV ≠ Agent-QA 분리 유지됨? | |

**감사 주기:** 각 Phase 종료 시
**감사 기록:** safety/doc/audits/OSR-FSA-NNN.md

---

## Confirmation Measure 3: 기능안전 평가 (Functional Safety Assessment) — Cl.6.4.9

"이 시스템이 의도한 안전 수준(ASIL-D)을 실제로 달성하였는가?"를 종합 평가.
Phase 5(인증 준비) 단계에서 수행.

| 평가 항목 | 평가 기준 |
|----------|----------|
| SafetyCase 완결성 | 모든 안전 목표에 대한 논증과 증거가 갖춰져 있는가 |
| FFI 달성 | QM 파티션이 ASIL-D 파티션에 간섭할 수 없음이 증명되었는가 |
| 잔여 위험 | 식별된 모든 위험원에 대한 완화 조치가 충분한가 |
| 프로세스 준수 | ISO 26262 Part 2/6/8 요건이 전체적으로 이행되었는가 |
| V&V 충분성 | 테스트 커버리지와 범위가 ASIL-D 요건을 충족하는가 |

**결과:** [ ] 적합 / [ ] 조건부 적합 (조건: ___) / [ ] 부적합

---

## 리뷰 이력 추적

| 문서 ID | 대상 산출물 | Measure | 날짜 | 결과 | 지적사항 |
|--------|-----------|---------|------|------|---------|
| OSR-CR-HARA-001 | OSR-HARA-001 v1.0 | 확인 검토 (CM1) | 2026-04-18 | 조건부 승인 | Major 4 (ISSUE-001~004), Minor 2 (ISSUE-005~006) |
| OSR-CR-FSC-001 | OSR-FSC-001 v1.0.0 | 확인 검토 (CM1) | 2026-04-18 | 조건부 승인 | Major 4 (ISSUE-001~004), Minor 2 (ISSUE-005~006) |
| OSR-CR-HARA-002 | OSR-HARA-001 v1.1 | 재확인 검토 (CM1) | 2026-04-19 | 조건부 승인 | 선행 Major 4건 해소 확인; 신규 Minor 4 (HARA-NI-001~004), 정보 1 (HARA-NI-005) |
| OSR-CR-FSC-002 | OSR-FSC-001 v1.1.0 | 재확인 검토 (CM1) | 2026-04-19 | 조건부 승인 | 선행 Major 4건 해소; 신규 Major 1 (FSC-NI-001: SG-07/08 FSR 부재 — 조건 C1/C2) |
| OSR-CR-FSC-003 | OSR-FSC-001 v1.2 | 재확인 검토 (CM1) | 2026-04-19 | 최종 승인 (내부) | C1/C2 해소 확인; Minor 1 (NI-001: FSR-04-01 ASIL 표기). ※ C2 수량 완화 무근거 — OSR-ISSUE-004 (Open) |
| OSR-CR-SSRS-001 | OSR-SSRS-001 v1.0 | 확인 검토 (CM1) | 2026-04-19 | 조건부 승인 | Major 1 (ISSUE-001: SSR-004 MPU 리전 ADR-002 불일치), NI 2 |
| OSR-CR-SSRS-002 | OSR-SSRS-001 v1.1 | 재검토 (CM1) | 2026-04-19 | 최종 승인 (내부) | ISSUE-001/NI-001/NI-002 전건 해소 확인 |
| OSR-CR-HARA-003 | OSR-HARA-001 v1.3 | 종결 검토 (CM1) | 2026-07-02 | 승인 (외부 ISA 확인 검토 조건부) | 선행 검토의 Table 4 계산 오류 4건 정정 (H-05/H-07/H-09/H-10 — OSR-ISSUE-002); v1.3 조치(D3/D6/D7 및 ②④⑤⑥⑦) 확인 |
| OSR-CR-ARCH-001 | 아키텍처 및 교차 문서 일관성 (2026-07-02 변경 세트) | 통합 검토 기록 (Cl.6.4.9 구조 준용) | 2026-07-02 | 기록 완료 | F-01~F-14; 처분: ADR-005, OSR-AOU-001, 문서 버전 개정, OSR-ISSUE-001~005 |

> 모든 내부 검토는 sub-I2 검증 지원 조치 — 외부 ISA 소급 확인 검토 대상 (OSR-ISSUE-005).

---

## 개정 이력

| 버전 | 날짜 | 변경 내용 |
|------|------|---------|
| 1.0 | 2026-04-18 | 최초 작성 |
| 1.1 | 2026-07-02 | 조항 인용 정정 (Cl.8/9/10 → ISO 26262-2:2018 Cl.6.4.9); 검토 ID 규칙을 실사용 형식(OSR-CR-<문서>-NNN)으로 정렬; 독립성 주석 추가 (검증 지원 조치 재분류); 리뷰 이력 추적 표에 완료된 검토 9건 기입 (OSR-CR-ARCH-001 F-09/F-10/F-11 반영) |
