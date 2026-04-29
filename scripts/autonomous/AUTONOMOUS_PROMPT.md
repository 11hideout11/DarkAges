# DarkAges Autonomous Development Loop Prompt

Reusable prompt for iterating through feature implementations on the DarkAges MMO project.

## Quick Iteration Checklist

```
□ Discover: What needs to be done?
  - AGENTS.md Gaps section
  - .demo_tasks.md REMAINING
  - Find minimal tests: grep "TEST_CASE" src/server/tests/Test{XYZ}.cpp | wc -l

□ Analyze: Current state
  - Check existing tests: grep "^TEST_CASE" src/server/tests/Test{XYZ}.cpp
  - Check API coverage: grep "virtual\|void\|bool" include/combat/XYZSystem.hpp
  - Identify smallest gap with clear expansion path

□ Implement: Make the change
  - Use terminal <<EOF for tests (safer than insert):
    cat >> src/server/tests/Test{XYZ}.cpp << 'EOF'
    TEST_CASE("System new behavior", "[xyz]") {
        // test code here
    }
    EOF
  - Follow: DarkAges:: namespace, registry.all_of<T>()

□ Build & Test:
  - cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF
  - cmake --build build_validate -j$(nproc)
  - ctest --output-on-failure -j1

□ Verify: All 11 tests pass? New tests explicitly tested?
  - Run specific: ./darkages_tests "[achievements]"

□ Commit: Push to branch autonomous/YYYYMMDD-{feature}
  
□ Iterate: Repeat until done
```

## Task Selection Criteria

**Pick the smallest gap** with:
1. Clear expansion path (API exists, just not tested)
2. Can add 2-3 tests in single session
3. Tests validate existing behavior, not new features

**Example**: AchievementSystem has only 3 tests, has 8+ API methods → expand to 5 tests

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