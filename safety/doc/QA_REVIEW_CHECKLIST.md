# SafetyCase 내부 리뷰 체크리스트 (Agent-QA)

**문서 ID:** OSR-QA-001
**역할:** Internal SafetyCase Reviewer (ISO 26262 Part 2)
**원칙:** Agent-Safety가 작성한 산출물을 Agent-QA가 독립 리뷰. 이 기록은 인증 증거.

---

## 리뷰 워크플로우

```
Agent-Safety 산출물 작성
        ↓
Agent-QA 독립 리뷰 (이 체크리스트)
        ↓
지적사항 → Agent-Safety 수정
        ↓
Agent-QA 재확인 → 승인
        ↓
리뷰 기록 safety/doc/reviews/ 에 보관
```

---

## HARA 리뷰 (OSR-QA-HARA)

- [ ] 운영 상황(Operational Situation) 목록이 실제 차량 사용 환경을 충분히 커버함
- [ ] 각 위험원(Hazard)에 대해 최악 조건(Worst Case)이 고려됨
- [ ] ASIL 등급 산정 근거 (S/E/C 매트릭스)가 문서화됨
- [ ] ASIL-D 이상 항목에 대해 안전 목표(Safety Goal)가 명확히 정의됨
- [ ] 안전 목표가 검증 가능한 형태로 기술됨 (모호한 표현 없음)

## FSC 리뷰 (OSR-QA-FSC)

- [ ] 각 안전 목표가 하나 이상의 기능안전 요구사항으로 분해됨
- [ ] Decomposition 전략(FreeRTOS QM + SafetyFunction ASIL-D)이 FSC에 반영됨
- [ ] FFI 달성 수단이 기능 수준에서 명시됨 (MPU 파티셔닝, Mailbox 패턴)
- [ ] 각 FSR이 HARA 안전 목표로 추적 가능함

## TSC 리뷰 (OSR-QA-TSC)

- [ ] 시스템 아키텍처(ARCHITECTURE.md)와 TSC 내용이 정합함
- [ ] QM 파티션과 ASIL-D 파티션 경계가 TSC에 명시됨
- [ ] HW/SW 인터페이스 (MPU 설정, 인터럽트 벡터)가 TSC에 기술됨
- [ ] 각 TSR이 FSR로 추적 가능함

## SSRS 리뷰 (OSR-QA-SSRS)

- [ ] 모든 SSR에 고유 ID (SSR-NNN) 부여됨
- [ ] 각 SSR이 TSR로 추적 가능함
- [ ] SSR이 구현 가능하고 테스트 가능한 형태로 기술됨
- [ ] SafetyFunction의 응답 시간 요구사항이 수치로 명시됨
- [ ] Mailbox CRC/타임스탬프/범위 검증에 대한 SSR이 존재함
- [ ] MPU 파티션 보호에 대한 SSR이 존재함

## FMEA / 안전 분석 리뷰 (OSR-QA-FMEA)

- [ ] 모든 SafetyFunction 모듈에 대한 Failure Mode가 식별됨
- [ ] 각 Failure Mode의 완화 조치가 SSR과 연결됨
- [ ] Single Point Failure에 대한 대응이 명시됨
- [ ] FMEA 결과가 아키텍처 설계와 모순되지 않음

## FFI 분석 리뷰 (OSR-QA-FFI)

- [ ] QM→ASIL-D 모든 간섭 경로가 식별됨 (메모리, 인터럽트, 타이밍, 전원)
- [ ] MPU Region 설정이 각 간섭 경로를 차단함이 명시됨
- [ ] Mailbox 검증 3단계(CRC/타임스탬프/범위)가 잔여 간섭 경로를 처리함
- [ ] HardFault 발생 시 SafetyFunction이 Safe State로 전이함이 증명됨

## 코드 리뷰 (OSR-QA-CODE) — kernel/safety/ 전용

- [ ] MISRA-C:2012 필수(Mandatory) Rule 위반 없음
- [ ] 각 함수에 SSR ID 주석 존재
- [ ] 정적 분석 결과 첨부됨
- [ ] 모든 분기 조건에 대한 테스트 케이스가 Agent-VnV에 의해 작성됨

## V&V 결과 리뷰 (OSR-QA-VNV)

- [ ] SafetyFunction 단위 테스트 MC/DC 100% 달성 확인
- [ ] 테스트 작성자가 Agent-VnV임을 확인 (Agent-Safety 작성 금지)
- [ ] 모든 SSR에 대응하는 테스트 케이스 존재 (RTM 확인)
- [ ] 실패한 테스트가 없음 (또는 waiver 존재)

---

## 리뷰 이력

| 리뷰 ID | 대상 산출물 | 날짜 | 결과 | 지적사항 수 | 비고 |
|--------|-----------|------|------|-----------|------|
| (Phase 진행 시 작성) | | | | | |

---

**리뷰어:** Agent-QA (Independent)
**작성자와 동일 에이전트 여부:** 반드시 No
