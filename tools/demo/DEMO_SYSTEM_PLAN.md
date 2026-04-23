# DarkAges MMO — Comprehensive Demo System Plan
**Based on:** DEMO_CAPABILITIES_REPORT.md + Project State Audit (April 23, 2026)
**Objective:** Turn validated server demo into full, gaurdrailed, end-to-end demo pipeline with agent skills, redundant services, and Godot client showcase.

## I. Current State Summary

### 1.1 What Works (Demo-Ready)
- ✅ Server Core — 60Hz ECS, 18 gameplay systems
- ✅ Test Suite — 1,212 cases, 91 files, all passing
- ✅ Performance — Phase 9: 400 entities <20ms
- ✅ Validator Harness — demo_validator.py + live_client_validator.py
- ✅ Server CLI Demo — darkages_server --npcs --npc-count 10
- ✅ Combat Validation — 7-phase
- ✅ Chaos Toolkit — chaos_monkey.py
- ✅ Monitoring — Prometheus/Grafana
- ✅ Godot Client — C# with prediction/interpolation/UI

### 1.2 Critical Gaps
- ⚠️ GNS Not Enabled in CI
- ⚠️ Godot Client Not Automated
- ⚠️ No Installer/Harness
- ⚠️ Agent Skills Lacking
- ⚠️ MCP Server Gap
- ⚠️ Demo Content Scope
- ⚠️ Documentation Drift

## II. Architecture
[INSTALLER] → [ORCHESTRATOR] → [AGENT SKILLS] → [MCP Godot] → [Content] → [Artifacts]

## III. Implementation Roadmap

Sprint 1 (GNS Enablement — 2h):
- Patch GNS CMakeLists.txt set_clientlib_target_properties macro
- Add: target_compile_features(${GNS_TARGET} PUBLIC ${C99_FEATURES} ${CXX11_FEATURES})
- Reconfigure with -DENABLE_GNS=ON
- Validate: full test suite passes, GNSNetworkManager links
- Update CI: add GNS build job
- Document fix

Sprint 2 (Demo Harness — 1d):
- Create tools/demo/installer/demo_launcher.py
- Implement dependency_check.py (Godot, .NET, OpenSSL)
- Implement build_server.py wrapper
- Implement start_server() from config
- Implement run_validator() wrapper
- Add --quick-demo (server-only validator)
- Add --full-demo (server+Godot placeholder)
- Create demo_orchestrator.py supervisor
- Create config/demo_config.yaml
- Test: python demo_launcher.py --quick-demo produces report
- Write DEMO_RUNBOOK.md

Sprint 3 (Godot MCP + Skills — 2d):
- Audit tools/automated-qa/godot-mcp/
- Complete minimal Godot MCP server (7 tools)
- Register in ~/.config/hermes/mcp_config.json
- Create DemoDirector.cs (Godot singleton)
- Create 5 agent skills: godot-demo-control, demo-narrator, scene-inspector, input-simulator, capture-screenshots
- Test full automation

Sprint 4 (Content Polish — 1d):
- Create zone_demo_01 definition
- Define 3 NPC archetypes (melee, archer, boss)
- Create 3-step demo quest chain
- Wire DemoDirector triggers
- Verify full quest + zone event

Sprint 5 (Redundancy — 1d):
- Supervisor health checks (ping server, client FPS, memory)
- Auto-restart (max 3)
- Circuit breaker + error report
- Add --chaos-demo flag
- Add --record flag (FFmpeg)
- Test failure scenarios

Sprint 6 (Docs — 4h):
- Update DarkAges_Comprehensive_Review.md to April 23
- Write DEMO_RUNBOOK.md
- Create DEMO_SCRIPT.md (presenter narration)
- Create ARTIFACTS_README.md
- Update README.md badges

Sprint 7 (GNS CI — 2h):
- After Sprint 1 fix validated, add GNS job to CI
- Add --gns-mode to demo launcher
- Add GNS metrics to narration
- Test GNS demo with --packet-loss 5

## V. Success Metrics
- One-command ≤2min, uptime 99.9%, FPS ≥55, success ≥95%, artifacts 100%, sync ≤0.5s, restart <10s, GNS demo works.

