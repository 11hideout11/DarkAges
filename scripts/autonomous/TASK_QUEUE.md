# Improvement Task Queue

Prioritized list of autonomous improvements for DarkAges.

## P0 — Build/Test Health
- [x] Verify full build compiles on Linux ✅
- [x] Fix any failing tests after build ✅ (all 11 suites pass)

## P1 — Test Coverage Expansion
- [x] ZoneServer: Tests expanded to 29 tests (1104 lines) ✅ 2026-04-19 (Complete, no further action required)
- [x] LagCompensatedCombat: 7 → 8 tests (433 lines) ✅ 2026-04-19
- [x] ZoneOrchestrator: 11 → 14 tests (587 lines) ✅ 2026-04-19
- [x] EntityMigration: 8 → 12 tests (623 lines) ✅ 2026-04-19
- [x] PerfettoProfiler: comprehensive counter/tracking tests ✅ 2026-04-18
- [x] CombatEventLogger: 48 tests for stub API ✅ 2026-04-18
- [x] PooledAllocator: 20 tests ✅ 2026-04-18
- [x] FixedVector: 11 tests ✅ 2026-04-18

## P2 — Documentation
- [x] Update stale documentation files (4 files reference old state) ✅ 2026-05-02
- [x] Add inline documentation to key ECS components
- [x] Document network protocol message format ✅ 2026-04-26 (docs/NETWORK_PROTOCOL.md)

## P2 — Phase 9 Completion
- [x] Performance test infrastructure ✅ (TestLoadTesting.cpp, phase9_report.py)
- [x] Benchmark runner ✅ (tick_benchmark.py)
- [x] All 7 budget checks PASS ✅
- [x] Extend load tests to 800+ entities ✅ 2026-04-19
- [x] Multi-zone load testing ✅ 2026-04-19

## P3 — Phase 10: Security Testing
- [x] Anti-cheat validation (speed hack, teleport, fly hack) ✅ 2026-04-19
- [x] DDoS protection simulation ✅ 2026-04-19
- [x] Fuzz testing (protocol fuzzing with Catch2-based tests) ✅ 2026-05-02
- [x] Penetration testing (packet manipulation, replay attacks) ✅ 2026-05-02

## P3 — Phase 11: Chaos Testing
- [ ] Zone failure simulation
- [ ] Network partition handling
- [ ] Database failure recovery
- [ ] Process crash resilience

## P2.5 — Demo Visual Polish & Missing Interactions
- [ ] NPC interaction system (E key to interact, interaction prompt UI, dialogue trigger)
- [ ] Sound effects integration (footsteps, weapon attacks, ambient, UI sounds)
- [ ] Particle effects for combat/hits/spells/deaths
- [ ] Replace capsule placeholder models with proper 3D character/monster models
- [ ] Full UI style overhaul (consistent theme, animations, visual polish)
- [ ] Foot IK for terrain alignment
- [ ] Lighting upgrades (SDFGI/SSAO) for visual fidelity

## Completed
- [x] All Phase 8 work packages ✅
- [x] All source files have test coverage ✅
- [x] Performance infrastructure ✅
- [x] Linux build passes ✅
- [x] 2129 test cases, 94+ test files, 11 suites all passing ✅
