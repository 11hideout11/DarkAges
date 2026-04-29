# DarkAges Autonomous Development Loop Prompt

Reusable prompt for iterating through feature implementations on the DarkAges MMO project.

## Quick Iteration Checklist

```
□ Discover: What needs to be done?
  - AGENTS.md Gaps section
  - .demo_tasks.md REMAINING
  - Run: grep -r "TODO\|FIXME" src/

□ Analyze: Current state
  - Check existing tests
  - Find integration points
  - Identify smallest change

□ Implement: Make the change
  - Write/modify code
  - Write tests
  - Follow: DarkAges:: namespace, registry.all_of<T>()

□ Build & Test:
  - cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
  - cmake --build build_validate -j$(nproc)
  - ctest --output-on-failure -j1

□ Verify: All 11 tests pass? No regression?

□ Commit: Push to branch autonomous/YYYYMMDD-{feature}
  
□ Iterate: Repeat until done
```

## Build Commands

| Step | Command |
|------|---------|
| Configure | `cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF` |
| Build | `cmake --build build_validate -j$(nproc)` |
| Test | `cd build_validate && ctest --output-on-failure -j1` |
| Test Filter | `ctest -R test_combat --output-on-failure` |

## Test Suites (11 total)

1. unit_tests
5. test_combat
6. test_aoi
7. test_network
8. test_zones
9. test_security
10. test_anticheat
11. test_database
- test_movement
- test_spatial
- test_memory

## Common Issues & Fixes

### Port Conflict (8080)
- **Symptom**: test_zones fails with SIGABRT
- **Fix**: Add graceful reinit check in MetricsExporter::Initialize()

### sizeof on forward-declared type
- **Symptom**: Test compilation fails
- **Fix**: Don't use sizeof() on forward-declared types

### EnTT pointer invalidation
- **Symptom**: Stale component data
- **Fix**: Re-fetch pointers after registry.emplace<T>()

## Project Conventions

- **Namespace**: `DarkAges::`
- **ECS**: use `registry.all_of<T>()` not `registry.has<T>()`
- **EnTT**: no `view.size()`, entity enum != int
- **Network**: Protocol.cpp excluded when ENABLE_GNS=OFF

## Pre-Completion Checklist

- [ ] build_compiles
- [ ] tests_pass (11/11)
- [ ] no_regression
- [ ] explicit_test_summary
- [ ] test_count_positive
- [ ] assert_count_positive
- [ ] baseline_readable (TEST_RESULTS.md)

## Loop Detection

> If same file edited > 3 times in a day: Consider reconsidering your approach

---

*Last Updated: 2026-04-29*