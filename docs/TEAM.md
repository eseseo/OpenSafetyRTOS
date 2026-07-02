# OpenSafetyRTOS — Team Structure and Roles

**Version:** 0.5
**Reference Standard:** ISO 26262 Part 2 (Functional Safety Management)

---

## Team Structure

```
                        ┌─────────────┐
                        │   PM / PO   │
                        │ (Safety Mgr)│
                        └──────┬──────┘
              ┌────────────────┼─────────────────┐
              │                │                 │
    ┌─────────▼──────┐  ┌──────▼────────────────┐  ┌─────▼──────────┐
    │  Dev Track     │  │   Safety Track         │  │ Quality Track  │
    └────────────────┘  └───────────────────────┘  └────────────────┘
    Agent-Build          Agent-Safety (author)        Agent-VnV
    Agent-Kernel         Agent-Docs   (support)       Agent-QA (internal reviewer)
```

---

## Safety Manager (Required — ISO 26262 Part 2 Cl.5.4)

**Role:** PM/PO acts as Safety Manager.

| Responsibility | Description |
|---------------|-------------|
| Safety Plan ownership | Approve and maintain SAFETY_PLAN.md |
| Safety activity coordination | Verify phase activities are executed per plan |
| Confirmation Measures delegation | Delegate Cl.8/9/10 to Agent-QA; approve results |
| External ISA engagement | Contract an external Independent Safety Assessor at Phase 5 |
| Safety Case final sign-off | Sign SAFETY_CASE.md before certification submission |

---

## Role Definitions

### 🔧 Agent-Build (Build Engineer)

| Item | Description |
|------|-------------|
| Responsibility | CMake build system, directory structure, git strategy, CI skeleton, toolchain |
| Deliverables | CMakeLists.txt hierarchy, toolchain files, .gitignore, CI scripts |
| Branch access | feature/build-*, develop merge |
| ASIL responsibility | QM (build infrastructure) |

### ⚙️ Agent-Kernel (Kernel Engineer)

| Item | Description |
|------|-------------|
| Responsibility | FreeRTOS QM partition integration, task/scheduler, HAL porting |
| Deliverables | kernel/src/*.c, hal/*, arch/arm-cortex-m/src/*, examples/ |
| Branch access | feature/kernel-*, develop merge |
| ASIL responsibility | QM(D) — FreeRTOS integration |

### 🛡️ Agent-Safety (Safety Engineer)

| Item | Description |
|------|-------------|
| Responsibility | SafetyFunction ASIL-D layer implementation + **ISO 26262 SafetyCase authoring** |
| Code deliverables | kernel/safety/src/*.c, arch/arm-cortex-m/src/mpu.c |
| SafetyCase deliverables | HARA → FSC → TSC → SSRS → Safety Analysis → SafetyCase (continuous updates by phase) |
| Branch access | safety/SSR-* (Agent-QA review mandatory) |
| ASIL responsibility | **ASIL-D(D)** — MISRA-C:2012 and MC/DC 100% coverage mandatory |
| SafetyCase flow | Phase 1: HARA, FSC → Phase 2: TSC → Phase 3: SSRS, FMEA → Phase 4: V&V evidence → Phase 5: SafetyCase completion |
| Special requirement | All code changes must include SSR traceability tag (e.g., `/* SSR-015-MBX */`); submit each deliverable to Agent-QA immediately |

### 📄 Agent-Docs (Safety Analyst)

| Item | Description |
|------|-------------|
| Responsibility | ADRs, FMEA, Safety Plan, architecture documents, certification evidence |
| Deliverables | docs/ADR-*.md, safety/doc/*, ARCHITECTURE.md |
| Branch access | docs/*, safety/doc/* |
| ASIL responsibility | Document quality — ASIL-D development evidence |

---

## V&V / QA Track

### 🧪 Agent-VnV (V&V Engineer) — ISO 26262 Part 4/6

| Item | Description |
|------|-------------|
| Responsibility | Unit testing, integration testing, test plan and result documentation |
| Deliverables | tests/unit/*, tests/integration/*, safety/tests/*, V&V plans, test result reports |
| Branch access | test/*, validation/* |
| ASIL responsibility | SafetyFunction tests require **MC/DC 100% coverage** |
| Key activities | Mailbox validation unit tests (CRC fail / stale / range); MPU access → HardFault tests; Watchdog kick/timeout tests; FFI isolation tests |
| Independence | Agent-VnV tests code written by Agent-Safety — **same agent prohibited** (Part 6) |

### 🔍 Agent-QA (Internal SafetyCase Reviewer) — Verification-Support Measures

Agent-Safety authors SafetyCase documents phase by phase. Agent-QA performs internal
reviews structured after the three **Confirmation Measures** of ISO 26262 Part 2
(Cl.6.4.9). **Note (v0.5):** because Agent-QA does not meet the person/organization
independence of ISO 26262-2:2018 Table 1 (see "Honest Independence Statement" below),
these internal reviews are verification-support measures, not ISO-conformant
confirmation measures. ISO-conformant confirmation measures require an external ISA.

| Confirmation Measure | ISO 26262 Basis | Description |
|--------------------|-----------------|-------------|
| **Confirmation Review** | Part 2 Cl.8 | Independent review of each SafetyCase deliverable for completeness, consistency, and ISO 26262 compliance |
| **Functional Safety Audit** | Part 2 Cl.9 | Audit that safety processes are followed per Safety Plan (deliverable existence, review history, approvals) |
| **Functional Safety Assessment** | Part 2 Cl.10 | Overall SafetyCase adequacy — "Is this system actually safe?" |

| Item | Description |
|------|-------------|
| Core role | **Internal reviewer** — internal reviews structured after the three Confirmation Measures (verification-support; ISO-conformant confirmation measures delegated to external ISA) |
| Review scope | All SafetyCase deliverables by Agent-Safety (HARA → FSC → TSC → SSRS → FMEA → SafetyCase) |
| Deliverables | Confirmation Review records (OSR-CR-NNN), Audit records (OSR-FSA-NNN), Assessment reports (OSR-FSAMNT-NNN) |
| Branch access | Mandatory approver on all safety/* PRs |
| Independence | Agent-QA reviews Agent-Safety documents — **same agent strictly prohibited** (Part 2) |

---

## Independence Matrix (ISO 26262 Mandatory)

```
               Code Dev  SafetyCase Author  SafetyCase Review  V&V Test  Docs
Agent-Kernel      O            -                  -               -        -
Agent-Safety      O            O                  -               -        -   ← author
Agent-VnV         -            -                  -               O        -   ← independent tester
Agent-QA          -            -                  O               -        -   ← independent reviewer
Agent-Docs        -          support              -               -        O
```

> O = performs / - = must not perform (independence requirement)

---

## Independence Levels — ISO 26262-2:2018 Table 1 (Corrected v0.5)

| Level | Definition |
|-------|-----------|
| I0 | No independence requirement |
| I1 | Different person |
| I2 | Different persons, different team |
| I3 | Different persons, different department/organization |

**Required independence per confirmation measure (ISO 26262-2:2018 Table 1 — "shall", not "recommended"):**

| Confirmation Measure | Required Independence |
|---------------------|----------------------|
| Confirmation review of the HARA | **I3 — at ALL ASILs** (ASIL not yet determined at review time) |
| Confirmation review of the safety plan | I3 at ASIL-D |
| Confirmation review of the safety case | I3 at ASIL-D |
| Functional safety audit | I3 at ASIL-D |
| Functional safety assessment | I3 at ASIL-D |

> **Correction (v0.5):** Earlier versions of this document stated I3 was "Recommended for ASIL-D."
> That was incorrect. ISO 26262-2:2018 Table 1 **requires** I3 for the measures above,
> and requires I3 for the HARA confirmation review regardless of ASIL.

### Honest Independence Statement (Governing Constraint)

The Agent-Safety / Agent-QA / Agent-VnV structure consists of **AI agent instances
operated by a single maintainer**. Persons-based independence in the sense of
ISO 26262-2:2018 Table 1 (different persons, different team/department) is therefore
**not met — not even I2**. Role separation between agents provides useful defect-finding
diversity but no organizational independence.

Consequences:

1. All internal review records (OSR-CR-*) produced by Agent-QA are classified as
   **verification-support measures** (technical reviews supporting ISO 26262-6 Cl.9
   verification), **not** as ISO 26262-conformant confirmation measures.
2. ISO-conformant confirmation reviews, the functional safety audit, and the functional
   safety assessment **require independent human reviewers / an external ISA**.
3. This is the **governing constraint** on any compliance claim of this project and is
   tracked as **OSR-RISK-001** in SAFETY_PLAN.md. Internal review records serve as
   preparatory input to the external ISA, nothing more.

**Plan (revised v0.5):**
- Phase 0–4: Agent-QA internal reviews continue as verification-support measures (sub-I2, documented as such in each record)
- **Before Phase 2 exit (at the latest):** external ISA engaged for a retrospective I3 confirmation review of all Phase 1 artifacts (HARA, FSC, SSRS, review records)
- Phase 3–5: external ISA (TÜV SÜD, TÜV Rheinland, SGS, or Bureau Veritas) performs ongoing I3 confirmation measures, functional safety audit, and final assessment
- Gap documented in Safety Plan as OSR-RISK-001; issue record OSR-ISSUE-005 (Class B, open)

---

## Branch Protection Rules

| Branch Pattern | Required Reviewers | Additional Conditions |
|---------------|-------------------|----------------------|
| `main` | PM approval | Tag + release notes |
| `develop` | Agent-Build or Agent-Kernel | CI passing |
| `safety/*` | **Agent-QA (mandatory)** + 1 additional | MISRA report attached, MC/DC results attached |
| `feature/*` | 1 reviewer | CI passing |
| `test/*` | Agent-VnV | Coverage report attached |

---

## Phase-by-Phase Agent Deployment

| Phase | Lead Agent(s) | Support Agent(s) |
|-------|--------------|-----------------|
| Phase 0: Setup | Agent-Build, Agent-Docs | — |
| Phase 1: Architecture & Safety Analysis | Agent-Safety, Agent-Docs | Agent-QA (review) |
| Phase 2: Kernel | Agent-Kernel, Agent-Build | Agent-VnV (unit tests) |
| Phase 3: SafetyFunction implementation | Agent-Safety | **Agent-QA, Agent-VnV** (independent) |
| Phase 4: Integration & Testing | Agent-VnV | Agent-QA (review), Agent-Safety (defect fix), External ISA (ongoing I3 confirmation measures) |
| Phase 5: Certification prep | Agent-Docs, Agent-QA | All agents + External ISA (functional safety assessment) |

> External ISA engagement begins **before Phase 2 exit** with a retrospective I3
> confirmation review of Phase 1 artifacts (see Independence Levels section).

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 0.4 | — | Prior baseline (agent roles, independence matrix, branch protection) |
| 0.5 | 2026-07-02 | Corrected ISO 26262-2:2018 Table 1 rendering (I3 is required, not "recommended", for ASIL-D confirmation measures; I3 required for HARA confirmation review at all ASILs). Added Honest Independence Statement: single-maintainer AI-agent structure does not meet I2/I3; internal OSR-CR-* records reclassified as verification-support measures. ISA engagement moved earlier: retrospective I3 confirmation review of Phase 1 artifacts before Phase 2 exit. Linked OSR-RISK-001 / OSR-ISSUE-005. (Per OSR-CR-ARCH-001 findings F-10, F-14.) |
