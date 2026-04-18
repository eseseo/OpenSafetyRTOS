# OpenSafetyRTOS — 팀 구성 및 R&R

**Version:** 0.3 (Agent-QA 역할 명확화 — SafetyCase 내부 리뷰어)
**기준:** ISO 26262 Part 2 (Functional Safety Management)

---

## 팀 구조도

```
                        ┌─────────────┐
                        │   PM / PO   │
                        │  (총괄 관리)  │
                        └──────┬──────┘
              ┌────────────────┼─────────────────┐
              │                │                 │
    ┌─────────▼──────┐  ┌──────▼────────────────┐  ┌─────▼──────────┐
    │  개발 트랙      │  │     Safety 트랙        │  │  품질 트랙      │
    └────────────────┘  └───────────────────────┘  └────────────────┘
    Agent-Build          Agent-Safety (작성)         Agent-VnV
    Agent-Kernel         Agent-Docs   (지원)         Agent-QA (SafetyCase 내부 리뷰어)
```

---

## 역할 정의 (R&R)

### 🔧 Agent-Build (Build Engineer)
| 항목 | 내용 |
|------|------|
| 책임 | CMake 빌드시스템, 폴더구조, git 전략, CI 뼈대, toolchain |
| 산출물 | CMakeLists.txt 계층, toolchain 파일, .gitignore, CI 스크립트 |
| 브랜치 권한 | feature/build-*, develop 머지 |
| ASIL 책임 | QM (빌드 인프라) |

### ⚙️ Agent-Kernel (Kernel Engineer)
| 항목 | 내용 |
|------|------|
| 책임 | FreeRTOS QM 파티션 통합, 태스크/스케줄러, HAL 포팅 |
| 산출물 | kernel/src/*.c, hal/*, arch/arm-cortex-m/src/*, examples/ |
| 브랜치 권한 | feature/kernel-*, develop 머지 |
| ASIL 책임 | QM(D) — FreeRTOS 통합 |

### 🛡️ Agent-Safety (Safety Engineer)
| 항목 | 내용 |
|------|------|
| 책임 | SafetyFunction ASIL-D 레이어 구현 + **기능안전 프로세스에 따른 SafetyCase 지속 작성** |
| 코드 산출물 | kernel/safety/src/*.c, arch/arm-cortex-m/src/mpu.c |
| SafetyCase 산출물 | HARA → FSC → TSC → SSRS → Safety Analysis → SafetyCase (단계별 지속 업데이트) |
| 브랜치 권한 | safety/SSR-* (Agent-QA 리뷰 필수) |
| ASIL 책임 | **ASIL-D(D)** — MISRA-C, MC/DC 커버리지 100% 필수 |
| SafetyCase 작성 흐름 | Phase 1: HARA, FSC → Phase 2: TSC → Phase 3: SSRS, FMEA → Phase 4: V&V 결과 통합 → Phase 5: SafetyCase 완성 |
| 특이사항 | 모든 코드 변경에 SSR ID 트레이스 필수, 각 산출물 완성 즉시 Agent-QA에 리뷰 요청 |

### 📄 Agent-Docs (Safety Analyst)
| 항목 | 내용 |
|------|------|
| 책임 | ADR, FMEA, Safety Plan, 아키텍처 문서, 인증 증거 문서 |
| 산출물 | docs/ADR-*.md, safety/doc/*, ARCHITECTURE.md |
| 브랜치 권한 | docs/*, safety/doc/* |
| ASIL 책임 | 문서 품질 — ASIL-D 개발 증거 작성 |

---

## 신규 역할 (V&V / QA 트랙)

### 🧪 Agent-VnV (V&V Engineer) ← NEW
**ISO 26262 Part 4/6: Verification & Validation 담당**

| 항목 | 내용 |
|------|------|
| 책임 | 단위 테스트(Unit Test), 통합 테스트(Integration Test), 테스트 계획/결과 문서 |
| 산출물 | tests/unit/*, tests/integration/*, safety/tests/*, V&V 계획서, 테스트 결과 리포트 |
| 브랜치 권한 | test/*, validation/* |
| ASIL 책임 | SafetyFunction 테스트는 **MC/DC 커버리지 100%** 필수 |
| 핵심 활동 | - Mailbox 검증 알고리즘 단위 테스트 (CRC fail / stale / range 각 케이스) |
|            | - MPU 설정 검증 테스트 (QM→ASIL-D 접근 시 HardFault 발생 확인) |
|            | - Watchdog kick/timeout 테스트 |
|            | - FFI 검증: QM 파티션 격리 테스트 |
| 독립성 요건 | Agent-Safety가 작성한 코드를 Agent-VnV가 독립적으로 테스트 (동일 에이전트 금지) |

### 🔍 Agent-QA (Internal SafetyCase Reviewer) ← UPDATED
**ISO 26262 Part 2: 내부 독립 SafetyCase 리뷰어**

Agent-Safety가 기능안전 프로세스에 따라 SafetyCase를 단계별로 작성하면,
Agent-QA가 각 산출물을 **독립적으로 리뷰**하여 완결성·정합성·ISO 26262 준수 여부를 검토한다.
이 리뷰 기록 자체가 인증 시 핵심 증거가 된다.

| 항목 | 내용 |
|------|------|
| 핵심 역할 | **Internal Safety Assessor** — Agent-Safety 산출물의 독립 리뷰 |
| 리뷰 대상 | Agent-Safety가 작성하는 모든 SafetyCase 산출물 (아래 목록 참조) |
| 산출물 | 각 산출물별 리뷰 기록 (OSR-QA-NNN), 지적사항 추적 로그 |
| 브랜치 권한 | safety/* PR 필수 승인자 |
| 독립성 원칙 | Agent-Safety가 작성한 문서를 Agent-QA가 리뷰 — **동일 에이전트 절대 불가** |

**리뷰 대상 SafetyCase 산출물 (Agent-Safety 작성 → Agent-QA 리뷰)**

| SafetyCase 산출물 | ISO 26262 근거 | 리뷰 포인트 |
|-----------------|---------------|------------|
| HARA (위험원 분석 및 위험도 평가) | Part 3 Cl.15 | 위험원 누락 여부, ASIL 등급 적절성 |
| FSC (기능안전 개념) | Part 3 Cl.8 | 안전 목표 커버리지, 독립성 확보 |
| TSC (기술안전 개념) | Part 4 Cl.7 | 시스템 아키텍처와 정합성 |
| SSRS (SW 안전 요구사항) | Part 6 Cl.7 | 완전성, 검증 가능성, SSR ID 체계 |
| FMEA / FTA | Part 9 | Failure Mode 누락, 완화조치 적절성 |
| FFI 분석 | Part 6 Cl.7.4.14 | 파티션 간 간섭 경로 완전성 |
| 코드 리뷰 (MISRA) | Part 6 Cl.8 | MISRA-C:2012 위반, SSR 트레이스 |
| V&V 결과 검토 | Part 6 Cl.9 | MC/DC 커버리지, 테스트 완전성 |
| SafetyCase 최종본 | Part 2 Cl.6 | 논증 완결성, 증거 충분성 |

---

## 독립성 매트릭스 (ISO 26262 필수)

```
               코드개발  SafetyCase작성  SafetyCase리뷰  V&V테스트  일반문서
Agent-Kernel      O           -               -              -          -
Agent-Safety      O           O               -              -          -   ← 작성자
Agent-VnV         -           -               -              O          -   ← 독립 테스트
Agent-QA          -           -               O              -          -   ← 독립 리뷰어
Agent-Docs        -           지원            -              -          O
```

> O = 수행 / - = 해당 역할 수행 불가 (독립성 보장)
> Agent-Safety가 쓴 SafetyCase를 Agent-Safety가 리뷰하는 것은 ISO 26262상 허용되지 않음

---

## 브랜치 보호 규칙 요약

| 브랜치 패턴 | 필수 리뷰어 | 추가 조건 |
|------------|------------|----------|
| `main` | PM 승인 | 태그 + 릴리즈 노트 |
| `develop` | Agent-Build or Agent-Kernel | CI 통과 |
| `safety/*` | **Agent-QA (필수)** + 1명 추가 | MISRA 리포트 첨부, MC/DC 결과 첨부 |
| `feature/*` | 1명 | CI 통과 |
| `test/*` | Agent-VnV | 커버리지 리포트 첨부 |

---

## Phase별 에이전트 투입 계획

| Phase | 주도 에이전트 | 지원 에이전트 |
|-------|-------------|-------------|
| Phase 0: 셋업 | Agent-Build, Agent-Docs | - |
| Phase 1: 아키텍처 | Agent-Safety, Agent-Docs | Agent-QA (ADR 리뷰) |
| Phase 2: 커널 | Agent-Kernel, Agent-Build | Agent-VnV (단위 테스트) |
| Phase 3: Safety Function | Agent-Safety | **Agent-QA, Agent-VnV** (독립 투입) |
| Phase 4: 통합/테스트 | Agent-VnV | Agent-QA (리뷰), Agent-Safety (결함 수정) |
| Phase 5: 인증 준비 | Agent-Docs, Agent-QA | 전체 |
