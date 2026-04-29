---
name: darkages-repo
type: repo
version: 1.0.0
agent: CodeActAgent
---

# DarkAges: Multiplayer MMO Server + Godot Client

## Technologies
- **Server**: C++20, EnTT ECS, 60 Hz fixed tick, protobuf UDP, CMake
- **Client**: Godot 4.2.4 Mono (C#), RemoteSceneTree multiplayer, AnimationTree state machines
- **Persistence**: Redis (caching), Scylla (long-term)
- **Infrastructure**: Docker Compose for demo harness, Prometheus metrics

## Build & Test Commands
```bash
# Build (with tests)
cmake -S . -B build -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
cmake --build build -j$(nproc)

# Test
cd build && ctest --output-on-failure -j1

# Run demo server (requires docker-compose)
docker compose -f scripts/docker/docker-compose.yml up -d server
```

## Conventions
- Namespace: `DarkAges::`
- EnTT: use `registry.view<Comp1, Comp2>()`; after emplace/remove, re-fetch pointers
- Test style: Catch2 v2, sectioned [given|when|then], BEHAVIOR naming
- Network: server-authoritative; client is view-only
- Git: conventional commits, two-agent review before merge (objective + subjective)

## Autonomous Workflow
Hourly cron runs `scripts/autonomous/cron_dev_loop.py` → task discovery → feature branch → implement → evaluate → PR → two-agent review → merge (after build+test PASS).

## Godot Client Notes
- Version pin: 4.2.4 (NOT 4.6)
- IK: SkeletonIK3D node (not SkeletonModifier3D)
- Physics: GodotPhysics3D (not Jolt)
- Rendering: Vulkan + ACESFilmic tonemap
- See `docs/engine-reference/godot/` for detailed pinning notes

## EnTT ECS Notes
- Component access: `registry.get<T>(entity)` requires existence
- Iteration: `registry.view<Position, Velocity>().each()` (do NOT use `view.size()`)
- Forward-declared structs: no `sizeof()` in tests
- Nested types: fully qualify (e.g., `RedisInternal::Pending`)
