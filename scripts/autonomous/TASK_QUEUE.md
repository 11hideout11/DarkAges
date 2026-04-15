# Improvement Task Queue

Prioritized list of autonomous improvements for DarkAges.

## P0 — Build/Test Health
- [ ] Verify full build compiles on Linux (currently MSVC-only CMake config)
- [ ] Fix any failing tests after build

## P1 — Code Quality: Raw Memory Management
- [x] ScyllaManager: Replace raw `new WriteCallbackData` with `make_unique` (lines 533, 600, 658) ✅ 2026-04-15 (PR #4)

## P1 — Unimplemented TODOs (smallest scope first)
- [x] ZoneServer:328 — Implement player state persistence stub ✅ 2026-04-15 (PR #3)
- [x] ZoneServer:553 — Implement event packet broadcast ✅ 2026-04-15
- [x] ZoneServer:639 — Implement respawn timer ✅ 2026-04-14 (PR #2)

## P1 — Combat System (core gameplay)
- [x] CombatSystem — Implement ability system stub ✅ 2026-04-15 (commit 107ad78)
- [x] CombatSystem — Implement projectile/raycast stub ✅ 2026-04-15 (commit 107ad78)

## P2 — Refactoring Candidates (large files)
- [ ] RedisManager.cpp (1655 lines) — extract connection pool to separate class
- [ ] ZoneServer.cpp (1632 lines) — extract player management, event handling

## P2 — Documentation
- [ ] Add inline documentation to key ECS components
- [ ] Document network protocol message format

## P2 — Anti-Cheat Logging
- [x] ZoneServer:1135 — Implement anti-cheat event logging to database ✅ 2026-04-15
- [x] ZoneServer:1151 — Implement ban persistence in Redis ✅ 2026-04-15
- [ ] AntiCheat:357 — Implement collision detection stub

## Completed
- [x] ZoneServer:639 — Implement respawn timer ✅ 2026-04-14 (PR #2)
- [x] Anti-cheat event logging to ScyllaDB ✅ 2026-04-15
- [x] Ban persistence to Redis ✅ 2026-04-15
- [x] Damage/hit packet sending to clients ✅ 2026-04-15
