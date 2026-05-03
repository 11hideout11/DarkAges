# PRD-031: NPC AI Behavior System

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** Medium  
**Prerequisite:** None - exists as stub

---

## Introduction

NPCs currently lack AI behavior patterns. The NPCAISystem.hpp exists but is likely a placeholder. Demo zones (98Tutorial, 99Combat, 100Boss) need enemies that patrol, chase, attack, and use abilities.

## Goals

- Implement basic NPC AI states (Idle, Patrol, Chase, Attack, Flee)
- Create patrol paths for zone enemies
- Add combat AI for enemies (attack + ability use)
- Support boss AI patterns (phases, special attacks)

## User Stories

### US-001: NPC State Machine
**Description:** As a game designer, I want NPCs to have behavior states so that they act intelligently.

**Acceptance Criteria:**
- [ ] State enum: Idle, Patrol, Chase, Attack, Flee, Dead
- [ ] State transitions happen correctly
- [ ] Animation states match behavior

### US-002: Patrol AI
**Description:** As a player, I want enemies to patrol so that zones feel alive.

**Acceptance Criteria:**
- [ ] NPCs follow waypoint paths
- [ ] Patrol speed configurable
- [ ] Wait at waypoints

### US-003: Chase/Attack AI
**Description:** As a player, I want enemies to chase and attack me so that combat is engaging.

**Acceptance Criteria:**
- [ ] Detection radius triggers chase
- [ ] Chase maintains engagement
- [ ] Attack triggers when in range

### US-004: Boss AI (Phases)
**Description:** As a player, I want boss enemies to have phases so that encounters are dynamic.

**Acceptance Criteria:**
- [ ] Phase transitions at health thresholds
- [ ] Special attacks per phase
- [ ] Enrage timer (optional)

## Functional Requirements

- FR-1: NPCAISystem must implement Update(delta_time) for all active NPCs
- FR-2: State enum must include: Idle, Patrol, Chase, Attack, Flee, Dead
- FR-3: SetState() must trigger animation transition callback
- FR-4: Patrol waypoints loaded from zone config
- FR-5: Detection radius configurable per NPC template

## Non-Goals

- Pathfinding integration (use existing NavigationGrid)
- Squad AI (deferred)
- Aggro radius (deferred to zone config)
- Loot table drops (use existing DropsSystem)

## Technical Considerations

- Integration: NPCAISystem::Update() called in NPC tick
- Existing: NPCAISystem.hpp (exists as stub)
- Zone configs: Use data/zones.json for patrol paths

## Success Metrics

- NPC behavior: 100% state正确
- Performance: <1ms per 100 NPCs
- Demo zones: 99Combat playable

---

**PRD Status:** Proposed  
**Author:** OpenHands Gap Analysis  
**Next Step:** Implement states in NPCAISystem