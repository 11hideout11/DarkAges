# DarkAges MMO — Agent Map

## LLM Cost Control (Updated 2026-04-29)
- **FREE Models Only**: All agents use Ollama local models (no API keys required):
  - Critical agents (3 directors): `ollama/llama3.1:8b`
  - Standard agents (leads): `ollama/llama3.2:3b`
  - Auxiliary/specialists (all others): `ollama/llama3.2:1b` (smallest/fastest)
- **Banned Models**: All paid APIs banned (Anthropic Claude, OpenAI GPT, Moonshot Kimi)
- **Aggressive Cost Control**: ALL non-critical agents forced to cheapest model (`ollama/llama3.2:1b`)
- **Token Optimization**: Context truncation (max 300 lines), prompt minimization, 512 token limit for auxiliary tasks.

## Build & Validate
```bash
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)
cd build_validate && ctest --output-on-failure -j1
```

## State (2026-04-29)
- Phase 0: COMPLETE — documented in PHASE0_SUMMARY.md
- Phase 1-5: **UNVERIFIED** — No documentation found for implementation
- Phase 6: COMPLETE — build system hardening
- Phase 7: PARTIAL — TEST_SUMMARY.md shows 77% pass rate
- Phase 8: PARTIAL — multiple work packages complete, GNS blocked
- Phase 9: COMPLETE — performance budgets validated
- Tests: **INCONSISTENT** — Various counts (100-2097) with varying pass rates (77%-100%)
- Server: ~32K LOC (C++20, EnTT ECS, 60Hz tick) | Client: ~9K LOC (C# Godot 4.2)
- **⚠️ IMPORTANT**: Project status is NOT CLEARLY DEMO READY. PROJECT_STATUS.md (dated 2026-04-28) explicitly states "NOT READY for Demo MVP" under updated criteria.

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

## Gaps (Updated 2026-04-29)

### 🔴 CRITICAL GAPS - NOT PREVIOUSLY DOCUMENTED

1. **Phase 1-5 Implementation Gap** — No documentation exists for Phases 1-5. Only Phase 0, 6, and 8 have summary docs. Major verification gap.

2. **Conflicting Demo Readiness Claims** — PROJECT_STATUS.md (2026-04-28) states project is "NOT READY for Demo MVP" but AGENTS.md claims "NOW FULLY DEMO READY". **This contradiction MUST be resolved.**

3. **GNS Integration Blocked** — WebRTC submodule access denied. WP-8-6 uses stub UDP instead of Steam's GameNetworkingSockets.

4. **Database Stubs Active** — Redis/ScyllaDB are stubbed. No real persistence testing in CI.

5. **Test Results Inconsistency** — TEST_SUMMARY.md shows 77% pass rate (23 failing tests). Current TEST_RESULTS.md shows 11/11 suites pass. Need reconciliation.

6. **Server Startup Crash (Jan 30)** — PERFORMANCE_BENCHMARK_REPORT.md documents CRITICAL failure. Later docs say fixed but no detailed root cause analysis documented.

7. **Memory Leaks Detected** — Test framework itself has memory leaks (4 blocks in test_scope, 4 bytes leaked in multiple test functions).

8. **No Node-Based FSM** — Uses inline state flags per CRITICAL rules. AGENTS.md incorrectly states "FSM integrated with AnimationStateMachine".

### 🟡 HIGH PRIORITY GAPS

9. **Godot Client Headless Artifacts** — DEMO_CAPABILITIES_REPORT documents: add_child() failures, Transform access before node ready, Quaternion normalization issues.

10. **Protocol.cpp Excluded** — Excluded when ENABLE_GNS=OFF. Protobuf protocol depends on GNS.

11. **Documentation Drift** — Multiple markdown files reference pre-April state. Inconsistent across codebase.

12. **Validator Connection Exhaustion** — live_client_validator.py creates aggressive UDP retries that can exhaust server connection slots.

### 🟢 MEDIUM PRIORITY GAPS

13. **Blend Spaces Not Implemented** — Marked MEDIUM in AGENTS.md. Animation blending incomplete.

14. **Test Count Inconsistent** — Documentation shows 2097 tests, 1300 cases, 100 cases across multiple sources.

### Historical Phases Completion Status

| Phase | Status | Evidence | Notes |
|-------|--------|----------|-------|
| Phase 0 | ✅ COMPLETE | PHASE0_SUMMARY.md | Foundation architecture |
| Phase 1 | ❓ UNVERIFIED | No documentation | Prediction & Reconciliation |
| Phase 2 | ❓ UNVERIFIED | No documentation | Multi-Player Sync |
| Phase 3 | ❓ UNVERIFIED | No documentation | Combat & Lag Compensation |
| Phase 4 | ❓ UNVERIFIED | No documentation | Spatial Sharding |
| Phase 5 | ❓ UNVERIFIED | No documentation | Optimization & Security |
| Phase 6 | ✅ COMPLETE | PHASE_6_COMPLETION_SUMMARY.md | Build System Hardening |
| Phase 7 | ⚠️ PARTIAL | TEST_SUMMARY.md (77% pass) | Unit Testing |
| Phase 8 | ⚠️ PARTIAL | Multiple docs, some failures | Production Hardening |

### Known Issues Requiring Resolution (NOT PREVIOUSLY TRACKED)

1. **Demo Readiness Contradiction** — Must reconcile PROJECT_STATUS.md vs AGENTS.md claims
2. **GNS Integration** — WebRTC access required or permanent stub documentation
3. **Test Suite Discrepancy** — 77% vs 100% pass rate must be reconciled
4. **Memory Leaks** — Test framework leaks must be fixed or documented as expected
5. **Phase 1-5 Documentation** — Cannot verify implementation without docs

---

## Recent Commits (last 10 — updated)

1. docs: comprehensive gap analysis - phase verification, test inconsistencies
2. docs: update Gaps section date to 2026-04-29
3. Merge PR #27: fix MetricsExporter reinitialization
4. docs: update Recent Commits with FootIKController fix
5. fix: resolve client build failures and refresh metrics
6. feat(combat): implement Foot IK and complete playability validation
7. docs: update test results with root cause analysis
8. Merge pull request #26: fix cmake installer and test results
9. fix: add cmake installer and document test results
10. fix(client): correct Player scene
---


## OpenHands Integration Updates (2026-04-29)

- Added 4 new standalone skills: `test-flakiness.py`, `coverage-report.py`, `pr-create.py`, `pr-comment.py`, `code-format.py`
- Fixed CMake JSON include bug — switched from direct `target_include_directories` to `target_link_libraries(nlohmann_json::nlohmann_json)` to resolve missing header error in `build_validate`
- Skills installed to `~/.hermes/skills/scripts/` as symlinks to `openhands-adaptation/skills/`
- Microagents present in `.openhands/microagents/` for Godot 4.2 pinning, server C++ conventions, networking, repo context
- Documentation: `OPENHANDS_SKILLS_REFERENCE.md` — comprehensive skill reference

Last updated: 2026-04-29