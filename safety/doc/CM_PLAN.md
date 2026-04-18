# Configuration Management Plan

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CM-001 |
| 표준 참조 | ISO 26262 Part 8 Cl.7 |
| 작성일 | 2026-04-18 |
| 버전 | 1.0 |
| 상태 | Draft |

---

## 1. 목적 (Purpose)

본 문서는 OpenSafetyRTOS 프로젝트의 형상 관리(Configuration Management) 계획을 정의한다.  
ISO 26262 Part 8 Cl.7의 요구사항에 따라 소프트웨어 및 안전 산출물의 무결성, 추적성, 재현성을 보장하는 것을 목적으로 한다.

### 1.1 범위 (Scope)

본 CM 계획은 OpenSafetyRTOS 프로젝트의 전체 개발 수명주기(Concept ~ Release)에 걸쳐 적용된다.  
ASIL-D 등급 소프트웨어 컴포넌트를 포함하는 모든 산출물이 적용 대상이다.

---

## 2. 형상 항목 (Configuration Items)

아래 항목들은 모두 형상 관리 대상(Configuration Item, CI)으로 지정된다.

| # | 형상 항목 | 경로 | 비고 |
|---|-----------|------|------|
| CI-01 | 소스 코드 - 커널 | `kernel/` | ASIL-D 대상 포함 |
| CI-02 | 소스 코드 - 아키텍처 | `arch/` | |
| CI-03 | 소스 코드 - HAL | `hal/` | |
| CI-04 | 안전 문서 | `safety/doc/*.md` | 인증 증거 산출물 |
| CI-05 | 설계 문서 | `docs/*.md` | |
| CI-06 | 빌드 스크립트 | `CMakeLists.txt`, `cmake/` | |
| CI-07 | 테스트 케이스 | `tests/` | |
| CI-08 | CI/CD 설정 | `.github/workflows/` 또는 동등 경로 | |

모든 형상 항목은 git 저장소에서 버전 관리된다.

---

## 3. 베이스라인 정의 (Baseline Definition)

### 3.1 베이스라인 생성 시점

- 각 개발 Phase 종료 시점에 베이스라인을 생성한다.
- Agent-QA의 Phase 종료 감사(Audit) 승인 후 태그를 부여한다.

### 3.2 태그 규칙

| 구분 | 태그 형식 | 예시 |
|------|-----------|------|
| Phase 베이스라인 (일반) | `phase-N-baseline-YYYYMMDD` | `phase-1-baseline-20260418` |
| ASIL-D 안전 릴리즈 | `safety-release-vX.Y.Z` | `safety-release-v1.0.0` |

### 3.3 베이스라인 생성 절차

1. Agent-QA가 Phase 종료 감사를 완료하고 승인 의견을 PR/이슈에 기록한다.
2. 담당자가 `main` 브랜치 최신 커밋에 규정 태그를 부여한다.
3. 태그 생성 시 베이스라인 항목, 버전, 감사 참조 번호를 태그 메시지에 기록한다.
4. ASIL-D 산출물 변경이 포함된 경우 `safety-release-vX.Y.Z` 태그를 추가로 부여한다.

---

## 4. 변경 관리 절차 (Change Control Procedure)

### 4.1 일반 변경 (Non-Safety Change)

```
feature/* 브랜치 생성
    ↓
개발 및 로컬 테스트
    ↓
Pull Request (PR) 생성
    ↓
리뷰어 1인 승인
    ↓
develop 브랜치 머지
```

- 브랜치 규칙: `feature/<기능명>` 또는 `fix/<이슈번호>-<요약>`
- 머지 전 빌드 및 단위 테스트 통과 필수

### 4.2 안전 관련 변경 (Safety-Related Change)

```
safety/* 브랜치 생성
    ↓
변경 영향 분석(Change Impact Analysis) 문서 작성
    ↓
Pull Request (PR) 생성 — 영향 분석 문서 첨부 필수
    ↓
Agent-QA 필수 승인 (ISO 26262 Part 8 Cl.7.4.14 준수)
    ↓
develop 브랜치 머지
    ↓
영향받은 테스트 재실행 확인
```

- 브랜치 규칙: `safety/<이슈번호>-<요약>`
- Agent-QA 승인 없이 머지 불가
- 영향 분석 문서 미첨부 시 PR 반려

### 4.3 베이스라인 이후 변경 (Post-Baseline Change)

- 베이스라인 태그 이후 모든 변경에 대해 **변경 영향 분석(Change Impact Analysis)** 작성이 필수이다.
- 변경 영향 분석 양식: `safety/doc/issues/CIA-<이슈번호>.md`
- 분석 내용: 변경 범위, 영향받는 요구사항, 영향받는 테스트, 재검증 계획

---

## 5. 식별 체계 (Identification Scheme)

### 5.1 문서 ID 규칙

형식: `OSR-[타입코드]-[일련번호(3자리)]`

| 타입코드 | 의미 | 예시 |
|----------|------|------|
| SS | Safety Strategy | OSR-SS-001 |
| CM | Configuration Management | OSR-CM-001 |
| TQ | Tool Qualification | OSR-TQ-001 |
| PR | Problem Resolution | OSR-PR-001 |
| ADR | Architecture Decision Record | OSR-ADR-003 |
| FMEA | Failure Mode & Effects Analysis | OSR-FMEA-001 |
| TC | Test Case | OSR-TC-010 |

### 5.2 이슈/문제 ID 규칙

형식: `OSR-ISSUE-[일련번호]` (GitHub Issue 번호와 연동)

---

## 6. 도구 (Tools)

| 도구 | 용도 | 비고 |
|------|------|------|
| git | 소스 코드 및 문서 형상 관리 | 버전 이력 전체 보존 |
| GitHub / GitLab | Pull Request, 코드 리뷰, 이슈 추적 | 변경 승인 증거 보관 |
| git tag | 베이스라인 식별 | 서명 태그(GPG signed tag) 권장 |

---

## 7. 감사 (Audit)

ISO 26262 Part 8 Cl.7.4.14에 따라 Agent-QA는 각 Phase 종료 시 CM 준수 여부를 감사한다.

### 7.1 감사 항목

| 감사 항목 | 확인 기준 |
|-----------|-----------|
| 모든 형상 항목이 git에 등록되어 있는가 | CI 목록과 저장소 비교 |
| 안전 관련 변경에 Agent-QA 승인이 있는가 | PR 이력 확인 |
| Phase 베이스라인 태그가 규칙에 맞게 부여되었는가 | 태그 목록 검토 |
| 베이스라인 이후 변경에 영향 분석이 첨부되었는가 | CIA 문서 존재 여부 확인 |
| 문서 ID 규칙이 준수되었는가 | 문서 목록 검토 |

### 7.2 감사 결과 처리

- 감사 결과는 `safety/doc/issues/` 에 기록한다.
- 부적합 사항은 Class B 문제로 등록하여 Problem Resolution 프로세스를 따른다 (OSR-PR-001 참조).
