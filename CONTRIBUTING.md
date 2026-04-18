# Contributing to OpenSafetyRTOS

Thank you for your interest in contributing. Because OpenSafetyRTOS targets ISO 26262 ASIL-D, contributions are divided into two distinct tracks with different requirements. **Please read the correct section before submitting any changes.**

---

## Table of Contents

1. [Two Contribution Tracks](#two-contribution-tracks)
2. [Track A: QM Contributions (Standard)](#track-a-qm-contributions-standard)
3. [Track B: Safety Contributions (ASIL-D Process)](#track-b-safety-contributions-asil-d-process)
4. [Coding Standards](#coding-standards)
5. [Commit Message Convention](#commit-message-convention)
6. [Branch Naming](#branch-naming)
7. [Pull Request Process](#pull-request-process)
8. [Code of Conduct](#code-of-conduct)

---

## Two Contribution Tracks

| Track | Directories | Standard | Reviewers Required |
|---|---|---|---|
| A — QM (Standard) | Everything outside `safety/` and `kernel/safety/` | C99 | 1 |
| B — Safety (ASIL-D) | `safety/`, `kernel/safety/` | MISRA-C:2012 + MC/DC | 2 (incl. @safety-team) |

The partition boundary is a hard line. **Do not mix QM and Safety changes in a single pull request.**

---

## Track A: QM Contributions (Standard)

This track covers FreeRTOS wrappers, board support packages, example applications, build system changes, documentation improvements, and any code outside the safety-critical partitions.

### Requirements

- Follow C99 standard
- Ensure existing CI tests pass
- Add or update unit tests for new functionality
- Write a clear description in your pull request explaining what changed and why
- One reviewer approval required before merge

### Getting Started

1. Fork the repository
2. Create a feature branch: `git checkout -b feat/my-feature`
3. Make your changes
4. Run the test suite locally: `cmake --build build --target test`
5. Open a pull request against `main`

---

## Track B: Safety Contributions (ASIL-D Process)

This track covers all changes to the `safety/` and `kernel/safety/` directories. These partitions operate at ASIL-D(D) and are subject to the full ISO 26262 Part 6 software development process.

**Do not submit safety contributions casually.** Each change must be traceable to a safety requirement, reviewed against MISRA-C:2012, and covered by MC/DC tests.

### Requirements

#### Compliance
- All code must comply with **MISRA-C:2012** (mandatory and required rules). Deviations require a formal deviation record referencing the applicable MISRA rule, justification, and compensating measure. Deviation records are stored in `safety/doc/misra_deviations/`.
- No use of dynamic memory allocation (`malloc`, `free`) in safety partition code.
- No recursion in safety partition code.
- All functions must have a single point of return (MISRA Rule 15.5).

#### Test Coverage
- **MC/DC (Modified Condition/Decision Coverage)** is required for all safety partition functions. 100% statement and branch coverage is the minimum bar; MC/DC is required to satisfy ISO 26262 Part 6 Table 10 for ASIL-D.
- Test results must be captured and archived as certification evidence.
- Tests must be deterministic and runnable in CI.

#### Traceability
- Every change must reference a software safety requirement (e.g., `SSR-042`) in the commit message body.
- If the change affects the FMEA, update `safety/doc/FMEA_TEMPLATE.md` accordingly.
- If the change affects the architectural design, update `docs/ARCHITECTURE.md` and note the change in the pull request.

#### Review
- **Two reviewer approvals are required**, at least one of which must be from @safety-team.
- Reviewers must explicitly confirm MISRA-C compliance and MC/DC coverage in their review comments.
- The @safety-team reviewer will add a `safety-approved` label when satisfied.

### ASIL-D Contribution Process

1. Open a GitHub issue describing the safety requirement or defect being addressed. Tag it `safety`.
2. Create a branch following the naming convention: `safety/SSR-NNN-short-description`
3. Implement changes with full MISRA-C compliance
4. Write MC/DC tests and verify coverage with your coverage tool
5. Update relevant safety documentation (FMEA, safety plan, requirements traceability matrix)
6. Open a pull request against `main`, filling in the safety PR template
7. Respond to review comments from @safety-team; all comments must be resolved before merge
8. After merge, safety-team will update the safety case evidence log

---

## Coding Standards

### SafetyFunction Partition (`safety/`, `kernel/safety/`)

- **Language standard:** C11 (compiled with `-std=c11 -pedantic`)
- **Compliance:** MISRA-C:2012 all mandatory and required rules
- **Forbidden constructs:**
  - Dynamic memory allocation
  - Recursion
  - `setjmp`/`longjmp`
  - Unbounded loops (use loop-count limits with safety assertions)
  - Implicit type conversions (cast explicitly)
- **Naming:**
  - Safety functions: `safety_<module>_<action>()` (e.g., `safety_mailbox_receive()`)
  - Safety types: `safety_<name>_t`
  - Constants: `SAFETY_<NAME>` (all caps)
- **Comments:** Every function must have a Doxygen block including `@safety_req` tag referencing the SSR ID

### QM Partition (All other directories)

- **Language standard:** C99
- **Style:** Follow the existing code style in the file you are editing
- **Naming:** Follow existing conventions per module
- **No safety-critical patterns required**, but clean and readable code is expected

### General Rules (Both Tracks)

- No trailing whitespace
- Unix line endings (LF)
- 4-space indentation (no tabs)
- Maximum line length: 100 characters
- `clang-format` configuration is provided in `.clang-format` — run it before committing

---

## Commit Message Convention

This project uses **Conventional Commits** (https://www.conventionalcommits.org/).

### Format

```
<type>(<scope>): <short summary>

[optional body]

[optional footer(s)]
```

### Types

| Type | When to use |
|---|---|
| `feat` | New feature |
| `fix` | Bug fix |
| `safety` | Safety partition change (ASIL-D track) |
| `docs` | Documentation only |
| `test` | Adding or updating tests |
| `refactor` | Code restructuring without behavior change |
| `build` | Build system or CI changes |
| `chore` | Maintenance (dependency updates, etc.) |

### Safety Commits

Safety commits MUST include a `Safety-Req` trailer referencing the relevant software safety requirement:

```
safety(mailbox): add stale timestamp rejection in receive path

CRC and range checks were already present. This commit adds the
timestamp freshness check to complete the three-part validation
required by SSR-017.

Safety-Req: SSR-017
MISRA-Deviation: none
Reviewed-by: @safety-team
```

### Examples

```
feat(arch): add MPU region configuration for Cortex-M7

fix(freertos): correct stack alignment on M4 exception entry

safety(watchdog): enforce minimum kick interval lower bound

docs: add ADR-002 MPU partition strategy

test(mailbox): add MC/DC tests for CRC validation path
```

---

## Branch Naming

| Branch type | Pattern | Example |
|---|---|---|
| Safety (ASIL-D) work | `safety/<SSR-ID>-description` | `safety/SSR-017-mailbox-timestamp` |
| Feature (QM) | `feat/description` | `feat/m7-mpu-support` |
| Bug fix | `fix/description` | `fix/stack-alignment` |
| Documentation | `docs/description` | `docs/adr-002` |
| Release | `release/vX.Y.Z` | `release/v0.2.0` |

The `safety/` branch prefix is a signal to CI that the ASIL-D pipeline (MISRA check, MC/DC report, two-reviewer gate) will be enforced.

---

## Pull Request Process

1. Ensure your branch is up to date with `main` before opening a PR
2. Fill in the pull request template completely — incomplete templates will be closed
3. Link the relevant GitHub issue in the PR description
4. For safety PRs: attach your MC/DC coverage report as a PR artifact
5. Do not merge your own PR
6. Squash-merge is preferred for QM track; merge commit is required for safety track (to preserve traceability)

---

## Code of Conduct

This project follows the [Contributor Covenant v2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/). Be respectful and constructive in all interactions. Safety review comments are technical, not personal.

---

## Questions?

Open a GitHub Discussion or contact the maintainers via the issue tracker. For safety process questions, tag @safety-team directly.
