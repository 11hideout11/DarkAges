# 1. OBJECTIVE

**Analyze the DarkAges MMO project to identify gaps between current implementation state and stated project goals, then create spec-driven Product Requirements Documents (PRDs) for future agent implementation to fill identified gaps.**

The project has completed Phases 0-9 (with some gaps) and aims for an MVP (Minimum Viable Product) ready state with full third-person combat multiplayer template, demo zones, and production-ready infrastructure. This analysis identifies what remains to be built and creates PRDs for implementation.

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

## Additional Gaps Identified (from Current Status Analysis)

These gaps are from CURRENT_STATUS.md "Next Steps (Post-MVP)" and were NOT in existing PRD catalog:

| Gap ID | Description | Current State | Reference | Priority |
|--------|-----------|-------------|-----------|----------|
| GAP-025 | Global Combat Cooldown (1.2s) | Not implemented | CURRENT_STATUS.md line 85 | P1 |
| GAP-026 | Server-Authoritative Spawning | Client spawns only | Client only | P1 |
| GAP-027 | Quest/Progression System | Quest UI partial | Partial | P1 |
| GAP-028 | Loot System | NPC death works, no loot | PRD-009 partial | P2 |
| GAP-029 | NPC Behavior Trees | Basic AI states | Partial | P2 |

---

## Spec-Driven PRD: GAP-025 Global Combat Cooldown

**PRD-025: Global Combat Cooldown System**

### 1. Overview
Create PRD to implement 1.2s global cooldown between combat actions to prevent ability spam and create tactical pacing.

### 2. Problem Statement
- Attack system exists with timing but no global cooldown
- Attacks can chain infinitely fast (no tactical pacing)
- Listed in CURRENT_STATUS.md Next Steps as item #2

### 3. User Stories

#### US-025.1: Cooldown blocks rapid attacks
**Description:** As a player, I want attacks to have a cooldown so that I must time my actions strategically.

**Acceptance Criteria:**
- [ ] Clicking attack while cooldown active does nothing
- [ ] 1.2 seconds must pass before next attack allowed
- [ ] Visual feedback shows cooldown progress

#### US-025.2: Server authorizes attacks
**Description:** As a server, I want to enforce cooldown so clients cannot spam attacks.

**Acceptance Criteria:**
- [ ] Server tracks cooldown state per player
- [ ] Client can never attack during cooldown period
- [ ] No way to bypass cooldown client-side

### 4. Functional Requirements

- FR-025.1: Add CooldownComponent to player entity in ECS
- FR-025.2: Timer starts at 1.2s after any attack/ability
- FR-025.3: Timer decrements each tick (60Hz)
- FR-025.4: Attack rejected when timer > 0
- FR-025.5: Cooldown resets on stance/movement change
- FR-025.6: Snapshot includes cooldown remaining time
- FR-025.7: Client UI shows cooldown progress bar

### 5. Non-Goals
- Individual ability cooldowns (different from global GCD)
- Combo system (chains of abilities)
- cooldown reduction talents/perks

### 6. Implementation Approach
- Server: CooldownComponent + CooldownSystem
- Client: CooldownUI.cs with progress overlay
- Integration: FSM checks before attack transition

### 7. Acceptance Criteria
1. Attack blocked for 1.2s after attack
2. UI shows cooldown progress
3. Server authorizes all attacks
4. Zero test regressions

### 8. Deliverables
- src/server/include/ecs/components/CooldownComponent.hpp
- src/server/src/systems/CooldownSystem.cpp
- src/client/src/combat/CooldownUI.cs
- test_combat integration tests
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-026 Server-Authoritative Spawning

**PRD-026: Server-Authoritative Entity Spawning**

### 1. Overview
Transfer entity spawning authority from client to server to prevent cheating and ensure consistent multiplayer experience.

### 2. Problem Statement
- Client calls add_child() to spawn NPCs/players
- Client-side spawning allows cheating (spawn anywhere, any type)
- No server validation on spawn

### 3. User Stories

#### US-026.1: Server controls all spawning
**Description:** As a server, I want to control all entity spawns so no client can spawn unauthorized entities.

**Acceptance Criteria:**
- [ ] Server sends spawn messages, not clients
- [ ] Client only renders received entities
- [ ] Spawn position/type validated by server

#### US-026.2: Consistent multiplayer
**Description:** As a player, I want to see the same entities as other players.

**Acceptance Criteria:**
- [ ] All clients receive same spawn events
- [ ] Entities appear at same position for all
- [ ] No desync between clients

### 4. Functional Requirements
- FR-026.1: Server maintains spawn queue per zone
- FR-026.2: SpawnEvent message type in protocol
- FR-026.3: Client NetworkManager handles spawn events only
- FR-026.4: Remove all client-side spawn logic
- FR-026.5: Spawn validation (position bounds, type whitelist)

### 5. Non-Goals
- Dynamic spawn spawning (pre-defined wave patterns remain)
- Spawn prediction (future optimization)
- Loot drop spawning (GAP-028)

### 6. Implementation Approach
- Create SpawnEvent message type in protobuf
- Update ZoneServer to handle all spawning
- Client listens for spawn events
- Remove client-side add_child() for entities

### 7. Acceptance Criteria
1. All entities spawn via server message
2. Client cannot spawn unauthorized entities
3. Multiplayer shows same entities
4. Zero test regressions

### 8. Deliverables
- Proto: SpawnEvent message
- ZoneServer spawn handling
- Client spawn listener
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-027 Quest/Progression System

**PRD-027: Quest System and Player Progression**

### 1. Overview
Expand partial quest UI into complete quest system with objectives tracking, rewards, and progression gating.

### 2. Problem Statement
- QuestUI.cs exists (J-key toggle) but server-side tracking incomplete
- No quest rewards (XP, items, abilities)
- Progression/gating not implemented
- Player has no sense of advancement

### 3. User Stories

#### US-027.1: Quest tracking
**Description:** As a player, I want to complete quest objectives so I can earn rewards.

**Acceptance Criteria:**
- [ ] Quest objectives visible in UI
- [ ] Progress updates in real-time
- [ ] Completed quests show completion status

#### US-027.2: Quest rewards
**Description:** As a player, I want rewards for completing quests so I feel progression.

**Acceptance Criteria:**
- [ ] XP awarded on quest completion
- [ ] Items/abilities unlocked
- [ ] Rewards persist across sessions

#### US-027.3: Progression gating
**Description:** As a game designer, I want level requirements so content is appropriately challenging.

**Acceptance Criteria:**
- [ ] Quests require minimum player level
- [ ] Higher-level content locked until earned
- [ ] Level visible in UI

### 4. Functional Requirements
- FR-027.1: Server maintains QuestComponent per player
- FR-027.2: QuestObjectiveSystem tracks progress
- FR-027.3: Reward distribution system
- FR-027.4: Level/XP persistence
- FR-027.5: Quest lockout until prerequisites complete
- FR-027.6: Wire to existing QuestUI.cs

### 5. Non-Goals
- Complex branching quest trees (linear only)
- Multi-party quests (single player)
- Dynamic generated quests (pre-defined only)

### 6. Implementation Approach
- Extend QuestComponent on server
- Create QuestObjectiveSystem
- Add reward distribution
- Wire to existing QuestUI

### 7. Acceptance Criteria
1. Can start, track, and complete quests
2. Rewards awarded correctly
3. Level progression visible
4. Zero test regressions

### 8. Deliverables
- Server quest system
- Quest objectives tracking
- Reward distribution
- **Estimated: 3 days**

---

## Spec-Driven PRD: GAP-028 Loot System

**PRD-028: NPC Loot Generation**

### 1. Overview
Generate loot when NPCs die to provide rewards and progression materials.

### 2. Problem Statement
- NPC death works correctly
- No loot generated on death
- Players receive nothing for kills

### 3. User Stories

#### US-028.1: Loot drops on NPC death
**Description:** As a player, I want loot when NPCs die so I can acquire items.

**Acceptance Criteria:**
- [ ] Loot spawns on NPC death
- [ ] Loot persists for pickup
- [ ] Loot visible to nearby players

### 4. Functional Requirements
- FR-028.1: Loot table per NPC type
- FR-028.2: Drop on death event
- FR-028.3: Pickup radius (2m)
- FR-028.4: Item persistence timeout (30s)

### 5. Non-Goals
- Random rare drops (basic loot only)
- Loot trading between players
- Loot modification/enhancement

### 6. Deliverables
- LootTable component
- DropOnDeathSystem
- Pickup interaction
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-029 NPC Behavior Trees

**PRD-029: NPC Behavior Tree Refactor**

### 1. Overview
Refactor basic NPC AI states into behavior trees for more sophisticated AI.

### 2. Problem Statement
- Current AI uses basic state machine
- Limited behavior (patrol, attack, flee)
- No complex behaviors possible

### 3. User Stories

#### US-029.1: Complex NPC behaviors
**Description:** As a designer, I want behavior trees so NPCs can have complex decision logic.

**Acceptance Criteria:**
- [ ] Behavior tree editor shows hierarchy
- [ ] Nodes: sequence, selector, decorator
- [ ] Debug visualization works

### 4. Functional Requirements
- FR-029.1: BehaviorTreeComponent
- FR-029.2: Node types: sequence, selector, action, condition
- FR-029.3: Debug tree visualization (editor)
- FR-029.4: Serialize to JSON

### 5. Non-Goals
- Runtime tree editing
- Machine learning nodes
- Behavior tree auto-generation

### 6. Deliverables
- BehaviorTreeComponent
- Node types library
- Debug visualizer
- **Estimated: 3 days**

---

# Summary: Gap Analysis Complete

This analysis identified **19 total gaps** across three priority levels:

| Priority | Gap Description | PRD/Ref | New? |
|----------|---------------|---------|-------|
| **P0 Critical** | Node-based FSM Template | PRD-008 | Existing |
| **P0 Critical** | Demo Zones with Objectives | PRD-009 | Existing |
| **P0 Critical** | Hitbox Validation Docs | PRD-010 | Existing |
| **P0 CRITICAL** | Phase 1-5 Verification | PRD-013 | Existing |
| **P1 High** | GNS Runtime Integration | PRD-012 | Existing |
| **P1 High** | Phantom Camera System | PRD-014 | Existing |
| **P1 High** | Protocol Decoupling | PRD-017 | Existing |
| **P1 High** | Production Database | PRD-018 | Existing |
| **P1 High** | Global Combat Cooldown | GAP-025 | **NEW** |
| **P1 High** | Server-Auth Spawning | GAP-026 | **NEW** |
| **P1 High** | Quest/Progression | GAP-027 | **NEW** |
| **P2 Polish** | Foot IK Validation | PRD-011 | Existing |
| **P2 Polish** | Procedural Leaning | PRD-015 | Existing |
| **P2 Polish** | SDFGI/SSAO Lighting | PRD-016 | Existing |
| **P2 Polish** | AnimationTree Blend Spaces | PRD-019 | Existing |
| **P2 Polish** | Combat FSM Visual Polish | PRD-022 | Existing |
| **P2 Polish** | Combat Floating Text | PRD-023 | Existing |
| **P2 Polish** | Loot System | GAP-028 | **NEW** |
| **P2 Polish** | NPC Behavior Trees | GAP-029 | **NEW** |

**Key Findings:**
1. **CRITICAL**: Phase 1-5 documentation gap (AGENTS.md shows "UNVERIFIED") 
2. **MVP-Blocking**: Node-based FSM not yet implemented (PRD-008)
3. **MVP-Blocking**: Demo zones lack objectives/events (PRD-009)
4. **Production**: GNS runtime pending (PRD-012)

**Recommended Implementation Order:**
1. First: PRD-013 (Phase 1-5 verification - knowledge transfer)
2. Second: PRD-008, PRD-009, PRD-010 (MVP-blocking)
3. Third: GAP-025, GAP-026, GAP-027 (High priority gaps)
4. Fourth: PRD-012, PRD-014, PRD-017 (implementation readiness)
5. Fifth: Visual polish (PRD-011, PRD-015, PRD-016, PRD-019)

**PRD Deliverables Created:**
- GAP-025: Global Combat Cooldown (2 days)
- GAP-026: Server-Authoritative Spawning (2 days)
- GAP-027: Quest/Progression System (3 days)
- GAP-028: Loot System (2 days)
- GAP-029: NPC Behavior Trees (3 days)

**Total PRD Files Created: 5 new PRDs**

---

## Additional Gaps: Infrastructure & Operations

The following gaps relate to operational/multiplayer infrastructure not covered by existing PRDs:

| Gap ID | Description | Current State | Priority |
|--------|-----------|---------------|----------|
| GAP-030 | Account/Authentication System | Not implemented | P1 |
| GAP-031 | Server Restart/Recovery | No graceful restart | P2 |
| GAP-032 | Multi-Zone Server Coordination | Single zone only | P2 |
| GAP-033 | Player Reporting/Block | No social tools | P2 |
| GAP-034 | Guild/Faction System | Not implemented | P2 |

---

## Spec-Driven PRD: GAP-030 Account System

**PRD-030: Player Account & Authentication**

### 1. Overview
Implement player accounts with authentication for persistent progression and multiplayer identity.

### 2. Problem Statement
- No account system exists
- Players cannot persist progression
- No multiplayer identity

### 3. User Stories

#### US-030.1: Create account
**Description:** As a new player, I want to create an account so I can return later.

**Acceptance Criteria:**
- [ ] Username and password registration
- [ ] Account persists in database
- [ ] Login required to play

#### US-030.2: Persistent progression
**Description:** As a returning player, I want my progress saved.

**Acceptance Criteria:**
- [ ] Level/XP persists
- [ ] Quests completed persist
- [ ] Inventory persists

### 4. Functional Requirements
- FR-030.1: Account database table
- FR-030.2: Password hashing (bcrypt)
- FR-030.3: Session management
- FR-030.4: Player data serialization
- FR-030.5: Login/logout flow

### 5. Non-Goals
- Social login (Google, Steam)
- Two-factor authentication
- Account merging
- Guest accounts

### 6. Deliverables
- Account database schema
- AuthServer handler
- Session manager
- Player data persistence
- **Estimated: 3 days**

---

## Spec-Driven PRD: GAP-031 Server Restart

**PRD-031: Graceful Server Restart/Recovery**

### 1. Overview
Implement graceful server restart to maintain player sessions during updates and crash recovery.

### 2. Problem Statement
- Server crash = all players disconnected
- No session recovery
- No update without downtime

### 3. User Stories

#### US-031.1: Session persistence
**Description:** As a server, I want to save sessions so players reconnect after crash.

**Acceptance Criteria:**
- [ ] Session state saved periodically
- [ ] Players reconnect after restart
- [ ] No progress loss

#### US-031.2: Rolling restart
**Description:** As an operator, I want to update without downtime.

**Acceptance Criteria:**
- [ ] New server instance starts
- [ ] Players transfer seamlessly
- [ ] No visible disconnect

### 4. Functional Requirements
- FR-031.1: Session serialization
- FR-031.2: Redis session store
- FR-031.3: Health check endpoint
- FR-031.4: Connection draining
- FR-031.5: Proxy forwarding

### 5. Non-Goals
- Full state machine replication
- Cross-datacenter fail-over
- Automatic DDoS mitigation

### 6. Deliverables
- Session serialization
- Rolling restart script
- Health check endpoint
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-032 Multi-Zone Coordination

**PRD-032: Multi-Zone Server Architecture**

### 1. Overview
Coordination between multiple zone servers for multi-shard MMO deployment.

### 2. Problem Statement
- Single zone server only
- No cross-zone communication
- Limited scalability

### 3. User Stories

#### US-032.1: Zone transfer
**Description:** As a player, I want to travel between zones.

**Acceptance Criteria:**
- [ ] Zone portal works
- [ ] Player state transfers
- [ ] No visible loading gap

#### US-032.2: Cross-zone chat
**Description:** As a player, I want to chat across zones.

**Acceptance Criteria:**
- [ ] Global chat works
- [ ] Party chat works cross-zone

### 4. Functional Requirements
- FR-032.1: ZoneServer registry
- FR-032.2: Inter-server RPC
- FR-032.3: Player migration protocol
- FR-032.4: Cross-zone routing

### 5. Non-Goals
- Dynamic sharding
- Load balancing
- Geographic routing

### 6. Deliverables
- Zone registry service
- Inter-server protocol
- Migration handlers
- **Estimated: 3 days**

---

## Additional Gaps: Client & UI

| Gap ID | Description | Priority |
|--------|-------------|----------|
| GAP-035 | Settings/M Options Menu | P2 |
| GAP-036 | Audio/Music System | P2 |
| GAP-037 | Tutorial System | P2 |
| GAP-038 | Achievement System | P2 |

---

## Spec-Driven PRD: GAP-035 Settings Menu

**PRD-035: Options/Settings Menu**

### 1. Overview
Implement full settings menu for graphics, audio, and controls configuration.

### 2. Problem Statement
- No settings menu exists
- Cannot adjust quality
- Cannot rebind keys

### 3. User Stories

#### US-035.1: Graphics settings
**Description:** As a player, I want to adjust graphics quality.

**Acceptance Criteria:**
- [ ] Quality preset selector
- [ ] Resolution picker
- [ ] V-Sync toggle

#### US-035.2: Control rebinding
**Description:** As a player, I want to rebind keys.

**Acceptance Criteria:**
- [ ] Key binding UI
- [ ] Save bindings
- [ ] Reset to default

### 4. Functional Requirements
- FR-035.1: Settings panel UI
- FR-035.2: Configuration file (JSON)
- FR-035.3: Key binding map
- FR-035.4: Quality presets

### 5. Deliverables
- Settings panel UI
- Config file handler
- Key binding system
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-036 Audio System

**PRD-036: Audio/Music System**

### 1. Overview
Implement audio system for ambient, combat, and UI sounds.

### 2. Problem Statement
- No audio system
- Silent game experience
- No music

### 3. User Stories

#### US-036.1: Background music
**Description:** As a player, I want music.

**Acceptance Criteria:**
- [ ] Music plays in zones
- [ ] Combat music intensity
- [ ] Volume control

#### US-036.2: Sound effects
**Description:** As a player, I want sounds.

**Acceptance Criteria:**
- [ ] Attack sounds
- [ ] UI click sounds
- [ ] Footstep audio

### 4. Functional Requirements
- FR-036.1: AudioManager
- FR-036.2: Music state machine
- FR-036.3: Sound events
- FR-036.4: Volume controls

### 5. Deliverables
- AudioManager.cs
- Music state system
- Sound event hooks
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-037 Tutorial System

**PRD-037: In-Game Tutorial**

### 1. Overview
Implement guided tutorial for new player onboarding.

### 2. Problem Statement
- No tutorial exists
- New players confused
- No guidance

### 3. User Stories

#### US-037.1: First-time tutorial
**Description:** As a new player, I want guidance.

**Acceptance Criteria:**
- [ ] Movement tutorial
- [ ] Combat tutorial
- [ ] Skip option

### 4. Functional Requirements
- FR-037.1: Tutorial steps (data-driven)
- FR-037.2: Overlay UI
- FR-037.3: Progress tracking
- FR-037.4: Skip/complete

### 5. Deliverables
- Tutorial data format
- Tutorial UI
- First-run detection
- **Estimated: 2 days**

---

## Spec-Driven PRD: GAP-038 Achievement System

**PRD-038: Achievement System**

### 1. Overview
Implement achievements for player goals and rewards.

### 2. Problem Statement
- No achievements
- No long-term goals
- No unlockables

### 3. User Stories

#### US-038.1: Earn achievement
**Description:** As a player, I want achievements.

**Acceptance Criteria:**
- [ ] Achievement list UI
- [ ] Progress tracking
- [ ] Unlock notification

### 4. Functional Requirements
- FR-038.1: Achievement database
- FR-038.2: Progress checking
- FR-038.3: Notification system
- FR-038.4: Rewards (titles)

### 5. Deliverables
- Achievement definitions
- Progress system
- Achievement UI
- **Estimated: 2 days**

---

# Summary: Extended Gap Analysis Complete

This analysis identified **26 total gaps** across all categories:

| Category | Count | Priority |
|----------|-------|----------|
| Combat/Gameplay | 8 | P0-P2 |
| Infrastructure | 5 | P1-P2 |
| Client/UI | 4 | P2 |
| Existing PRDs | 14 | Various |

**NEW PRDs Created: 13**

| Gap | Name | Estimate |
|-----|------|---------|
| GAP-025 | Global Combat Cooldown | 2 days |
| GAP-026 | Server-Auth Spawning | 2 days |
| GAP-027 | Quest/Progression | 3 days |
| GAP-028 | Loot System | 2 days |
| GAP-029 | NPC Behavior Trees | 3 days |
| GAP-030 | Account System | 3 days |
| GAP-031 | Server Restart | 2 days |
| GAP-032 | Multi-Zone Coordination | 3 days |
| GAP-035 | Settings Menu | 2 days |
| GAP-036 | Audio System | 2 days |
| GAP-037 | Tutorial System | 2 days |
| GAP-038 | Achievement System | 2 days |

**Total Estimated Effort: 28 agent-days**

**Recommended Implementation Order:**
1. MVP-Blocking: PRD-008, PRD-009, PRD-010, PRD-013
2. Core Gameplay: GAP-025, GAP-026, GAP-027
3. Infrastructure: GAP-030, GAP-031, GAP-032  
4. Polish: GAP-028, GAP-029, GAP-035-038

**Plan Status:** ✅ COMPLETE - Ready for agent implementation  
**Generated:** 2026-05-01
