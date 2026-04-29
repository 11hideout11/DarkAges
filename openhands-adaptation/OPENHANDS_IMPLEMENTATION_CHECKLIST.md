# OpenHands Implementation Checklist
Phase-by-phase action items for OpenHands + Hermes integration

---

## Phase 0: Foundation — COMPLETE

### Completed
- [x] Created `openhands-adaptation/` directory
- [x] Copied 26 OpenHands skill .md files to `reference/`
- [x] Wrote skill conversion matrix (`OPENHANDS_SKILL_MAPPING.md`)
- [x] Wrote full technical review (`OPENHANDS_ADAPTATION_REVIEW.md`)
- [x] Wrote setup guide (`OPENHANDS_SETUP_GUIDE.md`)
- [x] Wrote usage patterns (`OPENHANDS_USAGE_PATTERNS.md`)
- [x] Created `.openhands/` directory structure with 4 microagents
- [x] Made `setup.sh` executable
- [x] Published index `OPENHANDS_INTEGRATION_INDEX.md` at repo root

---

## Phase 1: Skill Conversion — NEXT (Day 2–3)

### Task 1.1: Enhance code-review.py (30 min)
- [ ] Open existing `~/.hermes/skills/scripts/code-review.py`
- [ ] Review OpenHands `code-review.md` scoring rubric
- [ ] Add categories to output: Style, Clarity, Security, Bugs
- [ ] Format output with `[file, line] : emoji : category: message`
- [ ] Test on `src/server/combat/CombatSystem.cpp`

### Task 1.2: Create testing-pipeline.py (2 hours)
- [ ] Script reads git diff to find changed test files
- [ ] Maps changed source → affected tests via Catch2 TEST_CASE name pattern
- [ ] Runs subset: `ctest -R <regex>`
- [ ] Optionally run flaky detection: repeat failure N times, bisect config
- [ ] Output JSON: `{"tests_to_run": [...], "flaky_candidates": [...]}`

### Task 1.3: Create docker-manage.py (1.5 hours)
- [ ] Subcommands: start|stop|restart|status|logs [service]
- [ ] Reads `docker-compose.yml` at repo root
- [ ] `docker compose` wrapper with health-check polling
- [ ] `status` returns running/ exited + exit codes
- [ ] `logs` mirrors compose logs with service filter
- [ ] Test: `docker-manage.py status` shows all services

### Task 1.4: Create security-audit.py (2 hours)
- [ ] Scan C++/C#/CMake/Python for hardcoded secrets (regex patterns)
- [ ] Call `git-secrets --scan` if available; else fallback grep
- [ ] Optionally call `pip-audit` if pyproject or requirements present
- [ ] Report JSON: `[{"file": str, "line": int, "severity": "high|med|low", "pattern": str}]`
- [ ] Exit code: 0 if clean, 1 if findings

### Task 1.5: Gate-check Integration (30 min)
- [ ] Edit `gate-check.py` to optionally call `security-audit.py`
- [ ] Fail gate if HIGH severity finding present
- [ ] Update gate report to include security section

---

## Phase 2: Validation (Day 4)

### Task 2.1: Skill Script Testing
- [ ] Each skill returns valid JSON (use `jq` or Python `json.loads()`)
- [ ] Each skill works on real DarkAges code
- [ ] Negative tests: dry-run mode doesn't modify any files
- [ ] Document known limitations in `--help` output

### Task 2.2: Microagent Loading
- [ ] Confirm `.openhands/setup.sh` runs without errors
- [ ] All 4 microagent files well-formed YAML frontmatter
- [ ] Hermes agents can read them (file paths accessible)
- [ ] No 4.6 Godot references leaked into our microagents

### Task 2.3: Documentation Completeness
- [ ] Update `AGENTS.md` with OpenHands integration section
- [ ] Add skill invocation examples to `ai_coordination.md`
- [ ] Ensure all docs reference Godot 4.2, not 4.6

### Task 2.4: End-to-End Integration
1. Open Hermes session
2. Delegate to `engine-programmer`: "Create new demo zone zone.json"
3. Verify agent uses `.openhands/microagents/` context appropriately
4. Confirm skill invocation works
5. Run `gate-check.py` manually on test branch

---

## Phase 3: Advanced Features (Week 2, On-Demand)

### Task 3.1: Optional OpenHands Service
- [ ] Evaluate need for Docker sandbox isolation
- [ ] If needed, deploy `openhands` via Docker Compose to /opt/openhands
- [ ] Write `openhands-client.py` thin wrapper for HTTP API
- [ ] Test submitting simple task through service
- [ ] Document service lifecycle in `OPENHANDS_USAGE_PATTERNS.md`

### Task 3.2: Additional Skill Conversion
- [ ] kubernetes.md → k8s-validate.py (only if pursuing cloud demo)
- [ ] github.md → github-pr-automation.py (if GitHub Actions integration wanted)
- [ ] address_pr_comments.md → pr-comment-responder.py (optional)

### Task 3.3: Fallback & Monitoring
- [ ] All skills check for optional deps and degrade gracefully
- [ ] Log warnings when OpenHands service not available
- [ ] Hermes falls back to internal capabilities automatically

---

## Phase 4: Maintenance

### Quarterly
- [ ] Review usage metrics of each skill (frequency in gate-check logs)
- [ ] Prune unused conversions
- [ ] Sync upstream OpenHands skills (`git pull` on /tmp/openhands)
- [ ] Re-apply custom patches if upstream changed skill content

### When DarkAges stack changes
- [ ] Update `.openhands/microagents/` with new conventions
- [ ] Adjust skill scripts to build/test changes
- [ ] Document new domain patterns

---

## Sign-off

When all Phase 1–2 tasks are done:
- [ ] Full test suite passes (1285 cases / 96 files / 11 suites)
- [ ] No new test regressions
- [ ] Gate-check runs clean on test branch
- [ ] Documentation updated in AGENTS.md
- [ ] Team notified of new capabilities

---

**Tracking:** Use GitHub Issues or linear project to track each task individually.

Last updated: 2026-04-29
