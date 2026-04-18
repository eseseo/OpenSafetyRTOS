# QA 독립 리뷰 체크리스트 (Agent-QA)

**문서 ID:** OSR-QA-001
**적용 대상:** safety/* 및 kernel/safety/* 모든 PR

---

## 1. 코드 품질 (MISRA-C:2012)

- [ ] 필수(Mandatory) Rule 위반 없음
- [ ] 권장(Required) Rule 위반 — 정당화 문서 첨부됨
- [ ] 조언(Advisory) Rule 위반 — 팀 합의 기록 존재
- [ ] 정적 분석 도구 실행 결과 첨부됨 (PC-lint / Polyspace / Cppcheck)

## 2. 안전 요구사항 추적성

- [ ] 변경된 코드에 SSR(Safety Software Requirement) ID 주석 존재
- [ ] SSR → 코드 → 테스트 케이스 매핑 RTM(요구사항 추적 매트릭스) 업데이트됨
- [ ] 신규 기능의 경우 FMEA에 해당 Failure Mode 추가됨

## 3. FFI (Freedom From Interference) 검증

- [ ] QM 파티션이 ASIL-D 메모리 영역에 접근하는 코드 없음
- [ ] MPU 설정 변경 시 ADR 업데이트됨
- [ ] Mailbox 수신 시 CRC + 타임스탬프 + 범위 검증 3단계 모두 수행됨
- [ ] SafetyFunction이 QM 데이터를 검증 없이 직접 사용하는 경우 없음

## 4. 테스트 커버리지

- [ ] SafetyFunction 단위 테스트 MC/DC 100% 달성
- [ ] 테스트 결과 리포트 PR에 첨부됨
- [ ] 테스트 작성자(Agent-VnV)가 코드 작성자(Agent-Safety)와 다른 에이전트임 확인

## 5. 문서 완결성

- [ ] 설계 변경 시 ARCHITECTURE.md 또는 ADR 업데이트됨
- [ ] 공개 API 변경 시 헤더 주석 업데이트됨
- [ ] 리뷰 기록이 PR 코멘트로 남아있음 (인증 증거)

---

**QA 리뷰어 서명:** Agent-QA
**리뷰 날짜:** ___________
**결과:** [ ] 승인 / [ ] 조건부 승인 (조건: ________) / [ ] 반려
