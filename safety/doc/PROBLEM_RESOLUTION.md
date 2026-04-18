# Problem Resolution Process

| 항목 | 내용 |
|------|------|
| 문서 ID | OSR-PR-001 |
| 표준 참조 | ISO 26262 Part 8 Cl.9 |
| 작성일 | 2026-04-18 |
| 버전 | 1.0 |
| 상태 | Draft |

---

## 1. 목적 (Purpose)

본 문서는 OpenSafetyRTOS 개발 중 발견된 결함 및 문제의 체계적인 추적과 해결을 위한 프로세스를 정의한다.  
ISO 26262 Part 8 Cl.9의 요구사항에 따라 문제의 발견부터 해결 및 검증, 이력 보관까지의 전 과정을 규정한다.

---

## 2. 문제 분류 (Problem Classification)

모든 문제는 안전 영향도에 따라 세 등급으로 분류한다.

### Class A — Safety Critical (안전 임계)

| 항목 | 내용 |
|------|------|
| 정의 | 직접적인 안전 기능 결함 또는 ASIL-D 요구사항 위반 |
| 해당 사례 | SafetyFunction 구현 결함, ASIL-D 파티션 무결성 위반, FFI(Freedom From Interference) 완전 침해 |
| 처리 기한 | 즉시 처리 (스프린트 경계 무관) |
| 필수 조치 | Agent-QA 즉시 알림 필수, 릴리즈 차단 |
| GitHub Label | `class-A` |

### Class B — Safety Related (안전 관련)

| 항목 | 내용 |
|------|------|
| 정의 | 안전 관련 기능에 영향을 주나 즉각적인 시스템 실패를 유발하지 않는 문제 |
| 해당 사례 | FFI 부분 침해, Mailbox 검증 로직 오류, 안전 관련 문서 불일치 |
| 처리 기한 | 다음 스프린트 내 처리 |
| 필수 조치 | 백로그 등록, 우선순위 높음으로 설정 |
| GitHub Label | `class-B` |

### Class C — General (일반)

| 항목 | 내용 |
|------|------|
| 정의 | 안전 기능에 직접 영향 없는 일반 결함 및 오류 |
| 해당 사례 | QM 파티션 버그, 문서 오타/오류, 성능 최적화 항목 |
| 처리 기한 | 일반 이슈 처리 (스프린트 계획에 따름) |
| 필수 조치 | 일반 이슈 등록 |
| GitHub Label | `class-C` |

---

## 3. 문제 보고 양식 (Problem Report Form)

GitHub Issue 등록 시 아래 양식을 사용한다. 템플릿 파일: `.github/ISSUE_TEMPLATE/problem_report.md`

```markdown
## 문제 보고 (Problem Report)

**제목 (Title):** [간결한 문제 설명]

**분류 (Class):** [ ] Class A  [ ] Class B  [ ] Class C

**발견 단계 (Discovery Phase):**
- [ ] 코드 리뷰
- [ ] 단위 테스트
- [ ] 통합 테스트
- [ ] 정적 분석
- [ ] 기타: ___

**재현 방법 (Reproduction Steps):**
1. 
2. 
3. 

**예상 동작 (Expected Behavior):**

**실제 동작 (Actual Behavior):**

**영향 분석 (Impact Analysis):**
- 영향받는 컴포넌트: 
- 영향받는 요구사항 ID: 
- 영향받는 테스트 케이스: 
- ASIL 등급에 미치는 영향: 

**담당자 (Assignee):**

**관련 문서 (Related Documents):**
```

---

## 4. 처리 흐름 (Resolution Flow)

```
문제 발견
    ↓
GitHub Issue 등록 (label: class-A / class-B / class-C)
    ↓
    ├─── Class A ──────────────────────────────────────┐
    │    Agent-QA 즉시 통보 (Issue에 @mention)          │
    │         ↓                                        │
    │    원인 분석 (Root Cause Analysis)                │
    │         ↓                                        │
    │    safety/* 브랜치에서 수정                        │
    │         ↓                                        │
    │    Agent-QA 검증 및 PR 승인                       │
    │         ↓                                        │
    │    수정 머지 → 영향받은 테스트 재실행               │
    │         ↓                                        │
    │    safety/doc/issues/에 이력 기록                  │
    │         ↓                                        │
    └─── Class B / C ───────────────────────────────── ┤
         백로그 등록 및 우선순위 결정                     │
              ↓                                        │
         스프린트 계획에 따라 수정                        │
              ↓                                        │
         PR 생성 및 리뷰 (Class B: 리뷰어 1인 이상)      │
              ↓                                        │
         수정 머지 → 영향받은 테스트 재실행               │
              ↓                                        │
         Class B는 safety/doc/issues/에 이력 기록 ──────┘
                                                       ↓
                                          베이스라인 업데이트 (Phase 종료 시)
```

### 4.1 Class A 처리 상세 절차

1. Issue 등록 즉시 `class-A` 레이블 부여 및 Agent-QA @mention
2. Agent-QA가 24시간 이내 영향 범위 확인 및 초기 대응 방향 결정
3. 담당자가 Root Cause Analysis 수행 및 Issue 코멘트에 기록
4. `safety/<이슈번호>-fix-<요약>` 브랜치에서 수정 구현
5. PR 생성 — 수정 내용, RCA 결과, 영향 분석, 재검증 계획 포함
6. Agent-QA 필수 승인 후 머지
7. 영향받은 모든 테스트 케이스 재실행 및 결과 기록
8. `safety/doc/issues/OSR-ISSUE-<번호>.md` 파일 생성 (이력 보관)

---

## 5. 미해결 문제 관리 (Open Problem Management)

### 5.1 릴리즈 차단 조건

- 릴리즈(베이스라인 생성) 시점에 **Class A 미해결 문제가 1건이라도 존재하면 릴리즈가 차단**된다.
- Agent-QA가 Phase 종료 감사 시 미해결 Class A 문제 유무를 반드시 확인한다.

### 5.2 Class B 미해결 문제

- 릴리즈를 차단하지 않으나, 다음 Phase 계획에 필수 포함한다.
- 미해결 사유와 수용 근거(Rationale)를 Issue에 문서화한다.

### 5.3 미해결 문제 현황 추적

- GitHub Issue 목록에서 `class-A`, `class-B` 레이블과 `open` 상태로 현황을 추적한다.
- Agent-QA는 매 Phase 감사 시 미해결 문제 목록을 CM 감사 체크리스트에 반영한다 (OSR-CM-001 참조).

---

## 6. 이력 보관 (Record Retention)

### 6.1 보관 대상

인증 증거로서 다음 문제의 이력을 물리적 문서로 보관한다.

| 대상 | 보관 필수 여부 |
|------|--------------|
| Class A 문제 | 필수 |
| Class B 문제 | 필수 |
| Class C 문제 | GitHub Issue 이력으로 대체 가능 |

### 6.2 보관 위치 및 파일 명명 규칙

- 위치: `safety/doc/issues/`
- 파일명: `OSR-ISSUE-<GitHub Issue 번호>.md`
- 예시: `safety/doc/issues/OSR-ISSUE-042.md`

### 6.3 이력 문서 필수 포함 내용

```markdown
# OSR-ISSUE-<번호>: <제목>

| 항목 | 내용 |
|------|------|
| Issue 번호 | # |
| 분류 | Class A / B |
| 발견일 | YYYY-MM-DD |
| 해결일 | YYYY-MM-DD |
| 담당자 | |
| 검증자 | |

## 원인 분석 (Root Cause Analysis)

## 수정 내용 (Fix Description)

## 영향 분석 (Impact Analysis)

## 검증 결과 (Verification Result)

## Agent-QA 승인 (Class A만 해당)
```
