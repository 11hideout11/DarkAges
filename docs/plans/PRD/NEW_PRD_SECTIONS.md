### PRD-013: Phase 1-5 Implementation Verification & Documentation

**Path:** `docs/plans/PRD/PRD-013-PHASE-VERIFICATION.md`

**Purpose:** Create a comprehensive audit and verification of Phases 1-5 implementation status, filling the Phase 1-5 Implementation Unverified gap (PROJECT_ISSUES_TRACKER.md Issue #3). This includes scanning implementation, documenting outcomes, identifying gaps, and creating traces to original phase objectives.

**Owner:** DOCUMENTATION_AGENT

**Priority:** CRITICAL (P0 — project history/knowledge transfer)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---

### PRD-014: Phantom Camera 3rd-Person Follow & Lock-On System

**Path:** `docs/plans/PRD/PRD-014-PHANTOM-CAMERA.md`

**Purpose:** Replace the basic SpringArm3D currently in Player.tscn with a proper PhantomCamera/ThirdPersonCamera that supports sophisticated behavior: collision avoidance, spring-arm length adjustment, lock-on with soft follow, and Orbit mode (wasd - strafe, mouse - orbit around target, right-click - manual look). Addresses the Third-Person Controller Completeness gap (COMPATIBILITY_ANALYSIS.md: missing proper third-person camera controls).

**Owner:** CAMERA_AGENT

**Priority:** HIGH (P1 — visual/UX critical gap)

**Estimated Effort:** 2–3 days

**Dependencies:** None

---

### PRD-015: Procedural Leaning System — Velocity-Based Character Tilt

**Path:** `docs/plans/PRD/PRD-015-PROCEDURAL-LEANING.md`

**Purpose:** Implement a procedural leaning system where character bones tilt/lean in the direction of velocity (WASD & strafe direction). Uses standard Unity/UE5 procedural leaning style using spine bone rotation. Addresses the demonstrated Procedural Leaning gap (COMPATIBILITY_ANALYSIS.md: "None").

**Owner:** ANIMATION_AGENT

**Priority:** HIGH (P1 — visual polish essential)

**Estimated Effort:** 1–2 days

**Dependencies:** PRD-014 (camera system) for target motion vectors

---

### PRD-016: SDFGI/SSAO/SSIL Post-Processing Lighting

**Path:** `docs/plans/PRD/PRD-016-SDFGI-LIGHTING.md`

**Purpose:** Configure the project's WorldEnvironment to use SDFGI (SDF-based Global Illumination), SSAO (Screen Space Ambient Occlusion), and SSIL (Screen Space Indirect Lighting) to match UE5.5ാ SDFGI quality and complete the visual gap noted in COMPATIBILITY_ANALYSIS.md ("Not configured"). Also considers volumetric fog/fog depth exposure color grading.

**Owner:** RENDERING_AGENT

**Priority:** HIGH (P1 — final visual quality)

**Estimated Effort:** 1 day

**Dependencies:** None

---

### PRD-017: Protocol Layer Decoupling from GNS

**Path:** `docs/plans/PRD/PRD-017-PROTOCOL-DECOUPLING.md`

**Purpose:** Currently Protocol.cpp (protobuf serialization) is compiled only when GNS_AVAILABLE=TRUE (per PROJECT_ISSUES_TRACKER.md Issue #4). This creates a blocking problem where protocol-dependent tests cannot run under test builds (ENABLE_GNS=OFF). Implement Protocol as an independent library that builds with or without GNS, using compile-time abstraction (ProtocolStub vs ProtocolGNS) and runtime injection.

**Owner:** NETWORKING_AGENT

**Priority:** HIGH (P2 — build/test coveage)

**Estimated Effort:** 2–3 days

**Dependencies:** None

---

### PRD-018: Production Database Integration — Redis + ScyllaDB Activation

**Path:** `docs/plans/PRD/PRD-018-PRODUCTION-DATABASE.md`

**Purpose:** The project compiles with Redis and ScyllaDB stubs across all standard test configurations (CMake preset: database-lite). PROJECT_ISSUES_TRACKER.md Issue #5 highlights no active test against a real database. This PRD creates a production-grade test environment using Docker Compose to provision real Redis (6.x) and ScyllaDB (5.x) containers, writes integration tests that talk to live instances, and implements graceful fallback to stubs when DB unavailable.

**Owner:** DATABASE_AGENT

**Priority:** HIGH (P2 — production-readiness)

**Estimated Effort:** 2–3 days

**Dependencies:** None

---

### PRD-019: AnimationTree Blend Space System

**Path:** `docs/plans/PRD/PRD-019-BLEND-SPACES.md`

**Purpose:** Establish BlendSpace2D nodes in the AnimationTree to produce smooth locomotion transitions: Walk/Run blend by speed, 8-direction strafe blending, turn-in-place blending, and combat movement blend spaces (idle-combat-ready-lunge). Addresses Issue #7 (Blend Spaces Not Implemented).

**Owner:** ANIMATION_AGENT

**Priority:** MEDIUM (P3 — required for smooth movement)

**Estimated Effort:** 2 days

**Dependencies:** PRD-015 (leaning), PRD-016 (visual polish)

---

### PRD-020: Godot Client Headless Artifacts — Test/CI Stability

**Path:** `docs/plans/PRD/PRD-020-HEADLESS-FIXES.md`

**Purpose:** Under  `godot --headless --script` or test runners, scenes attempt to `add_child()` on non-root nodes during `_Ready()`, causing "Can't add child node to... non scene tree context" errors. These flaky CI issues are documented as PROJECT_ISSUES_TRACKER.md Issue #8. We must harden client code against headless context, use `SceneTree` or `call_deferred` appropriately.

**Owner:** TESTING_AGENT

**Priority:** MEDIUM (P3 — CI stability)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---

### PRD-021: Demo Validator — Connection Pooling & Rate Limiting

**Path:** `docs/plans/PRD/PRD-021-VALIDATOR-CONNECTIONS.md`

**Purpose:** tools/demo/live_client_validator.py spawns many concurrent client connections for stress test runs; after ~250 connections, FD exhaustion occurs (PROJECT_ISSUES_TRACKER.md Issue #9). Refactor validator to reuse TCP connections in a pool, implement exponential backoff, allow per-client teardown, and add circuit breaker to prevent cascading failures.

**Owner:** TESTING_AGENT

**Priority:** MEDIUM (P3 — stress test reliability)

**Estimated Effort:** 1–2 days

**Dependencies:** None

---

### PRD-022: Combat State Machine Visual Polish & Node Template Finalization

**Path:** `docs/plans/PRD/PRD-022-COMBAT-FSM-FINALIZE.md`

**Purpose:** PRD-008 (CombatStateMachine FSM) creates the initial node-based FSM template. This PRD refines it: ensure proper icon assignment for the packed scene, add Ready-state debug visualization (godot Inspector), publish usage hints/graph-tips, implement transition-to-Node animation sync, and create official documentation "COMBAT-FSM-USAGE.md" with screenshots. Completes Issue #6 (No Node-Based FSM) finalization.

**Owner:** COMBAT_AGENT

**Priority:** MEDIUM (P3 — polish & shareability)

**Estimated Effort:** 1 day

**Dependencies:** PRD-008

---

### PRD-023: Combat Floating Text Integration — Damage Numbers & Healing Indicators

**Path:** `docs/plans/PRD/PRD-023-COMBAT-TEXT.md`

**Purpose:** DamageNumber.cs and CombatTextSystem.cs classes exist but are NOT integrated into any scene structure (players, UI). This PRD wires CombatTextSystem into Player.tscn to spawn floating numbers on `OnDamageTaken`/`OnHeal` events, implements pooling to avoid GC churn, and adds visual variety (color, size) for hit-crit/crash/block/parry/dodge — closing COMPATIBILITY_ANALYSIS.md Floating Combat Text gap.

**Owner:** UI_AGENT

**Priority:** MEDIUM (P3 — core combat feedback)

**Estimated Effort:** 2 days

**Dependencies:** PRD-010 (damage calculation)

---

### PRD-024: Documentation Audit & Drift Correction

**Path:** `docs/plans/PRD/PRD-024-DOCUMENTATION-AUDIT.md`

**Purpose:** Audit every .md file for structural errors, inaccurate status fields, broken links, stale content. Resolve PROJECT_ISSUES_TRACKER.md Issue #10 (Documentation Drift). Create a "living documents" badge list on AGENTS.md, update all frontmatter statuses (In Progress → Complete, etc.) with associated hash-ID, and add link-check CI workflow. Provide `scripts/docs_validate.py` for continuous integration.

**Owner:** DOCUMENTATION_AGENT

**Priority:** MEDIUM (P3 — maintainability)

**Estimated Effort:** 1–2 days

**Dependencies:** None