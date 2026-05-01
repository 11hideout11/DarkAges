# 1. OBJECTIVE

Analyze the DarkAges MMO project to identify gaps between the current implementation state and the project's future goals/vision, then create spec-driven Product Requirements Documents (PRDs) to plan implementation work for future agents.

# 2. CONTEXT SUMMARY

## Project Current State (May 2026)
- **Server**: ~32K LOC, C++20 with EnTT ECS, 60Hz tick rate, 2129 tests passing
- **Client**: ~9K LOC Godot 4.2 Mono (C#), with prediction, interpolation, combat UI
- **Demo Pipeline**: Operational (`./tools/demo/demo --quick`)
- **Test Suite**: 2129 test cases, 12644 assertions, 100% passing

## Phase Status (from AGENTS.md)
- Phase 0: ✅ Complete (documented in PHASE0_SUMMARY.md)
- Phase 1-5: ❌ **UNVERIFIED** — No documentation found (CRITICAL GAP)
- Phase 6: ✅ Complete (build system hardening)
- Phase 7: ✅ Complete (all tests passing)
- Phase 8: ⚠️ PARTIAL (GNS compile-time fixed; runtime integration pending)
- Phase 9: ✅ Complete (performance budgets validated)
- Phase 10: 🔄 In Progress (Security validation)

## Project Future Goals (Updated MVP Criteria April 28)
From PROJECT_STATUS.md and MVP_DEMO_STANDARDS.md:

### P0 — Must-Have for MVP
1. **Full Third-Person Combat Multiplayer Template**
   - Node-based FSM (not code-only inline flags)
   - Hitbox/Hurtbox collision layers with server validation
   - AnimationTree with procedural features (blend spaces, foot IK, hit stop)
   - Lock-on targeting with camera follow

2. **Demo Zones**
   - Multiple curated demo zones (minimum 3: tutorial, combat, boss)
   - Proper gameplay pacing and encounter design
   - Zone-specific objectives and events

3. **Complete Playable Demo Loop**
   - Combat → loot → quest → progression
   - Human-playable (WASD + mouse, not just bot-mode)
   - Visual feedback (damage numbers, health bars, floating text)

### Post-MVP Visual Polish
- Phantom Camera (third-person follow + lock-on)
- Procedural Leaning (velocity-based tilt)
- Procedural Foot IK (terrain alignment)
- AnimationTree Blend Spaces
- SDFGI/SSAO Lighting

## Gap Analysis Summary (from GAP-ANALYSIS-2026-05-01.md)

| Category | Gap | Current State | PRD Reference |
|----------|-----|--------------|---------------|
| P0 Critical | Combat FSM Template | Code exists (AnimationStateMachine.cs), lacks node-based FSM template | PRD-008 |
| P0 Critical | Demo Zones | Zone configs exist (tutorial/arena/boss.json), need objectives/events | PRD-009 |
| P0 Critical | Hitbox Validation | Components + tests exist, need validation docs | PRD-010 |
| P1 | Foot IK | Implemented but unvalidated | PRD-011 |
| P1 | GNS Runtime | Compile-time fixed, runtime pending | PRD-012 |
| P0 | Phase 1-5 Verification | UNVERIFIED, no documentation | PRD-013 |
| P1 | Phantom Camera | Basic SpringArm3D, needs sophisticated follow | PRD-014 |
| P1 | Procedural Leaning | None implemented | PRD-015 |

# 3. APPROACH OVERVIEW

Gap analysis was conducted by:
1. Reviewing project documentation (READMEs, AGENTS.md, gap analysis docs)
2. Analyzing PRD catalog for implementation status
3. Examining codebase for evidence of gap implementations
4. Cross-referencing MVP criteria with current state

## Three Categories of Gaps Identified:

### Category A: MVP-Blocking Gaps (Critical)
- PRD-008: Combat FSM Template (node-based FSM not yet implemented)
- PRD-009: Demo Zones (need objectives, events, pacing refinement)
- PRD-010: Hitbox Validation (need validation docs, edge-case tests)

### Category B: Implementation Readiness Gaps (High Priority)
- PRD-012: GNS Runtime Integration (compile fixed, runtime pending)
- PRD-013: Phase 1-5 Documentation (CRITICAL - knowledge transfer)
- PRD-014: Phantom Camera (needs Sophisticated follow system)

### Category C: Visual Polish Gaps (Post-MVP)
- PRD-011: Foot IK (implemented but unvalidated)
- PRD-015: Procedural Leaning (not yet implemented)
- PRD-016: SDFGI/SSAO Lighting
- PRD-019: Blend Spaces

## PRD Status Overview

From `docs/plans/PRD/README.md`:

| PRD | Title | Status | Priority |
|-----|-------|--------|----------|
| PRD-008 | CombatStateMachine Node-Based FSM Template | 🔄 In Progress | P0 |
| PRD-009 | Demo Zones System | 🔄 In Progress | P0 |
| PRD-010 | Server-Authoritative Hitbox/Hurtbox Validation | 🔄 In Progress | P0 |
| PRD-011 | Foot IK System Validation | 🔄 In Progress | P1 |
| PRD-012 | GNS Runtime Integration | 🔄 In Progress | P1 |
| PRD-013 | Phase 1-5 Implementation Verification | 🟡 In Progress | P0 |
| PRD-014 | Phantom Camera 3rd-Person | 🟡 In Progress | P1 |
| PRD-015 | Procedural Leaning System | 🟡 In Progress | P1 |
| PRD-016 | SDFGI/SSAO/SSIL Post-Processing | 🟡 In Progress | P2 |
| PRD-017 | Protocol Layer Decoupling | 🟡 In Progress | P2 |
| PRD-018 | Production Database Integration | 🟡 In Progress | P2 |
| PRD-019 | AnimationTree Blend Spaces | 🟡 In Progress | P2 |
| PRD-020 | Godot Client Headless Fixes | 🟡 In Progress | P3 |
| PRD-021 | Demo Validator Connection Pooling | 🟡 In Progress | P3 |
| PRD-022 | Combat FSM Visual Polish | 🟡 In Progress | P2 |
| PRD-023 | Combat Floating Text Integration | 🟡 In Progress | P2 |
| PRD-024 | Documentation Audit | ✅ Complete | - |

# 4. IMPLEMENTATION STEPS

## Critical Path: MVP-Blocking Gaps (Must Complete First)

### Step 4.1: PRD-013 - Phase 1-5 Documentation (CRITICAL - Knowledge Transfer)
**Gap**: Phases 1-5 are marked 'UNVERIFIED' in AGENTS.md with zero documentation.
**Risk**: Cannot accurately assess project completeness or train new contributors.
**Action**: 
- Audit codebase for Phase 1-5 feature implementations
- Create PHASE1_SUMMARY.md through PHASE5_SUMMARY.md
- Cross-reference AGENTS.md claimed phases vs actual code
- Document any missing/skipped work

**Owner**: DOCUMENTATION_AGENT  
**Estimated Effort**: 1-2 days

### Step 4.2: PRD-008 - Node-Based Combat FSM Template
**Gap**: AnimationStateMachine.cs uses code-only inline flags; designers cannot visually edit.
**Evidence**: 
- Code exists: `src/client/src/combat/fsm/AnimationStateMachine.cs` (~330 lines)
- Missing: `CombatStateMachine.tscn` — visual state machine resource
- Missing: AnimationTree `.tres` with configured state machine

**Action**:
- Create `CombatStateMachine.tscn` scene with node hierarchy
- Configure AnimationNodeStateMachine in AnimationTree
- Integrate with existing Player.tscn and RemotePlayer.tscn

**Owner**: COMBAT_AGENT  
**Estimated Effort**: 2-3 days

### Step 4.3: PRD-009 - Demo Zones with Pacing/Objectives
**Gap**: Zone configs exist but lack objectives, events, demo sequence orchestration.
**Evidence**:
- Zone configs exist: `tutorial.json`, `arena.json`, `boss.json`
- Missing: `pacing`, `objectives`, `events` blocks
- Missing: ZoneObjectiveSystem (server-side)
- Missing: ZoneObjectivesUI (client)

**Action**:
- Enrich zone JSONs with objectives/events
- Create ZoneObjectiveSystem for tracking
- Create UI for displaying objectives
- Update demo orchestrator for multi-zone sequence

**Owner**: ZONES_AGENT  
**Estimated Effort**: 2-3 days

### Step 4.4: PRD-010 - Hitbox/Hurtbox Server Validation
**Gap**: Components exist and tests exist but need validation docs, edge cases, anti-cheat.
**Action**:
- Document collision matrix in `docs/collision-matrix.md`
- Add edge-case tests (multihit, iframes, rewind boundaries)
- Implement anti-cheat validation
- Benchmark collision throughput (1000 entities)

**Owner**: PHYSICS_AGENT  
**Estimated Effort**: 2 days

## High Priority: Implementation Readiness

### Step 4.5: PRD-012 - GNS Runtime Integration
**Gap**: Compile-time fixed in Phase 8, runtime networking still uses old custom UDP.
**Evidence**:
- GNS library compiles (ENABLE_GNS=ON works)
- Runtime still uses `socket()`, `bind()`, `recvfrom()`, `sendto()`
- No `#if ENABLE_GNS` guards in networking source

**Owner**: NETWORKING_AGENT  
**Estimated Effort**: 2-3 days

### Step 4.6: PRD-014 - Phantom Camera System
**Gap**: Basic SpringArm3D in use; needs sophisticated follow + lock-on.
**Owner**: CAMERA_AGENT  
**Estimated Effort**: 2-3 days

### Step 4.7: PRD-017 - Protocol Decoupling from GNS
**Gap**: Protocol.cpp excluded when ENABLE_GNS=OFF (tests cannot verify protocol).
**Owner**: NETWORKING_AGENT  
**Estimated Effort**: 1-2 days

## Post-MVP: Visual Polish

### Step 4.8: PRD-011 - Foot IK Validation
**Gap**: FootIKController.cs exists but unvalidated, no benchmark.
**Owner**: ANIMATION_AGENT  
**Estimated Effort**: 1 day

### Step 4.9: PRD-015 - Procedural Leaning System
**Gap**: No velocity-based character tilt implementation.
**Owner**: ANIMATION_AGENT  
**Estimated Effort**: 1-2 days

### Step 4.10: PRD-019 - AnimationTree Blend Spaces
**Gap**: No BlendSpace2D nodes for smooth locomotion.
**Owner**: ANIMATION_AGENT  
**Estimated Effort**: 2 days

### Step 4.11: PRD-016 - SDFGI/SSAO Post-Processing
**Gap**: Lighting needs SDFGI for coherence.
**Owner**: RENDERING_AGENT  
**Estimated Effort**: 2 days

# 5. TESTING AND VALIDATION

## Verification Approach
1. All new PRDs will define acceptance criteria that can be tested
2. Each PRD will specify test locations and expected outcomes
3. No new features should cause test regressions (baseline: 2129 tests, 12644 assertions)

## Success Criteria for Planning
- [x] Current project state documented (from AGENTS.md)
- [x] Future goals identified (from MVP_DEMO_STANDARDS.md)
- [x] Gaps categorized by priority (Critical/High/Post-MVP)
- [x] PRD status cataloged
- [x] Implementation steps prioritized

## Gap Summary

| Category | Count | Critical PRDs |
|-----------|-------|----------------|
| MVP-Blocking (P0) | 4 | PRD-008, PRD-009, PRD-010, PRD-013 |
| Implementation Readiness (P1) | 4 | PRD-012, PRD-014, PRD-017, PRD-018 |
| Visual Polish (P2) | 6 | PRD-011, PRD-015, PRD-016, PRD-019, PRD-022, PRD-023 |
| **TOTAL Gaps Requiring Implementation** | **14** | - |

---

# Summary: Gap Analysis Complete

This analysis identified **14 gaps** across three priority levels:

| Priority Level | Gap Description | Key PRD References |
|----------------|-----------------|-------------------|
| **P0 (Critical)** | Node-based FSM Template | PRD-008 |
| **P0 (Critical)** | Demo Zones with Objectives | PRD-009 |
| **P0 (Critical)** | Hitbox Validation Documentation | PRD-010 |
| **P0 (CRITICAL)** | Phase 1-5 Verification | PRD-013 |
| **P1 (High)** | GNS Runtime Integration | PRD-012 |
| **P1 (High)** | Phantom Camera System | PRD-014 |
| **P1 (High)** | Protocol Decoupling | PRD-017 |
| **P1 (High)** | Production Database | PRD-018 |
| **P2 (Polish)** | Foot IK Validation | PRD-011 |
| **P2 (Polish)** | Procedural Leaning | PRD-015 |
| **P2 (Polish)** | SDFGI/SSAO Lighting | PRD-016 |
| **P2 (Polish)** | AnimationTree Blend Spaces | PRD-019 |
| **P2 (Polish)** | Combat FSM Visual Polish | PRD-022 |
| **P2 (Polish)** | Combat Floating Text | PRD-023 |

**Key Findings:**
1. **CRITICAL**: Phase 1-5 documentation gap (AGENTS.md shows "UNVERIFIED") 
2. **MVP-Blocking**: Node-based FSM not yet implemented (PRD-008)
3. **MVP-Blocking**: Demo zones lack objectives/events (PRD-009)
4. **Production**: GNS runtime pending (PRD-012)

**Recommended Implementation Order:**
1. First: PRD-013 (Phase 1-5 verification - knowledge transfer)
2. Second: PRD-008, PRD-009, PRD-010 (MVP-blocking)
3. Third: PRD-012, PRD-014, PRD-017 (implementation readiness)
4. Fourth: Visual polish (PRD-011, PRD-015, PRD-016, PRD-019)

**Plan Status:** ✅ COMPLETE - Ready for agent implementation  
**Generated:** 2026-05-01
