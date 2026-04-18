# Confirmation Measures 체크리스트 (Agent-QA)

**문서 ID:** OSR-QA-001
**근거:** ISO 26262 Part 2 Cl.8 (확인 검토), Cl.9 (기능안전 감사), Cl.10 (기능안전 평가)
**수행자:** Agent-QA (Agent-Safety와 독립된 에이전트 — Part 2 독립성 요건)

---

## Confirmation Measure 1: 확인 검토 (Confirmation Review) — Part 2 Cl.8

Agent-Safety가 작성한 SafetyCase 산출물을 단계별로 독립 검토한다.

### HARA 검토 (OSR-CR-001)
- [ ] 운영 상황(Operational Situation) 목록이 실제 차량 사용 환경을 충분히 커버함
- [ ] 각 위험원(Hazard)에 대해 최악 조건(Worst Case)이 고려됨
- [ ] ASIL 등급 산정 근거 (S/E/C 매트릭스)가 문서화됨
- [ ] ASIL-D 이상 항목에 대해 안전 목표(Safety Goal)가 명확히 정의됨
- [ ] 안전 목표가 검증 가능한 형태로 기술됨

### FSC 검토 (OSR-CR-002)
- [ ] 각 안전 목표가 기능안전 요구사항으로 분해됨
- [ ] Decomposition 전략(QM + ASIL-D)이 FSC에 반영됨
- [ ] FFI 달성 수단이 기능 수준에서 명시됨
- [ ] 각 FSR → HARA 안전 목표 추적 가능

### TSC 검토 (OSR-CR-003)
- [ ] ARCHITECTURE.md와 TSC 내용이 정합함
- [ ] QM/ASIL-D 파티션 경계가 명시됨
- [ ] HW/SW 인터페이스 기술됨
- [ ] 각 TSR → FSR 추적 가능

### SSRS 검토 (OSR-CR-004)
- [ ] 모든 SSR에 고유 ID (SSR-NNN) 부여됨
- [ ] 각 SSR → TSR 추적 가능
- [ ] SSR이 구현·테스트 가능한 형태로 기술됨
- [ ] Mailbox 3단계 검증 SSR 존재
- [ ] MPU 파티션 보호 SSR 존재

### FMEA 검토 (OSR-CR-005)
- [ ] 모든 SafetyFunction 모듈의 Failure Mode 식별됨
- [ ] 완화 조치가 SSR과 연결됨
- [ ] Single Point Failure 대응 명시됨

### FFI 분석 검토 (OSR-CR-006)
- [ ] QM→ASIL-D 모든 간섭 경로 식별됨 (메모리/인터럽트/타이밍/전원)
- [ ] MPU 설정이 각 경로를 차단함 명시됨
- [ ] HardFault → Safe State 전이 증명됨

### V&V 결과 검토 (OSR-CR-007)
- [ ] SafetyFunction MC/DC 100% 달성
- [ ] 테스트 작성자 = Agent-VnV (Agent-Safety 아님) 확인
- [ ] 모든 SSR에 대응 테스트 케이스 존재 (RTM)

---

## Confirmation Measure 2: 기능안전 감사 (Functional Safety Audit) — Part 2 Cl.9

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

## Confirmation Measure 3: 기능안전 평가 (Functional Safety Assessment) — Part 2 Cl.10

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
| | | | | | |
