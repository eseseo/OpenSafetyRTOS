# Unit Tests — Agent-VnV 담당

**ASIL 요건:** SafetyFunction 관련 테스트는 MC/DC 커버리지 100% 필수 (ISO 26262-6 Table 10)

## 빌드/실행 방법 — 독립(standalone) 호스트 프로젝트

본 디렉터리는 **자체 `project()`를 가진 독립 CMake 프로젝트**이다. 메인 빌드 트리는
arm-none-eabi 크로스 컴파일 대상이므로, 호스트에서 실행되어야 하는 단위 테스트는
메인 트리에 포함되지 않는다 (tests/CMakeLists.txt 참조).

```bash
# 저장소 루트에서:
cmake -S tests/unit -B build-host
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

테스트는 타깃 빌드와 **동일한 소스**(kernel/safety/src, arch/arm-cortex-m/src)를
호스트 gcc로 컴파일한다. 하드웨어 접근은 ops 구조체 주입으로 분리되어 있고,
타깃 전용 코드(레지스터 백엔드, 예외 진입점)는 `__arm__` 가드로 제외된다.

## 현재 테스트 (구현 완료)

| 테스트 파일 | 대상 모듈 | 검증 내용 | SSR |
|------|-------------|-------------|-----|
| test_crc32.c | kernel/safety/src/crc32.c | IEEE 802.3 known-answer 벡터("123456789"→0xCBF43926 등), 증분/일괄 일치, 단일 비트 민감도 | SSR-015-MBX |
| test_mailbox.c | kernel/safety/src/mailbox.c | 정상 수신 / CRC 불일치 / Stale 경계값(MAX_AGE, MAX_AGE+1) / 범위 위반 / 길이 초과 / 3회 연속 실패 → Level 2 에스컬레이션 / 성공 시 카운터 리셋 | SSR-015~017 |
| test_safe_state.c | kernel/safety/src/safe_state.c | 에스컬레이션 매트릭스(L1 카운터→L2, L2 재진입→L3, 직접 L2/L3), 단조성(무단 하향 전이 거부), 콜백 호출 검증 | SSR-020, SSR-021 |
| test_watchdog.c | kernel/safety/src/watchdog.c | 체크포인트 비트마스크 kick 정책: 전체 통과 시에만 kick, 누락 시 kick 억제, 주기별 마스크 초기화 | SSR-030, SSR-031 |
| test_mpu.c | arch/arm-cortex-m/src/mpu.c | 리전 인코딩(2의 거듭제곱 크기/정렬) 검증, 섀도 CRC 자체 손상 감지, 무결성 검사(리전/CTRL/베이스 변조 감지 + 즉시 복구) — 시뮬레이션 레지스터 파일 사용 | SSR-004, SSR-005 |
| test_fault.c | arch/arm-cortex-m/src/fault_handlers.c | Fault 레코드 저장 + Safe State Level 3 전이 | SSR-006 |

## 커버리지 목표 (Phase 3에서 측정 도구 연동 예정)

| 모듈 | 커버리지 목표 |
|------|-------------|
| Mailbox (CRC/타임스탬프/범위) | MC/DC 100% |
| Watchdog kick 정책 | MC/DC 100% |
| Safe State FSM | MC/DC 100% |
| MPU 섀도/무결성 로직 | MC/DC 100% |
| Fault Handler | MC/DC 100% |

현재는 기능 단위 테스트만 구축된 상태이며, MC/DC **측정**(gcov+검증 도구)은
아직 연동되지 않았다. 측정 결과가 인증 증거로 보관되기 전까지 커버리지 100%
주장은 하지 않는다.

## 독립성 원칙

- 테스트 작성자(Agent-VnV) ≠ 코드 작성자(Agent-Safety)
- 테스트 결과는 Agent-QA가 최종 확인 후 증거로 보관
- **한계 고지:** AI 에이전트 간 역할 분리는 ISO 26262-2의 인적 독립성(I1~I3)을
  대체하지 못한다. 상세는 docs/TEAM.md 참조.
