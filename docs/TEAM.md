# OpenSafetyRTOS — Team Structure and Roles

**Version:** 0.4
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

### 🔍 Agent-QA (Internal SafetyCase Reviewer) — ISO 26262 Part 2 Confirmation Measures

Agent-Safety authors SafetyCase documents phase by phase. Agent-QA independently performs all three **Confirmation Measures** per ISO 26262 Part 2.

| Confirmation Measure | ISO 26262 Basis | Description |
|--------------------|-----------------|-------------|
| **Confirmation Review** | Part 2 Cl.8 | Independent review of each SafetyCase deliverable for completeness, consistency, and ISO 26262 compliance |
| **Functional Safety Audit** | Part 2 Cl.9 | Audit that safety processes are followed per Safety Plan (deliverable existence, review history, approvals) |
| **Functional Safety Assessment** | Part 2 Cl.10 | Overall SafetyCase adequacy — "Is this system actually safe?" |

| Item | Description |
|------|-------------|
| Core role | **Internal Safety Assessor** — all three Confirmation Measures |
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

## Independence Levels — ISO 26262 Part 2 Table 1

| Level | Definition | ASIL-D Status |
|-------|-----------|---------------|
| I1 | Independent person (same team) | Insufficient |
| I2 | Independent team (same organization) | Partially satisfies |
| I3 | Independent organization or external body | **Recommended for ASIL-D** |

**Plan:**
- Phase 0–4: Agent-QA as internal reviewer (I2)
- Phase 5: External ISA (TÜV SÜD, TÜV Rheinland, SGS, or Bureau Veritas) for I3
- Gap documented in Safety Plan as OSR-RISK-001

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
| Phase 4: Integration & Testing | Agent-VnV | Agent-QA (review), Agent-Safety (defect fix) |
| Phase 5: Certification prep | Agent-Docs, Agent-QA | All agents + External ISA |
