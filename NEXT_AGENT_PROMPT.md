     1|Hermes Agent — DarkAges MMO Continuation Session
     2|Start date: 2026-04-28 (or later)
     3|
## Immediate Context

- Repository: /root/projects/DarkAges
- Branch: main (up to date with origin)
- Working tree: clean
- **MVP Criteria Updated (2026-04-28)**: See MVP_DEMO_STANDARDS.md for NEW requirements
- **PROJECT NOT READY for Demo MVP**: Requires full third-person combat multiplayer template with demo zones and gameplay
- Last merged PRs:
  * #21: combat(P2) — hit stop, procedural leaning, animation blend polish
  * #22: docs+test(depth) — PROJECT/ComprehensiveReview sync; ZoneServer depth tests (6 new)
  * #23: feat(npc) — E-key interaction system
- Test status: 2097 cases / 12586 assertions — ALL PASS
- Build: `cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF && cmake --build build_validate -j$(nproc)` — passes

**Read these files first:**
1. AGENTS.md (architecture, critical rules, **GAPS section for updated MVP criteria**)
2. MVP_DEMO_STANDARDS.md (**NEW criteria**: third-person combat template, demo zones, gameplay loop)
3. MVP_IMPLEMENTATION_PLAN.md (**implementation roadmap** for new criteria)
4. PROJECT_STATUS.md — Current state summary

---

## Your Mission (UPDATED: 2026-04-28)

**The demo MVP criteria have been updated since project start.** Previous completion of Phases 0-9 and visual polish does NOT satisfy the new MVP bar.

### NEW MVP Requirements:

1. **Full Third-Person Combat Multiplayer Template**
   - Complete FSM (node-based state machine) - NOT YET DONE
   - Hitbox/hurtbox server-authoritative validation - NEEDS VALIDATION
   - AnimationTree procedural features (Foot IK) - PARTIAL
   - Lock-on targeting - COMPLETED

2. **Demo Zones (minimum 3)**
   - Tutorial zone - NOT YET DONE
   - Combat arena - NOT YET DONE  
   - Boss zone - NOT YET DONE
   - Current: Only basic zone 99 exists

3. **Complete Gameplay Loop**
   - Human-playable (WASD + mouse + attack) - NEEDS VALIDATION
   - Combat → loot → quest → progression - NEEDS INTEGRATION
   - Curated demo experience - NOT YET DONE

**Choose ONE item from the CRITICAL gaps and implement following autonomous workflow:**
    25|
**Option A — Lock-on Targeting** ✅ COMPLETED (2026-04-27)
 - Server-authoritative target locking system
 - Client prediction + server validation
 - Glow/ping UI indicator on locked target
 - Auto-attack integration merged (CombatSystem respects confirmed lock)
 - Scope implemented: server-side TargetLockSystem + ZoneServer integration; client-side UI/input already complete

**Option B — Camera Polish** ✅ COMPLETED
 - Smooth follow with configurable deadzone
 - Collision avoidance (raycast push-in)
 - Height/rotation smoothing
 - Scope: CameraController node

**Option C — NPC Interaction** ✅ COMPLETED (2026-04-27)
 - E-key interaction system
 - Extended input protocol
 - Dialogue system integration

---

## Current CRITICAL Gaps (2026-04-28)

### 1. CombatStateMachine (FSM) — CRITICAL
Status: CombatSystem exists but lacks formal node-based state machine
Action: Create CombatStateMachine.tscn with Idle, Moving, Attacking, Hit, Dodging, Dead states

### 2. Demo Zones — CRITICAL
Status: Only zone 99 exists
Action: Create tutorial.json, arena.json, boss.json zone configs with proper pacing

### 3. Hitbox/Hurtbox Validation — CRITICAL
Status: Collision layers exist, need server-authoritative validation tests
Action: Add comprehensive tests, document collision layer matrix

### 4. Foot IK — HIGH
Status: AnimationTree operational but no Foot IK
Action: Add SkeletonIK3D node, configure terrain alignment

---

## Suggested Next Tasks

**Recommended:** CombatStateMachine (FSM)
- High impact on "third-person combat template"
- Creates reusable architecture
- Foundation for other combat polish features

**Alternative:** Demo Zones
- Tangible progress toward MVP bar
- Can be done in parallel with combat work
- Clear definition of done

**Alternative:** Hitbox/Hurtbox Validation
- Required for "server-authoritative validation"
- Test-focused, low risk
- Solid foundation for combat tuning
    40|
    41|---
    42|
    43|## Required Autonomous Workflow (non-negotiable)
    44|
    45|1. Create feature branch: `autonomous/YYYYMMDD-{slug}` (e.g., autonomous/20260427-lock-on)
    46|2. Implement in small, testable increments
    47|3. After each logical change:
    48|   - Build: `cmake --build build_validate -j$(nproc)`
    49|   - Test: `ctest --output-on-failure -j1`
    50|   - Verify zero regressions (compare test count/assert count to baseline 2096/12585)
    51|4. When complete:
    52|   - Run objective evaluator: `python3 scripts/autonomous/evaluate_change.py autonomous/YYYYMMDD-{slug} --base main`
    53|   - Address any FAIL before proceeding
    54|   - Run subjective reviewer: `python3 scripts/autonomous/evaluate_change_review.py --branch autonomous/YYYYMMDD-{slug} --base main`
    55|   - If reviewer reports critical issues → fix; timeout = inconclusive (OK)
    56|5. Push branch → create PR via `gh pr create`
    57|6. Merge only after BOTH evaluators report PASS (or reviewer timeout with no critical issues)
    58|7. Update AGENTS.md Recent Commits section on main after merge
    59|8. Delete branch after merge
    60|
    61|---
    62|
    63|## Critical Rules Reminder
    64|
    65|- Namespace `DarkAges::` everywhere (server C++)
    66|- EnTT: `registry.all_of<T>()`, never `view.size()`, no entity→int implicit casts
    67|- After `registry.emplace<T>()` re-fetch any stored pointers/refs
    68|- Forward-declared types: no `sizeof()` in tests
    69|- Nested types: fully qualified (`ScyllaInternal::Callback`)
    70|- Test builds: `ENABLE_GNS=OFF`, `ENABLE_REDIS=OFF`, `ENABLE_SCYLLA=OFF` — use stubs, never test real DB/network in unit tests
    71|- Objective evaluator requires explicit checklist items (build_compiles, tests_pass, no_regression, explicit_test_summary, test_count_positive, assert_count_positive, baseline_readable)
    72|
    73|---
    74|
    75|## Skill References
    76|
    77|Load these skills before starting:
    78|- `autonomous-codebase-iteration` — general autonomous workflow
    79|- `darkades-codebase-conventions` — DarkAges-specific gotchas (EnTT, namespace, forward-decl)
    80|- `cpp-build-fix` — systematic build failure diagnosis
    81|- `autonomous-test-generation` — write meaningful behavioral tests (not stubs)
    82|- `github-pr-workflow` — branch→commit→push→PR→merge pattern
    83|
    84|---
    85|
    86|## Evaluation Criteria (objective evaluator checklist)
    87|
    88|Your PR must satisfy:
    89|1. Build compiles cleanly (zero errors; warnings OK but not ideal)
    90|2. All tests pass (no regressions; baseline 2097 cases, 12586 assertions)
    91|3. No test count decrease from baseline 2097/12586
    92|4. No assertion count decrease
    93|5. Test summary explicitly listed in evaluator output
    94|6. Baseline metrics readable (build_validate test log)
    95|7. Subjective reviewer: no critical issues blocking merge
    96|
    97|---
    98|
    99|## Final Git Safety Check
   100|
   101|Before you start, verify main is clean:
   102|```bash
   103|git checkout main
   104|git pull --ff-only
   105|git status  # should report 'nothing to commit, working tree clean'
   106|```
   107|
   108|If any `autonomous/*` branches exist locally that aren't merged, merge or delete them first.
   109|
   110|---
   111|
   112|**You now have full context. Proceed with lock-on targeting (recommended) or camera polish. Follow the workflow strictly, update docs/AGENTS.md after merge, and keep the codebase in merge-ready state at all times.**
