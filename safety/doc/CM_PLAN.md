# Configuration Management Plan

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-CM-001 |
| 표준 참조 | ISO 26262 Part 8 Cl.7 |
| 작성일 | 2026-04-18 |
| 개정일 | 2026-07-02 |
| 버전 | 1.1 |
| 상태 | Released (운영 중 — 본 계획은 Phase 0부터 프로젝트에 적용되고 있으므로 상태를 실제와 일치시킴) |

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

### 3.4 베이스라인 이행 현황 및 미이행 조치 (v1.1 신설)

**Phase 베이스라인 태그는 필수 요건이다.** 2026-07-02 감사(OSR-CR-ARCH-001 F-13) 결과,
Phase 1이 완료 선언되었음에도 저장소에 **어떠한 베이스라인 태그도 존재하지 않음**이
확인되었다 (`git tag` 결과 0건). 이는 본 계획 §3.1~3.3 미이행이며,
OSR-CR-ARCH-001 finding F-13 (Part 8 실행 증거 갭)으로 추적한다.

**조치:** 다음 릴리즈(머지) 시점에 Phase 1 베이스라인 태그
(예: `baseline/phase1-v1` 또는 `phase-1-baseline-YYYYMMDD`)를 소급 생성해야 하며,
태그 메시지에 Phase 1 산출물 버전(HARA v1.3, FSC v1.3, SSRS v1.2, OSR-AOU-001 v1.0)과
검토 기록 참조(OSR-CR-HARA-003, OSR-CR-FSC-003, OSR-CR-SSRS-002)를 기록한다.
이후 Phase 종료 시 태그 미생성 상태로 다음 Phase 착수를 금지한다.

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

기본 형식: `OSR-[타입코드]-[일련번호(3자리)]`

| 타입코드 | 의미 | 예시 |
|----------|------|------|
| SP | Safety Plan | OSR-SP-001 |
| CM | Configuration Management | OSR-CM-001 |
| TQ | Tool Qualification | OSR-TQ-001 |
| PR | Problem Resolution | OSR-PR-001 |
| QA | QA 체크리스트 / Confirmation Measures | OSR-QA-001 |
| HARA | Hazard Analysis & Risk Assessment | OSR-HARA-001 |
| FSC | Functional Safety Concept | OSR-FSC-001 |
| SSRS | SW Safety Requirements Specification | OSR-SSRS-001 |
| AOU | Assumption of Use 레지스트리 | OSR-AOU-001 |
| RTM | Requirements Traceability Matrix | OSR-RTM-001 |
| ADR | Architecture Decision Record | OSR-ADR-003 |
| FMEA | Failure Mode & Effects Analysis | OSR-FMEA-001 |
| TC | Test Case | OSR-TC-010 |
| ISSUE | 문제 이력 기록 | OSR-ISSUE-001 |
| FSA | 기능안전 감사 기록 | OSR-FSA-001 |

**검토 기록(Confirmation/Verification Review Record) 형식:** `OSR-CR-[대상문서코드]-[일련번호(3자리)]`
예시: `OSR-CR-HARA-001`, `OSR-CR-FSC-003`, `OSR-CR-SSRS-002`, `OSR-CR-ARCH-001`
(대상 문서별 검토 회차를 일련번호로 구분한다.)

> **v1.1 정정:** 구버전의 타입코드 "SS(Safety Strategy)"는 실제로 사용되지 않았고
> Safety Plan은 OSR-SP-001로 발행되었다. 실사용 ID 체계(OSR-SP/QA/CR-*/HARA/FSC/SSRS/RTM/AOU 등)를
> 본 표에 반영하였다 (OSR-CR-ARCH-001 F-09 참조). 일부 과거 검토 기록의 입력 문서 목록에
> "OSR-SS-001"로 표기된 것은 OSR-SP-001을 지칭한다.

### 5.2 이슈/문제 ID 규칙

형식: `OSR-ISSUE-[일련번호]` (GitHub Issue 번호와 연동)

---

## 6. 도구 (Tools)

| 도구 | 용도 | 비고 |
|------|------|------|
| git | 소스 코드 및 문서 형상 관리 | 버전 이력 전체 보존 |
| GitHub | Pull Request, 코드 리뷰, 이슈 추적 | 변경 승인 증거 보관 (v1.1: "GitHub / GitLab" 모호성 제거 — 본 프로젝트의 형상 관리 플랫폼은 GitHub로 확정) |
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

---

## 8. 형상 상태 관리 (Configuration Status Accounting) — ISO 26262 Part 8 Cl.7 (v1.1 신설)

형상 항목의 상태를 언제든 조회·보고할 수 있어야 한다.

### 8.1 상태 기록 수단

| 항목 | 수단 |
|------|------|
| 각 CI의 현재 버전 | 문서 헤더의 버전 필드 + git 커밋 이력 |
| 각 안전 문서의 검토/승인 상태 | SAFETY_PLAN.md §4a 문서 상태 표 (매 Phase 갱신) + SAFETY_CASE.md 증거 표 |
| 검토 기록 목록 | QA_REVIEW_CHECKLIST.md(OSR-QA-001) 리뷰 이력 추적 표 |
| 미해결 문제 현황 | GitHub Issue (`class-A`/`class-B` 레이블) + `safety/doc/issues/` 기록 |
| 베이스라인 구성 | git tag 메시지 (§3.3) |

### 8.2 상태 보고

- 각 Phase 종료 감사 시 Agent-QA는 **형상 상태 보고서**(CI 버전, 승인 상태, 미해결 문제,
  베이스라인 태그)를 감사 기록(OSR-FSA-NNN)에 포함한다.
- 문서 헤더 버전/상태 필드와 실제 승인 상태의 불일치는 Class B 문제로 등록한다
  (2026-07-02 감사에서 다수 발견 — OSR-CR-ARCH-001 F-11 참조).

## 9. 보관 및 장기 재현성 (Archival & Long-term Reproducibility) — ISO 26262 Part 8 Cl.7 (v1.1 신설)

안전 관련 산출물과 빌드 환경은 제품 서비스 수명 동안 재현 가능해야 한다.

| 요건 | 방법 |
|------|------|
| 인증 증거 보존 | `safety-release-vX.Y.Z` 태그 시점의 저장소 전체를 아카이브(원격 미러 + 오프라인 사본)로 보존. 최소 보존 기간: 최종 릴리즈 후 15년 (차량 서비스 수명 가정, 통합자 AoU로 조정 가능) |
| 빌드 재현성 | 릴리즈 베이스라인마다 도구체인 정확 버전(GCC, CMake, binutils, 테스트 프레임워크)을 태그 메시지 및 `safety/doc/tool-qualification/` 기록에 고정 (OSR-TQ-001 §5 연계). 권장: 컨테이너 이미지 또는 lockfile로 빌드 환경 스냅샷 보존 |
| 외부 의존물 보존 | FreeRTOS 등 서드파티 소스는 저장소 내 벤더링 또는 커밋 해시 고정으로 보존 |
| 접근 통제 및 무결성 | 아카이브 사본에 대한 체크섬(SHA-256) 기록, GPG 서명 태그 사용 |

---

## 10. 개정 이력 (Revision History)

| 버전 | 날짜 | 작성자 | 변경 내용 |
|------|------|--------|---------|
| 1.0 | 2026-04-18 | Agent-Docs | 최초 작성 (Draft) |
| 1.1 | 2026-07-02 | Agent-Docs | OSR-CR-ARCH-001 지적 사항 반영: §5.1 ID 체계를 실사용 ID(OSR-SP/QA/HARA/FSC/SSRS/AOU/RTM/ISSUE/FSA, OSR-CR-<문서>-NNN 검토 기록 형식)로 확장 및 "SS→SP" 정정 (F-09); §3.4 신설 — Phase 1 베이스라인 태그 미생성 확인 및 소급 생성 요구 (F-13); §8 형상 상태 관리, §9 보관/장기 재현성 신설 (Part 8 Cl.7 갭 보완); §6 도구 표 "GitHub / GitLab" 모호성 제거 → GitHub 확정; 문서 상태 Draft → Released로 실제와 일치화 |
