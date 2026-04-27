     1|Hermes Agent — DarkAges MMO Continuation Session
     2|Start date: 2026-04-27 (or later)
     3|
     4|## Immediate Context
     5|
     6|- Repository: /root/projects/DarkAges
     7|- Branch: main (up to date with origin)
     8|- Working tree: clean
     9|- Last merged PRs:
    10|  * #21: combat(P2) — hit stop, procedural leaning, animation blend polish
    11|  * #22: docs+test(depth) — PROJECT/ComprehensiveReview sync; ZoneServer depth tests (6 new)
    12|- Test status: 1284 cases / 7211 assertions — ALL PASS
    13|- Build: `cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF && cmake --build build_validate -j$(nproc)` — passes
    14|
    15|**Read these files first:**
    16|1. AGENTS.md (architecture, critical rules, workflow)
    17|2. PROJECT_STATUS.md (current version 5.4, what's done/remaining)
    18|3. HERMES_HANDOFF_20260426.md (this handoff, created 2026-04-26)
    19|
    20|---
    21|
    22|## Your Mission
    23|
    24|Choose ONE of the remaining Phase 2 (P2) items and implement it following the autonomous workflow:
    25|
    26|**Option A — Lock-on Targeting** ✅ COMPLETED (2026-04-27)
    27|  - Server-authoritative target locking system
    28|  - Client prediction + server validation
    29|  - Glow/ping UI indicator on locked target
    30|  - Auto-attack integration merged (CombatSystem respects confirmed lock)
    31|  - Scope implemented: server-side TargetLockSystem + ZoneServer integration; client-side UI/input already complete
    32|
    33|**Option B — Camera Polish** (recommended next)
    34|  - Smooth follow with configurable deadzone
    35|  - Collision avoidance (raycast push-in)
    36|  - Height/rotation smoothing
    37|  - Scope: modify `CameraController` (C#) + optional server-side camera state replication
    38|
    39|**Phase 3/4 backlog** (abilities, inventory, full UI overhaul) remains after P2 completion.
    40|
    41|---
    42|
    43|## Required Autonomous Workflow (non-negotiable)
    44|
    45|1. Create feature branch: `autonomous/YYYYMMDD-{slug}` (e.g., autonomous/20260427-lock-on)
    46|2. Implement in small, testable increments
    47|3. After each logical change:
    48|   - Build: `cmake --build build_validate -j$(nproc)`
    49|   - Test: `ctest --output-on-failure -j8`
    50|   - Verify zero regressions (compare test count/assert count to baseline 1284/7211)
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
    90|2. All tests pass (no regressions; baseline 1284 cases, 7211 assertions)
    91|3. No test count decrease
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