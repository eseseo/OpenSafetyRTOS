# Unit Tests — Agent-VnV 담당

**ASIL 요건:** SafetyFunction 관련 테스트는 MC/DC 커버리지 100% 필수

## 테스트 대상 (Phase 3 이후 추가 예정)

| 모듈 | 테스트 케이스 | 커버리지 목표 |
|------|-------------|-------------|
| Mailbox CRC 검증 | 정상 / CRC 불일치 / 빈 데이터 | MC/DC 100% |
| Mailbox 타임스탬프 | 유효 / Stale (초과) / 경계값 | MC/DC 100% |
| Mailbox 범위 검증 | 정상 / 하한 위반 / 상한 위반 | MC/DC 100% |
| Watchdog | kick 정상 / timeout / 이중 kick | MC/DC 100% |
| MPU 설정 | 각 region 속성 검증 | Statement 100% |
| Fault Handler | 각 Fault 타입 → Safe State 전이 | MC/DC 100% |

## 독립성 원칙

- 테스트 작성자(Agent-VnV) ≠ 코드 작성자(Agent-Safety)
- 테스트 결과는 Agent-QA가 최종 확인 후 인증 증거로 보관
