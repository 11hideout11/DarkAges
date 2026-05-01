# Multi-Agent Coordination Protocol
**Hermes Local Agent + OpenHands Cloud Agents | DarkAges Monorepo**

> Last updated: 2026-05-01
> Scope: Synchronous coordination between local Hermes workspace and cloud-based OpenHands agents that share the same GitHub repository.

---

## 1. Overview

Two agent systems operate on the same codebase:
- **Hermes Agent** — local AI coding assistant, working interactively in your terminal workspace
- **OpenHands Agents** — autonomous cloud processes running via GitHub Actions / scheduled jobs, performing batch iterations

Both push to and pull from the same `origin` remote. This protocol establishes safe, non-destructive coordination to avoid conflicts, race conditions, and regressions.

---

## 2. Repository Synchronization Contract

### 2.1 Assumptions
- Both systems use **branch+PR workflow**: work occurs on feature branches and merges via PR
- All pushes require **passing tests** (no new test regressions)
- **Two-agent review** for PRs: objective (automated) + subjective (architectural) both PASS
- Pre-existing failures are acceptable; new regressions block merge
- `BUILD_TESTS=ON` for all builds; `GNS=OFF/REDIS=OFF/SCYLLA=OFF` for local iteration

### 2.2 Fetch-First Policy
Every Hermes session begins with:
```bash
git fetch origin
git status -sb
```
This reveals:
- `## main...origin/main` → behind; need to pull/rebase
- `## main...origin/<branch>` → local branch diverged
- Clean working tree → safe to proceed

### 2.3 Before Any Git Write
Before committing/pushing:
```bash
git fetch origin
git log --oneline --decorate -5 --all   # see what others pushed
git status -sb                          # confirm tree is clean
```
If local commits diverge, rebase onto `origin/main` before pushing.

---

## 3. Conflict Avoidance Rules

### 3.1 Domain Partitioning (Preferred)
Agents agree on work domains to minimize overlap:
| Domain | Primary Agent | Secondary |
|--------|--------------|-----------|
| CI/CD, build system, CMake   | Hermes (local) | OpenHands |
| Test infrastructure, harness | OpenHands | Hermes |
| Core gameplay ECS systems    | Either (coordinated) | — |
| Art pipeline, asset manifest | Hermes | — |
| Server networking GNS layer  | OpenHands | Hermes for review |

**When in doubt:** communicate via PR description; request explicit review from the other agent system.

### 3.2 File Ownership Awareness
Check `git blame` or recent commit history before modifying files heavily touched by another agent recently:
```bash
git log --since="1 week" -p -- <relative/path>
```
If the file was just modified by OpenHands (visible via recent remote commits), consider:
- Requesting OpenHands to handle follow-up work via a new job
- Opening a draft PR and @-mentioning the other agent system for review

---

## 4. Change Visibility & Awareness

### 4.1 Hermes → OpenHands
Hermes commits are pushed to `origin` and appear in OpenHands' next fetch cycle (within 15 minutes). No explicit notification needed; OpenHands naturally sees the new commit on its next run.

### 4.2 OpenHands → Hermes
OpenHands commits appear on `origin` within minutes of job completion. Hermes sees them on next `git fetch`.  
To inspect precisely:
```bash
git fetch origin
git log --oneline origin/main -10
```

### 4.3 PR-Based Communication
Both agents SHOULD use PRs as the handoff mechanism:
- Open PR → other agent can review, test, comment
- PR description cites which agent owns this work package
- PR title prefix convention: `[openhands]` or `[hermes]`

---

## 5. Verification Process (Pre-Merge Gate)

Every PR must pass:

1. **Build** — `cmake -B build -S . -DBUILD_TESTS=ON && cmake --build build -j$(nproc)`
2. **All Tests** — `ctest --test-dir build -j$(nproc) --output-on-failure`
3. **No New Regressions** — test count/assertion count matches baseline
4. **Two-Agent Review** — at least one APPROVE from each review category:
   - Objective: automated tests only
   - Subjective: architectural fit, style, maintainability
5. **Remote Sync Verified** — post-merge:
   ```bash
   git fetch origin
   git status -sb   # should show "up to date"
   ```

---

## 6. Automation Scripts

### 6.1 `.hermes/sync-check.sh`
Dash-line script that prints:
```
BRANCH:       main → origin/main [✓ synced]
REMOTE HEAD:  abc1234 [2 commits ahead]
UNTRACKED:    0 files
UNSTAGED:     0 files
STAGED:       2 files
```
Run before starting work and before pushing.

### 6.2 OpenHands Job Output
OpenHands cron jobs write to:
- `AUTONOMOUS_LOG.md` — human-readable transcript
- `cron_audit_YYYY-MM-DD.json` — structured metrics per run

Hermes can read these files to understand what OpenHands did recently.

---

## 7. Emergency Brakes

If concurrent work threatens conflicts:
1. **Pause:** both agents stop committing to `main` immediately
2. **Coordinate:** PR-based discussion decides merge order
3. **Rebase & Test:** whichever branch lags rebases onto the other
4. **Resume:** both resume after merge lands on `origin/main`

---

## 8. Model & Cost Conventions

- Daily Autonomous Iteration: `stepfun/step-3.5-flash` (cost-effective, reliable)
- Deep iteration: `stepfun/step-3.5-flash` with 3-cycle deep mode
- Fallback: Nous → NVIDIA → OpenAI Codex (last resort)
- Track spends via provider dashboards

---

## 9. References
- `AGENTS.md` — project-wide conventions and State
- `OPENHANDS_INTEGRATION_INDEX.md` — OpenHands setup
- `scripts/autonomous/cron_dev_loop.py` — orchestrator
- `scripts/autonomous/discover_tasks.py` — task discovery (NOW includes PRD parsing)

---

*"Coordinate through PRs, verify with tests, communicate through code."*