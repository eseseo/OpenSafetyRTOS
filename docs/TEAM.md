# OpenSafetyRTOS — 팀 구성 및 R&R

**Version:** 0.2 (V&V / QA 추가)
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
    ┌─────────▼──────┐  ┌──────▼───────┐  ┌─────▼──────────┐
    │  개발 트랙      │  │  Safety 트랙  │  │  품질 트랙      │
    └────────────────┘  └──────────────┘  └────────────────┘
    Agent-Build          Agent-Safety      Agent-QA
    Agent-Kernel         Agent-Docs        Agent-VnV
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
| 책임 | SafetyFunction ASIL-D 레이어, MPU 설정, Mailbox, Watchdog, Fault Detection |
| 산출물 | kernel/safety/src/*.c, arch/arm-cortex-m/src/mpu.c, safety/ 산출물 |
| 브랜치 권한 | safety/SSR-* (2명 리뷰 필수 + QA 승인) |
| ASIL 책임 | **ASIL-D(D)** — MISRA-C, MC/DC 커버리지 100% 필수 |
| 특이사항 | 모든 코드 변경에 SSR(Safety Software Requirement) ID 트레이스 필수 |

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

### 🔍 Agent-QA (Quality Assurance / Independent Reviewer) ← NEW
**ISO 26262 Part 2: 독립 안전 검토 담당**

| 항목 | 내용 |
|------|------|
| 책임 | 코드 리뷰 독립성 보장, MISRA-C 준수 감사, 안전 계획 준수 확인, 리뷰 기록 |
| 산출물 | QA 리뷰 체크리스트, MISRA 위반 보고서, 독립 리뷰 기록 (인증 증거) |
| 브랜치 권한 | safety/* PR에 **필수 승인자** (Agent-Safety 제외) |
| ASIL 책임 | 독립 검증자 역할 — Agent-Safety의 작업을 **독립적으로** 검토 |
| 핵심 원칙 | **개발자(Agent-Safety)와 리뷰어(Agent-QA)는 반드시 분리** |
| 활동 범위 | - PR 리뷰: safety/ 및 kernel/safety/ 모든 변경사항 |
|            | - MISRA-C:2012 Rule 위반 여부 확인 |
|            | - Safety 요구사항 추적성 (SSR → 코드 → 테스트) 검증 |
|            | - FMEA 업데이트 적절성 검토 |
|            | - 설계 변경 시 ADR 작성 여부 확인 |

---

## 독립성 매트릭스 (ISO 26262 필수)

```
              개발  리뷰  테스트  문서
Agent-Kernel    O    -      -      -
Agent-Safety    O    -      -      -
Agent-VnV       -    -      O      -    ← Safety 코드 테스트 (독립)
Agent-QA        -    O      -      O    ← Safety 코드 리뷰 (독립)
Agent-Docs      -    -      -      O
```

> O = 수행 가능 / - = 해당 모듈에 대해 수행 불가 (독립성 보장)

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
