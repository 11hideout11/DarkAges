DarkAges MMO - Agent Context
## Project State (Updated 2026-04-19)
**Phase 8: COMPLETE** — All 8 work packages finished. **798 test cases** across **66 test files**, **3,444 assertions**. All passing (11 suites). **51,144 LOC** total. Server: ~25K lines C++ (EnTT ECS, 60Hz tick loop).
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
- **Zones**: `ZoneServer` (1104 lines, largest file), `ZoneOrchestrator`, `EntityMigration`, `ZoneHandoff`
- **Security**: `PacketValidator`, `AntiCheat`, `StatisticalDetector`, `MovementValidator`, `RateLimiter`
- **Monitoring**: `MetricsExporter` (Prometheus/Grafana)
### Key Gotchas
- `Protocol.cpp` depends on GNS types — excluded when `ENABLE_GNS=OFF`
- Redis/Scylla stubs are used when services disabled — don't test real behavior in CI
- Forward-declared types (`struct Foo;`) can't use `sizeof()` in tests
- Nested types like `RedisInternal::PendingCallback` need qualified names
- Namespace is `DarkAges::` everywhere — don't use `darkages`
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