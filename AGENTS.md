# DarkAges MMO — Agent Map

## Build & Validate
```bash
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)
cd build_validate && ctest --output-on-failure -j8
```

## State (2026-04-24)
- Phase 8: COMPLETE — all core gameplay systems
- Phase 9: COMPLETE — performance budgets pass (400 ents <20ms, 800 <30ms)
- Networking: STABLE — live validator passes (1-10 clients, snapshots OK)
- Tests: 1978 cases / 88 files / 11 suites — ALL PASS
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
- Client/Server Integration: Godot client entity interpolation end-to-end (C# fixes applied, pending build test)
- Documentation Sync: PROJECT_STATUS.md, DarkAges_Comprehensive_Review.md may be stale
- Test Depth: ZoneServer tests flagged as shallow by ratio heuristic (source under active expansion)

## Recent Commits (last 10)
1. visual(P1): complete floating combat text for local player
2. Combat(P1): wire PredictedPlayer combat events for hit/die feedback (merged)
3. Combat(P1): server-authoritative RPC handshake with GCD (merged)
4. Harness engineering: two-layer evaluator architecture (objective + subjective) + tool subtraction
5. Fix lag comp `calculateAttackTime` double-count latency; tests updated (124 combat tests pass)
6. Validator: combat validation phase (`--combat`, `--combat-duration`)
7. Fix C# `EntityFrame` visibility (private→public) for `RemotePlayerManager`
8. Validator: NPC replication over network (`--npcs` + `--npc-count`)
9. Validator: latency/packet-loss sim (30ms + 5% loss resilient)
10. Fix live client validator: snapshot replication includes viewer, yaw/pitch clamp, threading crash fix, input dedup
---
Last updated: 2026-04-26
