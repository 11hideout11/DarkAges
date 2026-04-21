# DarkAges MMO - Agent Context

## Project State (Updated 2026-04-20)

**Phase 8: COMPLETE** — All 8 work packages finished.
**917 test cases** across **72 test files**, **5,286 assertions**. All passing (11 suites).
**~56K LOC** total. Server: ~26K lines C++ (EnTT ECS, 60Hz tick loop).

### Build
```bash
cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON \
  -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build_validate -j$(nproc)
cd build_validate && ctest --output-on-failure -j8
```

### Architecture
- **ECS**: EnTT, `DarkAges` namespace. Components in `ecs/`, systems in `combat/`, `physics/`, `zones/`
- **Netcode**: `NetworkManager` (stub), `GNSNetworkManager`, `ProtobufProtocol` — GNS disabled in test builds
- **DB**: `RedisManager`, `ScyllaManager` — stubs when Redis/Scylla disabled
- **Zones**: `ZoneServer`, `ZoneOrchestrator`, `EntityMigration`, `ZoneHandoff`
- **Security**: `PacketValidator`, `AntiCheat`, `StatisticalDetector`, `MovementValidator`, `RateLimiter`
- **Monitoring**: `MetricsExporter` (Prometheus/Grafana)
- **Gameplay Systems**: `CombatSystem`, `AbilitySystem`, `ItemSystem`, `QuestSystem`, `ChatSystem`, `CraftingSystem`, `NPCAISystem`, `ExperienceSystem`, `LootSystem`, `ProjectileSystem`, `StatusEffectSystem`

### Key Gotchas
- `Protocol.cpp` depends on GNS types — excluded when `ENABLE_GNS=OFF`
- Redis/Scylla stubs are used when services disabled — don't test real behavior in CI
- Forward-declared types (`struct Foo;`) can't use `sizeof()` in tests
- Nested types like `RedisInternal::PendingCallback` need qualified names
- Namespace is `DarkAges::` everywhere — don't use `darkages`
- EnTT: use `registry.all_of<T>()` not `registry.has<T>()`; no `view.size()`; entity enum can't compare with int
- **EnTT pointer invalidation**: After `registry.emplace<T>()`, existing pointers/references to component T may be invalidated. Always re-fetch after emplace calls. Use `registry.get<T>()` (reference) instead of `try_get<T>()` (pointer) when component must exist.

### Core Gameplay Systems (Implemented)
- **Combat**: Melee attacks, damage, lag compensation, hit detection
- **Abilities**: 4-slot loadout, casting, cooldowns, mana costs, damage/heal effects
- **NPC AI**: Idle/Wander/Chase/Attack/Flee behaviors, aggro, leash, archetype configs
- **NPC Pathfinding**: A* grid-based pathfinding with 8-directional movement, line-of-sight checks, path caching. NPCs navigate around obstacles during chase/wander/flee. Falls back to direct movement when no grid is configured or LOS is clear.
- **Loot**: Drop tables, loot entities, pickup, despawn timers
- **XP/Progression**: Kill XP, level-up, stat points
- **Inventory**: 24-slot, stacking, add/remove, equip/unequip
- **Items**: Weapons, armor, accessories, consumables, materials, quest items
- **Consumables**: Health/mana potion use
- **Quests**: Accept/track/complete, kill/collect/level objectives, rewards
- **Chat**: Local/Global/Whisper/Party/Guild channels, rate limiting, mute
- **Crafting**: Recipe registry, material consumption, instant/timed crafting, profession XP

### Remaining Gameplay Gaps
- Guild/party system (ChatSystem has placeholder routing ready)
- NPC dialogue system (quest hand-in uses programmatic flow, not dialogue trees)
- Trading between players
- Zone events / world bosses

### Phase 9 Focus
- Performance testing infrastructure
- Load testing with bot swarms
- Profiling with Perfetto
- Documentation alignment with actual implementation status

### Autonomous Cron Jobs
- Hourly (c5d9): Quick tasks, `once` mode — discover → implement → build → test → commit
- 4-hourly (6ec7): Deep iteration, `deep` mode — 3 tasks per run
- Orchestrator: `scripts/autonomous/cron_dev_loop.py`
- Discovery: `scripts/autonomous/discover_tasks.py`
- Test matrix: `build_validate` directory
