# DarkAges Autonomous Dev Loop — Reusable Prompt for Hermes

Copy and paste this entire block to Hermes in a fresh session to start
autonomous development on the DarkAges project:

---

```
I want you to enter an autonomous development loop on the DarkAges MMO project at /root/projects/DarkAges/.

## Load these skills first
1. darkages-codebase-conventions
2. darkages-catch2-testing
3. test-driven-development
4. systematic-debugging
5. github-pr-workflow
6. autonomous-codebase-iteration
7. autonomous-test-generation
8. darkages-autonomous-dev-prompt (this skill — contains your mission)

## Mission
Make progress on the prioritized gaps from Research/COMPATIBILITY_ANALYSIS.md,
in order of priority, working autonomously until all gaps at the current priority
level are resolved. Use the automatic dev loop pattern:

1. Pull latest main
2. Create feature branch: autonomous/YYYYMMDD-{short-slug}
3. Make changes following TDD (test first, then implement)
4. Build with: cmake -S . -B build_validate -DBUILD_TESTS=ON -DFETCH_DEPENDENCIES=ON -DENABLE_GNS=OFF -DENABLE_REDIS=OFF -DENABLE_SCYLLA=OFF && cmake --build build_validate -j$(nproc)
5. Test with: cd build_validate && ctest --output-on-failure -j8
6. If tests fail, fix them (all 1978 must pass)
7. Push branch and create PR
8. Merge to main only when build + all tests pass
9. Clean up branch (local + remote)
10. Update COMPATIBILITY_ANALYSIS.md to reflect completed items
11. Update DOCUMENTATION_SYNC_REPORT.md to track progress
12. Repeat with next gap

## Priority Order (from COMPATIBILITY_ANALYSIS.md)

P1 — Networking & Combat Logic:
- Hitbox/Hurtbox collision layers (Layer 3=Hitbox, Layer 4=Hurtbox)
- Server-authoritative damage RPC handshake
- Global cooldown (1.2s)

P2 — Animation & Feel:
- Procedural leaning (spine tilt on velocity)
- Foot IK (SkeletonIK3D for terrain alignment)
- Animation blend enhancement (BlendTree)
- Hit stop (0.05s freeze on hit)

P3 — Visual Quality:
- SDFGI WorldEnvironment configuration
- SSAO post-processing
- Floating combat text integration

P4 — Polish:
- Phantom Camera plugin evaluation/install
- Godot State Charts evaluation

## Hard Constraints
1. Non-destructive: additions only, never rewrite existing systems
2. All tests must pass before merge: 1978 test cases, 11 suites, 0 failures
3. Branch workflow: feature branch -> implement -> build -> test -> PR -> merge -> delete branch
4. No direct pushes to main — always use branches + PRs
5. EnTT rules: use registry.all_of<T>() not registry.has<T>(); no view.size(); entity enum != int
6. Namespace: DarkAges:: everywhere, never darkages
7. Build flags: ENABLE_GNS=OFF ENABLE_REDIS=OFF ENABLE_SCYLLA=OFF for test builds
8. Update COMPATIBILITY_ANALYSIS.md and DOCUMENTATION_SYNC_REPORT.md after each gap completed
```