# DarkAges MMO - Agent Context

## Project State (Updated 2026-04-22)

**Phase 8: COMPLETE** — All core gameplay systems implemented.
**Phase 9: COMPLETE** — Performance testing infrastructure operational, all budget checks passing.
**1165 test cases** across **88 test files**. All passing (11 suites).
**~32K LOC** in server core (C++20, EnTT ECS, 60Hz tick). Client: ~6.2K LOC (C# Godot).

### Build
```bash
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON \
  -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)
cd build_validate && ctest --output-on-failure -j8
```

### Architecture
- **ECS**: EnTT, `DarkAges::` namespace. Components in `ecs/`, systems in `combat/`, `physics/`, `zones/`
- **Netcode**: `NetworkManager` (stub), `GNSNetworkManager` (implementation exists but ENABLE_GNS=OFF in test builds), `ProtobufProtocol` — GNS disabled in test builds; protocol stubbed with custom delta encoding
- **DB**: `RedisManager`, `ScyllaManager` — stubs when Redis/Scylla disabled
- **Zones**: `ZoneServer`, `ZoneOrchestrator`, `EntityMigration`, `ZoneHandoff`
- **Security**: `PacketValidator`, `AntiCheat`, `StatisticalDetector`, `MovementValidator`, `RateLimiter` (functional but not actively developed per current scope)
- **Monitoring**: `MetricsExporter` (Prometheus/Grafana format)
- **Gameplay Systems**: `CombatSystem`, `AbilitySystem`, `NPCAISystem` + A* Pathfinding, `PartySystem`, `GuildSystem`, `LootSystem`, `XP`/Progression, `Inventory`, `ItemSystem`, `ConsumableSystem`, `QuestSystem`, `ChatSystem` (Party/Guild/Global/Whisper), `CraftingSystem`, `TradingSystem`, `ZoneEventSystem`, `DialogueSystem`, `SpawnSystem`

### Key Gotchas
- `Protocol.cpp` depends on GNS types — excluded when `ENABLE_GNS=OFF`; test builds use stubbed network layer
- Redis/Scylla stubs are used when services disabled — don't test real behavior in CI
- Forward-declared types (`struct Foo;`) can't use `sizeof()` in tests
- Nested types like `RedisInternal::PendingCallback` need qualified names
- Namespace is `DarkAges::` everywhere — don't use `darkages`
- EnTT: use `registry.all_of<T>()` not `registry.has<T>()`; no `view.size()`; entity enum can't compare with int
- **EnTT pointer invalidation**: After `registry.emplace<T>()`, existing pointers/references to component T may be invalidated. Always re-fetch after emplace calls. Use `registry.get<T>()` (reference) instead of `try_get<T>()` (pointer) when component must exist.

### Core Gameplay Systems (Implemented)
- **Combat**: Melee/ranged attacks, damage, lag compensation, hit detection, status effects
- **Abilities**: 4-slot loadout, casting, cooldowns, mana costs, damage/heal effects, status application
- **NPC AI**: Idle/Wander/Chase/Attack/Flee behaviors, aggro, leash, archetype configuration via Lua
- **NPC Pathfinding**: A* grid-based pathfinding with 8-directional movement, LOS checks, path caching; falls back to direct movement
- **Loot**: Drop tables, loot entities, pickup, despawn timers, rarity tiers
- **XP/Progression**: Kill XP, level-up thresholds, stat point allocation
- **Inventory**: 24-slot grid, stacking limits, add/remove, equip/unequip, weight capacity
- **Items**: Weapons, armor, accessories, consumables, materials, quest items; rarity and stats
- **Consumables**: Health/mana potions, food buffs, cooldown-based use
- **Quests**: Accept/track/complete, kill/collect/level objectives, multi-step chains, rewards
- **Chat**: Local/Global/Whisper/Party/Guild channels, rate limiting, mute, color codes
- **Crafting**: Recipe registry, material consumption, instant/timed crafting, profession XP, quality tiers
- **Trading**: Player-to-player item and gold exchange, trade request/accept/decline, item escrow, lock/confirm flow, timeout handling
- **Zone Events**: World bosses, wave defense, timed kill events. Multi-phase with objectives, participation tracking, scaled rewards, boss spawning via callbacks
- **NPC Dialogue**: Branching conversation trees, conditional responses (quest state, level, items), quest integration (give/complete), item/gold rewards, multi-player independent conversations
- **SpawnSystem**: NPC spawn groups, respawn timers, weighted selection, spawn regions, per-zone spawn positions, density limiting

### Recent Major Additions (Last 10 Commits)
- AchievementSystem + LeaderboardSystem with comprehensive tests
- SpawnSystem refactor: single-entity spawn design, forceSpawn tracking
- Per-zone spawn positions and NavigationGrid wiring to NPCAISystem
- NPC Dialogue System (branching, conditions, quest hooks)
- Zone Event System (multi-phase world events)
- Player Trading System (full lifecycle with escrow)
- CraftingSystem (instant + timed recipes, profession XP)
- Chat System (all 5 channels, routing, rate limiting)

### Performance Testing (Phase 9 — Complete)
- `TestLoadTesting.cpp`: 690 lines, 13 test cases covering 50/200/400/800/1000 entity ticks
- Budgets validated: 400 entities < 20ms tick, 800 entities < 30ms, spatial hash < 5ms rebuild, AOI < 5ms per 500
- All 7 budget checks PASS (April 19 report)
- Test infrastructure: `tools/perf/phase9_report.py`, `docs/performance/reports/`

### Remaining Strategic Gaps
- **Networking (GNS/Protobuf)**: Production UDP networking via GameNetworkingSockets is implemented in `GNSNetworkManager.cpp` but conditionally compiled (ENABLE_GNS=OFF in CI). Test builds use `NetworkManager_stub.cpp`. Client uses custom UDP (Godot `UdpClient`). This is the primary blocker for live multiplayer testing.
- **Client ↔ Server Integration**: Full network protocol integration untested in live environment; stub-based test coverage exists but real network traversal, latency compensation, and packet loss handling need validation
- **Documentation Sync**: AGENTS.md (test count/features stale), PROJECT_STATUS.md (Jan 30), DarkAges_Comprehensive_Review.md (Feb 18) require updates to match current codebase state
- **Test Depth**: Minor gaps in shallow test files (TestPartySystem 19 lines, TestGuildSystem 30 lines, TestProtocol 49 lines)

### Autonomous Dev Loop
- Hourly quick scan (c5d9): `once` mode, task discovery → implementation → validate
- Twice-daily deep iteration (6ec7): 6am/6pm UTC, `deep` mode (3 tasks per run)
- Orchestrator: `scripts/autonomous/cron_dev_loop.py`
- Discovery: `scripts/autonomous/discover_tasks.py` with cache at `.task_cache.json`
- Current queue: ZoneServer test-depth (P1), documentation updates (P2), include reduction (P3)
- Branch workflow: all changes in feature branches → PR → merge to main after passing 11 test suites

---

**Last updated by autonomous review on 2026-04-22**
