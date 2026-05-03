# PRD-GAP-003: NPC AI Behavior Implementation

**Version:** 1.0  
**Date:** 2026-05-03  
**Status:** Proposed  
**Priority:** P1 - Medium  
**Category:** Gameplay - AI System

---

## Introduction

PRD-031 (NPC AI Behavior System) exists and defines the required states (Idle, Patrol, Chase, Attack, Flee), BUT the NPCAISystem implementation is likely incomplete or missing. Demo zones need enemies that act intelligently - wolves that chase, bandits that attack, bosses with phase abilities.

**Problem:** The NPCAISystem.hpp exists but Update() may be a stub that does nothing. Enemies stand still or only play idle animations without behavior.

---

## Goals

- NPCs transition between behavioral states correctly (Idle → Patrol → Chase → Attack)
- Patrol behavior follows waypoint paths
- Chase behavior pursues player within detection radius
- Attack behavior triggers combat damage
- Boss behavior includes phase abilities

---

## User Stories

### US-001: NPC State Machine
**Description:** As an NPC, I want to change states so that I behave appropriately for the situation.

**Acceptance Criteria:**
- [ ] State enum includes: Idle, Patrol, Chase, Attack, Flee, Dead
- [ ] State transitions happen based on triggers
- [ ] Animation state updates to match behavior state
- [ ] Debug log shows state transitions

### US-002: Patrol Behavior
**Description:** As a patrol NPC, I want to walk my route so that players see activity.

**Acceptance Criteria:**
- [ ] NPCs follow waypoint paths from zone config
- [ ] Patrol speed is configurable (default: 2.0 units/s)
- [ ] Wait at each waypoint (default: 2.0s)
- [ ] Loop back to start after last waypoint

### US-003: Chase Behavior
**Description:** As an aggressive NPC, I want to chase players who get close.

**Acceptance Criteria:**
- [ ] Detection radius configurable (default: 10.0 units)
- [ ] Chase starts when player enters radius
- [ ] Chase pursues at running speed (default: 4.0 units/s)
- [ ] Chase ends if player exits disengage radius (default: 20.0 units)

### US-004: Attack Behavior
**Description:** As an NPC in combat, I want to attack the player.

**Acceptance Criteria:**
- [ ] Attack triggers when within attack range (default: 3.0 units)
- [ ] Attack cooldown configurable per NPC template
- [ ] Damage applies to player CombatComponent
- [ ] Attack animation plays

### US-005: Boss AI (Phases)
**Description:** As a boss NPC, I want to use phase abilities so that the encounter is dynamic.

**Acceptance Criteria:**
- [ ] Phase transitions at health thresholds from boss.json
- [ ] Ability set changes per phase
- [ ] Minion spawn triggers in specific phases
- [ ] Phase abilities execute with telegraph warnings

---

## Functional Requirements

- FR-1: NPCAISystem::Update(delta_time) processes all active NPCs
- FR-2: SetState(new_state) triggers animation transition callback
- FR-3: PatrolSystem loads waypoints from zone config
- FR-4: Detection uses PhysicsComponent overlap sphere
- FR-5: CombatSystem integration for damage application
- FR-6: Boss phase handler reads from boss.json

---

## Non-Goals

- Pathfinding (NavigationGrid) integration - use direct pursuit
- Squad AI (coordinated group behavior) - deferred
- Aggro table (multiple targets) - deferred to later
- Loot drops - use existing DropsSystem

---

## Technical Considerations

- **Existing Code:**
  - NPCAISystem.hpp - exists as stub
  - boss.json has "behavior": "boss" field
  - zone configs have "behavior": "aggressive"/"ranged"/"passive"

- **State Machine:**
  ```
  Idle → (detection) → Chase
  Chase → (in range) → Attack
  Chase → (out of range) → Patrol
  Attack → (out of range) → Chase
  Attack → (low health) → Flee
  Any → (dead) → Dead
  ```

- **Integration:**
  - NPCAISystem::Update() called in NPC tick
  - Animation state set via AnimationComponent
  - Damage via CombatComponent::ApplyDamage()
  - Phase via BossPhaseComponent

---

## Success Metrics

- **Functional:**
  - 100% state transitions correct
  - Enemies chase when player approaches
  - Attack damages player

- **Performance:**
  - < 1ms per 100 NPCs
  - No frame drops during combat

- **Demo Validation:**
  - Zone 99 (Arena) is playable with enemy AI
  - Zone 100 (Boss) boss AI executes phases