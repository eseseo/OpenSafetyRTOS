# ADR-001: ASIL Decomposition 전략 채택

**날짜:** 2026-04-18
**상태:** 승인됨
**결정자:** 프로젝트 설계팀

---

## 컨텍스트

ISO 26262 ASIL-D를 만족하는 오픈소스 RTOS를 만들기 위해, 전체 커널을 처음부터
ASIL-D로 재개발하는 대신 기존 FreeRTOS 생태계를 최대한 활용하면서 Safety 목표를
달성하는 방법이 필요하다.

## 결정

**ISO 26262 Part 9 ASIL Decomposition 전략을 채택한다.**

- FreeRTOS: QM(D) 파티션 — 기존 RTOS 기능 담당
- SafetyFunction: ASIL-D(D) 파티션 — 안전 감시/강제 담당
- 둘의 조합으로 시스템 전체 ASIL-D 달성

## 파티션 간 통신 원칙

1. QM → ASIL-D 직접 Write: **금지** (MPU 하드웨어 강제)
2. QM → ASIL-D 통신 필요 시: **Mailbox 패턴** 사용
3. Mailbox 수신 시 SafetyFunction은 반드시 CRC + 타임스탬프 + 범위 검증 수행

## 채택 근거

- FreeRTOS 생태계(드라이버, 포팅, 커뮤니티) 재활용 가능
- SafetyFunction 레이어만 집중적으로 ASIL-D 개발 — 범위 최소화
- TTTech, ETAS의 동일 구조 선례 존재 — 인증 기관 수용성 높음
- 오픈소스로 SafetyFunction 레이어 공개 시 업계 기여 가능

## 거부된 대안

| 대안 | 거부 이유 |
|------|-----------|
| 커널 전체 ASIL-D 재개발 | 개발 비용/기간 과다, FreeRTOS 생태계 포기 |
| SafeRTOS 상용 구매 | 오픈소스 정신 위배, 비용, 소스 비공개 |
| AUTOSAR OS 채택 | 복잡도 과다, 오픈소스 생태계 부재 |

## 결과 및 트레이드오프

- (+) FreeRTOS 자산 활용, 개발 범위 집중
- (+) 인증 선례 존재
- (-) FFI 분석 문서화가 핵심 난관 — 반드시 설계 초기부터 병행
- (-) Mailbox 검증 로직의 ASIL-D 개발 부담

---

*다음 ADR: ADR-002 MPU 설정 전략*
