# DarkAges MMO — Agent Map

## Build & Validate
```bash
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)
cd build_validate && ctest --output-on-failure -j8
```

## State (2026-04-27)
- Phase 8: COMPLETE — all core gameplay systems
- Phase 9: COMPLETE — performance budgets pass (400 ents <20ms, 800 <30ms)
- Networking: STABLE — live validator passes (1-10 clients, snapshots OK)
- Tests: 1284 cases / 94 files / 11 suites — ALL PASS
+ Tests: 1284 cases / 94 files / 11 suites — ALL PASS (7211 assertions)
- Server: ~32K LOC (C++20, EnTT ECS, 60Hz tick) | Client: ~6.2K LOC (C# Godot 4.2)
- Harness: Two-layer evaluator architecture operational (objective + subjective)

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

## Gaps
- (none — all previously noted gaps resolved as of 2026-04-26)

## Recent Commits (last 10)

1. feat(combat): integrate confirmed lock-on target into auto-attack selection (merged)
2. docs: sync test metrics and completed tasks across status files (1284 tests, 7211 asserts) (merged)
3. test(server): add unit tests for NetworkManager_udp (+27 tests) (merged)
4. test(server): fix ZoneServer test failures from auto-populate and JSON schema (merged)
5. test(server): add unit tests for ServerStateExporter (merged)
6. docs: update test metrics after ZoneServer expansion (merged)
7. test(server): expand ZoneServer unit tests (+13 tests, 58 assertions) (merged)
8. docs: update state metrics and recent commits (2026-04-27) (merged)
9. test(server): fix CombatEventHandler unit test compilation (merged)
10. docs: update Recent Commits for lock-on client confirmation merge (merged)
---
Last updated: 2026-04-27
