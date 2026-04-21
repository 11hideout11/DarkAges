# DarkAges MMO - Agent Context

## Project State (Updated 2026-04-21)

**Phase 8: COMPLETE** — All core gameplay systems implemented.
**1070 test cases** across **81 test files**. All passing (11 suites).
**~58K LOC** total. Server: ~28K lines C++ (EnTT ECS, 60Hz tick loop).

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
- **Gameplay Systems**: `CombatSystem`, `AbilitySystem`, `NPCAI` + Pathfinding, `PartySystem`, `GuildSystem`, `LootSystem`, `XP`/Progression, `Inventory`, `Items`, `Consumables`, `Quests`, `Chat` (Party/Guild routing), `Crafting`

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
- **Trading**: Player-to-player item and gold exchange, trade request/accept/decline, item escrow, lock/confirm flow, timeout handling
- **Zone Events**: World bosses, wave defense, timed kill events. Multi-phase with objectives, participation tracking, scaled rewards, boss spawning via callbacks

### Remaining Gameplay Gaps
- NPC dialogue system (quest hand-in uses programmatic flow, not dialogue trees)

### Phase 9 Focus
- Performance testing infrastructure
- Load testing with bot swarms
- Profiling with Perfetto
- Documentation alignment with actual implementation status

### Autonomous Cron Jobs
- Every 6h (c5d9): Quick tasks, `once` mode — backlog → discovery → fallback
- Twice daily 9am/9pm (6ec7): Deep iteration, `deep` mode — 3 tasks per run
- Orchestrator: `scripts/autonomous/cron_dev_loop.py`
- Task backlog: `scripts/autonomous/task_backlog.json` (18 test-expansion tasks)
- Behavioral test generator: `scripts/autonomous/generate_behavioral_tests.py`
- Discovery: `scripts/autonomous/discover_tasks.py`
- Test matrix: `build_validate` directory

