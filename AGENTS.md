## Recent Commits (last 10 — updated)
1. docs(research): add comprehensive world-building capabilities research and asset pipeline
2. fix(gns): enable C language to support compile features required by GameNetworkingSockets
3. feat(combat): complete FSM refactor; circular dependency resolved; hitbox component added; proper headers and final review fixes
4. fix(skill): correct namespace heuristic to avoid false positives on DarkAges namespace
5. docs(agents): update Recent Commits ordering and restore State section with test metrics
6. fix(combat): address subjective review: proper headers, Hitbox component, clean damage
7. fix(combat): resolve circular dependency in combat state machine; restore copy semantics
8. fix(combat): rewrite AttackState without Hitbox component and fix RecoveryState timing
9. docs: resolve demo readiness contradiction, validate all phases complete
10. Merge PR #28: fix combat FSM entity types and complete OpenHands integration

## State (2026-05-01)

- Phase 0: COMPLETE — documented in PHASE0_SUMMARY.md
- Phase 1-5: ✅ VERIFIED — Summary docs created (PHASE1-5_SUMMARY.md)
- Phase 6: COMPLETE — build system hardening
- Phase 7: COMPLETE — All tests pass (2129 cases, 12644 assertions, 100%)
- Phase 8: PARTIAL — multiple work packages complete, GNS compile-time fix merged; runtime integration pending
- Phase 9: COMPLETE — performance budgets validated
- **Art Pipeline**: RESEARCH PHASE COMPLETE — world-building capabilities documented; asset pipeline spec exists (43 assets); CC0 sourcing strategies cataloged; Godot 4.2 PBR workflow defined; manifest.json created at assets/manifest.json
- **Tests**: All suites passing (2129 cases, 12644 assertions, 100%)
- **Test breakdown**:
  - unit_tests: 1302 cases, 7244 assertions
  - test_combat: 140 cases, 666 assertions
  - test_zones: 198 cases, 1265 assertions
  - test_security: 234 cases, 1660 assertions
  - test_anticheat: 50 cases, 445 assertions
  - test_database: 53 cases, 260 assertions
  - remaining suites: 152 cases, 1104 assertions
- Server: ~32K LOC (C++20, EnTT ECS, 60Hz tick) | Client: ~9K LOC (C# Godot 4.2)
- **PR #29 status**: MERGED — Combat FSM refactor completed; two-agent review passed (objective test + subjective architectural); all 2129 tests passing.

## OpenHands Integration Updates (2026-04-29)

- Added 4 new standalone skills: `test-flakiness.py`, `coverage-report.py`, `pr-create.py`, `pr-comment.py`, `code-format.py`
- Fixed CMake JSON include bug — switched from direct `target_include_directories` to `target_link_libraries(nlohmann_json::nlohmann_json)` to resolve missing header error in `build_validate`
- Skills installed to `~/.hermes/skills/scripts/` as symlinks to `openhands-adaptation/skills/`
- Microagents present in `.openhands/microagents/` for Godot 4.2 pinning, server C++ conventions, networking, repo context
- Documentation: `OPENHANDS_SKILLS_REFERENCE.md` — comprehensive skill reference

Last updated: 2026-04-29
