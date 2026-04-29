# DarkAges MMO — Agent Map

## LLM Cost Control (Updated 2026-04-28)
- **Model Config**: `.hermes/config.json` enforces banned models (Kimi 2.6/2.5), auxiliary agent model tiers (forced to haiku), and token limits.
- **Auxiliary Agents**: `writer`, `community-manager`, `prototyper`, `qa-tester`, `accessibility-specialist` forced to `haiku` (cheapest tier).
- **Banned Models**: `moonshot/kimi-2.6`, `moonshot/kimi-2.5`, `kimi-2.6` — automatically replaced with `haiku` if requested.
- **Token Optimization**: Context truncation (max 500 lines for aux), prompt minimization for non-critical agents, enforced token limits per tier.

## Build & Validate
```bash
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)
cd build_validate && ctest --output-on-failure -j1
```

## State (2026-04-29)
- Phase 8: COMPLETE — all core gameplay systems
- Phase 9: COMPLETE — performance budgets pass (400 ents <20ms, 800 <30ms)
- Networking: STABLE — live validator passes (1-10 clients, snapshots OK)
- Tests: **1300 cases / 11 suites — ALL PASS (7243 assertions)** (as of 2026-04-29 build)
- Server: ~32K LOC (C++20, EnTT ECS, 60Hz tick) | Client: ~9K LOC (C# Godot 4.2)
- Harness: Two-layer evaluator architecture operational (objective + subjective)
- **MVP Criteria Updated (2026-04-28)**: New criteria require full third-person combat multiplayer template with demo zones and gameplay. **Project is NOW FULLY DEMO READY under updated standards** — all critical systems + Foot IK + Controls docs complete.

## Architecture
- **ECS**: EnTT, `DarkAges::` namespace. Components in `ecs/`, systems in `combat/`, `physics/`, `zones/`
- **Netcode**: `NetworkManager` (single-threaded, UDP, main-tick I/O). `GNSNetworkManager` gated by `ENABLE_GNS`. `ProtobufProtocol`. Test builds use stubbed net layer.
- **DB**: `RedisManager`, `ScyllaManager` — stubs when disabled. Do NOT test real DB behavior in CI.
- **Zones**: `ZoneServer`, `ZoneOrchestrator`, `EntityMigration`, `ZoneHandoff`
- **Security**: `PacketValidator`, `AntiCheat`, `StatisticalDetector`, `MovementValidator`, `RateLimiter` (functional, not active scope)
- **Monitoring**: `MetricsExporter` (Prometheus/Grafana format)
- **Gameplay**: `CombatSystem`, `AbilitySystem`, `NPCAISystem` + A*, `PartySystem`, `GuildSystem`, `LootSystem`, `XP`/Progression, `Inventory`, `ItemSystem`, `ConsumableSystem`, `QuestSystem`, `ChatSystem` (Local/Global/Whisper/Party/Guild), `CraftingSystem`, `TradingSystem`, `ZoneEventSystem`, `DialogueSystem`, `SpawnSystem`

## Critical Rules
- Namespace is `DarkAges::` everywhere — never `darkages`
- EnTT: use `registry.all_of<T>()` not `registry.has<T>()`; no `view.size()`; entity enum != int
- **EnTT pointer invalidation**: after `registry.emplace<T>()`, re-fetch pointers/refs. Use `registry.get<T>()` when component must exist.
- Forward-declared types (`struct Foo;`) cannot use `sizeof()` in tests
- Nested types need qualified names (e.g., `RedisInternal::PendingCallback`)
- `Protocol.cpp` excluded when `ENABLE_GNS=OFF`; test builds use stubbed network layer
- Redis/Scylla stubs used when disabled — don't test real behavior in CI

## Autonomous Loop
- **Orchestrator**: `scripts/autonomous/cron_dev_loop.py` (Generator)
- **Objective Evaluator**: `scripts/autonomous/evaluate_change.py` — NEVER modifies code; runs `cmake --build` + `ctest` independently on current tree
- **Subjective Evaluator**: `scripts/autonomous/evaluate_change_review.py` — OpenCode CLI skeptical reviewer; reads diff + AGENTS.md and critiques without modifying code
- **Discovery**: `scripts/autonomous/discover_tasks.py` — 77% of heuristics disabled (tool subtraction); cache at `.task_cache.json`
- **Wrapper**: `scripts/autonomous/cron_robust_wrapper.py` — locks, retries, failure backoff
- **Schedule**: hourly quick (`once`), 6am/6pm UTC deep (`deep`, 3 tasks)
- **Workflow**: feature branch `autonomous/YYYYMMDD-{slug}` → implement → loop-detect → budget-check → objective evaluator PASS → subjective reviewer PASS → commit → merge to main
- **Loop Detection**: max 3 edits/file/day; exceeded → skip with "Consider reconsidering your approach"
- **Pre-Completion Gate**: BOTH evaluators must report PASS + zero test regression before merge. Subjective reviewer: critical issues block merge; warnings are logged but non-blocking.
- **Pre-Completion Checklist** (objective evaluator): explicit 7-item checklist (build_compiles, tests_pass, no_regression, explicit_test_summary, test_count_positive, assert_count_positive, baseline_readable) — prevents premature "done"
- **Fail Closed**: if objective evaluator is missing, generator aborts — never falls back to inline self-test
- **Reasoning Budget**: per-category wall-clock limits (test=10min, test-depth=15min, refactor=5min); hard abort if exceeded
- **Operation Budget**: per-category action limits (subprocess+file ops as proxy for token ceiling); hard abort if exceeded
- **Sprint Decomposition**: `once` mode skips tasks >2h estimated; `deep` mode allows up to 1.5x budget
- **Harness Audit**: components in `HARNESS_COMPONENTS` dict with expiration dates; review quarterly

## Gaps (Updated 2026-04-28 per new MVP criteria)
- **MVP CRITICAL**: Full third-person combat multiplayer template **COMPLETE** — FSM integrated with AnimationStateMachine
- **MVP CRITICAL**: Demo zones — 3 zones created (tutorial.json, arena.json, boss.json) — COMPLETE
- **MVP HIGH**: Foot IK — SkeletonIK3D nodes configured with terrain alignment — COMPLETE
- **MVP HIGH**: Human playability — All controls validated and documented — COMPLETE
- **MVP MEDIUM**: Blend spaces — Not yet implemented (can be added post-MVP)
- **MVP MEDIUM**: Visual polish — SDFGI+SSAO+SSIL enabled in Main.tscn; extended demo deep validation passed (all artifacts non-fatal)

## Recent Commits (last 10 — updated)

1. Merge PR #27: fix MetricsExporter reinitialization
2. docs: update Recent Commits with FootIKController fix
3. fix: resolve client build failures and refresh metrics
4. feat(combat): implement Foot IK and complete playability validation
5. docs: update test results with root cause analysis
6. Merge pull request #26: fix cmake installer and test results
7. fix: add cmake installer and document test results
8. fix(client): correct Player scene
9. fix(client): correct Player scene
10. fix(server): correct CMakeLists.txt dependency handling
11. fix(server): correct CMakeLists.txt dependency handling
---


## OpenHands Integration Updates (2026-04-29)

- Added 4 new standalone skills: `test-flakiness.py`, `coverage-report.py`, `pr-create.py`, `pr-comment.py`, `code-format.py`
- Fixed CMake JSON include bug — switched from direct `target_include_directories` to `target_link_libraries(nlohmann_json::nlohmann_json)` to resolve missing header error in `build_validate`
- Skills installed to `~/.hermes/skills/scripts/` as symlinks to `openhands-adaptation/skills/`
- Microagents present in `.openhands/microagents/` for Godot 4.2 pinning, server C++ conventions, networking, repo context
- Documentation: `OPENHANDS_SKILLS_REFERENCE.md` — comprehensive skill reference

Last updated: 2026-04-29