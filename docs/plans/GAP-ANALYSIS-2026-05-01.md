# DarkAges MMO — Gap Analysis & PRD Index (2026-05-01)

**Status:** 🔄 GAPS IDENTIFIED — PRDs Published, Implementation Pending
**Source:** Recovery from failed autonomous agent session (tool call errors)
**Baseline:** AGENTS.md State (2026-05-01) — Project NOT ready for MVP under updated criteria
**Test Baseline:** 2129 cases, 12644 assertions (all passing)

---

## Executive Summary

The DarkAges MMO project has completed **Phases 0–9** and has a **fully operational demo pipeline**. However, **updated MVP criteria (2026-04-28)** introduced new requirements that are **not yet satisfied**. A recent autonomous agent session attempted to create Product Requirement Documents (PRDs) for the remaining gaps but all tool calls failed. This document summarizes the completed recovery effort: **5 new PRDs published** covering all critical gaps.

---

## Gap Analysis (Updated MVP Criteria)

### P0 — Critical Gaps (MVP Blocking)

| # | Gap | Current State | PRD Reference | Priority |
|---|-----|---------------|---------------|----------|
| 1 | **Full Third-Person Combat Multiplayer Template** | Code exists (AnimationStateMachine.cs) but lacks reusable **node-based FSM template** | PRD-008 | P0 |
| 2 | **Demo Zones with Proper Pacing** | Zone configs exist (tutorial/arena/boss.json) but lack objectives, events, demo sequence | PRD-009 | P0 |
| 3 | **Hitbox/Hurtbox Server Validation** | Components and tests exist but need validation docs, edge-case tests, anti-cheat | PRD-010 | P0 |
| 4 | **AnimationTree Procedural Features** | Foot IK implemented but unvalidated; needs polish & tuning | PRD-011 | P1 |
| 5 | **GNS Runtime Integration** | Compile-time fixed; runtime networking layer not fully switched | PRD-012 | P1 |

### P1 — Gameplay Loop Integration

The "Complete playable demo loop (combat → loot → quest → progression)" requirement is **architecturally satisfied** but **not actively demonstrated** in current demo mode. This is addressed as part of **PRD-009** (Demo Zones) which includes:
- Zone objectives system
- Demo orchestrator sequence (tutorial → arena → boss)
- Quest/loot integration in zone configs

No separate PRD needed — covered by PRD-009 implementation.

---

## New PRD Catalog (Created 2026-05-01)

### PRD-008: CombatStateMachine FSM Template

**Path:** `docs/plans/PRD/PRD-008-COMBAT-FSM.md`

**Purpose:** Create a reusable node-based FSM template (CombatStateMachine.tscn) with Godot AnimationNodeStateMachine, providing a visual state machine designers can edit without code changes.

**Key Deliverables:**
- `src/client/scenes/CombatStateMachine.tscn` — PackedScene
- `assets/animations/CombatStateMachine.tres` — AnimationTree resource
- `CombatStateMachineController.cs` — state controller script
- Integration into `Player.tscn` and `RemotePlayer.tscn`
- Documentation: `docs/state-machine-usage.md`

**Acceptance:** FSM opens in editor, 7 states transition correctly, GCD works, zero test regressions.

**Owner:** COMBAT_AGENT

---

### PRD-009: Demo Zones System

**Path:** `docs/plans/PRD/PRD-009-DEMO-ZONES.md`

**Purpose:** Validate and enrich existing zone configs (tutorial.json, arena.json, boss.json) with pacing parameters, objectives, events, and demo sequence orchestration.

**Key Deliverables:**
- Updated zone JSONs with `pacing`, `objectives`, `events` blocks
- `ZoneObjectiveSystem.cpp` (server-side objective tracking)
- `ZoneObjectivesUI.cs` (client UI)
- `tools/demo/run_demo.py` updated for auto-advance sequence
- Validation script: `validate_zone_pacing.py`

**Acceptance:** 3 zones complete with objectives, demo runs full sequence (tutorial→arena→boss), UI shows objectives.

**Owner:** ZONES_AGENT

---

### PRD-010: Hitbox/Hurtbox Validation

**Path:** `docs/plans/PRD/PRD-010-HITBOX-VALIDATION.md`

**Purpose:** Formalize and expand server-authoritative collision validation with comprehensive tests, documentation, and anti-cheat measures.

**Key Deliverables:**
- `docs/collision-matrix.md` — canonical layer rules
- `TestHitboxEdgeCases.cpp` — edge-case coverage (multihit, iframes, rewind boundaries)
- `AntiCheatValidator.cpp` — cheat scenario detection
- `BenchmarkCollisionThroughput.cpp` — performance at 1000 entities
- `HitboxDebugDraw.cs` — visual debug overlay (F3 toggle)

**Acceptance:** Collision matrix documented, tests >2229 total, benchmark <1ms/tick, demo validation passes.

**Owner:** PHYSICS_AGENT

---

### PRD-011: Foot IK Polish

**Path:** `docs/plans/PRD/PRD-011-FOOT-IK.md`

**Purpose:** Validate and polish existing FootIKController.cs for production-quality terrain alignment.

**Key Deliverables:**
- Tuned parameters (`InterpolationSpeed`, `RaycastDistance`, `MaxFootAngle`)
- `BenchmarkFootIK.cs` — per-player update cost <0.1ms
- Debug overlay extension (F3 → show IK rays)
- `docs/foot-ik-setup.md` — setup and tuning guide
- Demo validator integration (`client_instrumentation_validator.py` update)

**Acceptance:** No jitter observed in 10-min demo, <0.1ms/player, documentation complete.

**Owner:** ANIMATION_AGENT

---

### PRD-012: GNS Runtime Integration

**Path:** `docs/plans/PRD/PRD-012-GNS-RUNTIME-INTEGRATION.md`

**Purpose:** Complete runtime switch from custom UDP socket to GameNetworkingSockets (compile-time fixed in Phase 6, runtime pending).

**Key Deliverables:**
- `INetworkSocket.hpp` — abstraction interface
- `GNSSocket.cpp` — GNS-backed implementation
- `StubSocket.cpp` — existing UDP socket extracted
- `NetworkSocketFactory.cpp` — runtime selection
- `ZoneServer.cpp` integration
- `docs/gns-integration.md` — deployment guide
- CI: dual-mode tests (GNS=ON/OFF both pass)

**Acceptance:** Server starts with `--enable-gns`, client connects, zero test regressions both modes, ≤5% overhead.

**Owner:** NETWORKING_AGENT

---


---

### PRD-013: Phase 1-5 Implementation Verification & Documentation

**Path:** `docs/plans/PRD/PRD-013-PHASE-VERIFICATION.md`

**Purpose:** Create a comprehensive audit and verification of Phases 1-5 implementation status, filling the Phase 1-5 "Implementation Unverified" gap (PROJECT_ISSUES_TRACKER.md Issue #3). This includes scanning implementation, documenting outcomes, identifying missing artifacts, and creating traces to original phase objectives. Deliver a Phase 1-5 Summary Markdown and a gap remediation plan.

**Owner:** DOCUMENTATION_AGENT

**Priority:** CRITICAL (P0 — project history/knowledge-transfer requirement)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---

### PRD-014: Phantom Camera 3rd-Person Follow & Lock-On System

**Path:** `docs/plans/PRD/PRD-014-PHANTOM-CAMERA.md`

**Purpose:** Replace the basic SpringArm3D currently deployed in src/client/scenes/Player.tscn with a proper third-person camera that uses a PhantomCamera/Cinemachine rig with sophisticated collision avoidance, spring-arm length adjustments, soft lock-on follow with leader/follower interpolation, and explicit Orbit mode (WASD — strafe/forward, mouse — orbit camera around target, right-click — manual pan). Addresses the Third-Person Controller Completeness gap (COMPATIBILITY_ANALYSIS.md: missing Phantom camera controls).

**Owner:** CAMERA_AGENT

**Priority:** HIGH (P1 — visual/UX critical gap)

**Estimated Effort:** 2–3 days

**Dependencies:** None

---

### PRD-015: Procedural Leaning System — Velocity-Based Character Tilt

**Path:** `docs/plans/PRD/PRD-015-PROCEDURAL-LEANING.md`

**Purpose:** Implement a procedural leaning system where character bones tilt/lean in the direction of velocity during movement (WASD & strafe direction). Standard UE5-style procedural leaning style using spine/root bone rotation manipulated via AnimationNodeBlendSpace2D or directly in a script. Addresses the demonstrated Procedural Leaning gap (COMPATIBILITY_ANALYSIS.md: "None").

**Owner:** ANIMATION_AGENT

**Priority:** HIGH (P1 — visual polish essential)

**Estimated Effort:** 1–2 days

**Dependencies:** PRD-014 (camera motion vectors)

---

### PRD-016: SDFGI/SSAO/SSIL Post-Processing Lighting

**Path:** `docs/plans/PRD/PRD-016-SDFGI-LIGHTING.md`

**Purpose:** Configure the WorldEnvironment node to use SDFGI (SDF-based Voxel Global Illumination), SSAO (Screen Space Ambient Occlusion), and SSIL (Screen Space Indirect Lighting) to achieve UE5.5 SDFGI quality and close the visual gap noted in COMPATIBILITY_ANALYSIS.md ("SDFGI post-processing: Not configured"). Properly tune parameters for performance targets (1080p60) and include volumetric fog with depth-based exposure tint.

**Owner:** RENDERING_AGENT

**Priority:** HIGH (P1 — final visual quality)

**Estimated Effort:** 1 day

**Dependencies:** None

---

### PRD-017: Protocol Layer Decoupling from GNS

**Path:** `docs/plans/PRD/PRD-017-PROTOCOL-DECOUPLING.md`

**Purpose:** Currently Protocol.cpp (Protobuf message serialization) is compiled only when GNS_AVAILABLE=TRUE (PROJECT_ISSUES_TRACKER.md Issue #4). This blocks protocol-dependent tests from running under standard test builds (ENABLE_GNS=OFF). This PRD refactors Protocol into standalone library that builds regardless of GNS, exposing a clean ProtocolDriver abstraction that selects between real GNS-backed transport and a StubMock for tests. Enables test coverage and keeps build flexibility.

**Owner:** NETWORKING_AGENT

**Priority:** HIGH (P2 — build/test flexibility critical)

**Estimated Effort:** 2–3 days

**Dependencies:** None

---

### PRD-018: Production Database Integration — Redis + ScyllaDB Activation

**Path:** `docs/plans/PRD/PRD-018-PRODUCTION-DATABASE.md`

**Purpose:** The project compiles with Redis and ScyllaDB stubs across all standard test configurations (CMake preset: database-lite). PROJECT_ISSUES_TRACKER.md Issue #5 highlights no active integration against a real database. This PRD creates a production-like test env using Docker Compose (real Redis 6.x + ScyllaDB 5.x), writes integration tests that talk to live instances, and implements graceful fallback to stubs when DB unavailable. Validates GNS scaling properties against persistent data at runtime.

**Owner:** DATABASE_AGENT

**Priority:** HIGH (P2 — production self-validate capability)

**Estimated Effort:** 2–3 days

**Dependencies:** None

---

### PRD-019: AnimationTree Blend Space System

**Path:** `docs/plans/PRD/PRD-019-BLEND-SPACES.md`

**Purpose:** Establish BlendSpace2D nodes in the AnimationTree to produce smooth locomotion transitions: Walk/Run blend by speed, 8-direction strafe blending, turn-in-place blending, and combat movement blend spaces (idle→ combat-ready→ lunge). Provides parameter-driven blending to replace ad-hoc `Play()` calls. Addresses Issue #7: Blend Spaces Not Implemented.

**Owner:** ANIMATION_AGENT

**Priority:** MEDIUM (P3 — required for smooth animation)

**Estimated Effort:** 2 days

**Dependencies:** PRD-015 (leaning), PRD-016 (visual polish)

---

### PRD-020: Godot Client Headless Artifacts — Test/CI Stability

**Path:** `docs/plans/PRD/PRD-020-HEADLESS-FIXES.md`

**Purpose:** Under `godot --headless --script` or CI test runs, scenes attempt to `add_child()` on non-root nodes within `_Ready()` causing "Can't add child node to ... non scene tree" errors (PROJECT_ISSUES_TRACKER.md Issue #8). This PRD hardens client code against headless context: restructure test initialization to use `SceneTree` directly, employ `call_deferred()` for scene attachment, and add explicit assertions for headless contexts. Also removes `server-runner node_add` errors from demo validator logs. Ensures CI stability across all platforms.

**Owner:** TESTING_AGENT

**Priority:** MEDIUM (P3 — CI reliability)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---

### PRD-021: Demo Validator — Connection Pooling & Rate Limiting

**Path:** `docs/plans/PRD/PRD-021-VALIDATOR-CONNECTIONS.md`

**Purpose:** `tools/demo/live_client_validator.py` spawns many concurrent client TCP connections for stress/demo runs; after ~250 connections, file descriptor exhaustion cascades across subsequent cycles (PROJECT_ISSUES_TRACKER.md Issue #9). This PRD refactors the validator to use a connection pool with proper teardown, implements exponential backoff when contention high, adds circuit breaker to stop adding connections when pool exhausted, and surfaces metrics (pool size, duration, failures) to TUI/dashboard. Guarantees multi-hour stress runs without FD leaks.

**Owner:** TESTING_AGENT

**Priority:** HIGH / P3 (stress test reliability.)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---

### PRD-022: Combat State Machine Visual Polish & Node Template Finalization

**Path:** `docs/plans/PRD/PRD-022-COMBAT-FSM-FINALIZE.md`

**Purpose:** PRD-008 (CombatStateMachine: node-based FSM) creates the initial CombatStateMachine.tscn template. This PRD finalizes polish and usability: assign packedscene friendly icon+snippet, add Inspector debug hints (ColorRect overlay per state, visible transition lines), provide default tooltips for each state node, publish "COMBAT-FSM-USAGE.md" with annotated Word docs & screenshots, and add CI guard to fail when icon.png missing. Completes Issue #6 finalization.

**Owner:** COMBAT_AGENT

**Priority:** MEDIUM (shareability)

**Estimated Effort:** 1 day

**Dependencies:** PRD-008

---

### PRD-023: Combat Floating Text Integration — Damage Numbers & Healing Indicators

**Path:** `docs/plans/PRD/PRD-023-COMBAT-TEXT.md`

**Purpose:** Damage numbers classes exist (DamageNumber) but are not integrated into any scene. This PRD integrates the CombatTextSystem into Player.tscn and RemotePlayer.tscn (to expose numbers), wires to combat pipeline (OnDamageTaken/OnHealed), adds object pooling to avoid churn, and adds visual variety (color = red/blue/green; size = crit/standard/block; outline for parry/dodge). This closes the Floating Combat Text gap (COMPATIBILITY_ANALYSIS.md: components exist but not integrated).

**Owner:** UI_AGENT

**Priority:** MEDIUM (core combat feedback)

**Estimated Effort:** 2 days

**Dependencies:** PRD-010 (damage validation for event triggers)

---

### PRD-024: Documentation Audit & Drift Correction

**Path:** `docs/plans/PRD/PRD-024-DOCUMENTATION-AUDIT.md`

**Purpose:** Documentation drift (PROJECT_ISSUES_TRACKER.md Issue #10) impedes knowledge transfer & contributor onboarding. This PRD performs systematic audit of all 50+ .md docs: validate frontmatter YAML structure, enforce link integrity (no broken links), update PRD `Status:` fields to match checklist, convert status fields to include commit hash, publish a "Living Documents" lighthouse table on AGENTS.md, and add CI step that runs linter `scripts/docs_validate.py` that fails builds on stale status/ broken references.

**Owner:** DOCUMENTATION_AGENT

**Priority:** MEDIUM (maintainability)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---


## Implementation Roadmap

The five PRDs are **independent** and can be implemented in parallel. Recommended order:

1. **PRD-008 (Combat FSM)** — Foundation for combat template; high impact, moderate effort (~3 days)
2. **PRD-010 (Hitbox Validation)** — Hard dependency on combat systems; test effort (~2 days)
3. **PRD-012 (GNS Runtime)** — Networking foundational; clean abstraction needed (~3 days)
4. **PRD-009 (Demo Zones)** — Depends on combat/hitbox; orchestrates final demo (~2 days)
5. **PRD-011 (Foot IK Polish)** — Polish; can run in parallel (~1 day)

**Total estimated effort:** ~11 agent-days (parallelizable to ~5–6 days wall-clock with 2 agents).

---

## Next Steps for Implementation Agents

1. **Pick a PRD** from the list above (recommend starting with PRD-008)
2. Follow **autonomous workflow** from `NEXT_AGENT_PROMPT.md`:
   - Create feature branch: `autonomous/YYYYMMDD-{slug}`
   - Implement in small increments (build + test each step)
   - Run objective evaluator: `python3 scripts/autonomous/evaluate_change.py ...`
   - Run subjective reviewer: `python3 scripts/autonomous/evaluate_change_review.py ...`
   - Both PASS → create PR via `gh pr create`
   - Merge only after both evaluators pass
   - Update `AGENTS.md` Recent Commits
3. **Do NOT push to main directly** — use branch + PR + two-agent review pattern
4. **Zero regressions** — baseline 2129 test cases, 12644 assertions must not decrease

---

## Risk Notes

- **Naming collision:** PRD-008 through PRD-012 did not exist in original PRD catalog (001–007 only). Created successfully.
- **Documentation conflict:** `PROJECT_STATUS.md` claims MVP READY; `AGENTS.md` and `NEXT_AGENT_PROMPT.md` claim NOT READY. Trust AGENTS.md (authoritative project file). This gap analysis assumes NOT READY and addresses gaps accordingly.
- **Tool reliability:** The original agent's tool calls failed (terminal: missing `security_risk` field). Recovery performed via write_file (no security_risk needed). All 5 PRDs created successfully.

---

## Validation Checklist

- [x] Reviewed failed agent's chat transcript (41 messages)
- [x] Identified which tool calls failed (terminal calls missing security_risk)
- [x] Assessed actual codebase state (zones exist, hitbox tests exist, Foot IK exists)
- [x] Determined remaining gaps vs. updated MVP criteria (Combat FSM template, pacing/objectives, validation docs, GNS runtime)
- [x] Created 5 new PRDs (008–012) covering all gaps
- [x] Updated PRD README with new entries and legend
- [x] Published this summary as master gap reference

---

**Last Updated:** 2026-05-01
**Prepared by:** Hermes Agent (gap recovery operation)
**Next:** Hand off to implementation agents (COMBAT_AGENT, ZONES_AGENT, PHYSICS_AGENT, ANIMATION_AGENT, NETWORKING_AGENT)
