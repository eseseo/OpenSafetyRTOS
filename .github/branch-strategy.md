# Branch Strategy — OpenSafetyRTOS

This document defines the branching model for the OpenSafetyRTOS project.
All branches follow [Conventional Commits](https://www.conventionalcommits.org/) for commit messages.

---

## Branch Definitions

### `main`
- **Purpose:** Stable, production-ready code. Every commit on `main` represents a released version.
- **Access:** Protected. No direct pushes. Merges only via pull request from `develop` or `hotfix/XXX`.
- **Tagging:** Every merge to `main` must be tagged with a semantic version (e.g., `v0.1.0`).
- **Required reviews:** 2 approvals (including at least 1 Safety Engineer for any kernel/safety changes).
- **CI required:** All checks must pass (build, unit tests, integration tests, static analysis).

### `develop`
- **Purpose:** Integration branch. Represents the latest development state that is intended for the next release.
- **Access:** Protected. No direct pushes. Merges via pull request from `feature/XXX` or `safety/XXX` branches.
- **Required reviews:** 1 approval minimum.
- **CI required:** All checks must pass.
- **Policy:** Must always be in a buildable, testable state.

### `feature/XXX`
- **Purpose:** Development of new features, refactoring, or non-safety improvements.
- **Naming convention:** `feature/<short-description>` (e.g., `feature/task-scheduler`, `feature/uart-hal-driver`)
- **Branched from:** `develop`
- **Merges into:** `develop` via pull request
- **Required reviews:** 1 approval
- **Lifecycle:** Delete branch after merge.

### `safety/XXX`
- **Purpose:** Implementation of ASIL-D safety functions, safety mechanisms, or changes to any code
  in the ASIL-D partition (`kernel/safety/`, `safety/`).
- **Naming convention:** `safety/<function-or-ticket>` (e.g., `safety/watchdog-monitor`, `safety/stack-overflow-guard`)
- **Branched from:** `develop`
- **Merges into:** `develop` via pull request
- **Required reviews:** **2 approvals required**, both must include at least one designated Safety Engineer.
- **Additional requirements:**
  - Must include or update relevant safety analysis artifacts in `safety/doc/`.
  - Must include unit tests under `tests/unit/` and safety-specific tests under `safety/tests/`.
  - Static analysis (e.g., PC-lint, Polyspace, or cppcheck with MISRA rules) must be clean.
  - Code review checklist for ASIL-D must be completed and attached to the PR.
- **Lifecycle:** Delete branch after merge.

### `hotfix/XXX`
- **Purpose:** Critical fixes for bugs found in a released version on `main`. Time-sensitive.
- **Naming convention:** `hotfix/<description>` (e.g., `hotfix/null-deref-in-scheduler`)
- **Branched from:** `main` (at the release tag being fixed)
- **Merges into:** Both `main` AND `develop` (to keep develop in sync)
- **Required reviews:** 2 approvals. If the fix touches safety-critical code, Safety Engineer approval mandatory.
- **Tagging:** Triggers a patch release tag on `main` (e.g., `v0.1.1`).
- **Lifecycle:** Delete branch after merge to both targets.

---

## Workflow Diagram

```
main        ──────────────────────────────────────── (tagged releases)
               ↑ merge + tag              ↑ hotfix merge + patch tag
develop     ──────────────────────────────────────── (integration)
               ↑ feature PR  ↑ safety PR
feature/XXX  ──────────────
safety/XXX                   ──────────────
hotfix/XXX                                  ── (from main, merges to main + develop)
```

---

## Pull Request Requirements

| Branch type  | Min. Reviewers | Safety Engineer Required | All CI Green | Safety Artifacts |
|--------------|---------------|--------------------------|--------------|-----------------|
| `main`       | 2             | Yes (if safety code)     | Yes          | N/A             |
| `develop`    | 1             | No                       | Yes          | N/A             |
| `feature/XXX`| 1             | No                       | Yes          | No              |
| `safety/XXX` | **2**         | **Yes (mandatory)**      | Yes          | **Yes**         |
| `hotfix/XXX` | 2             | Yes (if safety code)     | Yes          | If applicable   |

---

## Commit Message Convention

All commits must follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <short description>

[optional body]

[optional footer(s)]
```

**Types:** `feat`, `fix`, `build`, `chore`, `docs`, `refactor`, `test`, `safety`, `perf`, `ci`

**Scopes (examples):** `kernel`, `scheduler`, `hal`, `arch`, `safety`, `tests`, `docs`, `cmake`

**Examples:**
```
feat(scheduler): add round-robin preemptive scheduler
safety(watchdog): implement ASIL-D hardware watchdog monitor
fix(hal): correct SPI clock polarity for STM32F4
build(cmake): add toolchain file for ARM Cortex-M4
```

---

## Release Process

1. Create a release PR from `develop` → `main`.
2. Ensure all tests pass and safety artifacts are up to date.
3. Merge to `main` with squash or merge commit (project preference: merge commit for traceability).
4. Tag the commit: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`.
5. Push tag: `git push origin vX.Y.Z`.
6. Update `CHANGELOG.md` on `develop`.
